find_package(Qt6 6.8 REQUIRED COMPONENTS Quick)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GST REQUIRED IMPORTED_TARGET gstreamer-1.0)
pkg_check_modules(GST_VID REQUIRED IMPORTED_TARGET gstreamer-video-1.0)
pkg_check_modules(GST_APP REQUIRED IMPORTED_TARGET gstreamer-app-1.0)

include(FetchContent)

FetchContent_Declare(
  readerwriterqueue
  GIT_REPOSITORY https://github.com/cameron314/readerwriterqueue
  GIT_TAG master # Using released version may trigger CMake deprecation warning (< 3.10)
)
FetchContent_MakeAvailable(readerwriterqueue)

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG 12.1.0
)
FetchContent_MakeAvailable(fmt)

if(GEN_TAU_LOG_ENABLED)
  set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "Use external fmt library" FORCE)
  FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.17.0
  )
  FetchContent_MakeAvailable(spdlog)
endif()

qt_standard_project_setup(REQUIRES 6.8)
