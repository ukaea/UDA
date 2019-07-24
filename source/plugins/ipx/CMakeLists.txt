find_package( Jasper QUIET )

if( NOT JASPER_FOUND )
  message( WARNING "Jasper not found - skipping IPX plugin" )
  return()
endif()

if( WIN32 OR MINGW )
  # Add flag to avoid error with strftime()
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=format" )
  # Add flag to avoid error with implicit declaration
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=deprecated-copy" )
  # Add flag to avoid error with strncpy()
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=sizeof-pointer-memaccess" )
endif()

include( plugins )

uda_plugin(
  NAME NEWIPX
  ENTRY_FUNC ipxPlugin
  DESCRIPTION "IPX Data plugin"
  EXAMPLE ""
  LIBNAME ipx_plugin
  SOURCES ipx_plugin.cpp ipx.cpp jimage.c
  EXTRA_INCLUDE_DIRS
    ${JASPER_INCLUDE_DIRS}
  EXTRA_LINK_LIBS
    ${JASPER_LIBRARIES}
	stdc++
)