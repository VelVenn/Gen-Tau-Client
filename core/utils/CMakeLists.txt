set(TYPE_REDEF_LIB ${GT_LIB_PREFIX}type-redef CACHE INTERNAL "Type Redefinition Library")
set(TYPE_REDEF_LIB_ALIAS ${GT_EXPORT_LIB_NS}TypeRedef)

add_library(
    ${TYPE_REDEF_LIB} INTERFACE
)
target_include_directories(
    ${TYPE_REDEF_LIB} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
)
add_library(${TYPE_REDEF_LIB_ALIAS} ALIAS ${TYPE_REDEF_LIB})

set(LOG_LIB ${GT_LIB_PREFIX}log CACHE INTERNAL "Logging Library")
set(LOG_LIB_ALIAS ${GT_EXPORT_LIB_NS}Log)

add_library(
    ${LOG_LIB} STATIC
    TLog.cpp
)
target_include_directories(
    ${LOG_LIB} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
)
target_link_libraries(
    ${LOG_LIB} PUBLIC
    ${CONF_LIB}

    spdlog::spdlog
)
add_library(${LOG_LIB_ALIAS} ALIAS ${LOG_LIB})