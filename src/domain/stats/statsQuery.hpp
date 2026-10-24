#ifndef fdoisjdfuhaloanivbjwpifjeuthuidiasf
#define fdoisjdfuhaloanivbjwpifjeuthuidiasf

#include "domain/model/goalEvent.hpp"
#include "domain/model/match.hpp"
#include "domain/model/season.hpp"

#include <optional>
#include <vector>

namespace madridista { namespace domain { namespace stats {

enum class Metric {
    GoalsScored,
    GoalsConceded,
};

enum class Venue {
    All,
    Home,
    Away,
};

struct MatchFilter {
    model::TeamId team;
    Venue venue = Venue::All;
    std::optional<model::CompetitionId> competition{};

    std::vector<model::Season> seasons{};
    std::optional<int> lastN{};

    bool operator==(const MatchFilter&) const = default;
};

struct GoalsByMinuteQuery {
    MatchFilter matches;
    Metric metric = Metric::GoalsScored;
    std::optional<model::PlayerId> scorer{};

    bool operator==(const GoalsByMinuteQuery&) const = default;
};

}}} // namespace madridista::domain::stats

#endif