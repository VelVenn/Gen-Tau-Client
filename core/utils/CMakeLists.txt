file(GLOB SRC_FILES CONFIGURE_DEPENDS "*.cpp")

gt_register_mod(
  NAME utils
  TYPE STATIC
  SRC ${SRC_FILES}
  DEPS
    conf
    spdlog::spdlog
    Pal::Sigslot
)
