set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

set(VID_RENDER_LIB gt-vid-render)

add_library(
    ${VID_RENDER_LIB} STATIC
    TVidRender.cpp
)

target_include_directories(
    ${VID_RENDER_LIB} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
)

target_link_libraries(
    ${VID_RENDER_LIB} PRIVATE
    PkgConfig::GST Gentau::Conf
)

# Make it safe to be linked into shared libraries 
set_target_properties(${VID_RENDER_LIB} PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(${GEN_TAU_LIB_NAMESPACE}VidRender ALIAS ${VID_RENDER_LIB})