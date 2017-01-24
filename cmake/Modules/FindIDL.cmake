# - Find IDL
#
# To provide the module with a hint about where to find your IDL
# installation, you can set the environment variable IDL_ROOT. The
# Find module will then look in this path when searching for IDL paths
# and libraries.
#
# Find the IDL includes and libraries
#
#  IDL_INCLUDES    - where to find idl_export.h, etc
#  IDL_LIBRARIES   - Link these libraries when using IDL
#  IDL_FOUND       - True if IDL found
#
# Normal usage would be:
#  find_package (IDL REQUIRED)
#  target_link_libraries (uses_IDL ${IDL_LIBRARIES})

if( IDL_INCLUDES AND IDL_LIBRARIES )
  # Already in cache, be silent
  set( IDL_FIND_QUIETLY TRUE )
endif( IDL_INCLUDES AND IDL_LIBRARIES )

find_path( IDL_INCLUDES idl_export.h
  HINTS ${IDL_ROOT}
  ENV IDL_DIR
  ENV IDL_ROOT
  ENV IDL_DIR
  PATH_SUFFIXES external/include )

if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
  set( IDL_PATH_SUFFIX bin.linux.x86_64 )
else()
  set( IDL_PATH_SUFFIX bin.linux.x86 )
endif()

find_library( IDL_LIBRARIES NAMES idl
  HINTS ${IDL_ROOT}
  ENV IDL_DIR
  ENV IDL_ROOT
  ENV IDL_DIR
  PATH_SUFFIXES bin/${IDL_PATH_SUFFIX} )

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( IDL DEFAULT_MSG IDL_LIBRARIES IDL_INCLUDES )

mark_as_advanced( IDL_LIBRARIES IDL_INCLUDES )
