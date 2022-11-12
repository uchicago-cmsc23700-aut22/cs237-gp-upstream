# CMake configuration for Group Project (Part 1) sources
#
# CMSC 23700 -- Introduction to Computer Graphics
# Autumn 2022
# University of Chicago
#
# COPYRIGHT (c) 2022 John Reppy
# All rights reserved.
#

set(TARGET part1)

set(SRCS
  buffer-cache.cpp
  camera.cpp
  map-cell.cpp
  map-objects.cpp
  map.cpp
  texture-cache.cpp)

# path to CS237 Library include files
include_directories(${CS237_INCLUDE_DIR})

add_executable(${TARGET} ${SRCS})

target_link_libraries(${TARGET} cs237)
add_dependencies(${TARGET} part1-shaders)
