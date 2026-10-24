#ifndef gjf_423igdhfgl43yg5734238457f923jfhsdf
#define gjf_423igdhfgl43yg5734238457f923jfhsdf

#include "domain/model/goalEvent.hpp"
#include "domain/model/match.hpp"
#include "domain/stats/statsQuery.hpp"

#include <span>

namespace madridista { namespace domain { namespace stats {

[[nodiscard]] int totalGoalsScored(model::TeamId teamId, std::span<const model::Match> matches);

[[nodiscard]] std::optional<int> bucketOf(const model::GoalEvent& goal);

[[nodiscard]] bool matchesQuery(const model::Match& match, const MatchFilter& filter);

}}} // namespace madridista::domain::stats

#endif