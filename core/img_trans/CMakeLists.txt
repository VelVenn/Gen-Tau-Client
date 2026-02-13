gt_register_mod(
  NAME img-trans
  TYPE STATIC
  SRC TVidRender.cpp
  DEPS
    utils
    readerwriterqueue
    Pal::Sigslot 

    PRIVATE
      conf
      PkgConfig::GST
      PkgConfig::GST_APP
)
