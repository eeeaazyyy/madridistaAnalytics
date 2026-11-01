#include "migrations.hpp"

#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QStringList>

#include <array>
#include <cstddef>
#include <string>

namespace madridista { namespace infrastructure { namespace persistence {

namespace {

constexpr std::array kMigrationFiles = {":/migrations/001_initial_schema.sql"};

std::unexpected<domain::Error> storageError(const std::string& message) {
    return std::unexpected(domain::Error{domain::ErrorCode::Storage, message});
}

std::expected<int, domain::Error> readUserVersion(QSqlDatabase& db) {
    QSqlQuery query(db);
    if (!query.exec("PRAGMA user_version") || !query.next()) {
        return std::unexpected(
            domain::Error{domain::ErrorCode::Storage, query.lastError().text().toStdString()});
    }

    bool ok = false;
    const int version = query.value(0).toInt(&ok);
    if (!ok) {
        return std::unexpected(
            domain::Error{domain::ErrorCode::Storage, "user_version is not a number"});
    }
    return version;
}

std::expected<QString, domain::Error> readMigrationFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return storageError("Cannot oper file migration " + path.toStdString());
    }

    return QString::fromUtf8(file.readAll());
}

std::expected<void, domain::Error> executeStatements(QSqlDatabase& db, const QString& sql,
                                                     int version) {
    for (const QString& statement : sql.split(';', Qt::SkipEmptyParts)) {
        if (statement.trimmed().isEmpty()) {
            continue;
        }

        QSqlQuery query(db);
        if (!query.exec(statement)) {
            return storageError("migration" + std::to_string(version) +
                                " failed: " + query.lastError().text().toStdString() +
                                " in: " + statement.trimmed().left(60).toStdString());
        }
    }

    return {};
}

std::expected<void, domain::Error> applyMigration(QSqlDatabase& db, int version,
                                                  const QString& path) {
    const auto sql = readMigrationFile(path);
    if (!sql) {
        return std::unexpected(sql.error());
    }

    if (!db.transaction()) {
        return storageError("Cannot start transaction: " + db.lastError().text().toStdString());
    }

    const auto executed = executeStatements(db, *sql, version);
    if (!executed) {
        db.rollback();
        return std::unexpected(executed.error());
    }

    QSqlQuery setVersion(db);
    if (!setVersion.exec(QString("PRAGMA user_version = %1").arg(version))) {
        db.rollback();
        return storageError("Cannot set user_version " +
                            setVersion.lastError().text().toStdString());
    }

    if (!db.commit()) {
        db.rollback();
        return storageError("Cannot commit migration " + std::to_string(version) + ": " +
                            db.lastError().text().toStdString());
    }

    return {};
}

} // namespace

[[nodiscard]] std::expected<void, domain::Error> applyMigrations(QSqlDatabase& db) {
    const auto currentVersion = readUserVersion(db);
    if (!currentVersion) {
        return std::unexpected(currentVersion.error());
    }

    const int enabledMigrations = static_cast<int>(kMigrationFiles.size());

    if (*currentVersion > enabledMigrations) {
        return std::unexpected(domain::Error{domain::ErrorCode::Storage,
                                             "Version is higher than database migrations"});
    }

    for (int version = *currentVersion + 1; version <= enabledMigrations; version++) {
        const auto path = QString::fromUtf8(kMigrationFiles[static_cast<std::size_t>(version - 1)]);
        const auto applied = applyMigration(db, version, path);
        if (!applied) {
            return std::unexpected(applied.error());
        }
    }

    return {};
}

}}} // namespace madridista::infrastructure::persistence