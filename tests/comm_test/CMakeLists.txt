# Try to find Protobuf via CONFIG mode first (e.g. for Homebrew on macOS)
find_package(Protobuf CONFIG QUIET)
if(NOT Protobuf_FOUND)
  # Fallback to MODULE mode (e.g. for system libraries on Linux)
  find_package(Protobuf REQUIRED)
endif()

protobuf_generate(
  OUT_VAR PROTO_SRCS
  LANGUAGE cpp
  PROTOS ${CMAKE_CURRENT_SOURCE_DIR}/proto/test.proto
)

gt_register_test(
  NAME mqtt-test-sub
  SRC
    mqtt-test-sub.cpp
    ${PROTO_SRCS}
  INC
    ${CMAKE_CURRENT_BINARY_DIR}
  DEPS
    comm 
    utils
    protobuf::libprotobuf
)

gt_register_test(
  NAME mqtt-test-pub
  SRC
    mqtt-test-pub.cpp
    ${PROTO_SRCS}
  INC
    ${CMAKE_CURRENT_BINARY_DIR}
  DEPS
    comm 
    utils
    protobuf::libprotobuf
)