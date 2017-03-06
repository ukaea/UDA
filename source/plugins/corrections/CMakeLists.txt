cmake_minimum_required( VERSION 2.8 )

find_package( LibXml2 QUIET )

if( NOT LIBXML2_FOUND )
  message( "Libxml2 not found - skipping corrections plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME ANBCORRECTIONS
  ENTRY_FUNC anbCorrections
  DESCRIPTION "Corrections to ANB Data"
  EXAMPLE "ANBCORRECTIONS::chequerBoardPower(signal=ANB_SW_FULL_POWER)"
  LIBNAME corrections_plugin
  SOURCES anbCorrections.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/client
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    m
)
