find_package(PkgConfig REQUIRED)
pkg_check_modules(Protobuf REQUIRED IMPORTED_TARGET protobuf)

gt_register_test(
  NAME mqtt-test
  SRC
    mqtt-test.cpp
    test.pb.cc
  WITH_HEADER
  DEPS
    comm 
    utils
    PkgConfig::Protobuf
)
