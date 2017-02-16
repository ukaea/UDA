cmake_minimum_required( VERSION 2.8 )

find_package( PostgreSQL QUIET )

if( NOT PostgreSQL_FOUND )
  message( WARNING "PostgreSQL not found - skipping XpadTree plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME XPADTREE
  ENTRY_FUNC idamXpadTree
  DESCRIPTION "Reads xpad tree and tag information, for use in XPAD."
  EXAMPLE "XPADTREE::gettags()"
  LIBNAME xpadtree_plugin
  SOURCES xpadtree.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${PostgreSQL_INCLUDE_DIRS}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${PostgreSQL_LIBRARIES}
)
