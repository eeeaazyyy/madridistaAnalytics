#ifndef ijfsghopnfisbipewof049u0r2jdosf42h4042
#define ijfsghopnfisbipewof049u0r2jdosf42h4042

#include <chrono>
#include <compare>

namespace madridista { namespace domain { namespace model {

// A season runs from July 1 (inclusive) to July 1 of the next year.
inline constexpr std::chrono::month kSeasonStartMonth = std::chrono::July;

struct Season {
    int startYear{};
    auto operator<=>(const Season&) const = default;
};

[[nodiscard]] Season seasonOf(std::chrono::sys_seconds kickOff);

}}} // namespace madridista::domain::model

#endif