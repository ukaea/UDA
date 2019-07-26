# Macro used to convert Windows path to Unix
macro( CONVERT_WIN_PATH VarName )
  if( MINGW AND NOT "${${VarName}}" STREQUAL "" )
    execute_process( COMMAND cygpath.exe -u ${${VarName}} OUTPUT_VARIABLE ${VarName} )
    string( STRIP ${${VarName}} ${VarName} )
    string( REGEX REPLACE "[\r\n]+" ";" ${VarName} ${${VarName}} )
  endif()
endmacro()

########################################################################################################################
# Dependencies
find_package( PostgreSQL QUIET )
find_package( LibXml2 QUIET )
find_package( OpenSSL QUIET )
if( MINGW )
  CONVERT_WIN_PATH( LIBXML2_INCLUDE_DIR )
  CONVERT_WIN_PATH( LIBXML2_LIBRARIES )
endif()

if( NOT PostgreSQL_FOUND )
  message( WARNING "PostgreSQL not found - skipping provenance plugin" )
  return()
elseif( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping provenance plugin" )
  return()
elseif( NOT OPENSSL_FOUND )
  message( WARNING "OpenSSL not found - skipping provenance plugin" )
  return()
endif()


if( WIN32 OR MINGW )
  # Add flag to avoid error with uninitialized variable
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error=maybe-uninitialized" )
endif()

include( plugins )

uda_plugin(
  NAME PROV
  ENTRY_FUNC admin
  DESCRIPTION "Issue and Register Unique Identifiers"
  EXAMPLE "PROV::get()"
  LIBNAME provenance_plugin
  SOURCES
    provenance.c
    help.c
    get.c
    status.c
    put.c
    putSignal.c
    listSignals.c
  EXTRA_INCLUDE_DIRS
    ${PostgreSQL_INCLUDE_DIRS}
    ${LIBXML2_INCLUDE_DIR}
    ${OPENSSL_INCLUDE_DIR}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${PostgreSQL_LIBRARY}
    ${OPENSSL_LIBRARIES}
)
