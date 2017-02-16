cmake_minimum_required( VERSION 2.8 )

find_package( PostgreSQL QUIET )
find_package( LibXml2 QUIET )
find_package( OpenSSL QUIET )

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

include( plugins )

uda_plugin(
  NAME PROVENANCE
  ENTRY_FUNC admin
  DESCRIPTION "Issue and Register Unique Identifiers"
  EXAMPLE "PROVENANCE::get()"
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
