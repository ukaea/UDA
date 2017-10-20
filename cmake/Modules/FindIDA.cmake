# - Find Ida
#
# To provide the module with a hint about where to find your IDA
# installation, you can set the environment variable IDA_ROOT. The
# Find module will then look in this path when searching for IDA paths
# and libraries.
#
# Find the Ida includes and libraries
#
#  IDA_INCLUDE_DIR - where to find mdslib.h, etc
#  IDA_LIBRARIES   - Link these libraries when using Ida
#  IDA_FOUND       - True if Ida found
#
# Normal usage would be:
#  find_package (Ida REQUIRED)
#  target_link_libraries (uses_ida ${IDA_LIBRARIES})

if( IDA_INCLUDE_DIR AND IDA_LIBRARIES )
  # Already in cache, be silent
  set( IDA_FIND_QUIETLY TRUE )
endif( IDA_INCLUDE_DIR AND IDA_LIBRARIES )

find_path( IDA_INCLUDE_DIR ida3.h
  HINTS ${IDA_ROOT}
  ENV IDA3_DIR
  ENV IDA_DIR
  PATH_SUFFIXES include include/ida3 )

find_library( IDA_LIBRARIES_LIB NAMES ida3
  HINTS ${IDA_ROOT}
  ENV IDA3_DIR
  ENV IDA_DIR
  PATH_SUFFIXES lib lib64 )

find_library( IDA_C_LIBRARIES_LIB NAMES ida3c
  HINTS ${IDA_ROOT}
  ENV IDA3_DIR
  ENV IDA_DIR
  PATH_SUFFIXES lib lib64 )

set( IDA_LIBRARIES ${IDA_LIBRARIES_LIB} ${IDA_C_LIBRARIES_LIB} )

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( IDA DEFAULT_MSG IDA_LIBRARIES IDA_INCLUDE_DIR )

mark_as_advanced( IDA_LIBRARIES IDA_INCLUDE_DIR )
