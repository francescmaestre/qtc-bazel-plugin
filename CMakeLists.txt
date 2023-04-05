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


# Generate Protobuf headers for selected Bazel APIs.
set(proto_files
  3rd_party/bazel/src/main/protobuf/build.proto
)
protobuf_generate(
  OUT_VAR bazel_proto_srcs
  LANGUAGE cpp
  PROTOS ${proto_files}
)

# Bazel's protobuf API and helper library.
add_qtc_library(
  bazel_api

  STATIC
  EXCLUDE_FROM_INSTALL

  SOURCES
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
  # TODO: Use `add_qtc_executable`?
  add_executable(qb qb.cpp)
  target_link_libraries(qb PRIVATE bazel_api)
endif()


# Library with some general plugin utilities/constants.
#
# NOTE: Using `add_qtc_library` from QtCreatorAPI.cmake instead of `add_library` sets some extra
# defines affecting QString API (`QT_RESTRICTED_CAST_FROM_ASCII` et al).
# Otherwise the API behaviour would be different from the code in the target created with the
# `add_qtc_plugin` function.
add_qtc_library(
  common

  STATIC
  EXCLUDE_FROM_INSTALL

  SOURCES
  logging.h
  logging.cpp
  plugin_constants.h
)
target_link_libraries(
  common
  PUBLIC ${QtX}::Core
)
generate_export_header(
  common
  BASE_NAME BazelProjectManager
)
target_include_directories(common INTERFACE ${CMAKE_CURRENT_BINARY_DIR})


# Library dealing with Bazel project parsing, model, and integration into IDE's project explorer.
add_qtc_library(
  project

  STATIC
  EXCLUDE_FROM_INSTALL

  SOURCES
  BazelProject.cpp
  BazelProject.h

  BazelBuildSystem.cpp
  BazelBuildSystem.h

  BazelProjectImporter.cpp
  BazelProjectImporter.h

  BazelWorkspace.cpp
  BazelWorkspace.h
)
set_target_properties(
  project

  PROPERTIES
    AUTOMOC ON
)
target_link_libraries(
  project

  PRIVATE
  bazel_api

  PUBLIC
  common
  QtCreator::Core
  QtCreator::CppEditor
  QtCreator::ProjectExplorer
  QtCreator::QtSupport
)
target_include_directories(project INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})


# Library supporting IDE build steps with Bazel.
add_qtc_library(
  building

  STATIC
  EXCLUDE_FROM_INSTALL

  SOURCES
  BazelBuildConfiguration.cpp
  BazelBuildConfiguration.h

  BazelBuildStep.cpp
  BazelBuildStep.h

  BazelBuildStepConfigWidget.cpp
  BazelBuildStepConfigWidget.h
  BazelBuildStepConfigWidget.ui

  BazelBuildTargetsModel.cpp
  BazelBuildTargetsModel.h

  BazelCleanStep.cpp
  BazelCleanStep.h
)
set_target_properties(
  building

  PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
)
target_link_libraries(
  building
  PRIVATE bazel_api common
  PUBLIC ${QtX}::Widgets QtCreator::Core
)
target_include_directories(building INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})


# Library supporting running executable Bazel artefacts.
add_qtc_library(
  running

  STATIC
  EXCLUDE_FROM_INSTALL

  SOURCES
  BazelRunConfiguration.cpp
  BazelRunConfiguration.h
)
target_link_libraries(
  running
  PRIVATE common
  PUBLIC QtCreator::Core QtCreator::ProjectExplorer
)
target_include_directories(running INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})


# The plugin binary. This is the entry point for the IDE.
add_qtc_plugin(
  BazelProjectManager

  PLUGIN_DEPENDS  # Qt Creator plugins that this plugin depends on.
    # This list shall be written into the JSON manifest in place of $$dependencyList.
    QtCreator::Core
    QtCreator::CppEditor
    QtCreator::ProjectExplorer

  DEPENDS  # Libraries that this binary depends on.

  SOURCES
    README.md
    BazelProjectManager.json.in
    resources.qrc

    BazelPlugin.cpp
    BazelPlugin.h
)
target_link_libraries(
  BazelProjectManager

  PRIVATE common project building running
)
set_target_properties(
  BazelProjectManager

  PROPERTIES
    AUTOMOC ON
    AUTORCC ON
)

add_subdirectory(tests)
