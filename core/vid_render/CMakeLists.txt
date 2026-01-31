# set(VID_RENDER_LIB ${GT_LIB_PREFIX}vid-render CACHE INTERNAL "Video Rendering Library")
# set(VID_RENDER_LIB_ALIAS ${GT_EXPORT_LIB_NS}VidRender)

# add_library(
#     ${VID_RENDER_LIB} STATIC
#     TVidRender.cpp
# )

# target_include_directories(
#     ${VID_RENDER_LIB} PUBLIC
#     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
# )

# target_link_libraries(
#     ${VID_RENDER_LIB} PRIVATE
#     ${CONF_LIB}
#     ${TYPE_REDEF_LIB}
#     ${LOG_LIB}

#     # Third-party libraries
#     PkgConfig::GST 
#     readerwriterqueue
# )

# # Make it safe to be linked into shared libraries 
# set_target_properties(${VID_RENDER_LIB} PROPERTIES POSITION_INDEPENDENT_CODE ON)

# add_library(${VID_RENDER_LIB_ALIAS} ALIAS ${VID_RENDER_LIB})

gt_register_mod(
  NAME vid-render
  TYPE STATIC
  SRC TVidRender.cpp
  DEPS
    conf
    utils
    readerwriterqueue

    PRIVATE 
      PkgConfig::GST
)
