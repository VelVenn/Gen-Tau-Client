file(GLOB SRC_FILES CONFIGURE_DEPENDS "*.cpp")

gt_register_mod(
  NAME img-trans
  TYPE STATIC
  SRC ${SRC_FILES}
  DEPS
    utils
    readerwriterqueue

    PRIVATE
      conf
      PkgConfig::GST
      PkgConfig::GST_APP
)
