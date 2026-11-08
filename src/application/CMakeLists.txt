add_library(application INTERFACE
    ports/matchRepository.hpp)

target_include_directories(application INTERFACE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(application INTERFACE domain Qt6::Core)
