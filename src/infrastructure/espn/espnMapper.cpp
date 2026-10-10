#include "espnMapper.hpp"

namespace madridista { namespace infrastructure { namespace espn {

std::expected<std::vector<domain::model::Match>, domain::Error>
parseSchedule(const QByteArray& json, domain::model::CompetitionId competition) {
    return std::expected<std::vector<domain::model::Match>, domain::Error>();
}

std::expected<std::vector<domain::model::GoalEvent>, domain::Error>
parseSummary(const QByteArray& json) {
    return std::expected<std::vector<domain::model::GoalEvent>, domain::Error>();
}

}}} // namespace madridista::infrastructure::espn