#include "domain/stats/statsEngine.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <vector>

using namespace madridista::domain;

namespace {

constexpr model::TeamId idRealMadrid{541};

constexpr int kRegularTimeBuckets = 6; // 0–15, 16–30, 31–45+, 46–60, 61–75, 76–90+

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
        EXPECT_LT(*bucket, kRegularTimeBuckets);
    }
}