#include "domain/model/season.hpp"

namespace madridista { namespace domain { namespace model {

Season seasonOf(std::chrono::sys_seconds kickOff) {
    const auto day = std::chrono::floor<std::chrono::days>(kickOff);
    const auto fullDate = std::chrono::year_month_day{day};

    if (fullDate.month() >= kSeasonStartMonth) {
        return Season{
            .startYear = static_cast<int>(fullDate.year()),
        };
    }

    return Season{
        .startYear = static_cast<int>(fullDate.year()) - 1,
    };
}

}}} // namespace madridista::domain::model