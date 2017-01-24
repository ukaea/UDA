# - Try to find UDUNITS
#
# To provide the module with a hint about where to find your UDUNITS
# installation, you can set the environment variable UDUNITS_ROOT. The Find
# module will then look in this path when searching for UDUNITS paths and
# libraries.
#
# Once done this will define
#  UDUNITS_FOUND - System has UDUNITS
#  UDUNITS_INCLUDE_DIRS - The UDUNITS include directories
#  UDUNITS_LIBRARIES - The libraries needed to use UDUNITS

if( UDUNITS_INCLUDE_DIRS AND UDUNITS_LIBRARIES )
    # Already in cache, be silent
    set( UDUNITS_FIND_QUIETLY TRUE )
endif( UDUNITS_INCLUDE_DIRS AND UDUNITS_LIBRARIES )

find_path( UDUNITS_INCLUDE_DIR udunits2.h
  HINTS ${UDUNITS_ROOT}
    ENV UDUNITS_ROOT
  PATH_SUFFIXES include
)

find_library( UDUNITS_LIBRARY NAMES udunits2
  HINTS ${UDUNITS_ROOT}
    ENV UDUNITS_ROOT
  PATH_SUFFIXES lib
)

set( UDUNITS_LIBRARIES ${UDUNITS_LIBRARY} )
set( UDUNITS_INCLUDE_DIRS ${UDUNITS_INCLUDE_DIR} )

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( UDUNITS DEFAULT_MSG UDUNITS_LIBRARY UDUNITS_INCLUDE_DIR )

mark_as_advanced( UDUNITS_LIBRARIES UDUNITS_INCLUDE_DIRS )
