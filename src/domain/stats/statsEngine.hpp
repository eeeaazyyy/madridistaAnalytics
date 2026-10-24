#ifndef gjf_423igdhfgl43yg5734238457f923jfhsdf
#define gjf_423igdhfgl43yg5734238457f923jfhsdf

#include "domain/model/goalEvent.hpp"
#include "domain/model/match.hpp"
#include "domain/stats/statsQuery.hpp"

#include <array>
#include <span>
#include <vector>

namespace madridista { namespace domain { namespace stats {

inline constexpr int kRegularTimeBuckets = 6;

struct StatsResult {
    int matchCount{};
    std::array<int, kRegularTimeBuckets> buckets{};
    std::array<int, 2> halves{}; // goals in first and second half

    bool operator==(const StatsResult&) const = default;
};

[[nodiscard]] int totalGoalsScored(model::TeamId teamId, std::span<const model::Match> matches);

[[nodiscard]] std::optional<int> bucketOf(const model::GoalEvent& goal);

[[nodiscard]] bool matchesQuery(const model::Match& match, const MatchFilter& filter);

[[nodiscard]] std::vector<model::MatchId> selectMatches(const MatchFilter& filter,
                                                        std::span<const model::Match> matches);

[[nodiscard]] StatsResult computeGoalStats(const GoalsByMinuteQuery& query,
                                           std::span<const model::Match> matches,
                                           std::span<const model::GoalEvent> goals);

}}} // namespace madridista::domain::stats

#endif