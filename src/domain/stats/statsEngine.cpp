#include "domain/stats/statsEngine.hpp"

#include <algorithm>
#include <functional>
#include <ranges>

#include <chrono>

namespace madridista { namespace domain { namespace stats {

namespace {

inline constexpr int kBucketMinutes = 15;
inline constexpr int kRegularTimeBuckets = 6;

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

}}} // namespace madridista::domain::stats