#ifndef kii03jsiofdnrojf990fsjfj4j20f9f3j
#define kii03jsiofdnrojf990fsjfj4j20f9f3j

#include <optional>

#include "match.hpp"

namespace madridista { namespace domain { namespace model {

enum class Period {
    FirstHalf,
    SecondHalf,
    ExtraTimeFirstHalf,
    ExtraTimeSecondHalf,
};

enum class GoalType {
    Regular,
    Penalty,
    OwnGoal,
};

struct PlayerId {
    std::int64_t value{};
    auto operator<=>(const PlayerId&) const = default;
};

struct Player {
    PlayerId id;
    std::string name;
    bool operator==(const Player&) const = default;
};

struct GoalEvent {
    MatchId match;
    TeamId team;

    Period period;
    std::chrono::seconds clock;
    std::chrono::minutes addedTime;

    GoalType type;
    std::optional<Player> scorer;

    bool operator==(const GoalEvent&) const = default;
};

}}} // namespace madridista::domain::model

#endif