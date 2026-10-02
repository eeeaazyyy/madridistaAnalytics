#ifndef ijuhfbnahdsi81348912E_sdfyu438asdan
#define ijuhfbnahdsi81348912E_sdfyu438asdan

#include "domain/error.hpp"

#include <chrono>
#include <compare>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>

namespace madridista { namespace domain { namespace model {

enum class MatchStatus {
    Scheduled = 0,
    Live,
    Finished,
    Postponed,
    Cancelled
};

struct TeamId {
    std::int64_t value{};
    auto operator<=>(const TeamId&) const = default;
};

struct MatchId {
    std::int64_t value{};
    auto operator<=>(const MatchId&) const = default;
};

struct CompetitionId {
    std::int64_t value{};
    auto operator<=>(const CompetitionId&) const = default;
};

struct Team {
    TeamId id;
    std::string name;

    bool operator==(const Team&) const = default;
};

struct Score {
    int home{};
    int away{};

    bool operator==(const Score&) const = default;
};

struct MatchData {
    MatchId id;
    std::chrono::sys_seconds kickoff;
    CompetitionId competition;
    Team homeTeam;
    Team awayTeam;
    MatchStatus status = MatchStatus::Scheduled;
    std::optional<Score> score{};

    bool operator==(const MatchData&) const = default;
};

class Match final {
public:
    [[nodiscard]] static std::expected<Match, Error> create(MatchData data);

    MatchId id() const;
    std::chrono::sys_seconds kickoff() const;
    CompetitionId competition() const;
    const Team &homeTeam() const;
    const Team &awayTeam() const;

    MatchStatus status() const;

    const std::optional<Score> &score() const;

    bool operator==(const Match&) const = default;

private:
    explicit Match(MatchData data);

    MatchData data_;
};

}}}

#endif