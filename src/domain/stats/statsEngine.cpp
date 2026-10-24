#include "domain/stats/statsEngine.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <functional>
#include <ranges>
#include <set>
#include <vector>

namespace madridista { namespace domain { namespace stats {

namespace {

constexpr int kBucketMinutes = 15;

constexpr int kFirstHalfFirstBucket = 0;
constexpr int kFirstHalfLastBucket = 2;
constexpr int kSecondHalfFirstBucket = 3;
constexpr int kSecondHalfLastBucket = 5;

[[nodiscard]] int goalsScoredIn(const model::Match& match, model::TeamId teamId) {
    const auto& score = match.score();
    if (!score) {
        return 0;
    }
    if (match.homeTeam().id == teamId) {
        return score->home;
    }
    if (match.awayTeam().id == teamId) {
        return score->away;
    }

    return 0;
}

bool countsForQuery(const model::GoalEvent& goal, const GoalsByMinuteQuery& query) {
    const bool byTeam = goal.team == query.matches.team;
    if (query.metric == Metric::GoalsScored && !byTeam) {
        return false;
    }

    if (query.metric == Metric::GoalsConceded && byTeam) {
        return false;
    }

    if (query.scorer && (!goal.scorer || goal.scorer->id != *query.scorer)) {
        return false;
    }

    return true;
}

} // namespace

int totalGoalsScored(model::TeamId teamId, std::span<const model::Match> matches) {
    auto goals = matches | std::views::transform([teamId](const model::Match& match) {
                     return goalsScoredIn(match, teamId);
                 });

    return std::ranges::fold_left(goals, 0, std::plus<>{});
}

std::optional<int> bucketOf(const model::GoalEvent& goal) {
    const auto elapsedMinutes =
        std::chrono::duration_cast<std::chrono::minutes>(goal.clock).count();
    const int bucket = static_cast<int>(elapsedMinutes / kBucketMinutes);

    switch (goal.period) {
    case model::Period::FirstHalf:
        return std::clamp(bucket, kFirstHalfFirstBucket, kFirstHalfLastBucket);
    case model::Period::SecondHalf:
        return std::clamp(bucket, kSecondHalfFirstBucket, kSecondHalfLastBucket);
    case model::Period::ExtraTimeFirstHalf:
    case model::Period::ExtraTimeSecondHalf:
        return std::nullopt;
    }

    return std::nullopt;
}

bool matchesQuery(const model::Match& match, const MatchFilter& filter) {
    if (match.status() != model::MatchStatus::Finished) {
        return false;
    }

    if (filter.competition.has_value() && match.competition() != *filter.competition) {
        return false;
    }

    switch (filter.venue) {
    case Venue::Home:
        if (match.homeTeam().id != filter.team) {
            return false;
        }
        break;
    case Venue::Away:
        if (match.awayTeam().id != filter.team) {
            return false;
        }
        break;
    case Venue::All:
        if (match.homeTeam().id != filter.team && match.awayTeam().id != filter.team) {
            return false;
        }
        break;
    }

    if (!filter.seasons.empty() &&
        !std::ranges::contains(filter.seasons, model::seasonOf(match.kickoff()))) {
        return false;
    }

    return true;
}

std::vector<model::MatchId> selectMatches(const MatchFilter& filter,
                                          std::span<const model::Match> matches) {
    std::vector<const model::Match*> selected;
    for (const model::Match& match : matches) {
        if (matchesQuery(match, filter)) {
            selected.push_back(&match);
        }
    }

    std::ranges::sort(selected, std::ranges::greater{}, &model::Match::kickoff);

    if (filter.lastN) {
        const auto limit = static_cast<std::size_t>(std::max(*filter.lastN, 0));
        if (limit < selected.size()) {
            selected.resize(limit);
        }
    }

    std::vector<model::MatchId> ids;
    ids.reserve(selected.size());
    for (const model::Match* match : selected) {
        ids.push_back(match->id());
    }

    return ids;
}

StatsResult computeGoalStats(const GoalsByMinuteQuery& query, std::span<const model::Match> matches,
                             std::span<const model::GoalEvent> goals) {
    const auto ids = selectMatches(query.matches, matches);

    const std::set<model::MatchId> selected(ids.begin(), ids.end());

    StatsResult result{.matchCount = static_cast<int>(ids.size())};

    for (const model::GoalEvent& goal : goals) {
        if (!selected.contains(goal.match) || !countsForQuery(goal, query)) {
            continue;
        }

        const auto bucket = bucketOf(goal);
        if (!bucket) {
            continue;
        }

        result.buckets[static_cast<std::size_t>(*bucket)] += 1;
        result.halves[goal.period == model::Period::FirstHalf ? 0 : 1] += 1;
    }

    return result;
}

}}} // namespace madridista::domain::stats