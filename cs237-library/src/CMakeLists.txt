# CMake configuration for CS237 utility library
#
# CMSC 23700 -- Introduction to Computer Graphics
# Autumn 2022
# University of Chicago
#
# COPYRIGHT (c) 2022 John Reppy
# All rights reserved.
#

set(SRCS
  aabb.cpp
  application.cpp
  image.cpp
  json.cpp
  json-parser.cpp
  memory-obj.cpp
  mtl-reader.cpp
  obj-reader.cpp
  obj.cpp
  shader.cpp
  texture.cpp
  tqt.cpp
  window.cpp)

# path to include files
include_directories(
  ${GLFW_INCLUDE_DIR}
  ${GLM_INCLUDE_DIR}
  ${VULKAN_INCLUDE_DIR}
  ../include)

add_library(cs237
  STATIC
  ${SRCS})
