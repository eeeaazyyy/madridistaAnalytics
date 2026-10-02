#include "domain/model/match.hpp"

#include <utility>

namespace madridista { namespace domain { namespace model {

std::expected<Match, Error> Match::create(MatchData data) {
    if (data.homeTeam.id == data.awayTeam.id) {
        return std::unexpected(Error{ErrorCode::InvalidArgument, "Same teams!"});
    }
    if (data.score && (data.score->home < 0 || data.score->away < 0)) {
        return std::unexpected(Error{ErrorCode::InvalidArgument, "Score can't be with minus!"});
    }
    if (data.status == MatchStatus::Finished && !data.score) {
        return std::unexpected(Error{ErrorCode::InvalidArgument, "Finished match must have score!"});
    }
    if (data.status == MatchStatus::Scheduled && data.score) {
        return std::unexpected(Error{ErrorCode::InvalidArgument, "Match doesnt start yet!"});
    }

    return Match{std::move(data)};
}

MatchId Match::id() const {
    return data_.id;
}

std::chrono::sys_seconds Match::kickoff() const {
    return data_.kickoff;
}

CompetitionId Match::competition() const {
    return data_.competition;
}

const Team &Match::homeTeam() const {
    return data_.homeTeam;
}

const Team &Match::awayTeam() const {
    return data_.awayTeam;
}

MatchStatus Match::status() const {
    return data_.status;
}

const std::optional<Score> &Match::score() const {
    return data_.score;
}

Match::Match(MatchData data) : data_{std::move(data)} {

}

}}}
