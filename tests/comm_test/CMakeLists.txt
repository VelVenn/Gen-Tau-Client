find_package(Protobuf REQUIRED)

protobuf_generate(
  OUT_VAR PROTO_SRCS
  LANGUAGE cpp
  PROTOS ${CMAKE_CURRENT_SOURCE_DIR}/proto/test.proto
)

gt_register_test(
  NAME mqtt-test
  SRC
    mqtt-test.cpp
    ${PROTO_SRCS}
  INC
    ${CMAKE_CURRENT_BINARY_DIR}
  DEPS
    comm 
    utils
    protobuf::libprotobuf
)
