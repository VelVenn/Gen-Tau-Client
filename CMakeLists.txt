cmake_minimum_required(VERSION 3.16)

project(gen_tau_framework VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(APP_NAME gen-tau)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(QML_MODULE_VERSION_MAJOR 1)
set(QML_MODULE_VERSION_MINOR 0)

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GST REQUIRED IMPORTED_TARGET gstreamer-1.0)
pkg_check_modules(GST_VID REQUIRED IMPORTED_TARGET gstreamer-video-1.0)

qt_standard_project_setup(REQUIRES 6.8)

option(BUILD_GEN_TAU_TESTS "Build Genτ tests" ON)

if(BUILD_GEN_TAU_TESTS)
    # enable_testing()
    add_subdirectory(tests)
endif()

add_subdirectory(src)