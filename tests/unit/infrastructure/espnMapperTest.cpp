#include "infrastructure/espn/espnMapper.hpp"

#include <QFile>
#include <QString>

#include <algorithm>
#include <chrono>

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
    EXPECT_EQ(first.id().value(), 401882865);
    EXPECT_EQ(first.homeTeam().id.value(), 1068);
    EXPECT_EQ(first.homeTeam().name, "Atlético Madrid");
    EXPECT_EQ(first.awayTeam().id.value(), 86);
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

// ---------- parseSummary ----------

namespace {

using std::chrono::minutes;
using std::chrono::seconds;

QByteArray summaryWithEvents(const char* keyEvents, int homeGoals, int awayGoals) {
    const QByteArray header = QString(R"({"id": "1", "competitions": [{"competitors": [
        {"homeAway": "home", "score": "%1", "team": {"id": "86"}},
        {"homeAway": "away", "score": "%2", "team": {"id": "93"}}]}]})")
                                  .arg(homeGoals)
                                  .arg(awayGoals)
                                  .toUtf8();
    return QByteArray(R"({"header": )") + header + R"(, "keyEvents": )" + keyEvents + "}";
}

} // namespace

TEST(EspnSummaryTest, ParsesAllGoalsOfTheMatch) {
    const auto goals =
        infrastructure::espn::parseSummary(readFixture("espn_summary_stoppage_time.json"));

    ASSERT_TRUE(goals.has_value());
    ASSERT_EQ(goals->size(), 6u);
    for (const auto& goal : *goals) {
        EXPECT_EQ(goal.match.value(), 748514);
    }
}

TEST(EspnSummaryTest, RegularTimeGoalHasNoAddedTime) {
    const auto goals =
        infrastructure::espn::parseSummary(readFixture("espn_summary_stoppage_time.json"));

    ASSERT_TRUE(goals.has_value());
    ASSERT_FALSE(goals->empty());
    const auto& first = goals->front(); // 12'
    EXPECT_EQ(first.period, domain::model::Period::FirstHalf);
    EXPECT_EQ(first.clock, seconds{715});
    EXPECT_EQ(first.addedTime, minutes{0});
    EXPECT_EQ(first.team.value(), 86);
}

TEST(EspnSummaryTest, ParsesFirstHalfStoppageTimeGoal) {
    const auto goals =
        infrastructure::espn::parseSummary(readFixture("espn_summary_stoppage_time.json"));

    ASSERT_TRUE(goals.has_value());
    ASSERT_EQ(goals->size(), 6u);
    const auto& goal = (*goals)[2]; // 45'+1'
    EXPECT_EQ(goal.period, domain::model::Period::FirstHalf);
    EXPECT_EQ(goal.clock, seconds{2700});
    EXPECT_EQ(goal.addedTime, minutes{1});
    EXPECT_EQ(goal.team.value(), 93);
}

TEST(EspnSummaryTest, ParsesSecondHalfStoppageTimeGoal) {
    const auto goals =
        infrastructure::espn::parseSummary(readFixture("espn_summary_stoppage_time.json"));

    ASSERT_TRUE(goals.has_value());
    ASSERT_EQ(goals->size(), 6u);
    const auto& goal = (*goals)[5]; // 90'+1'
    EXPECT_EQ(goal.period, domain::model::Period::SecondHalf);
    EXPECT_EQ(goal.clock, seconds{5400});
    EXPECT_EQ(goal.addedTime, minutes{1});
}

TEST(EspnSummaryTest, ParsesScorer) {
    const auto goals =
        infrastructure::espn::parseSummary(readFixture("espn_summary_stoppage_time.json"));

    ASSERT_TRUE(goals.has_value());
    ASSERT_FALSE(goals->empty());
    ASSERT_TRUE(goals->front().scorer.has_value());
    EXPECT_EQ(goals->front().scorer->name, "Gonzalo García");
}

TEST(EspnSummaryTest, OwnGoalIsCreditedToBenefitingTeam) {
    const auto goals =
        infrastructure::espn::parseSummary(readFixture("espn_summary_own_goal.json"));

    ASSERT_TRUE(goals.has_value());
    ASSERT_EQ(goals->size(), 5u);
    const auto& ownGoal = (*goals)[3]; // 85', scored by a Real Madrid player
    EXPECT_EQ(ownGoal.type, domain::model::GoalType::OwnGoal);
    EXPECT_EQ(ownGoal.team.value(), 3751); // Elche
}

TEST(EspnSummaryTest, RecognizesPenalties) {
    const auto goals =
        infrastructure::espn::parseSummary(readFixture("espn_summary_penalties.json"));

    ASSERT_TRUE(goals.has_value());
    const auto penalties = std::ranges::count_if(
        *goals, [](const auto& goal) { return goal.type == domain::model::GoalType::Penalty; });
    EXPECT_EQ(penalties, 3);
}

TEST(EspnSummaryTest, NotFoundResponseReturnsNotFound) {
    const auto goals =
        infrastructure::espn::parseSummary(readFixture("espn_summary_not_found.json"));

    ASSERT_FALSE(goals.has_value());
    EXPECT_EQ(goals.error().code, domain::ErrorCode::NotFound);
}

TEST(EspnSummaryTest, IgnoresNonScoringEvents) {
    // A missed penalty comes as a key event without scoringPlay
    const auto goals = infrastructure::espn::parseSummary(summaryWithEvents(R"([
        {"type": {"text": "Penalty - Missed"}, "clock": {"value": 600.0, "displayValue": "10'"},
         "period": {"number": 1}, "scoringPlay": false, "team": {"id": "86"}, "participants": []}
    ])",
                                                                            0, 0));

    ASSERT_TRUE(goals.has_value());
    EXPECT_TRUE(goals->empty());
}

TEST(EspnSummaryTest, IgnoresShootoutPenalties) {
    const auto goals = infrastructure::espn::parseSummary(summaryWithEvents(R"([
        {"type": {"text": "Penalty - Scored"}, "clock": {"value": 7200.0, "displayValue": "120'"},
         "period": {"number": 5}, "scoringPlay": true, "team": {"id": "86"}, "participants": []}
    ])",
                                                                            0, 0));

    ASSERT_TRUE(goals.has_value());
    EXPECT_TRUE(goals->empty());
}

TEST(EspnSummaryTest, GoalWithoutClockRejectsWholeSummary) {
    const auto goals = infrastructure::espn::parseSummary(summaryWithEvents(R"([
        {"type": {"text": "Goal"}, "clock": {"displayValue": "10'"},
         "period": {"number": 1}, "scoringPlay": true, "team": {"id": "86"}, "participants": []}
    ])",
                                                                            1, 0));

    ASSERT_FALSE(goals.has_value());
    EXPECT_EQ(goals.error().code, domain::ErrorCode::InvalidResponse);
}

TEST(EspnSummaryTest, GoalWithoutScorerIsStillCounted) {
    const auto goals = infrastructure::espn::parseSummary(summaryWithEvents(R"([
        {"type": {"text": "Goal"}, "clock": {"value": 600.0, "displayValue": "10'"},
         "period": {"number": 1}, "scoringPlay": true, "team": {"id": "86"}, "participants": []}
    ])",
                                                                            1, 0));

    ASSERT_TRUE(goals.has_value());
    ASSERT_EQ(goals->size(), 1u);
    EXPECT_FALSE(goals->front().scorer.has_value());
}

TEST(EspnSummaryTest, MissingGoalRejectsSummary) {
    // Final score 2:0, but only one goal in keyEvents
    const auto goals = infrastructure::espn::parseSummary(summaryWithEvents(R"([
        {"type": {"text": "Goal"}, "clock": {"value": 600.0, "displayValue": "10'"},
         "period": {"number": 1}, "scoringPlay": true, "team": {"id": "86"}, "participants": []}
    ])",
                                                                            2, 0));

    ASSERT_FALSE(goals.has_value());
    EXPECT_EQ(goals.error().code, domain::ErrorCode::InvalidResponse);
}

TEST(EspnSummaryTest, GoalOfTeamNotInMatchRejectsSummary) {
    // Final score 1:0; the second goal belongs to a team that didn't play (529)
    const auto goals = infrastructure::espn::parseSummary(summaryWithEvents(R"([
        {"type": {"text": "Goal"}, "clock": {"value": 600.0, "displayValue": "10'"},
         "period": {"number": 1}, "scoringPlay": true, "team": {"id": "86"}, "participants": []},
        {"type": {"text": "Goal"}, "clock": {"value": 1200.0, "displayValue": "20'"},
         "period": {"number": 1}, "scoringPlay": true, "team": {"id": "529"}, "participants": []}
    ])",
                                                                            1, 0));

    ASSERT_FALSE(goals.has_value());
    EXPECT_EQ(goals.error().code, domain::ErrorCode::InvalidResponse);
}

TEST(EspnSummaryTest, SummaryWithoutScoreIsRejected) {
    const auto goals =
        infrastructure::espn::parseSummary(R"({"header": {"id": "1"}, "keyEvents": []})");

    ASSERT_FALSE(goals.has_value());
    EXPECT_EQ(goals.error().code, domain::ErrorCode::InvalidResponse);
}