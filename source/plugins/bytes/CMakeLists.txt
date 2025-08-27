########################################################################################################################
# Dependencies
find_package( LibXml2 QUIET )
find_package( Boost COMPONENTS )

if( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping template plugin" )
  return()
endif()

include( plugins )

set ( STDFS_VAR "" )
if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 9.1 )
    set ( STDFS_VAR "stdc++fs" )
elseif ( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 9.0 )
    set ( STDFS_VAR "c++fs" )
endif()

uda_plugin(
  NAME BYTES
  ENTRY_FUNC bytesPlugin
  DESCRIPTION "data reader to access files as a block of bytes without interpretation"
  EXAMPLE "BYTES::read()"
  LIBNAME bytes_plugin
  SOURCES bytesPlugin.cpp readBytesNonOptimally.cpp md5Sum.cpp ../server/filesystemUtils.cpp
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ../server/filesystemUtils.h
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    Boost::boost
    ${STDFS_VAR}
)
