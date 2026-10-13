#include "infrastructure/espn/espnMapper.hpp"

#include <QFile>
#include <QString>

#include <gtest/gtest.h>

using namespace madridista;

namespace {

QByteArray readFixture(const char* name) {
    QFile file(QString(FIXTURES_DIR) + "/" + name);
    if (!file.open(QIODevice::ReadOnly)) {
        ADD_FAILURE() << "cannot open fixture " << name;
        return {};
    }
    return file.readAll();
}

constexpr domain::model::CompetitionId kLaLiga{1};

} // namespace

TEST(EspnScheduleTest, ParsesFinishedMatches) {
    const auto matches =
        infrastructure::espn::parseSchedule(readFixture("espn_schedule_finished.json"), kLaLiga);

    ASSERT_TRUE(matches.has_value());
    ASSERT_EQ(matches->size(), 3u);

    const auto& first = matches->front();
    EXPECT_EQ(first.id().value, 401882865);
    EXPECT_EQ(first.homeTeam().id.value, 1068);
    EXPECT_EQ(first.homeTeam().name, "Atlético Madrid");
    EXPECT_EQ(first.awayTeam().id.value, 86);
    EXPECT_EQ(first.status(), domain::model::MatchStatus::Finished);
    EXPECT_EQ(first.competition(), kLaLiga);
    ASSERT_TRUE(first.score().has_value());
    EXPECT_EQ(first.score()->home, 2);
    EXPECT_EQ(first.score()->away, 1);
}

TEST(EspnScheduleTest, ParsesKickoffWithoutSeconds) {
    // "2026-09-20T14:15Z": ESPN omits seconds
    const auto matches =
        infrastructure::espn::parseSchedule(readFixture("espn_schedule_finished.json"), kLaLiga);

    ASSERT_TRUE(matches.has_value());
    ASSERT_FALSE(matches->empty());
    using namespace std::chrono;
    const sys_seconds expected = sys_days{year{2026} / 9 / 20} + hours{14} + minutes{15};
    EXPECT_EQ(matches->front().kickoff(), expected);
}

TEST(EspnScheduleTest, UpcomingMatchesHaveNoScore) {
    const auto matches =
        infrastructure::espn::parseSchedule(readFixture("espn_schedule_upcoming.json"), kLaLiga);

    ASSERT_TRUE(matches.has_value());
    ASSERT_EQ(matches->size(), 2u);
    EXPECT_EQ(matches->front().status(), domain::model::MatchStatus::Scheduled);
    EXPECT_FALSE(matches->front().score().has_value());
}

TEST(EspnScheduleTest, NotFoundResponseReturnsNotFound) {
    const auto matches =
        infrastructure::espn::parseSchedule(readFixture("espn_summary_not_found.json"), kLaLiga);

    ASSERT_FALSE(matches.has_value());
    EXPECT_EQ(matches.error().code, domain::ErrorCode::NotFound);
}

TEST(EspnScheduleTest, InvalidJsonReturnsInvalidResponse) {
    const auto matches = infrastructure::espn::parseSchedule("not json", kLaLiga);

    ASSERT_FALSE(matches.has_value());
    EXPECT_EQ(matches.error().code, domain::ErrorCode::InvalidResponse);
}

TEST(EspnScheduleTest, ResponseWithoutEventsReturnsInvalidResponse) {
    const auto matches = infrastructure::espn::parseSchedule(R"({"team": {}})", kLaLiga);

    ASSERT_FALSE(matches.has_value());
    EXPECT_EQ(matches.error().code, domain::ErrorCode::InvalidResponse);
}