##=============================================================================
##
## COPYRIGHT (c) 2022 John Reppy
## All rights reserved.
##
##=============================================================================
#
# Find module for GLM headers
# Once done this will define
#
# GLM_FOUND
# GLM_INCLUDE_DIR
#


include(FindPackageHandleStandardArgs)

if (WIN32)

# not sure what to do for Windows

endif ()

if (${CMAKE_HOST_UNIX})
    find_path( GLM_INCLUDE_DIR
        NAMES
            glm/glm.hpp
        PATHS
            ${GLM_LOCATION}/include
            $ENV{GLM_LOCATION}/include
            /usr/include
            /usr/local/include
            /sw/include
            /opt/local/include
            /opt/homebrew/include
            NO_DEFAULT_PATH
            DOC "The directory where vulkan/vulkan.h resides"
    )
endif ()

# get the version from the glm/detail/setup.hpp header file
#
if(GLM_INCLUDE_DIR AND EXISTS "${GLM_INCLUDE_DIR}/glm/detail/setup.hpp")
  file(STRINGS "${GLM_INCLUDE_DIR}/glm/detail/setup.hpp" GLM_MAJOR_LINE REGEX "^[ \t]*#define[ \t]+GLM_VERSION_MAJOR[ \t]+[0-9]+")
  file(STRINGS "${GLM_INCLUDE_DIR}/glm/detail/setup.hpp" GLM_MINOR_LINE REGEX "^[ \t]*#define[ \t]+GLM_VERSION_MINOR[ \t]+[0-9]+")
  file(STRINGS "${GLM_INCLUDE_DIR}/glm/detail/setup.hpp" GLM_PATCH_LINE REGEX "^[ \t]*#define[ \t]+GLM_VERSION_PATCH[ \t]+[0-9]+")
  file(STRINGS "${GLM_INCLUDE_DIR}/glm/detail/setup.hpp" GLM_REVISION_LINE REGEX "^[ \t]*#define[ \t]+GLM_VERSION_REVISION[ \t]+[0-9]+")
  string(REGEX REPLACE "^[ \t]*#define[ \t]+GLM_VERSION_MAJOR[ \t]+([0-9]+)$" "\\1" GLM_MAJOR "${GLM_MAJOR_LINE}")
  string(REGEX REPLACE "^[ \t]*#define[ \t]+GLM_VERSION_MINOR[ \t]+([0-9]+)$" "\\1" GLM_MINOR "${GLM_MINOR_LINE}")
  string(REGEX REPLACE "^[ \t]*#define[ \t]+GLM_VERSION_PATCH[ \t]+([0-9]+)$" "\\1" GLM_PATCH "${GLM_PATCH_LINE}")
  string(REGEX REPLACE "^[ \t]*#define[ \t]+GLM_VERSION_REVISION[ \t]+([0-9]+)$" "\\1" GLM_REVISION "${GLM_REVISION_LINE}")
  set(GLM_VERSION "${GLM_MAJOR}.${GLM_MINOR}.${GLM_PATCH}.${GLM_REVISION}")
  unset(GLM_MAJOR_LINE)
  unset(GLM_MINOR_LINE)
  unset(GLM_PATCH_LINE)
  unset(GLM_REVISION_LINE)
  unset(GLM_MAJOR)
  unset(GLM_MINOR)
  unset(GLM_PATCH)
  unset(GLM_REVISION)
endif ()

find_package_handle_standard_args(glm
  REQUIRED_VARS GLM_INCLUDE_DIR
  VERSION_VAR GLM_VERSION
)

mark_as_advanced( GLM_FOUND )
