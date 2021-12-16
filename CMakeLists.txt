cmake_minimum_required(VERSION 3.10)

project(BazelPlugin)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)

# Add support for Conan to be able to find the dependencies managed by it.
include(${CMAKE_BINARY_DIR}/conan_paths.cmake)

# Discover dependencies.
find_package(QtCreator COMPONENTS Core REQUIRED)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
set(QtX Qt${QT_VERSION_MAJOR})
find_package(Protobuf REQUIRED)
find_package(fmt REQUIRED)


set(proto_files
  3rd_party/bazel/src/main/protobuf/build.proto
)

protobuf_generate(
  OUT_VAR bazel_proto_srcs
  LANGUAGE cpp
  PROTOS ${proto_files}
)
add_library(
  bazel_api STATIC

  ${proto_files}
  ${bazel_proto_srcs}
  bazel_helpers.cpp
  bazel_helpers.h
)
target_include_directories(bazel_api PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(
  bazel_api
  PUBLIC
    protobuf::libprotobuf
  PRIVATE
    fmt::fmt
)

# Just a POC tool to test Bazel's protobuf API
add_executable(qb qb.cpp)
target_link_libraries(qb PRIVATE bazel_api)


add_qtc_plugin(BazelProjectManager
  PLUGIN_DEPENDS
    QtCreator::Core
    QtCreator::ProjectExplorer

  DEPENDS
    ${QtX}::Widgets
    QtCreator::QtSupport

  SOURCES
    README.md
    BazelProjectManager.json.in

    logging.h
    logging.cpp

    plugin_constants.h
    plugin_global.h

    BazelPlugin.cpp
    BazelPlugin.h

    BazelProject.cpp
    BazelProject.h

    BazelProjectImporter.cpp
    BazelProjectImporter.h

    BazelBuildConfiguration.cpp
    BazelBuildConfiguration.h

    BazelBuildSystem.cpp
    BazelBuildSystem.h

    BazelBuildStep.cpp
    BazelBuildStep.h
)
