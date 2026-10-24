#include "domain/stats/statsEngine.hpp"

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

using namespace madridista::domain;

namespace {

constexpr model::TeamId idRealMadrid{541};

// Finished match with score
model::Match finishedMatch(std::int64_t homeId, std::int64_t awayId, int homeGoals, int awayGoals) {
    static std::int64_t nextId = 1;
    return model::Match::create(
               {
                   .id = model::MatchId{nextId++},
                   .kickoff = std::chrono::sys_days{std::chrono::year{2025} / 8 / 18},
                   .competition = model::CompetitionId{140},
                   .homeTeam = model::Team{model::TeamId{homeId}, ""},
                   .awayTeam = model::Team{model::TeamId{awayId}, ""},
                   .status = model::MatchStatus::Finished,
                   .score = model::Score{homeGoals, awayGoals},
               })
        .value();
}

// Schedule Match
model::Match scheduledMatch(std::int64_t homeId, std::int64_t awayId) {
    return model::Match::create(
               {
                   .id = model::MatchId{999},
                   .kickoff = std::chrono::sys_days{std::chrono::year{2026} / 10 / 25},
                   .competition = model::CompetitionId{140},
                   .homeTeam = model::Team{model::TeamId{homeId}, ""},
                   .awayTeam = model::Team{model::TeamId{awayId}, ""},
               })
        .value();
}
// Goal at the given period and second; other fields don't matter for bucketOf
model::GoalEvent goalAt(model::Period period, std::chrono::seconds clock) {
    return {
        .match = model::MatchId{1},
        .team = idRealMadrid,
        .period = period,
        .clock = clock,
        .addedTime = std::chrono::minutes{0},
        .type = model::GoalType::Regular,
        .scorer = std::nullopt,
    };
}

using std::chrono::seconds;

} // namespace

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

TEST(StatsEngineTest, IgnoresMatchesOfOtherTeams) {
    const std::vector matches = {
        finishedMatch(541, 798, 2, 1),
        finishedMatch(529, 530, 4, 4),
    };
    EXPECT_EQ(stats::totalGoalsScored(idRealMadrid, matches), 2);
}

TEST(StatsEngineTest, GoallessDrawGivesZero) {
    const std::vector matches = {finishedMatch(541, 798, 0, 0)};
    EXPECT_EQ(stats::totalGoalsScored(idRealMadrid, matches), 0);
}

TEST(StatsEngineTest, IgnoresMatchesWithoutScore) {
    const std::vector matches = {
        finishedMatch(541, 798, 2, 1),
        scheduledMatch(541, 529),
    };
    EXPECT_EQ(stats::totalGoalsScored(idRealMadrid, matches), 2);
}
TEST(BucketOfTest, EarlyGoalGoesToFirstBucket) {
    // 8:56 → 9th minute → 0–15
    EXPECT_EQ(stats::bucketOf(goalAt(model::Period::FirstHalf, seconds{536})), 0);
}

TEST(BucketOfTest, LastSecondOfFirstBucketStaysInFirstBucket) {
    // 14:59 → 15th minute → 0–15
    EXPECT_EQ(stats::bucketOf(goalAt(model::Period::FirstHalf, seconds{899})), 0);
}

TEST(BucketOfTest, FifteenMinutesExactlyStartsSecondBucket) {
    // 15:00 → 16th minute → 16–30
    EXPECT_EQ(stats::bucketOf(goalAt(model::Period::FirstHalf, seconds{900})), 1);
}

TEST(BucketOfTest, EndOfFirstHalfGoesToThirdBucket) {
    // 44:59 → 45th minute → 31–45+
    EXPECT_EQ(stats::bucketOf(goalAt(model::Period::FirstHalf, seconds{2699})), 2);
}

TEST(BucketOfTest, FirstHalfStoppageTimeStaysInFirstHalf) {
    // 45'+1': ESPN caps the clock at 45:00, but the goal was scored in the first half → 31–45+
    EXPECT_EQ(stats::bucketOf(goalAt(model::Period::FirstHalf, seconds{2700})), 2);
}

TEST(BucketOfTest, StartOfSecondHalfGoesToFourthBucket) {
    // 45:00 in the second half → 46th minute → 46–60
    EXPECT_EQ(stats::bucketOf(goalAt(model::Period::SecondHalf, seconds{2700})), 3);
}

TEST(BucketOfTest, EndOfSecondHalfGoesToLastBucket) {
    // 89:54 → 90th minute → 76–90+
    EXPECT_EQ(stats::bucketOf(goalAt(model::Period::SecondHalf, seconds{5394})), 5);
}

TEST(BucketOfTest, SecondHalfStoppageTimeStaysInLastBucket) {
    // 90'+4': the clock is capped at 90:00 → 76–90+, not a non-existent bucket 6
    EXPECT_EQ(stats::bucketOf(goalAt(model::Period::SecondHalf, seconds{5400})), 5);
}

TEST(BucketOfTest, ExtraTimeGoalsHaveNoBucket) {
    EXPECT_FALSE(
        stats::bucketOf(goalAt(model::Period::ExtraTimeFirstHalf, seconds{5700})).has_value());
    EXPECT_FALSE(
        stats::bucketOf(goalAt(model::Period::ExtraTimeSecondHalf, seconds{7000})).has_value());
}

TEST(BucketOfTest, BucketIsAlwaysInsideRegularTimeRange) {
    // Every regular-time goal falls into one of the six buckets
    for (int second = 0; second <= 5400; second += 30) {
        const auto period = second < 2700 ? model::Period::FirstHalf : model::Period::SecondHalf;
        const auto bucket = stats::bucketOf(goalAt(period, seconds{second}));
        ASSERT_TRUE(bucket.has_value());
        EXPECT_GE(*bucket, 0);
        EXPECT_LT(*bucket, stats::kRegularTimeBuckets);
    }
}

namespace {

constexpr model::CompetitionId kLaLiga{140};
constexpr model::CompetitionId kChampionsLeague{2};

// Finished 1:0 match with explicit competition and kickoff date
model::Match matchOn(std::int64_t homeId, std::int64_t awayId, model::CompetitionId competition,
                     std::chrono::sys_days day) {
    static std::int64_t nextId = 1000;
    return model::Match::create({
                                    .id = model::MatchId{nextId++},
                                    .kickoff = day,
                                    .competition = competition,
                                    .homeTeam = model::Team{model::TeamId{homeId}, ""},
                                    .awayTeam = model::Team{model::TeamId{awayId}, ""},
                                    .status = model::MatchStatus::Finished,
                                    .score = model::Score{1, 0},
                                })
        .value();
}

constexpr std::chrono::sys_days kSeptember2026 =
    std::chrono::sys_days{std::chrono::year{2026} / 9 / 20};

} // namespace

TEST(MatchesQueryTest, EmptyFilterAcceptsAnyFinishedMatchOfTheTeam) {
    const stats::MatchFilter filter{.team = idRealMadrid};

    EXPECT_TRUE(stats::matchesQuery(matchOn(541, 798, kLaLiga, kSeptember2026), filter));
    EXPECT_TRUE(stats::matchesQuery(matchOn(529, 541, kChampionsLeague, kSeptember2026), filter));
}

TEST(MatchesQueryTest, RejectsMatchThatIsNotFinished) {
    const stats::MatchFilter filter{.team = idRealMadrid};

    EXPECT_FALSE(stats::matchesQuery(scheduledMatch(541, 798), filter));
}

TEST(MatchesQueryTest, RejectsMatchOfOtherTeams) {
    const stats::MatchFilter filter{.team = idRealMadrid};

    EXPECT_FALSE(stats::matchesQuery(matchOn(529, 530, kLaLiga, kSeptember2026), filter));
}

TEST(MatchesQueryTest, HomeVenueAcceptsOnlyHomeMatches) {
    const stats::MatchFilter filter{.team = idRealMadrid, .venue = stats::Venue::Home};

    EXPECT_TRUE(stats::matchesQuery(matchOn(541, 798, kLaLiga, kSeptember2026), filter));
    EXPECT_FALSE(stats::matchesQuery(matchOn(798, 541, kLaLiga, kSeptember2026), filter));
}

TEST(MatchesQueryTest, AwayVenueAcceptsOnlyAwayMatches) {
    const stats::MatchFilter filter{.team = idRealMadrid, .venue = stats::Venue::Away};

    EXPECT_TRUE(stats::matchesQuery(matchOn(798, 541, kLaLiga, kSeptember2026), filter));
    EXPECT_FALSE(stats::matchesQuery(matchOn(541, 798, kLaLiga, kSeptember2026), filter));
}

TEST(MatchesQueryTest, CompetitionFilterRejectsOtherCompetitions) {
    const stats::MatchFilter filter{.team = idRealMadrid, .competition = kChampionsLeague};

    EXPECT_TRUE(stats::matchesQuery(matchOn(541, 798, kChampionsLeague, kSeptember2026), filter));
    EXPECT_FALSE(stats::matchesQuery(matchOn(541, 798, kLaLiga, kSeptember2026), filter));
}

TEST(MatchesQueryTest, SeasonFilterAcceptsOnlyListedSeasons) {
    const stats::MatchFilter filter{.team = idRealMadrid, .seasons = {model::Season{2026}}};
    const auto march2026 = std::chrono::sys_days{std::chrono::year{2026} / 3 / 14}; // season 2025

    EXPECT_TRUE(stats::matchesQuery(matchOn(541, 798, kLaLiga, kSeptember2026), filter));
    EXPECT_FALSE(stats::matchesQuery(matchOn(541, 798, kLaLiga, march2026), filter));
}

// ---------- selectMatches ----------

namespace {

std::chrono::sys_days dayOf(int y, unsigned m, unsigned d) {
    return std::chrono::sys_days{std::chrono::year{y} / std::chrono::month{m} /
                                 std::chrono::day{d}};
}

} // namespace

TEST(SelectMatchesTest, KeepsOnlyMatchesThatPassTheFilter) {
    const std::vector matches = {
        matchOn(541, 798, kLaLiga, dayOf(2026, 9, 1)),          // Real at home
        matchOn(529, 530, kLaLiga, dayOf(2026, 9, 2)),          // other teams
        matchOn(798, 541, kChampionsLeague, dayOf(2026, 9, 3)), // Real away
    };
    const stats::MatchFilter filter{.team = idRealMadrid};

    const auto selected = stats::selectMatches(filter, matches);

    ASSERT_EQ(selected.size(), 2u);
    EXPECT_EQ(selected[0], matches[2].id()); // newest first
    EXPECT_EQ(selected[1], matches[0].id());
}

TEST(SelectMatchesTest, ReturnsNewestMatchesFirst) {
    const std::vector matches = {
        matchOn(541, 798, kLaLiga, dayOf(2026, 9, 20)),
        matchOn(541, 798, kLaLiga, dayOf(2026, 8, 16)),
        matchOn(541, 798, kLaLiga, dayOf(2026, 10, 4)),
    };
    const stats::MatchFilter filter{.team = idRealMadrid};

    const auto selected = stats::selectMatches(filter, matches);

    ASSERT_EQ(selected.size(), 3u);
    EXPECT_EQ(selected[0], matches[2].id()); // 4 October
    EXPECT_EQ(selected[1], matches[0].id()); // 20 September
    EXPECT_EQ(selected[2], matches[1].id()); // 16 August
}

TEST(SelectMatchesTest, LastNKeepsOnlyMostRecentMatches) {
    const std::vector matches = {
        matchOn(541, 798, kLaLiga, dayOf(2026, 8, 16)),
        matchOn(541, 798, kLaLiga, dayOf(2026, 8, 23)),
        matchOn(541, 798, kLaLiga, dayOf(2026, 8, 30)),
        matchOn(541, 798, kLaLiga, dayOf(2026, 9, 6)),
        matchOn(541, 798, kLaLiga, dayOf(2026, 9, 13)),
    };
    const stats::MatchFilter filter{.team = idRealMadrid, .lastN = 2};

    const auto selected = stats::selectMatches(filter, matches);

    ASSERT_EQ(selected.size(), 2u);
    EXPECT_EQ(selected[0], matches[4].id());
    EXPECT_EQ(selected[1], matches[3].id());
}

TEST(SelectMatchesTest, LastNLargerThanMatchCountReturnsAll) {
    const std::vector matches = {
        matchOn(541, 798, kLaLiga, dayOf(2026, 8, 16)),
        matchOn(541, 798, kLaLiga, dayOf(2026, 8, 23)),
    };
    const stats::MatchFilter filter{.team = idRealMadrid, .lastN = 10};

    EXPECT_EQ(stats::selectMatches(filter, matches).size(), 2u);
}

TEST(SelectMatchesTest, LastNCountsOnlyFilteredMatches) {
    // The two newest matches are in the Champions League; lastN applies after the competition
    // filter
    const std::vector matches = {
        matchOn(541, 798, kLaLiga, dayOf(2026, 8, 16)),
        matchOn(541, 798, kLaLiga, dayOf(2026, 8, 23)),
        matchOn(541, 798, kChampionsLeague, dayOf(2026, 9, 16)),
        matchOn(541, 798, kChampionsLeague, dayOf(2026, 9, 30)),
    };
    const stats::MatchFilter filter{.team = idRealMadrid, .competition = kLaLiga, .lastN = 1};

    const auto selected = stats::selectMatches(filter, matches);

    ASSERT_EQ(selected.size(), 1u);
    EXPECT_EQ(selected[0], matches[1].id()); // newest La Liga match, not a Champions League one
}

TEST(SelectMatchesTest, NoMatchesGiveEmptyResult) {
    const stats::MatchFilter filter{.team = idRealMadrid, .lastN = 5};

    EXPECT_TRUE(stats::selectMatches(filter, {}).empty());
}

TEST(SelectMatchesTest, NonPositiveLastNSelectsNothing) {
    const std::vector matches = {matchOn(541, 798, kLaLiga, dayOf(2026, 8, 16))};

    EXPECT_TRUE(stats::selectMatches({.team = idRealMadrid, .lastN = 0}, matches).empty());
    EXPECT_TRUE(stats::selectMatches({.team = idRealMadrid, .lastN = -3}, matches).empty());
}

// ---------- computeGoalStats ----------

namespace {

constexpr model::PlayerId kGuler{380286};
constexpr model::PlayerId kBellingham{291281};

// Goal in a concrete match: team it is credited to, period and second of the match clock
model::GoalEvent goalIn(const model::Match& match, model::TeamId team, model::Period period,
                        std::chrono::seconds clock,
                        std::optional<model::PlayerId> scorer = std::nullopt,
                        model::GoalType type = model::GoalType::Regular) {
    std::optional<model::Player> player;
    if (scorer) {
        player = model::Player{*scorer, ""};
    }
    return {
        .match = match.id(),
        .team = team,
        .period = period,
        .clock = clock,
        .addedTime = std::chrono::minutes{0},
        .type = type,
        .scorer = player,
    };
}

constexpr model::TeamId kAthletic{93};

} // namespace

TEST(ComputeGoalStatsTest, NoMatchesGiveZeroes) {
    const stats::GoalsByMinuteQuery query{.matches = {.team = idRealMadrid}};

    const auto result = stats::computeGoalStats(query, {}, {});

    EXPECT_EQ(result, stats::StatsResult{});
}

TEST(ComputeGoalStatsTest, CountsGoalsByBucketsAndHalves) {
    const std::vector matches = {matchOn(541, 93, kLaLiga, dayOf(2026, 9, 20))};
    const std::vector goals = {
        goalIn(matches[0], idRealMadrid, model::Period::FirstHalf, seconds{536}),   // 9'    -> 0
        goalIn(matches[0], idRealMadrid, model::Period::FirstHalf, seconds{2700}),  // 45'+1 -> 2
        goalIn(matches[0], idRealMadrid, model::Period::SecondHalf, seconds{3000}), // 51'   -> 3
        goalIn(matches[0], idRealMadrid, model::Period::SecondHalf, seconds{5400}), // 90'+3 -> 5
    };
    const stats::GoalsByMinuteQuery query{.matches = {.team = idRealMadrid}};

    const auto result = stats::computeGoalStats(query, matches, goals);

    EXPECT_EQ(result.matchCount, 1);
    EXPECT_EQ(result.buckets, (std::array{1, 0, 1, 1, 0, 1}));
    EXPECT_EQ(result.halves, (std::array{2, 2}));
}

TEST(ComputeGoalStatsTest, ScoredIgnoresOpponentGoals) {
    const std::vector matches = {matchOn(541, 93, kLaLiga, dayOf(2026, 9, 20))};
    const std::vector goals = {
        goalIn(matches[0], idRealMadrid, model::Period::FirstHalf, seconds{536}),
        goalIn(matches[0], kAthletic, model::Period::FirstHalf, seconds{1200}),
    };
    const stats::GoalsByMinuteQuery query{.matches = {.team = idRealMadrid}};

    const auto result = stats::computeGoalStats(query, matches, goals);

    EXPECT_EQ(result.buckets, (std::array{1, 0, 0, 0, 0, 0}));
}

TEST(ComputeGoalStatsTest, ConcededCountsOnlyOpponentGoals) {
    const std::vector matches = {matchOn(541, 93, kLaLiga, dayOf(2026, 9, 20))};
    const std::vector goals = {
        goalIn(matches[0], idRealMadrid, model::Period::FirstHalf, seconds{536}),
        goalIn(matches[0], kAthletic, model::Period::FirstHalf, seconds{1200}), // 21' -> 1
    };
    const stats::GoalsByMinuteQuery query{.matches = {.team = idRealMadrid},
                                          .metric = stats::Metric::GoalsConceded};

    const auto result = stats::computeGoalStats(query, matches, goals);

    EXPECT_EQ(result.buckets, (std::array{0, 1, 0, 0, 0, 0}));
}

TEST(ComputeGoalStatsTest, OpponentOwnGoalCountsAsScored) {
    // ESPN credits an own goal to the benefiting team
    const std::vector matches = {matchOn(541, 93, kLaLiga, dayOf(2026, 9, 20))};
    const std::vector goals = {goalIn(matches[0], idRealMadrid, model::Period::SecondHalf,
                                      seconds{4000}, std::nullopt, model::GoalType::OwnGoal)};
    const stats::GoalsByMinuteQuery query{.matches = {.team = idRealMadrid}};

    const auto result = stats::computeGoalStats(query, matches, goals);

    EXPECT_EQ(result.halves, (std::array{0, 1}));
}

TEST(ComputeGoalStatsTest, IgnoresGoalsOfMatchesOutsideTheFilter) {
    const std::vector matches = {
        matchOn(541, 93, kLaLiga, dayOf(2026, 9, 20)),
        matchOn(541, 93, kChampionsLeague, dayOf(2026, 9, 30)),
    };
    const std::vector goals = {
        goalIn(matches[0], idRealMadrid, model::Period::FirstHalf, seconds{536}),
        goalIn(matches[1], idRealMadrid, model::Period::FirstHalf, seconds{536}),
    };
    const stats::GoalsByMinuteQuery query{
        .matches = {.team = idRealMadrid, .competition = kLaLiga}};

    const auto result = stats::computeGoalStats(query, matches, goals);

    EXPECT_EQ(result.matchCount, 1);
    EXPECT_EQ(result.buckets[0], 1);
}

TEST(ComputeGoalStatsTest, ExtraTimeGoalsAreNotCounted) {
    const std::vector matches = {matchOn(541, 93, kChampionsLeague, dayOf(2026, 9, 30))};
    const std::vector goals = {
        goalIn(matches[0], idRealMadrid, model::Period::ExtraTimeFirstHalf, seconds{5700})};
    const stats::GoalsByMinuteQuery query{.matches = {.team = idRealMadrid}};

    const auto result = stats::computeGoalStats(query, matches, goals);

    EXPECT_EQ(result.matchCount, 1);
    EXPECT_EQ(result.buckets, (std::array{0, 0, 0, 0, 0, 0}));
    EXPECT_EQ(result.halves, (std::array{0, 0}));
}

TEST(ComputeGoalStatsTest, ScorerFilterKeepsOnlyThatPlayersGoals) {
    const std::vector matches = {matchOn(541, 93, kLaLiga, dayOf(2026, 9, 20))};
    const std::vector goals = {
        goalIn(matches[0], idRealMadrid, model::Period::FirstHalf, seconds{536}, kBellingham),
        goalIn(matches[0], idRealMadrid, model::Period::SecondHalf, seconds{5300}, kGuler),
        goalIn(matches[0], idRealMadrid, model::Period::SecondHalf, seconds{5390}), // no scorer
    };
    const stats::GoalsByMinuteQuery query{.matches = {.team = idRealMadrid}, .scorer = kGuler};

    const auto result = stats::computeGoalStats(query, matches, goals);

    EXPECT_EQ(result.buckets, (std::array{0, 0, 0, 0, 0, 1}));
}

TEST(ComputeGoalStatsTest, LastNLimitsMatchesAndTheirGoals) {
    const std::vector matches = {
        matchOn(541, 93, kLaLiga, dayOf(2026, 8, 16)),
        matchOn(541, 93, kLaLiga, dayOf(2026, 9, 20)),
    };
    const std::vector goals = {
        goalIn(matches[0], idRealMadrid, model::Period::FirstHalf, seconds{536}), // older match
        goalIn(matches[1], idRealMadrid, model::Period::SecondHalf, seconds{5300}),
    };
    const stats::GoalsByMinuteQuery query{.matches = {.team = idRealMadrid, .lastN = 1}};

    const auto result = stats::computeGoalStats(query, matches, goals);

    EXPECT_EQ(result.matchCount, 1);
    EXPECT_EQ(result.buckets, (std::array{0, 0, 0, 0, 0, 1}));
}

TEST(ComputeGoalStatsTest, GoallessMatchesStillCountInMatchCount) {
    const std::vector matches = {
        matchOn(541, 93, kLaLiga, dayOf(2026, 8, 16)),
        matchOn(541, 93, kLaLiga, dayOf(2026, 9, 20)),
    };
    const std::vector goals = {
        goalIn(matches[1], idRealMadrid, model::Period::FirstHalf, seconds{536})};
    const stats::GoalsByMinuteQuery query{.matches = {.team = idRealMadrid}};

    const auto result = stats::computeGoalStats(query, matches, goals);

    EXPECT_EQ(result.matchCount, 2);
}