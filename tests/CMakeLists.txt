find_package(catch2 3 REQUIRED)


add_executable(
  helpers_test

  helpers_test.cpp
  test_main.cpp
)
target_link_libraries(
  helpers_test

  PRIVATE
  Catch2::Catch2
  bazel_api
)

add_test(helpers_test helpers_test)
