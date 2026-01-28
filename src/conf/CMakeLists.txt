if(CMAKE_BUILD_TYPE MATCHES "^[Dd][Ee][Bb][Uu][Gg]$")
    set(GEN_TAU_DEBUG 1)
else()
    set(GEN_TAU_DEBUG 0)
endif()

configure_file(
    version.hpp.in
    "${CMAKE_CURRENT_SOURCE_DIR}/inc/conf/version.hpp"
    @ONLY
    NEWLINE_STYLE UNIX
)

set(CONF_LIB gt-conf)

add_library(
    ${CONF_LIB} INTERFACE
)

target_include_directories(
    ${CONF_LIB} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
)

add_library(${GEN_TAU_LIB_NAMESPACE}Conf ALIAS ${CONF_LIB})

# ---------------- Clangd anchor target ----------------
add_library(
    gt_conf_clangd_anchor STATIC EXCLUDE_FROM_ALL
    clangd-anchor.cpp
)

target_link_libraries(
    gt_conf_clangd_anchor
    PRIVATE ${CONF_LIB}
)
# ------------------- DO NOT REMOVE ---------------------