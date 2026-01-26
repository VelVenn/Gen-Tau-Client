cmake_minimum_required(VERSION 3.16)

project(gen_tau_framework VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(APP_NAME gen-tau)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick)
find_package(PkgConfig REQUIRED)

qt_standard_project_setup(REQUIRES 6.8)

option(BUILD_GEN_TAU_TESTS "Build Genτ tests" OFF)

if(BUILD_GEN_TAU_TESTS)
    # enable_testing()
    add_subdirectory(tests)
endif()

add_subdirectory(src)