cmake_minimum_required(VERSION 3.24)

project(gen_tau_client VERSION 0.1.0 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(TProjOptions)
include(TProjSettings)
include(TDependencies)
include(TRegister)
include(TExtraInfo)

set_property(GLOBAL PROPERTY GEN_TAU_MODULES "")
set_property(GLOBAL PROPERTY GEN_TAU_TESTS "")

add_subdirectory(core)
add_subdirectory(app)

# ============================= GEN_TAU ENABLE TESTS =============================
if(GEN_TAU_BUILD_TESTS)
  message(STATUS "-> Building tests")
  # enable_testing()
  add_subdirectory(tests)
endif()
# ============================= GEN_TAU ENABLE TESTS =============================

# ====================== EXTRA GEN_TAU CMAKE VERBOSE OUTPUT ======================
if(GEN_TAU_CMAKE_VERBOSE)
  message(STATUS "-> Enabling Gen-τ CMake verbose output")
  gt_extra_link_info()
endif()
# ====================== EXTRA GEN_TAU CMAKE VERBOSE OUTPUT ======================