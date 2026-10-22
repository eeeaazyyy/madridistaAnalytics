set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

include(FetchContent)
FetchContent_Declare(googletest 
    URL
    https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    SYSTEM
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_executable(domain_tests
    unit/domain/statsEngineTest.cpp
    unit/domain/seasonTest.cpp
    unit/domain/matchTest.cpp
)

target_link_libraries(domain_tests PRIVATE domain
                                    GTest::gtest_main)
madridista_set_warnings(domain_tests)

gtest_discover_tests(domain_tests)

#infrastructure_tests
add_executable(infrastructure_tests
    unit/infrastructure/espnMapperTest.cpp)

target_link_libraries(infrastructure_tests PRIVATE infrastructure
                                        GTest::gtest_main)

target_compile_definitions(infrastructure_tests PRIVATE
    FIXTURES_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures")

madridista_set_warnings(infrastructure_tests)

gtest_discover_tests(infrastructure_tests)