find_package(GTest CONFIG REQUIRED)

include(GoogleTest)

add_executable(greeter_test greeter_test.cpp)
target_link_libraries(
  greeter_test
  PRIVATE greeter
          GTest::gtest
          GTest::gtest_main
          quickstart_project_options
)

gtest_discover_tests(greeter_test)

add_executable(klv_parser_test klv_parser_test.cpp)
target_link_libraries(
  klv_parser_test
  PRIVATE sensor_pipeline
          GTest::gtest
          GTest::gtest_main
          quickstart_project_options
)

gtest_discover_tests(klv_parser_test)
