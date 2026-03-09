find_package(Protobuf REQUIRED)

protobuf_generate(
  OUT_VAR PROTO_SRCS
  LANGUAGE cpp
  PROTOS ${CMAKE_CURRENT_SOURCE_DIR}/test.proto
)

gt_register_test(
  NAME mqtt-test
  SRC
    mqtt-test.cpp
    ${PROTO_SRCS}
  DEPS
    comm 
    utils
    protobuf::libprotobuf
)

target_include_directories(mqtt-test PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
