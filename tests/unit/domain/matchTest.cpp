#include "domain/model/match.hpp"

#include <gtest/gtest.h>

#include <chrono>

using namespace madridista::domain;

namespace {

model::MatchData validData() {
    return {
        .id = model::MatchId{1208021},
        .kickoff = std::chrono::sys_days{std::chrono::year{2025} / 8 / 18},
        .competition = model::CompetitionId{140},
        .homeTeam = model::Team{model::TeamId{798}, "Mallorca"},
        .awayTeam = model::Team{model::TeamId{541}, "Real Madrid"},
        .status = model::MatchStatus::Finished,
        .score = model::Score{1, 1},
    };
}

} // namespace

TEST(MatchTest, CreatesMatchFromValidData) {
    const model::MatchData data = validData();

    const auto match = model::Match::create(data);

    ASSERT_TRUE(match.has_value());
    EXPECT_EQ(match->id(), data.id);
    EXPECT_EQ(match->kickoff(), data.kickoff);
    EXPECT_EQ(match->competition(), data.competition);
    EXPECT_EQ(match->homeTeam(), data.homeTeam);
    EXPECT_EQ(match->awayTeam(), data.awayTeam);
    EXPECT_EQ(match->status(), data.status);
    EXPECT_EQ(match->score(), data.score);
}

TEST(MatchTest, CreatesScheduledMatchWithoutScore) {
    model::MatchData data = validData();
    data.status = model::MatchStatus::Scheduled;
    data.score.reset();

    const auto match = model::Match::create(data);

    ASSERT_TRUE(match.has_value());
    EXPECT_FALSE(match->score().has_value());
}

TEST(MatchTest, RejectsTeamPlayingItself) {
    model::MatchData data = validData();
    data.awayTeam.id = data.homeTeam.id;

    const auto match = model::Match::create(data);

    ASSERT_FALSE(match.has_value());
    EXPECT_EQ(match.error().code, ErrorCode::InvalidArgument);
}

TEST(MatchTest, RejectsNegativeScore) {
    model::MatchData data = validData();
    data.score = model::Score{-1, 0};

    const auto match = model::Match::create(data);

    ASSERT_FALSE(match.has_value());
    EXPECT_EQ(match.error().code, ErrorCode::InvalidArgument);
}

TEST(MatchTest, RejectsFinishedMatchWithoutScore) {
    model::MatchData data = validData();
    data.score.reset();

    const auto match = model::Match::create(data);

    ASSERT_FALSE(match.has_value());
    EXPECT_EQ(match.error().code, ErrorCode::InvalidArgument);
}

TEST(MatchTest, RejectsScheduledMatchWithScore) {
    model::MatchData data = validData();
    data.status = model::MatchStatus::Scheduled;

    const auto match = model::Match::create(data);

    ASSERT_FALSE(match.has_value());
    EXPECT_EQ(match.error().code, ErrorCode::InvalidArgument);
}