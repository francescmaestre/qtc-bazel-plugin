cmake_minimum_required(VERSION 3.10)

project(BazelPlugin LANGUAGES CXX)
enable_testing()

include(GenerateExportHeader)
include(compiler.cmake)
include(utils.cmake)

# Add support for Conan to be able to find the dependencies managed by it.
include(${CMAKE_BINARY_DIR}/conan_paths.cmake)

# Discover dependencies.
find_package(QtCreator COMPONENTS Core REQUIRED)
find_package(QT NAMES Qt6 COMPONENTS Widgets REQUIRED)
set(QtX Qt${QT_VERSION_MAJOR})
find_package(Protobuf REQUIRED)

# Figure out the install prefix based on the location of Qt Creator installation.
strip_path_suffix("${QtCreator_DIR}" "${IDE_CMAKE_INSTALL_PATH}/QtCreator" CMAKE_INSTALL_PREFIX)
message(STATUS "Install prefix: '${CMAKE_INSTALL_PREFIX}'")
message(STATUS "Plugin shall be installed to: '${CMAKE_INSTALL_PREFIX}/${IDE_PLUGIN_PATH}'")

# Go deeper and create actual build targets.
add_subdirectory(src)
add_subdirectory(tests)
