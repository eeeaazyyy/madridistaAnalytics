#include "database.hpp"

#include <QSqlError>
#include <QSqlQuery>

#include <string>
#include <utility>

namespace madridista { namespace infrastructure { namespace persistence {

namespace {

constexpr int kForeignKeyIndex = 0;
constexpr const char* kDriverName = "QSQLITE";
constexpr const char* kEnableForeignKey = "PRAGMA foreign_keys = ON";
constexpr const char* kReadForeignKey = "PRAGMA foreign_keys";

domain::Error storageError(std::string message) {
    return domain::Error{domain::ErrorCode::Storage, std::move(message)};
}

std::expected<void, domain::Error> enableForeignKeys(QSqlDatabase& db) {
    QSqlQuery query(db);
    if (!query.exec(kEnableForeignKey)) {
        return std::unexpected(
            storageError("Cannot enable foreign key: " + query.lastError().text().toStdString()));
    }

    if (!query.exec(kReadForeignKey) || !query.next()) {
        return std::unexpected(
            storageError("cannot read foreign key " + query.lastError().text().toStdString()));
    }

    if (query.value(kForeignKeyIndex).toInt() != 1) {
        return std::unexpected(
            storageError("Foreign key hadn't found " + query.lastError().text().toStdString()));
    }

    return {};
}

[[nodiscard]] std::unexpected<domain::Error>
closeAndRemove(QSqlDatabase& db, const QString& connectionName, domain::Error error) {
    db.close();
    db = QSqlDatabase();
    QSqlDatabase::removeDatabase(connectionName);
    return std::unexpected(std::move(error));
}

} // namespace

std::expected<QSqlDatabase, domain::Error> openDatabase(const QString& path,
                                                        const QString& connectionName) {
    if (QSqlDatabase::contains(connectionName)) {
        return std::unexpected(
            storageError("Connection is still alive " + connectionName.toStdString()));
    }

    auto db = QSqlDatabase::addDatabase(kDriverName, connectionName);
    if (!db.isValid()) {
        return closeAndRemove(db, connectionName, storageError("QSQLITE driver isn't available"));
    }

    db.setDatabaseName(path);
    if (!db.open()) {
        return closeAndRemove(db, connectionName,
                              storageError("Cannot open database " + path.toStdString() + ":" +
                                           db.lastError().text().toStdString()));
    }

    const auto foreignKeys = enableForeignKeys(db);
    if (!foreignKeys) {
        return closeAndRemove(db, connectionName, foreignKeys.error());
    }

    return db;
}

}}} // namespace madridista::infrastructure::persistence