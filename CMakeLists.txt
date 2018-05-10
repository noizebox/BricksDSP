cmake_minimum_required(VERSION 3.8)
project(bricks_dsp)

# Compiler settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_CXX_FLAGS "-Wall -fno-rtti -ffast-math -march=native -g")
set(PROJECT_LINK_FLAGS "")

# Build optimization flags
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3")
set(CMAKE_CXX_FLAGS_DEBUG   "-DDEBUG -O0 -g")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PROJECT_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${PROJECT_LINK_FLAGS}")

# Sources
set(COMPILATION_UNITS src/envelope_bricks.cpp
                      src/filter_bricks.cpp
                      src/modulator_bricks.cpp
                      src/oscillator_bricks.cpp)

# Include headers separately, mostly for CLion convenience
set(EXTRA_CLION_SOURCES include/dsp_brick.h
        include/utility_bricks.h
        include/envelope_bricks.h
        include/filter_bricks.h
        include/modulator_bricks.h
        include/oscillator_bricks.h)

set(SOURCE_FILES "${COMPILATION_UNITS}" "${EXTRA_CLION_SOURCES}")

# Include dirs
set(INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/src"
                 "${PROJECT_SOURCE_DIR}/include")

include_directories(${INCLUDE_DIRS})

# Subprojects and tests
if(${BRICKS_DSP_BUILD_WITHOUT_TESTS})
else()
    add_subdirectory(test)
endif()

add_subdirectory(examples EXCLUDE_FROM_ALL)

# Library target
add_library(bricks_dsp STATIC ${SOURCE_FILES})

