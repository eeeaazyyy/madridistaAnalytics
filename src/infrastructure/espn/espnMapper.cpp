#include "espnMapper.hpp"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLatin1StringView>
#include <QString>
#include <QtDebug>

#include <chrono>
#include <cstdint>
#include <optional>
#include <utility>

namespace madridista { namespace infrastructure { namespace espn {

namespace {

constexpr int kNotFoundCode = 404;

constexpr QLatin1StringView kStatusPostponed{"STATUS_POSTPONED"};
constexpr QLatin1StringView kStatusCanceled{"STATUS_CANCELED"};
constexpr QLatin1StringView kStatusAbandoned{"STATUS_ABANDONED"};

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

} // namespace

std::expected<std::vector<domain::model::Match>, domain::Error>
parseSchedule(const QByteArray& json, domain::model::CompetitionId competition) {
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return std::unexpected(
            domain::Error{domain::ErrorCode::InvalidResponse, "schedule is not a JSON object"});
    }

    const QJsonObject root = document.object();
    if (root.contains("code") && !root.contains("events")) {
        const auto code = root["code"].toInt() == kNotFoundCode
                              ? domain::ErrorCode::NotFound
                              : domain::ErrorCode::InvalidResponse;
        return std::unexpected(domain::Error{code, root["message"].toString().toStdString()});
    }

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
    Q_UNUSED(json)

    return std::unexpected(
        domain::Error{domain::ErrorCode::InvalidResponse, "not implemented yet"});
}

}}} // namespace madridista::infrastructure::espn