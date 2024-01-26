# Find out where conan files have been written.
file(
  GLOB_RECURSE conan_paths_file
  "${CMAKE_BINARY_DIR}/**/conan_paths.cmake"
)

set(conan_dir "${CMAKE_BINARY_DIR}/conan")
if (NOT EXISTS "${conan_paths_file}")
  message(STATUS "Could not find conan_paths.cmake. Running it for you...")

  file(MAKE_DIRECTORY "${conan_dir}")

  # TODO: Detect changes to conanfile.txt and rerun install automatically.
  # Install conan packages.
  execute_process(
    COMMAND conan install "${CMAKE_SOURCE_DIR}"
    WORKING_DIRECTORY "${conan_dir}"
    RESULT_VARIABLE conan_install_result
  )

  set(conan_paths_file "${conan_dir}/conan_paths.cmake")
endif()

# Enable us to find libraries installed by conan.
# list(APPEND CMAKE_MODULE_PATH "${conan_dir}")
include("${conan_paths_file}")
