cmake_minimum_required( VERSION 2.8 )

find_package( LibXml2 QUIET )

if( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping LiveDisplay plugin" )
  return()
endif()

include( plugins )

idam_plugin(
  NAME LIVEDISPLAY
  ENTRY_FUNC livedisplay
  DESCRIPTION "Live Display IMAS MAGNETICS IDS plugin"
  EXAMPLE "LIVEDISPLAY::get()"
  LIBNAME livedisplay_plugin
  SOURCES livedisplay.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/clientserver
    ${CMAKE_SOURCE_DIR}/structures
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
)

target_link_libraries( livedisplay_plugin LINK_PUBLIC client-static )