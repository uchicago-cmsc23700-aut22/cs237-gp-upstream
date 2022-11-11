##=============================================================================
##
## COPYRIGHT (c) 2022 John Reppy
## All rights reserved.
##
##=============================================================================
#
# Find module for Vulkan headers and library.
# Once done this will define
#
# VULKAN_FOUND
# VULKAN_BIN_DIR
# VULKAN_INCLUDE_DIR
# VULKAN_LIBRARY
#


include(FindPackageHandleStandardArgs)

if (WIN32)

# not sure what to do for Windows

endif ()

if (${CMAKE_HOST_UNIX})
    find_path( VULKAN_INCLUDE_DIR
        NAMES
            vulkan/vulkan.h
        PATHS
            ${VULKAN_SDK}/include
            $ENV{VULKAN_SDK}/include
            /usr/include
            /usr/local/include
            /sw/include
            /opt/local/include
            NO_DEFAULT_PATH
            DOC "The directory where vulkan/vulkan.h resides"
    )
    find_library( VULKAN_LIBRARY
        NAMES
            vulkan
        PATHS
            ${VULKAN_SDK}/lib
            ${VULKAN_SDK}/macOS/lib
            $ENV{VULKAN_SDK}/lib
            $ENV{VULKAN_SDK}/macOS/lib
            /usr/lib64
            /usr/lib
            /usr/local/lib64
            /usr/local/lib
            /sw/lib
            /opt/local/lib
            /usr/lib/x86_64-linux-gnu
            NO_DEFAULT_PATH
            DOC "The VULKAN library")
    find_path( VULKAN_BIN_DIR
        NAMES
            glslangValidator
        PATHS
            ${VULKAN_SDK}/bin
            ${VULKAN_SDK}/macOS/bin
            $ENV{VULKAN_SDK}/bin
            $ENV{VULKAN_SDK}/macOS/bin
            /usr/bin64
            /usr/bin
            /usr/local/bin64
            /usr/local/bin
            /sw/bin
            /opt/local/bin
            /usr/bin/x86_64-linux-gnu
            NO_DEFAULT_PATH
            DOC "The directory where glslangValidator resides")
endif ()

find_package_handle_standard_args(vulkan DEFAULT_MSG
    VULKAN_INCLUDE_DIR
    VULKAN_LIBRARY
    VULKAN_BIN_DIR
)

mark_as_advanced( VULKAN_FOUND )
