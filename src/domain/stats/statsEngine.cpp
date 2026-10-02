#include "domain/stats/statsEngine.hpp"

#include <algorithm>
#include <functional>
#include <ranges>

namespace madridista { namespace domain { namespace stats {

[[nodiscard]] int goalsScoredIn(const model::Match &match, model::TeamId teamId) {
    const auto &score = match.score();
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

int totalGoalsScored(model::TeamId teamId,
            std::span<const model::Match> matches) {
    auto goals = matches | std::views::transform([teamId](const model::Match &match) {
        return goalsScoredIn(match, teamId);
    });

    return std::ranges::fold_left(goals, 0, std::plus<>{});
}

}}}