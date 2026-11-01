#include "infrastructure/persistence/migrations.hpp"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QStringList>

#include <gtest/gtest.h>

using namespace madridista;

namespace {

// Fresh in-memory database with a unique connection name per test
class MigrationsTest : public testing::Test {
protected:
    void SetUp() override {
        const auto* info = testing::UnitTest::GetInstance()->current_test_info();
        connectionName_ = QString("migrations_") + info->name();
        db_ = QSqlDatabase::addDatabase("QSQLITE", connectionName_);
        db_.setDatabaseName(":memory:");
        ASSERT_TRUE(db_.open());
    }

    void TearDown() override {
        db_.close();
        db_ = QSqlDatabase();
        QSqlDatabase::removeDatabase(connectionName_);
    }

    int userVersion() {
        QSqlQuery query(db_);
        query.exec("PRAGMA user_version");
        return query.next() ? query.value(0).toInt() : -1;
    }

    QStringList objects(const char* type) {
        QSqlQuery query(db_);
        query.prepare("SELECT name FROM sqlite_master WHERE type = :type ORDER BY name");
        query.bindValue(":type", QString(type));
        query.exec();
        QStringList names;
        while (query.next()) {
            names << query.value(0).toString();
        }
        return names;
    }

    QSqlDatabase db_;
    QString connectionName_;
};

} // namespace

TEST_F(MigrationsTest, CreatesAllTablesOnEmptyDatabase) {
    ASSERT_TRUE(infrastructure::persistence::applyMigrations(db_).has_value());

    EXPECT_EQ(objects("table"), (QStringList{"goal_events", "match_summaries", "matches"}));
    EXPECT_EQ(objects("index"), (QStringList{"idx_goal_events_match"}));
}

TEST_F(MigrationsTest, SetsUserVersionToLatestMigration) {
    ASSERT_TRUE(infrastructure::persistence::applyMigrations(db_).has_value());

    EXPECT_EQ(userVersion(), 1);
}

TEST_F(MigrationsTest, SecondRunChangesNothing) {
    ASSERT_TRUE(infrastructure::persistence::applyMigrations(db_).has_value());

    const auto second = infrastructure::persistence::applyMigrations(db_);

    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(userVersion(), 1);
}

TEST_F(MigrationsTest, RejectsDatabaseFromNewerApplication) {
    QSqlQuery(db_).exec("PRAGMA user_version = 99");

    const auto result = infrastructure::persistence::applyMigrations(db_);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, domain::ErrorCode::Storage);
    EXPECT_EQ(userVersion(), 99); // untouched
    EXPECT_TRUE(objects("table").isEmpty());
}