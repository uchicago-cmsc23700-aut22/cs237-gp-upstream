##=============================================================================
##
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2016 Sandia Corporation.
##  Copyright 2016 UT-Battelle, LLC.
##  Copyright 2016 Los Alamos National Security.
##
##  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
##  the U.S. Government retains certain rights in this software.
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##
##=============================================================================
#
# Try to find the GLFW library and include dir.
# Once done this will define
#
# GLFW_FOUND
# GLFW_INCLUDE_DIR
# GLFW_LIBRARY
#

include(FindPackageHandleStandardArgs)

if (WIN32)
    find_path( GLFW_INCLUDE_DIR
        NAMES
            GLFW/glfw3.h
        PATHS
            ${PROJECT_SOURCE_DIR}/shared_external/glfw/include
            ${PROJECT_SOURCE_DIR}/../shared_external/glfw/include
            ${GLFW_LOCATION}/include
            $ENV{GLFW_LOCATION}/include
            $ENV{PROGRAMFILES}/GLFW/include
            ${GLFW_LOCATION}
            $ENV{GLFW_LOCATION}
            DOC "The directory where GLFW/glfw3.h resides" )
    if(ARCH STREQUAL "x86")
      find_library( GLFW_LIBRARY
          NAMES
              glfw3
          PATHS
              ${GLFW_LOCATION}/lib
              $ENV{GLFW_LOCATION}/lib
              $ENV{PROGRAMFILES}/GLFW/lib
              DOC "The GLFW library")
    else()
      find_library( GLFW_LIBRARY
          NAMES
              glfw3
          PATHS
              ${GLFW_LOCATION}/lib
              $ENV{GLFW_LOCATION}/lib
              $ENV{PROGRAMFILES}/GLFW/lib
              DOC "The GLFW library")
    endif()
endif ()

if (${CMAKE_HOST_UNIX})
    find_path( GLFW_INCLUDE_DIR
        NAMES
            GLFW/glfw3.h
        PATHS
            ${GLFW_LOCATION}/include
            $ENV{GLFW_LOCATION}/include
            /usr/include
            /usr/local/include
            /sw/include
            /opt/local/include
            /opt/homebrew/include
            NO_DEFAULT_PATH
            DOC "The directory where GLFW/glfw3.h resides"
    )
    find_library( GLFW_LIBRARY
        NAMES
            glfw3 glfw
        PATHS
            ${GLFW_LOCATION}/lib
            $ENV{GLFW_LOCATION}/lib
            /usr/lib64
            /usr/lib
            /usr/local/lib64
            /usr/local/lib
            /sw/lib
            /opt/local/lib
            /opt/homebrew/lib
            /usr/lib/x86_64-linux-gnu
            NO_DEFAULT_PATH
            DOC "The GLFW library")
endif ()

# get the version from the GLFW/glfw3.h header file
#
if(GLFW_INCLUDE_DIR AND EXISTS "${GLFW_INCLUDE_DIR}/GLFW/glfw3.h")

  file(STRINGS "${GLFW_INCLUDE_DIR}/GLFW/glfw3.h" GLFW_VERSION_MAJOR_LINE REGEX "^#define[ \t]+GLFW_VERSION_MAJOR[ \t]+[0-9]+$")
  file(STRINGS "${GLFW_INCLUDE_DIR}/GLFW/glfw3.h" GLFW_VERSION_MINOR_LINE REGEX "^#define[ \t]+GLFW_VERSION_MINOR[ \t]+[0-9]+$")
  file(STRINGS "${GLFW_INCLUDE_DIR}/GLFW/glfw3.h" GLFW_VERSION_REVISION_LINE REGEX "^#define[ \t]+GLFW_VERSION_REVISION[ \t]+[0-9]+$")
  string(REGEX REPLACE "^#define[ \t]+GLFW_VERSION_MAJOR[ \t]+([0-9]+)$" "\\1" GLFW_VERSION_MAJOR "${GLFW_VERSION_MAJOR_LINE}")
  string(REGEX REPLACE "^#define[ \t]+GLFW_VERSION_MINOR[ \t]+([0-9]+)$" "\\1" GLFW_VERSION_MINOR "${GLFW_VERSION_MINOR_LINE}")
  string(REGEX REPLACE "^#define[ \t]+GLFW_VERSION_REVISION[ \t]+([0-9]+)$" "\\1" GLFW_VERSION_REVISION "${GLFW_VERSION_REVISION_LINE}")
  set(GLFW_VERSION ${GLFW_VERSION_MAJOR}.${GLFW_VERSION_MINOR}.${GLFW_VERSION_REVISION})
  unset(GLFW_VERSION_MAJOR_LINE)
  unset(GLFW_VERSION_MINOR_LINE)
  unset(GLFW_VERSION_REVISION_LINE)
  unset(GLFW_VERSION_MAJOR)
  unset(GLFW_VERSION_MINOR)
  unset(GLFW_VERSION_REVISION)
endif ()

find_package_handle_standard_args(glfw3
  REQUIRED_VARS GLFW_INCLUDE_DIR GLFW_LIBRARY
  VERSION_VAR GLFW_VERSION
)

mark_as_advanced( GLFW_FOUND )
