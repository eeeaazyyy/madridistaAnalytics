#include "espnMapper.hpp"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLatin1StringView>
#include <QString>
#include <QStringList>
#include <QtDebug>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

namespace madridista { namespace infrastructure { namespace espn {

namespace {

constexpr int kNotFoundCode = 404;

constexpr QLatin1StringView kStatusPostponed{"STATUS_POSTPONED"};
constexpr QLatin1StringView kStatusCanceled{"STATUS_CANCELED"};
constexpr QLatin1StringView kStatusAbandoned{"STATUS_ABANDONED"};

constexpr int kShootoutPeriod = 5;
constexpr int kFirstHalfPeriod = 1;
constexpr int kSecondHalfPeriod = 2;
constexpr int kExtraTimeFirstHalfPeriod = 3;
constexpr int kExtraTimeSecondHalfPeriod = 4;

constexpr QLatin1StringView kOwnGoal{"Own Goal"};
constexpr QLatin1StringView kPenaltyPrefix{"Penalty"};

struct Side {
    domain::model::Team team;
    std::optional<int> score;
};

std::optional<std::int64_t> parseId(const QJsonValue& value) {
    if (value.isString()) {
        bool ok = false;
        const qlonglong id = value.toString().toLongLong(&ok);

        if (ok && id > 0) {
            return id;
        }

        return std::nullopt;
    }
    if (value.isDouble()) {
        const qint64 id = value.toInteger(-1);
        if (id > 0) {
            return id;
        }
    }

    return std::nullopt;
}

std::optional<std::chrono::sys_seconds> parseKickOff(const QJsonValue& value) {
    const QDateTime dateTime = QDateTime::fromString(value.toString(), Qt::ISODate);
    if (!dateTime.isValid()) {
        return std::nullopt;
    }

    return std::chrono::sys_seconds{std::chrono::seconds{dateTime.toSecsSinceEpoch()}};
}

domain::model::MatchStatus parseStatus(const QJsonObject& statusType) {
    const QString name = statusType["name"].toString();
    if (name == kStatusPostponed) {
        return domain::model::MatchStatus::Postponed;
    }
    if (name == kStatusCanceled || name == kStatusAbandoned) {
        return domain::model::MatchStatus::Cancelled;
    }

    const QString state = statusType["state"].toString();
    if (state == "in") {
        return domain::model::MatchStatus::Live;
    }
    if (state == "post" && statusType["completed"].toBool()) {
        return domain::model::MatchStatus::Finished;
    }

    return domain::model::MatchStatus::Scheduled;
}

std::optional<int> parseScore(const QJsonValue& value) {
    if (!value.isObject()) {
        return std::nullopt;
    }
    const QJsonValue goals = value.toObject()["value"];
    if (!goals.isDouble()) {
        return std::nullopt;
    }
    return goals.toInt();
}

std::optional<Side> parseCompetitor(const QJsonObject& competitor) {
    const QJsonObject team = competitor["team"].toObject();

    const auto id = parseId(team["id"]);
    if (!id) {
        return std::nullopt;
    }
    return Side{
        .team = domain::model::Team{domain::model::TeamId{*id},
                                    team["displayName"].toString().toStdString()},
        .score = parseScore(competitor["score"]),
    };
}

std::expected<domain::model::Match, domain::Error>
parseEvent(const QJsonObject& event, domain::model::CompetitionId competition) {
    const auto invalid = [](const char* reason) {
        return std::unexpected(domain::Error{domain::ErrorCode::InvalidResponse, reason});
    };

    const auto id = parseId(event["id"]);
    if (!id) {
        return invalid("event without a valid id");
    }

    const auto kickOff = parseKickOff(event["date"]);
    if (!kickOff) {
        return invalid("event without a valid date");
    }

    const QJsonArray competitions = event["competitions"].toArray();
    if (competitions.isEmpty()) {
        return invalid("event without competitions");
    }
    const QJsonObject game = competitions.first().toObject();

    std::optional<Side> homeSide;
    std::optional<Side> awaySide;

    for (const QJsonValue& value : game["competitors"].toArray()) {
        const QJsonObject competitor = value.toObject();
        const QString homeAway = competitor["homeAway"].toString();
        if (homeAway == "home") {
            homeSide = parseCompetitor(competitor);
        } else if (homeAway == "away") {
            awaySide = parseCompetitor(competitor);
        }
    }

    if (!homeSide || !awaySide) {
        return invalid("event without both home and away teams");
    }

    std::optional<domain::model::Score> score;
    if (homeSide->score && awaySide->score) {
        score = domain::model::Score{*homeSide->score, *awaySide->score};
    }

    return domain::model::Match::create({
        .id = domain::model::MatchId{*id},
        .kickoff = *kickOff,
        .competition = competition,
        .homeTeam = std::move(homeSide->team),
        .awayTeam = std::move(awaySide->team),
        .status = parseStatus(game["status"]["type"].toObject()),
        .score = score,
    });
}

std::optional<domain::model::Period> parsePeriod(int number) {
    switch (number) {
    case kFirstHalfPeriod:
        return domain::model::Period::FirstHalf;
    case kSecondHalfPeriod:
        return domain::model::Period::SecondHalf;
    case kExtraTimeFirstHalfPeriod:
        return domain::model::Period::ExtraTimeFirstHalf;
    case kExtraTimeSecondHalfPeriod:
        return domain::model::Period::ExtraTimeSecondHalf;
    default:
        return std::nullopt;
    }
}

std::optional<std::chrono::seconds> parseClock(const QJsonObject& clock) {
    const QJsonValue value = clock["value"];
    if (!value.isDouble() || value.toDouble() < 0.0) {
        return std::nullopt;
    }

    return std::chrono::seconds{static_cast<std::int64_t>(value.toDouble())};
}

std::chrono::minutes parseAddedTime(const QString& displayValue) {
    const QStringList parts = displayValue.split('\'');
    if (parts.size() < 2 || !parts[1].startsWith('+')) {
        return std::chrono::minutes{0};
    }

    bool ok = false;
    const int added = parts[1].mid(1).toInt(&ok);
    return std::chrono::minutes{ok && added > 0 ? added : 0};
}

domain::model::GoalType parseGoalType(const QString& text) {
    if (text == kOwnGoal) {
        return domain::model::GoalType::OwnGoal;
    }
    if (text.startsWith(kPenaltyPrefix)) {
        return domain::model::GoalType::Penalty;
    }

    return domain::model::GoalType::Regular;
}

std::optional<domain::model::Player> parseScorer(const QJsonArray& participants) {
    if (participants.isEmpty()) {
        return std::nullopt;
    }
    const QJsonObject athlete = participants.first().toObject()["athlete"].toObject();
    const auto id = parseId(athlete["id"]);
    if (!id) {
        return std::nullopt;
    }

    return domain::model::Player{domain::model::PlayerId{*id},
                                 athlete["displayName"].toString().toStdString()};
}

std::expected<domain::model::GoalEvent, domain::Error> parseGoal(const QJsonObject& keyEvent,
                                                                 domain::model::MatchId match) {
    const auto invalid = [](const char* reason) {
        return std::unexpected(domain::Error{domain::ErrorCode::InvalidResponse, reason});
    };

    const auto period = parsePeriod(keyEvent["period"]["number"].toInt());
    if (!period) {
        return invalid("goal with an unknown period");
    }

    const QJsonObject clock = keyEvent["clock"].toObject();
    const auto elapsed = parseClock(clock);
    if (!elapsed) {
        return invalid("goal without a valid clock");
    }

    const auto team = parseId(keyEvent["team"]["id"]);
    if (!team) {
        return invalid("goal without a team");
    }

    const std::chrono::minutes addedTime = parseAddedTime(clock["displayValue"].toString());
    const domain::model::GoalType type = parseGoalType(keyEvent["type"]["text"].toString());
    const std::optional<domain::model::Player> scorer =
        parseScorer(keyEvent["participants"].toArray());

    return domain::model::GoalEvent{
        .match = match,
        .team = domain::model::TeamId{*team},
        .period = *period,
        .clock = *elapsed,
        .addedTime = addedTime,
        .type = type,
        .scorer = scorer,
    };
}

struct FinalScore {
    domain::model::TeamId home;
    int homeGoals;
    domain::model::TeamId away;
    int awayGoals;
};

std::expected<FinalScore, domain::Error> parseFinalScore(const QJsonObject& header) {
    const auto invalid = [](const char* reason) {
        return std::unexpected(domain::Error{domain::ErrorCode::InvalidResponse, reason});
    };

    const QJsonArray competitions = header["competitions"].toArray();
    if (competitions.isEmpty()) {
        return invalid("summary without competitions");
    }

    std::optional<std::pair<domain::model::TeamId, int>> home;
    std::optional<std::pair<domain::model::TeamId, int>> away;

    for (const QJsonValue& value : competitions.first()["competitors"].toArray()) {
        const QJsonObject competitor = value.toObject();
        const auto id = parseId(competitor["team"]["id"]);
        bool ok = false;

        const int goals = competitor["score"].toString().toInt(&ok);
        if (!id || !ok) {
            return invalid("summary competitor without team or score");
        }

        const auto side = std::pair{domain::model::TeamId{*id}, goals};
        if (competitor["homeAway"].toString() == "home") {
            home = side;
        } else {
            away = side;
        }
    }
    if (!home || !away) {
        return invalid("summary without both teams");
    }

    return FinalScore{home->first, home->second, away->first, away->second};
}

std::expected<void, domain::Error>
verifyGoalsMatchScore(const std::vector<domain::model::GoalEvent>& goals, const FinalScore& score) {
    const auto goalsOf = [&goals](domain::model::TeamId team) {
        return static_cast<int>(std::ranges::count_if(
            goals, [team](const domain::model::GoalEvent& goal) { return goal.team == team; }));
    };

    const int homeGoals = goalsOf(score.home);
    const int awayGoals = goalsOf(score.away);
    const int totalGoals = static_cast<int>(goals.size());

    if (homeGoals != score.homeGoals || awayGoals != score.awayGoals ||
        totalGoals != score.homeGoals + score.awayGoals) {
        return std::unexpected(domain::Error{
            domain::ErrorCode::InvalidResponse,
            "goals don't match the final score: events " + std::to_string(homeGoals) + ":" +
                std::to_string(awayGoals) + " (" + std::to_string(totalGoals) + " total), score " +
                std::to_string(score.homeGoals) + ":" + std::to_string(score.awayGoals)});
    }

    return {};
}

std::expected<QJsonObject, domain::Error> parseRoot(const QByteArray& json) {
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return std::unexpected(
            domain::Error{domain::ErrorCode::InvalidResponse, "response is not a JSON object"});
    }

    QJsonObject root = document.object();
    if (root["code"].isDouble() && root.contains("message")) {
        const auto code = root["code"].toInt() == kNotFoundCode
                              ? domain::ErrorCode::NotFound
                              : domain::ErrorCode::InvalidResponse;
        return std::unexpected(domain::Error{code, root["message"].toString().toStdString()});
    }

    return root;
}

} // namespace

std::expected<std::vector<domain::model::Match>, domain::Error>
parseSchedule(const QByteArray& json, domain::model::CompetitionId competition) {
    const auto parsedRoot = parseRoot(json);
    if (!parsedRoot) {
        return std::unexpected(parsedRoot.error());
    }

    const QJsonObject& root = *parsedRoot;

    if (!root["events"].isArray()) {
        return std::unexpected(
            domain::Error{domain::ErrorCode::InvalidResponse, "schedule without events"});
    }

    const QJsonArray events = root["events"].toArray();

    std::vector<domain::model::Match> matches{};
    matches.reserve(static_cast<std::size_t>(events.size()));

    for (const QJsonValue& value : events) {
        auto match = parseEvent(value.toObject(), competition);
        if (!match) {
            qWarning() << "skipping ESPN event:" << match.error().message.c_str();
            continue;
        }
        matches.push_back(std::move(*match)); // emplace_back will do same things because object
    }
    return matches;
}

std::expected<std::vector<domain::model::GoalEvent>, domain::Error>
parseSummary(const QByteArray& json) {
    const auto parsedRoot = parseRoot(json);
    if (!parsedRoot) {
        return std::unexpected(parsedRoot.error());
    }
    const QJsonObject& root = *parsedRoot;

    const auto matchId = parseId(root["header"]["id"]);
    if (!matchId) {
        return std::unexpected(
            domain::Error{domain::ErrorCode::InvalidResponse, "summary without a match id"});
    }

    const auto finalScore = parseFinalScore(root["header"].toObject());
    if (!finalScore) {
        return std::unexpected(finalScore.error());
    }

    std::vector<domain::model::GoalEvent> goals;
    for (const QJsonValue& value : root["keyEvents"].toArray()) {
        const QJsonObject keyEvent = value.toObject();

        if (!keyEvent["scoringPlay"].toBool()) {
            continue;
        }
        if (keyEvent["period"]["number"].toInt() == kShootoutPeriod) {
            continue;
        }

        auto goal = parseGoal(keyEvent, domain::model::MatchId{*matchId});
        // A missing goal would silently corrupt statistics, so reject the whole summary
        if (!goal) {
            return std::unexpected(goal.error());
        }

        goals.push_back(std::move(*goal));
    }

    if (const auto verified = verifyGoalsMatchScore(goals, *finalScore); !verified) {
        return std::unexpected(verified.error());
    }

    return goals;
}

}}} // namespace madridista::infrastructure::espn