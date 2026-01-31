cmake_minimum_required(VERSION 3.24)

project(gen_tau_client VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
pkg_check_modules(GST_APP REQUIRED IMPORTED_TARGET gstreamer-app-1.0)

include(FetchContent)

FetchContent_Declare(
  readerwriterqueue
  GIT_REPOSITORY https://github.com/cameron314/readerwriterqueue
  GIT_TAG master
)
FetchContent_MakeAvailable(readerwriterqueue)

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG 12.1.0
)
FetchContent_MakeAvailable(fmt)

set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "Use external fmt library" FORCE)
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.17.0
)
FetchContent_MakeAvailable(spdlog)

qt_standard_project_setup(REQUIRES 6.8)

# ========================= GEN_TAU GLOBAL DEBUG OPTIONS =========================
string(TOUPPER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_UPPER)
message(STATUS "Configuring version.hpp with build type: ${CMAKE_BUILD_TYPE_UPPER}")

if(CMAKE_BUILD_TYPE_UPPER STREQUAL "DEBUG")
  message(STATUS "-> Debug mode detected. set GEN_TAU_DEBUG to ON")
  set(GEN_TAU_DEBUG 1)
else()
  message(STATUS "-> Release/Other mode. set GEN_TAU_DEBUG to OFF")
  set(GEN_TAU_DEBUG 0)
endif()
# ========================= GEN_TAU GLOBAL DEBUG OPTIONS =========================

# =========================== GEN_TAU LOGGING OPTIONS ============================
option(GEN_TAU_LOG_ENABLED "Enable Gen-τ logging" ON)

option(GEN_TAU_LOG_TO_FILE "Enable Gen-τ logging to file" ${GEN_TAU_DEBUG})

set(GEN_TAU_LOG_LEVEL "DEFAULT" CACHE STRING "The minimum log level to compile in")
set_property(
  CACHE GEN_TAU_LOG_LEVEL 
  PROPERTY STRINGS 
  "TRACE" 
  "DEBUG" 
  "INFO" 
  "WARN" 
  "ERROR" 
  "CRITICAL" 
  "DEFAULT"
)

if(GEN_TAU_LOG_LEVEL STREQUAL "DEFAULT")
  message(STATUS "-> Log level set to DEFAULT")
  if(GEN_TAU_DEBUG)
    set(GEN_TAU_ACTIVE_LOG_LEVEL "TRACE")
  else()
    set(GEN_TAU_ACTIVE_LOG_LEVEL "INFO")
  endif()
else()
  message(STATUS "-> Log level set to ${GEN_TAU_LOG_LEVEL}")
  set(GEN_TAU_ACTIVE_LOG_LEVEL "${GEN_TAU_LOG_LEVEL}")
endif()

if(GEN_TAU_LOG_ENABLED)
  message(STATUS "-> Logging enabled")
  add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_${GEN_TAU_ACTIVE_LOG_LEVEL})
endif()
# =========================== GEN_TAU LOGGING OPTIONS ============================

# ======================= GEN_TAU INTERNAL NAMING OPTIONS ========================
set(GT_APP_NAME "gen-tau")
set(GT_QML_MOD_NAME "Gentau.")
set(GT_LIB_PREFIX "gt-")
set(GT_EXPORT_LIB_NS "Gentau::")
# ======================= GEN_TAU INTERNAL NAMING OPTIONS ========================

add_subdirectory(src)

# ============================ GEN_TAU TESTS OPTIONS =============================
option(GEN_TAU_BUILD_TESTS "Build Gen-τ tests" ON)
if(GEN_TAU_BUILD_TESTS)
  message(STATUS "-> Building tests")
  # enable_testing()
  add_subdirectory(tests)
endif()
# ============================ GEN_TAU TESTS OPTIONS =============================