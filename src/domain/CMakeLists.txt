set(HEADERS 
        model/match.hpp
        model/goalEvent.hpp
        model/id.hpp
        model/season.hpp
        stats/statsEngine.hpp
        stats/statsQuery.hpp)

set(SOURCES
        model/match.cpp
        model/season.cpp
        stats/statsEngine.cpp)

add_library(domain STATIC ${SOURCES} ${HEADERS})

target_include_directories(domain PUBLIC ${PROJECT_SOURCE_DIR}/src)
madridista_set_warnings(domain)