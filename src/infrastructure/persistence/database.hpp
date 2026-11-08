#ifndef owfjhboalkfriueflsndlaopdksdfs
#define owfjhboalkfriueflsndlaopdksdfs

#include "domain/error.hpp"

#include <QSqlDatabase>

#include <expected>

namespace madridista { namespace infrastructure { namespace persistence {

[[nodiscard]] std::expected<QSqlDatabase, domain::Error>
openDatabase(const QString& path, const QString& connectionName);

}}} // namespace madridista::infrastructure::persistence

#endif