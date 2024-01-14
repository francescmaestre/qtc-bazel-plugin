# Generate Protobuf headers for selected Bazel APIs.
set(proto_files
  bazel/src/main/protobuf/build.proto
)
protobuf_generate(
  OUT_VAR bazel_proto_srcs
  LANGUAGE cpp
  PROTOS ${proto_files}
)

# Bazel's protobuf API library.
add_library(
  bazel_api

  STATIC

  ${proto_files}
  ${bazel_proto_srcs}
)
target_link_libraries(
  bazel_api

  PUBLIC
  protobuf::libprotobuf
)
target_include_directories(
  bazel_api

  PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
)
