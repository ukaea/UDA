# - Try to find plplot
#
# To provide the module with a hint about where to find your plplot
# installation, you can set the environment variable PLPLOT_ROOT. The Find
# module will then look in this path when searching for plplot paths and
# libraries.
#
# Once done this will define
#  PLPLOT_FOUND - System has plplot
#  PLPLOT_INCLUDE_DIRS - The plplot include directories
#  PLPLOT_LIBRARIES - The libraries needed to use plplot

if( PLPLOT_INCLUDE_DIRS AND PLPLOT_LIBRARIES )
  # Already in cache, be silent
  set( PLPLOT_FIND_QUIETLY TRUE )
endif( PLPLOT_INCLUDE_DIRS AND PLPLOT_LIBRARIES )

find_path( PLPLOT_INCLUDE_DIR plplot.h
  PATHS /usr /usr/local
  HINTS ${PLPLOT_ROOT}
    ENV PLPLOT_ROOT
  PATH_SUFFIXES include/plplot
  #NO_DEFAULT_PATH
  #NO_SYSTEM_ENVIRONMENT_PATH
  )

find_library( PLPLOT_LIBRARY NAMES plplotd
  PATHS /usr /usr/local
  HINTS ${PLPLOT_ROOT}
    ENV PLPLOT_ROOT
  PATH_SUFFIXES lib lib64
  #NO_DEFAULT_PATH
  #NO_SYSTEM_ENVIRONMENT_PATH
  )

set( PLPLOT_LIBRARIES ${PLPLOT_LIBRARY} )
set( PLPLOT_INCLUDE_DIRS ${PLPLOT_INCLUDE_DIR} )

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( plplot DEFAULT_MSG PLPLOT_LIBRARY PLPLOT_INCLUDE_DIR )

mark_as_advanced( PLPLOT_LIBRARIES PLPLOT_INCLUDE_DIRS )
