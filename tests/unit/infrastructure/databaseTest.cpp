#include "infrastructure/persistence/database.hpp"
#include "infrastructure/persistence/migrations.hpp"

#include <QDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>

#include <gtest/gtest.h>

using namespace madridista;

namespace {

// Opens nothing itself: each test calls openDatabase; TearDown removes the connection
class DatabaseTest : public testing::Test {
protected:
    void SetUp() override {
        const auto* info = testing::UnitTest::GetInstance()->current_test_info();
        connectionName_ = QString("database_") + info->name();
    }

    void TearDown() override {
        db_ = QSqlDatabase(); // release the handle before removeDatabase
        if (QSqlDatabase::contains(connectionName_)) {
            QSqlDatabase::database(connectionName_, false).close();
            QSqlDatabase::removeDatabase(connectionName_);
        }
    }

    QString connectionName_;
    QSqlDatabase db_;
};

} // namespace

TEST_F(DatabaseTest, OpensInMemoryDatabaseWithForeignKeysEnabled) {
    const auto opened = infrastructure::persistence::openDatabase(":memory:", connectionName_);
    ASSERT_TRUE(opened.has_value()) << opened.error().message;
    db_ = *opened;

    EXPECT_TRUE(db_.isOpen());
    QSqlQuery query(db_);
    ASSERT_TRUE(query.exec("PRAGMA foreign_keys"));
    ASSERT_TRUE(query.next());
    EXPECT_EQ(query.value(0).toInt(), 1);
}

TEST_F(DatabaseTest, RejectsGoalOfUnknownMatch) {
    const auto opened = infrastructure::persistence::openDatabase(":memory:", connectionName_);
    ASSERT_TRUE(opened.has_value()) << opened.error().message;
    db_ = *opened;
    ASSERT_TRUE(infrastructure::persistence::applyMigrations(db_).has_value());

    QSqlQuery insert(db_);
    const bool inserted = insert.exec(
        "INSERT INTO goal_events (match_id, team_id, period, clock_seconds, added_minutes, type) "
        "VALUES (999, 86, 'first_half', 600, 0, 'regular')");

    EXPECT_FALSE(inserted);
}

TEST_F(DatabaseTest, RejectsDuplicateConnectionName) {
    const auto first = infrastructure::persistence::openDatabase(":memory:", connectionName_);
    ASSERT_TRUE(first.has_value()) << first.error().message;
    db_ = *first;

    const auto second = infrastructure::persistence::openDatabase(":memory:", connectionName_);

    ASSERT_FALSE(second.has_value());
    EXPECT_EQ(second.error().code, domain::ErrorCode::Storage);
    EXPECT_TRUE(db_.isOpen()); // the first connection is untouched
}

TEST_F(DatabaseTest, FailedOpenLeavesNoConnection) {
    const QString path = QDir::tempPath() + "/madridista_no_such_dir/db.sqlite";

    const auto opened = infrastructure::persistence::openDatabase(path, connectionName_);

    ASSERT_FALSE(opened.has_value());
    EXPECT_EQ(opened.error().code, domain::ErrorCode::Storage);
    EXPECT_FALSE(QSqlDatabase::contains(connectionName_));
}