# Disable tests for google benchmark library
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(gbench)

SET(BENCH_SOURCES bricks_benchmark/benchmark_main.cpp)

add_executable(benchmarks ${BENCH_SOURCES})

target_include_directories(benchmarks PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_include_directories(benchmarks PRIVATE ${PROJECT_SOURCE_DIR}/include/bricks_dsp)
target_link_libraries(benchmarks bricks_dsp benchmark pthread)

target_compile_definitions(benchmarks PRIVATE -DDSP_BRICKS_BLOCK_SIZE=${BRICKS_BLOCK_SIZE})
target_compile_features(benchmarks PUBLIC cxx_std_17)
target_compile_options(benchmarks PUBLIC ${EXTRA_COMPILER_FLAGS})