gt_register_mod(
  NAME utils
  TYPE STATIC
  SRC TLog.cpp
  DEPS
    conf
    spdlog::spdlog
    Pal::Sigslot
)
