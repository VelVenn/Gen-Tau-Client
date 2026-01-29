set(TYPE_REDEF_LIB gt-type-redef)

add_library(
    ${TYPE_REDEF_LIB} INTERFACE
)
target_include_directories(
    ${TYPE_REDEF_LIB} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
)
add_library(${GEN_TAU_LIB_NAMESPACE}TypeRedef ALIAS ${TYPE_REDEF_LIB})

set(LOG_LIB gt-log)

add_library(
    ${LOG_LIB} STATIC
    TLog.cpp
)
target_include_directories(
    ${LOG_LIB} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
)
target_link_libraries(
    ${LOG_LIB} PRIVATE
    spdlog::spdlog
)
add_library(${GEN_TAU_LIB_NAMESPACE}Log ALIAS ${LOG_LIB})