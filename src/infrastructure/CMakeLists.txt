set(HEADERS 
        espn/espnMapper.hpp
        persistence/migrations.hpp)

set(SOURCES
        espn/espnMapper.cpp
        persistence/migrations.cpp)

add_library(infrastructure STATIC ${SOURCES} ${HEADERS})

qt_add_resources(infrastructure migrations
    PREFIX "/migrations"
    BASE ${PROJECT_SOURCE_DIR}/sql/migrations
    FILES
        ${PROJECT_SOURCE_DIR}/sql/migrations/001_initial_schema.sql)

target_include_directories(infrastructure PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(infrastructure PUBLIC domain Qt6::Core Qt6::Sql)
madridista_set_warnings(infrastructure)