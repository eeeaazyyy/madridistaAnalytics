add_library(domain STATIC
    model/match.hpp
    model/match.cpp
    stats/statsEngine.hpp
    stats/statsEngine.cpp
)

target_include_directories(domain PUBLIC ${PROJECT_SOURCE_DIR}/src)
madridista_set_warnings(domain)