cmake_minimum_required(VERSION 3.20)
set(BRICKS_DSP_VERSION_MAJOR 0)
set(BRICKS_DSP_VERSION_MINOR 3)

project(bricks_dsp VERSION ${BRICKS_DSP_VERSION_MAJOR}.${BRICKS_DSP_VERSION_MINOR})

option(BRICKS_DSP_BUILD_TESTS "Build and run unit tests" OFF)
option(BRICKS_DSP_BUILD_BENCHMARKS "Build performance benchmarks" OFF)
option(BRICKS_DSP_INTERNAL_AUDIO_BUFFERS "Audio output buffers are owned by bricks" ON)
set(BRICKS_BLOCK_SIZE 32 CACHE STRING "Internal processing block size")

# Source Files
set(SOURCE_FILES src/envelope_bricks.cpp
                 src/filter_bricks.cpp
                 src/modulator_bricks.cpp
                 src/oscillator_bricks.cpp
                 src/random_device.cpp)

set(SOURCE_FILES "${SOURCE_FILES}")

add_library(bricks_dsp STATIC ${SOURCE_FILES})

# Library sources can include directly, users must include from bricks_dsp/bricks.h
target_include_directories(bricks_dsp PUBLIC include)
target_include_directories(bricks_dsp PRIVATE include/bricks_dsp)
target_include_directories(bricks_dsp PRIVATE src)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(EXTRA_COMPILER_FLAGS "-Wall" "/std:c++17")
    target_compile_definitions(bricks_dsp PUBLIC /D WINDOWS /D _USE_MATH_DEFINES NOMINMAX)
else()
    set(EXTRA_COMPILER_FLAGS -Wall -fno-rtti -ffast-math -march=native -fpic)
    target_compile_definitions(bricks_dsp PUBLIC LINUX)
endif()

# Compile time constants can use functions from <cmath>, non-standard GCC feature
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_definitions(bricks_dsp PUBLIC BRICKS_DSP_CONSTEXPR_MATH)
endif()

if(BRICKS_DSP_INTERNAL_AUDIO_BUFFERS)
    target_compile_definitions(bricks_dsp PUBLIC BRICKS_DSP_INTERNAL_BUFFERS)
endif()

target_compile_definitions(bricks_dsp PUBLIC DSP_BRICKS_BLOCK_SIZE=${BRICKS_BLOCK_SIZE}
                                             BRICKS_DSP_VERSION_MAJOR=${BRICKS_DSP_VERSION_MAJOR}
                                             BRICKS_DSP_VERSION_MINOR=${BRICKS_DSP_VERSION_MINOR})
target_compile_features(bricks_dsp PUBLIC cxx_std_20)
target_compile_options(bricks_dsp PUBLIC ${EXTRA_COMPILER_FLAGS})

# Subprojects and tests
if(${BRICKS_DSP_BUILD_TESTS})
    add_subdirectory(test)
endif()

if(${BRICKS_DSP_BUILD_BENCHMARKS})
    add_subdirectory(benchmark)
endif()


add_subdirectory(examples EXCLUDE_FROM_ALL)