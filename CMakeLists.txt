cmake_minimum_required(VERSION 3.10)

project(BazelPlugin LANGUAGES CXX)
enable_testing()

include(GenerateExportHeader)
include(compiler.cmake)

# Add support for Conan to be able to find the dependencies managed by it.
include(${CMAKE_BINARY_DIR}/conan_paths.cmake)

# Discover dependencies.
find_package(QtCreator COMPONENTS Core REQUIRED)
find_package(QT NAMES Qt6 COMPONENTS Widgets REQUIRED)
set(QtX Qt${QT_VERSION_MAJOR})
find_package(Protobuf REQUIRED)


# Bazel's protobuf API library.
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
generate_export_header(bazel_api)
target_include_directories(
  bazel_api

  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(
  bazel_api
  PUBLIC
    protobuf::libprotobuf
    ${QtX}::Core
)


# Just a POC tool to test Bazel's protobuf API
if (NOT CMAKE_BUILD_TYPE STREQUAL "Release")
  add_executable(qb qb.cpp)
  target_link_libraries(qb PRIVATE bazel_api)
endif()


# The plugin binary.
add_qtc_plugin(
  BazelProjectManager

  PLUGIN_DEPENDS
    QtCreator::Core
    QtCreator::CppEditor
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

    BazelBuildStepConfigWidget.cpp
    BazelBuildStepConfigWidget.h
    BazelBuildStepConfigWidget.ui

    BazelBuildTargetsModel.cpp
    BazelBuildTargetsModel.h

    BazelCleanStep.cpp
    BazelCleanStep.h

    BazelRunConfiguration.cpp
    BazelRunConfiguration.h

    BazelWorkspace.cpp
    BazelWorkspace.h
)
set_target_properties(
  BazelProjectManager

  PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)
target_include_directories(
  BazelProjectManager

  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(
  BazelProjectManager

  PUBLIC
    bazel_api
    QtCreator::Core
)
generate_export_header(BazelProjectManager)

add_subdirectory(tests)
