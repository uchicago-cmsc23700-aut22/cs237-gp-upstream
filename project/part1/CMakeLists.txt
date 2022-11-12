# CMake configuration for Group Project (Part 1)
#
# CMSC 23700 -- Introduction to Computer Graphics
# Autumn 2022
# University of Chicago
#
# COPYRIGHT (c) 2022 John Reppy
# All rights reserved.
#

project(CMSC237_GROUP_PROJ_1
  VERSION 1
  HOMEPAGE_URL "https://classes.cs.uchicago.edu/archive/2022/fall/23700-1/labs/lab3/index.html"
  LANGUAGES C CXX)

# shader source code
#
add_subdirectory(shaders)

# the source code for the project
#
add_subdirectory(src)
