find_package(Catch2 2.13 REQUIRED)


add_executable(
  helpers_test

  bazel_workspace_test.cpp
  test_main.cpp
)
target_link_libraries(
  helpers_test

  PRIVATE
  Catch2::Catch2
  BazelProjectManager
)

add_test(helpers_test helpers_test)
