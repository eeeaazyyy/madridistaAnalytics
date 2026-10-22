#include "domain/model/season.hpp"

#include <gtest/gtest.h>

#include <chrono>

using namespace madridista::domain;

namespace {

// Moment in UTC: at(2026, 7, 1) is July 1, 2026, 00:00:00
std::chrono::sys_seconds at(int y, unsigned m, unsigned d, int h = 0, int min = 0, int s = 0) {
    using namespace std::chrono;
    return sys_days{year{y} / month{m} / day{d}} + hours{h} + minutes{min} + seconds{s};
}

} // namespace

TEST(SeasonTest, SpringMatchBelongsToSeasonStartedLastYear) {
    EXPECT_EQ(model::seasonOf(at(2026, 5, 15, 19)), model::Season{2025});
}

TEST(SeasonTest, AutumnMatchBelongsToSeasonStartedThisYear) {
    EXPECT_EQ(model::seasonOf(at(2026, 9, 20, 14, 15)), model::Season{2026});
}

TEST(SeasonTest, NewYearDoesNotStartNewSeason) {
    EXPECT_EQ(model::seasonOf(at(2025, 12, 31, 23, 59, 59)), model::Season{2025});
    EXPECT_EQ(model::seasonOf(at(2026, 1, 1)), model::Season{2025});
}

TEST(SeasonTest, LastSecondOfJuneIsStillOldSeason) {
    EXPECT_EQ(model::seasonOf(at(2026, 6, 30, 23, 59, 59)), model::Season{2025});
}

TEST(SeasonTest, FirstSecondOfJulyStartsNewSeason) {
    EXPECT_EQ(model::seasonOf(at(2026, 7, 1)), model::Season{2026});
}

TEST(SeasonTest, SeasonsAreOrderedByStartYear) {
    EXPECT_LT(model::Season{2025}, model::Season{2026});
}