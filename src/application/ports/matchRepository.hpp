#ifndef opjoughsuidf0_dfijr02dalsdfoubrada
#define opjoughsuidf0_dfijr02dalsdfoubrada

#include "domain/error.hpp"
#include "domain/model/goalEvent.hpp"
#include "domain/model/id.hpp"
#include "domain/model/match.hpp"

#include <QByteArray>

#include <expected>
#include <span>
#include <vector>

namespace madridista { namespace application {

class IMatchRepository {
public:
    virtual ~IMatchRepository() = default;

    [[nodiscard]] virtual std::expected<void, domain::Error>
    saveMatches(std::span<const domain::model::Match> matches) = 0;

    [[nodiscard]] virtual std::expected<void, domain::Error>
    saveSummary(domain::model::MatchId match, std::span<const domain::model::GoalEvent> goals,
                const QByteArray& rawSummary) = 0;

    [[nodiscard]] virtual std::expected<std::vector<domain::model::Match>, domain::Error>
    matchesOf(domain::model::TeamId team) const = 0;

    [[nodiscard]] virtual std::expected<std::vector<domain::model::GoalEvent>, domain::Error>
    goalsOf(std::span<const domain::model::MatchId> matches) const = 0;

    [[nodiscard]] virtual std::expected<std::vector<domain::model::MatchId>, domain::Error>
    finishedMatchesWithoutSummary(domain::model::TeamId team) const = 0;
};

}} // namespace madridista::application

#endif