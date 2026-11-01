#ifndef idfnsdy_smdfou_sdfouqlkms
#define idfnsdy_smdfou_sdfouqlkms

#include "domain/error.hpp"

#include <QSqlDatabase>

#include <expected>

namespace madridista { namespace infrastructure { namespace persistence {

[[nodiscard]] std::expected<void, domain::Error> applyMigrations(QSqlDatabase& db);

}}} // namespace madridista::infrastructure::persistence

#endif