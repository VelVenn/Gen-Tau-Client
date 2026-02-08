gt_register_mod(
  NAME vid-render
  TYPE STATIC
  SRC TVidRender.cpp
  DEPS
    utils
    readerwriterqueue
    Pal::Sigslot 
    PkgConfig::GST

    PRIVATE
      conf
      PkgConfig::GST_APP
)
