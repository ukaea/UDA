# - Try to find IMAS
#
# To provide the module with a hint about where to find your IMAS
# installation, you can set the environment variable IMAS_ROOT. The Find
# module will then look in this path when searching for IMAS paths and
# libraries.
#
# Once done this will define
#  IMAS_FOUND - System has IMAS
#  IMAS_INCLUDE_DIRS - The IMAS include directories
#  IMAS_LIBRARIES - The libraries needed to use IMAS

if( IMAS_INCLUDE_DIRS AND IMAS_LIBRARIES )
  # Already in cache, be silent
  set( IMAS_FIND_QUIETLY TRUE )
endif( IMAS_INCLUDE_DIRS AND IMAS_LIBRARIES )

find_path( IMAS_INCLUDE_DIR ual_low_level.h
  HINTS ${IMAS_ROOT}
    ENV IMAS_ROOT
  PATH_SUFFIXES lowlevel
)

find_library( IMAS_LIBRARY NAMES UALLowLevel
  HINTS ${IMAS_ROOT}
    ENV IMAS_ROOT
  PATH_SUFFIXES lowlevel
)

set( IMAS_LIBRARIES ${IMAS_LIBRARY} )
set( IMAS_INCLUDE_DIRS ${IMAS_INCLUDE_DIR} )

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( IMAS DEFAULT_MSG IMAS_LIBRARY IMAS_INCLUDE_DIR )

mark_as_advanced( IMAS_LIBRARIES IMAS_INCLUDE_DIRS )

