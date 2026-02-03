# set(MOD_NAME "gt_cmake_test")

# message(STATUS "DEBUG CHECK: MOD_NAME='${MOD_NAME}'")
# get_property(already_registered GLOBAL PROPERTY GEN_TAU_MODULES_${MOD_NAME} SET)
# message(STATUS "DEBUG CHECK: already_registered (SET)=${already_registered}")

# gt_register_mod(
#   NAME ${MOD_NAME}
#   TYPE STATIC
#   SRC tmp.cpp
#   DEPS
#     PUBLIC conf
#     PRIVATE utils
#     PUBLIC vid-render
#     PRIVATE fmt::fmt
# )
