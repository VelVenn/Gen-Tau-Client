gt_register_mod(
  NAME vid-render
  TYPE STATIC
  SRC TVidRender.cpp
  DEPS
    conf
    utils
    readerwriterqueue
    Pal::Sigslot 
    PkgConfig::GST
)
