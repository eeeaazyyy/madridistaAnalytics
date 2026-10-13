set(HEADERS 
        espn/espnMapper.hpp)

set(SOURCES
        espn/espnMapper.cpp)

add_library(infrastructure STATIC ${SOURCES} ${HEADERS})

target_include_directories(infrastructure PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(infrastructure PUBLIC domain Qt6::Core)
madridista_set_warnings(infrastructure)