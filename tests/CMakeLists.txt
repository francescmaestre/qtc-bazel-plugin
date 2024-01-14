find_package(Catch2 2.13 REQUIRED)


add_qtc_test(
  plugin_tests

  SOURCES
  test_main.cpp
  bazel_workspace_test.cpp
)
target_link_libraries(
  plugin_tests

  PRIVATE
  Catch2::Catch2
  project
)

add_test(helpers_test helpers_test)
