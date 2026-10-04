cmake_minimum_required(VERSION 3.25)
project(MadridistaAnalytics VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(CompilerWarnings)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets)
qt_standard_project_setup()

add_subdirectory(src)

option(BUILD_TESTS "Build unit test" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()