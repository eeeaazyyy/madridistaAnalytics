#ifndef gjf_423igdhfgl43yg5734238457f923jfhsdf
#define gjf_423igdhfgl43yg5734238457f923jfhsdf

#include "domain/model/match.hpp"

#include <span>

namespace madridista { namespace domain { namespace stats {


[[nodiscard]] int totalGoalsScored(model::TeamId teamId,
                        std::span<const model::Match> matches);

}}}

#endif 