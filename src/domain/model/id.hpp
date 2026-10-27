#ifndef j0f_fsjdifjr28oFsijnifsl
#define j0f_fsjdifjr28oFsijnifsl

#include <compare>
#include <cstdint>

namespace madridista { namespace domain { namespace model {

template <typename Tag>
class Id final {
public:
    constexpr explicit Id(std::int64_t value) : value_{value} {}

    constexpr std::int64_t value() const { return value_; }

    auto operator<=>(const Id&) const = default;

private:
    std::int64_t value_;
};

using TeamId = Id<struct TeamTag>;
using MatchId = Id<struct MatchTag>;
using CompetitionId = Id<struct CompetitionTag>;
using PlayerId = Id<struct PlayerTag>;

}}} // namespace madridista::domain::model

#endif