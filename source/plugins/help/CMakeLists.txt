########################################################################################################################
# Dependencies

find_package( LibXml2 QUIET )
find_package( fmt REQUIRED )

if( ENABLE_OIDC_AUTH )
  find_package( CURL REQUIRED )
  set( HELP_AUTHZ_LINK_LIBS CURL::libcurl )
  # HELP::authorise() calls out over HTTP. It compiles the shared http_get translation
  # unit directly rather than linking the server auth objects, because plugins link the
  # UDA client library, which is deliberately free of any libcurl dependency.
  set( HELP_AUTHZ_SOURCES ${CMAKE_SOURCE_DIR}/source/authentication/http_get.cpp )
endif()

if( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping help plugin" )
  return()
endif()

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
  include_directories( ${XDR_INCLUDE_DIR} )
else()
  find_package( TIRPC QUIET )
  if( TIRPC_FOUND )
    include_directories( ${TIRPC_INCLUDE_DIR} )
    add_definitions(-D__TIRPC__)   
  endif()
endif()

include( plugins )

uda_plugin(
  NAME HELP
  ENTRY_FUNC helpPlugin
  DESCRIPTION "Service Discovery: list the details on all registered services"
  EXAMPLE "HELP::services()"
  LIBNAME help_plugin
  SOURCES help_plugin.cpp ${HELP_AUTHZ_SOURCES}
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    fmt::fmt
    ${HELP_AUTHZ_LINK_LIBS}
)
