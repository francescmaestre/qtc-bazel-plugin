cmake_minimum_required(VERSION 3.10)

project(BazelPlugin LANGUAGES CXX)
enable_testing()

include(GenerateExportHeader)
include(compiler.cmake)
include(utils.cmake)

# Add support for Conan to be able to find the dependencies managed by it.
include(conan_setup.cmake)

# Discover dependencies.
find_package(QtCreator COMPONENTS Core REQUIRED)
find_package(QT NAMES Qt6 COMPONENTS Widgets REQUIRED)
set(QtX Qt${QT_VERSION_MAJOR})
find_package(Protobuf REQUIRED)

# Figure out the major version number of the taget IDE.
if (IDE_VERSION MATCHES "^([0-9]*)\\.([0-9]*)")
  set(IDE_VERSION_MAJOR ${CMAKE_MATCH_1})
  set(IDE_VERSION_MINOR ${CMAKE_MATCH_2})
  message(STATUS "Target IDE version numbers: ${IDE_VERSION_MAJOR} (major), ${IDE_VERSION_MINOR} (minor)")
else()
  message(FATAL_ERROR "Could not parse IDE version string '${IDE_VERSION}'")
endif()

# Figure out the install prefix based on the location of Qt Creator installation.
strip_path_suffix("${QtCreator_DIR}" "${IDE_CMAKE_INSTALL_PATH}/QtCreator" CMAKE_INSTALL_PREFIX)
message(STATUS "Install prefix: '${CMAKE_INSTALL_PREFIX}'")
message(STATUS "Plugin shall be installed to: '${CMAKE_INSTALL_PREFIX}/${IDE_PLUGIN_PATH}'")

# Go deeper and create actual build targets.
add_subdirectory(src)
add_subdirectory(tests)
