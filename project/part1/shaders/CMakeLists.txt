# CMake configuration for Group Project (Part 1) shaders
#
# CMSC 23700 -- Introduction to Computer Graphics
# Autumn 2022
# University of Chicago
#
# COPYRIGHT (c) 2022 John Reppy
# All rights reserved.
#

# the shader source files
set(SRCS
  )

# custom commands for compiling shaders
#
foreach(SHADER_SRC ${SRCS})
  get_filename_component(FILE_NAME ${SHADER_SRC} NAME)
  set(SHADER_FILE "${PROJECT_SOURCE_DIR}/shaders/${SHADER_SRC}")
  set(SPIRV_FILE "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
    COMMAND ${GLSLC} -V -o ${SPIRV_FILE} ${SHADER_FILE}
    DEPENDS ${SHADER_FILE})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV_FILE})
endforeach(SHADER_SRC)

add_custom_target(
  part1-shaders
  DEPENDS ${SPIRV_BINARY_FILES})
