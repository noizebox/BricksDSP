cmake_minimum_required(VERSION 2.8)
project(bricks_dsp)

####################
#  Compiler Flags  #
####################

# Global flags
set(PROJECT_CXX_FLAGS "-Wall -Wextra -std=c++17 -fno-rtti")
set(PROJECT_LINK_FLAGS "")


# Build optimization flags
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3")
set(CMAKE_CXX_FLAGS_DEBUG   "-DDEBUG -O0 -g")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PROJECT_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${PROJECT_LINK_FLAGS}")

##################
#  Source Files  #
##################

set(COMPILATION_UNITS src/envelope_bricks.cpp
                      src/filter_bricks.cpp
                      src/oscillator_bricks.cpp)

# Include heades separately, mostly for CLion convenience

set(EXTRA_CLION_SOURCES src/dsp_brick.h
                        src/utility_bricks.h
                        src/envelope_bricks.h
                        src/filter_bricks.h
                        src/oscillator_bricks.h)

set(SOURCE_FILES "${COMPILATION_UNITS}" "${EXTRA_CLION_SOURCES}")

#########################
#  Include Directories  #
#########################

set(INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/src"
                 "${PROJECT_SOURCE_DIR}/include")


include_directories(${INCLUDE_DIRS})


#############################################
#  Subdirectory projects                    #
#############################################

add_subdirectory(test)
add_subdirectory(examples EXCLUDE_FROM_ALL)

####################
#  Target Objects  #
####################

add_library(bricks_dsp ${SOURCE_FILES})

