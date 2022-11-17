# CMake configuration for individual programming assignments
#
# CMSC 23700 -- Introduction to Computer Graphics
# Autumn 2022
# University of Chicago
#
# COPYRIGHT (c) 2022 John Reppy
# All rights reserved.
#

cmake_minimum_required(VERSION 3.20.0)

project(CMSC237_AUT_2022
  VERSION 1
  HOMEPAGE_URL "https://classes.cs.uchicago.edu/archive/2022/fall/23700-1/index.html"
  DESCRIPTION "individual programming projects for CMSC 23700 (Autumn 2022)"
  LANGUAGES C CXX)

# Disable builds in the source tree to prevent corruption.
if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "
ERROR: builds in the source tree are not allowed.
       Create a separate directory for build files.
")
endif()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build type selected, default to Debug")
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (default Debug)" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard to conform to")
set(CMAKE_CXX_STANDARD_REQUIRED YES)

# Add path for custom modules
set(CMAKE_MODULE_PATH
  "${CMAKE_MODULE_PATH}"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
  )

# find GLM, GLFW, and Vulkan; note that the naming conventions for GLFW and
# Vulkan differ between macOS and Linux
find_package(glm 0.9.9 REQUIRED)
if (${CMAKE_HOST_APPLE})
  find_package(glfw 3.3 REQUIRED)
  find_package(vulkan 1.3 REQUIRED)
  # compiler for shader programs
  #
  set(GLSLC "${VULKAN_BIN_DIR}/glslangValidator")
else()
  find_package(glfw3 3.3 REQUIRED)
  find_package(Vulkan 1.3 REQUIRED)
  find_package(X11 REQUIRED)

  # compiler for shader programs
  #
  if (Vulkan_GLSLANG_VALIDATOR_EXECUTABLE)
    set(GLSLC ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE})
  elseif (VULKAN_BIN_DIR)
    set(GLSLC "${VULKAN_BIN_DIR}/glslangValidator")
  else()
    message(FATAL_ERROR "unable to determine where glslangValidator lives")
  endif()
  #
  if (Vulkan_LIBRARY)
    set (VULKAN_LIBRARY ${Vulkan_LIBRARY})
  endif()

endif()
find_package(PNG 1.5 REQUIRED)

option (CS237_ENABLE_DOXYGEN "Enable doxygen for generating cs237 library documentation." OFF)
option (CS237_VERBOSE_MAKEFILE "Enable verbose makefiles." OFF)

# enable verbose makefiles
#
set(CMAKE_VERBOSE_MAKEFILE ${CS237_VERBOSE_MAKEFILE})

# definitions for the configuration header file
#
include(cs237-config)

# Generate the configuration header file
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/config/cs237-config.h.cmake
  ${CMAKE_CURRENT_SOURCE_DIR}/cs237-library/include/cs237-config.h
  @ONLY)

# cs237 library
#
add_subdirectory(cs237-library)
set(CS237_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/cs237-library/include)
set(CS237_DOXYGEN_HTML_DIR ${CMAKE_SOURCE_DIR}/cs237-library/doc)

if (CS237_ENABLE_DOXYGEN)
  message(STATUS "Doxygen enabled.")
  find_package(Doxygen REQUIRED)
  if (DOXYGEN_FOUND)
   # TODO
  endif()
endif()

# path to include files
include_directories(
  ${GLFW_INCLUDE_DIR}
  ${GLM_INCLUDE_DIR}
  ${VULKAN_INCLUDE_DIR}
  ${PNG_INCLUDE_DIR}
  ${CS237_INCLUDE_DIR})

# libraries
link_libraries(${PNG_LIBRARY})
link_libraries(${VULKAN_LIBRARY})
link_libraries(${GLFW_LIBRARY})

# on Linux, we need X11
if (${CMAKE_HOST_LINUX})
include_directories(${X11_INCLUDE_DIR})
link_libraries(${X11_LIBRARIES})
endif()

# the subdirectory for the group project
#
add_subdirectory(project EXCLUDE_FROM_ALL)

