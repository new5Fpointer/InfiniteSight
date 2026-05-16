# FindVIPS.cmake - Find libvips (C and C++ bindings)
# This module respects VIPS_ROOT hint variable/env-var.
# After finding, it sets:
#   VIPS_FOUND         - TRUE if found
#   VIPS_INCLUDE_DIRS  - include dirs (C + C++ + glib)
#   VIPS_LIBRARIES     - libraries to link (vips-cpp + vips + deps)
#   VIPS_VERSION       - version string
#   VIPS_DEFINITIONS   - compile definitions (HAS_LIBVIPS=1)

if(DEFINED VIPS_ROOT)
    set(_VIPS_ROOT_HINTS "${VIPS_ROOT}")
elseif(DEFINED ENV{VIPS_ROOT})
    set(_VIPS_ROOT_HINTS "$ENV{VIPS_ROOT}")
endif()

find_path(VIPS_C_INCLUDE_DIR
    vips/vips.h
    HINTS ${_VIPS_ROOT_HINTS}
    PATH_SUFFIXES include
)

find_path(VIPS_CXX_INCLUDE_DIR
    vips/vips8
    HINTS ${_VIPS_ROOT_HINTS}
    PATH_SUFFIXES include
)

find_library(VIPS_C_LIBRARY
    NAMES vips libvips
    HINTS ${_VIPS_ROOT_HINTS}
    PATH_SUFFIXES lib
)

find_library(VIPS_CXX_LIBRARY
    NAMES vips-cpp libvips-cpp
    HINTS ${_VIPS_ROOT_HINTS}
    PATH_SUFFIXES lib
)

find_path(VIPS_GLIB_INCLUDE_DIR
    glib-object.h
    HINTS ${_VIPS_ROOT_HINTS}
    PATH_SUFFIXES include/glib-2.0
)

find_path(VIPS_GLIB_CONFIG_INCLUDE_DIR
    glibconfig.h
    HINTS ${_VIPS_ROOT_HINTS}
    PATH_SUFFIXES lib/glib-2.0/include
)

set(VIPS_REQUIRED_INCLUDES
    VIPS_C_INCLUDE_DIR
    VIPS_CXX_INCLUDE_DIR
)

set(VIPS_REQUIRED_LIBRARIES
    VIPS_C_LIBRARY
    VIPS_CXX_LIBRARY
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VIPS
    REQUIRED_VARS VIPS_C_INCLUDE_DIR VIPS_CXX_INCLUDE_DIR
                  VIPS_C_LIBRARY VIPS_CXX_LIBRARY
    FAIL_MESSAGE "Could NOT find libvips. Set VIPS_ROOT to a vips-dev installation."
)

if(VIPS_FOUND)
    set(VIPS_INCLUDE_DIRS
        ${VIPS_C_INCLUDE_DIR}
        ${VIPS_CXX_INCLUDE_DIR}
    )
    if(VIPS_GLIB_INCLUDE_DIR)
        list(APPEND VIPS_INCLUDE_DIRS ${VIPS_GLIB_INCLUDE_DIR})
    endif()
    if(VIPS_GLIB_CONFIG_INCLUDE_DIR)
        list(APPEND VIPS_INCLUDE_DIRS ${VIPS_GLIB_CONFIG_INCLUDE_DIR})
    endif()

    set(VIPS_LIBRARIES
        ${VIPS_CXX_LIBRARY}
        ${VIPS_C_LIBRARY}
    )

    set(VIPS_DEFINITIONS HAS_LIBVIPS=1)

    if(EXISTS "${VIPS_C_INCLUDE_DIR}/vips/version.h")
        file(READ "${VIPS_C_INCLUDE_DIR}/vips/version.h" _VIPS_VERSION_H)
        string(REGEX MATCH "#define VIPS_VERSION[ \t]+\"([0-9.]+)\"" _VIPS_VERSION_MATCH "${_VIPS_VERSION_H}")
        if(_VIPS_VERSION_MATCH)
            set(VIPS_VERSION "${CMAKE_MATCH_1}")
        endif()
    endif()

    mark_as_advanced(
        VIPS_C_INCLUDE_DIR
        VIPS_CXX_INCLUDE_DIR
        VIPS_C_LIBRARY
        VIPS_CXX_LIBRARY
        VIPS_GLIB_INCLUDE_DIR
        VIPS_GLIB_CONFIG_INCLUDE_DIR
    )
endif()
