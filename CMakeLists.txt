cmake_minimum_required(VERSION 2.8)
project(bricks_dsp)

####################
#  Compiler Flags  #
####################

# Global flags
if(CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_CXX_FLAGS "-Wall -std=c++14 -fno-rtti")
elsif(MSVC)
	set(CMAKE_CXX_FLAGS "-Wall" "/std:c++17")
endif()

#set(PROJECT_CXX_FLAGS "-std=c++17 -fno-rtti -ffast-math -march=native")
set(PROJECT_LINK_FLAGS "")


# Build optimization flags
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3")
set(CMAKE_CXX_FLAGS_DEBUG   "-DDEBUG -O0 -g")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PROJECT_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${PROJECT_LINK_FLAGS}")

if(MSVC)
    add_definitions(/D _USE_MATH_DEFINES)
endif()

##################
#  Source Files  #
##################

set(COMPILATION_UNITS src/envelope_bricks.cpp
                      src/filter_bricks.cpp
                      src/modulator_bricks.cpp
                      src/oscillator_bricks.cpp)

# Include heades separately, mostly for CLion convenience

set(EXTRA_CLION_SOURCES include/dsp_brick.h
        include/utility_bricks.h
        include/envelope_bricks.h
        include/filter_bricks.h
        include/modulator_bricks.h
        include/oscillator_bricks.h)

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

if(${BRICKS_DSP_BUILD_WITHOUT_TESTS})
else()
    add_subdirectory(test)
endif()

add_subdirectory(examples EXCLUDE_FROM_ALL)

####################
#  Target Objects  #
####################

add_library(bricks_dsp STATIC ${SOURCE_FILES})

