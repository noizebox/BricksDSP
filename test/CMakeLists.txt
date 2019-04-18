add_subdirectory(gtest)
enable_testing()

# Test sources
SET(TEST_SOURCES  unittests/test_utils.cpp
                  unittests/utility_bricks_test.cpp
                  unittests/utilities_test.cpp
                  unittests/envelope_bricks_test.cpp
                  unittests/filter_brick_test.cpp
                  unittests/oscillator_bricks_test.cpp
                  unittests/modulator_bricks_test.cpp)

add_executable(unit_tests ${TEST_SOURCES})

target_include_directories(unit_tests PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_include_directories(unit_tests PRIVATE ${PROJECT_SOURCE_DIR}/include/bricks_dsp)
target_link_libraries(unit_tests bricks_dsp gtest gtest_main)
add_test(unit_tests unit_tests)

# Target for running the tests
add_custom_target(run_tests ALL COMMAND "./unit_tests")
add_dependencies(run_tests unit_tests)

