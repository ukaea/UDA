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
#  IDL_VERSION     - SDK version read from idl_export.h (no interpreter execution)
#  IDL::IDL        - Imported SDK library and include directory
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
  PATH_SUFFIXES external/include )

set( _IDL_ARCH "${CMAKE_SYSTEM_PROCESSOR}" )
if( APPLE AND CMAKE_OSX_ARCHITECTURES )
  list( LENGTH CMAKE_OSX_ARCHITECTURES _IDL_ARCH_COUNT )
  if( _IDL_ARCH_COUNT GREATER 1 )
    message( FATAL_ERROR "Select one CMAKE_OSX_ARCHITECTURES value to match the IDL SDK" )
  endif()
  set( _IDL_ARCH "${CMAKE_OSX_ARCHITECTURES}" )
endif()
string( TOLOWER "${_IDL_ARCH}" _IDL_ARCH )
if( _IDL_ARCH MATCHES "^(arm64|aarch64)$" )
  if( APPLE )
    set( _IDL_ARCH arm64 )
  else()
    set( _IDL_ARCH aarch64 )
  endif()
elseif( CMAKE_SIZEOF_VOID_P EQUAL 8 )
  set( _IDL_ARCH x86_64 )
else()
  set( _IDL_ARCH x86 )
endif()

if( WIN32 )
  set( IDL_PATH_SUFFIX "bin.${_IDL_ARCH}" )
elseif( APPLE )
  set( IDL_PATH_SUFFIX "bin.darwin.${_IDL_ARCH}" )
else()
  string( TOLOWER "${CMAKE_SYSTEM_NAME}" _IDL_SYSTEM )
  set( IDL_PATH_SUFFIX "bin.${_IDL_SYSTEM}.${_IDL_ARCH}" )
endif()

find_library( IDL_LIBRARIES NAMES idl
  HINTS ${IDL_ROOT}
  ENV IDL_DIR
  ENV IDL_ROOT
  PATH_SUFFIXES bin/${IDL_PATH_SUFFIX} )

unset( IDL_VERSION )
if( IDL_INCLUDES AND EXISTS "${IDL_INCLUDES}/idl_export.h" )
  file( STRINGS "${IDL_INCLUDES}/idl_export.h" _IDL_VERSION_LINES
    REGEX "^#[ \t]*define[ \t]+IDL_VERSION_(MAJOR|MINOR|SUB)[ \t]+[0-9]+" )
  foreach( _IDL_PART IN ITEMS MAJOR MINOR SUB )
    string( REGEX MATCH "IDL_VERSION_${_IDL_PART}[ \t]+([0-9]+)" _IDL_MATCH "${_IDL_VERSION_LINES}" )
    set( _IDL_VERSION_${_IDL_PART} "${CMAKE_MATCH_1}" )
  endforeach()
  if( NOT "${_IDL_VERSION_MAJOR}" STREQUAL "" AND
      NOT "${_IDL_VERSION_MINOR}" STREQUAL "" AND
      NOT "${_IDL_VERSION_SUB}" STREQUAL "" )
    set( IDL_VERSION "${_IDL_VERSION_MAJOR}.${_IDL_VERSION_MINOR}.${_IDL_VERSION_SUB}" )
  endif()
endif()

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( IDL
  REQUIRED_VARS IDL_LIBRARIES IDL_INCLUDES
  VERSION_VAR IDL_VERSION
)

mark_as_advanced( IDL_LIBRARIES IDL_INCLUDES )

if( IDL_FOUND AND NOT TARGET IDL::IDL )
  add_library( IDL::IDL UNKNOWN IMPORTED )
  set_target_properties( IDL::IDL PROPERTIES
    IMPORTED_LOCATION "${IDL_LIBRARIES}"
    INTERFACE_INCLUDE_DIRECTORIES "${IDL_INCLUDES}"
  )
endif()
