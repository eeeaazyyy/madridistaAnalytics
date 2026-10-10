#ifndef jgiuh42984fhiiheofsdbfdsfir43ujfdfs
#define jgiuh42984fhiiheofsdbfdsfir43ujfdfs

#include <expected>
#include <vector>

#include <QByteArray>

#include "domain/model/goalEvent.hpp"
#include "domain/model/match.hpp"

namespace madridista { namespace infrastructure { namespace espn {

[[nodiscard]] std::expected<std::vector<domain::model::Match>, domain::Error>
parseSchedule(const QByteArray& json, domain::model::CompetitionId competition);

[[nodiscard]] std::expected<std::vector<domain::model::GoalEvent>, domain::Error>
parseSummary(const QByteArray& json);

}}} // namespace madridista::infrastructure::espn

#endif