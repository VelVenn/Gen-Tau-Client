cmake_minimum_required(VERSION 3.24)

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

set(GEN_TAU_LIB_NAMESPACE Gentau::)

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GST REQUIRED IMPORTED_TARGET gstreamer-1.0)
pkg_check_modules(GST_VID REQUIRED IMPORTED_TARGET gstreamer-video-1.0)

include(FetchContent)

FetchContent_Declare(
  readerwriterqueue
  GIT_REPOSITORY    https://github.com/cameron314/readerwriterqueue
  GIT_TAG           master
)
FetchContent_MakeAvailable(readerwriterqueue)

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG        12.1.0
)
FetchContent_MakeAvailable(fmt)

set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "Use external fmt library" FORCE)
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.17.0
)
FetchContent_MakeAvailable(spdlog)

qt_standard_project_setup(REQUIRES 6.8)

add_subdirectory(src)

option(BUILD_GEN_TAU_TESTS "Build Gen-τ tests" ON)
if(BUILD_GEN_TAU_TESTS)
    # enable_testing()
    add_subdirectory(tests)
endif()
