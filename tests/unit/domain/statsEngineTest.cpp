#include "domain/stats/statsEngine.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <vector>

using namespace madridista::domain;

namespace {

constexpr model::TeamId idRealMadrid {541};

//Finished match with score
model::Match finishedMatch(std::int64_t homeId, std::int64_t awayId,
                        int homeGoals, int awayGoals) {
    static std::int64_t nextId = 1;
    return model::Match::create({
        .id = model::MatchId{nextId++},
        .kickoff = std::chrono::sys_days{std::chrono::year{2025}/8/18},
        .competition = model::CompetitionId{140},
        .homeTeam = model::Team{model::TeamId{homeId}, ""},
        .awayTeam = model::Team{model::TeamId{awayId}, ""},
        .status = model::MatchStatus::Finished,
        .score = model::Score{homeGoals, awayGoals},
    }).value();
}

//Schedule Match
model::Match scheduleMatch(std::int64_t homeId, std::int64_t awayId) {
    return model::Match::create({
        .id = model::MatchId{999},
        .kickoff = std::chrono::sys_days{std::chrono::year{2026}/10/25},
        .competition = model::CompetitionId{140},
        .homeTeam = model::Team{model::TeamId{homeId}, ""},
        .awayTeam = model::Team{model::TeamId{awayId}, ""},
    }).value();
}

} //namespace



TEST(StatsEngineTest, EmptyListGivesZero) {
    const std::vector<model::Match> matches;
    EXPECT_EQ(stats::totalGoalsScored(idRealMadrid, matches), 0);
}

TEST(StatsEngineTest, CountGoalsAtHomeAndAway) {
    const std::vector matches = {
        finishedMatch(541, 798, 2, 1),
        finishedMatch(529, 541, 0, 3),
        finishedMatch(541, 530, 1, 0),
    };

    EXPECT_EQ(stats::totalGoalsScored(idRealMadrid, matches), 6);
}

