cmake_minimum_required(VERSION 3.7)
project(bricks_dsp CXX)

option(BRICKS_DSP_BUILD_TESTS "Build and run unit tests" ON)
option(BRICKS_DSP_BUILD_BENCHMARKS "Build performance benchmarks" ON)
set(BRICKS_BLOCK_SIZE 32 CACHE STRING "Internal processing block size")

# Source Files
set(SOURCE_FILES src/envelope_bricks.cpp
                 src/filter_bricks.cpp
                 src/modulator_bricks.cpp
                 src/oscillator_bricks.cpp
                 src/random_device.cpp)

# Include headers separately, mostly for CLion convenience
set(SOURCE_HEADERS  include/bricks_dsp/dsp_brick.h
                    include/bricks_dsp/utility_bricks.h
                    include/bricks_dsp/envelope_bricks.h
                    include/bricks_dsp/filter_bricks.h
                    include/bricks_dsp/modulator_bricks.h
                    include/bricks_dsp/oscillator_bricks.h
                    include/bricks_dsp/aligned_array.h
                    include/bricks_dsp/random_device.h)

set(SOURCE_FILES "${SOURCE_FILES}" "${SOURCE_HEADERS}")

add_library(bricks_dsp STATIC ${SOURCE_FILES})

# Library sources can include directly, users must include from bricks_dsp/bricks.h
target_include_directories(bricks_dsp PUBLIC include)
target_include_directories(bricks_dsp PRIVATE include/bricks_dsp)
target_include_directories(bricks_dsp PRIVATE src)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(EXTRA_COMPILER_FLAGS "-Wall" "/std:c++17")
    target_compile_definitions(bricks_dsp PUBLIC /D WINDOWS /D _USE_MATH_DEFINES)
else()
    set(EXTRA_COMPILER_FLAGS -Wall -fno-rtti -ffast-math -march=native -fpic)
    target_compile_definitions(bricks_dsp PUBLIC -DLINUX)
endif()

target_compile_definitions(bricks_dsp PRIVATE -DDSP_BRICKS_BLOCK_SIZE=${BRICKS_BLOCK_SIZE})
target_compile_features(bricks_dsp PUBLIC cxx_std_17)
target_compile_options(bricks_dsp PUBLIC ${EXTRA_COMPILER_FLAGS})

# Subprojects and tests
if(${BRICKS_DSP_BUILD_TESTS})
    add_subdirectory(test)
endif()

if(${BRICKS_DSP_BUILD_BENCHMARKS})
    add_subdirectory(benchmark)
endif()


add_subdirectory(examples EXCLUDE_FROM_ALL)