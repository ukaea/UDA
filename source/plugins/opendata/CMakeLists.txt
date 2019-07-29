########################################################################################################################
# Dependencies

find_package( PostgreSQL QUIET )
find_package( LibXml2 QUIET )

if( NOT PostgreSQL_FOUND )
  message( WARNING "PostgreSQL not found - skipping opendata plugin" )
  return()
elseif( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping opendata plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME OPENDATA
  ENTRY_FUNC openData
  DESCRIPTION "Open Data"
  EXAMPLE "OPENDATA::put()"
  LIBNAME opendata_plugin
  SOURCES openData.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${PostgreSQL_INCLUDE_DIRS}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${PostgreSQL_LIBRARY}
)
