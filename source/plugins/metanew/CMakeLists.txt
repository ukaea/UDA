########################################################################################################################
# Dependencies

find_package( PostgreSQL QUIET )
find_package( LibXml2 QUIET )

if( NOT PostgreSQL_FOUND )
  message( WARNING "PostgreSQL not found - skipping meta new plugin" )
  return()
elseif( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping meta new plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME METANEW
  ENTRY_FUNC readMetaNew
  DESCRIPTION "Meta New Data Plugin"
  EXAMPLE "METANEW::listsignals()"
  LIBNAME metanew_plugin
  SOURCES 
    readMetaNew.c
    readIdamMeta.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${PostgreSQL_INCLUDE_DIRS}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${PostgreSQL_LIBRARY}
)
