find_package( LibXml2 QUIET )
find_package( MDSplus QUIET )
find_package( LibSSH QUIET )
find_package( GTK3 QUIET )

if( NOT MDSPLUS_FOUND )
  message( WARNING "mdsplus not found - skipping exp2imas plugin" )
  return()
elseif( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping exp2imas plugin" )
  return()
elseif( NOT LIBSSH_FOUND )
  message( WARNING "Libssh not found - skipping exp2imas plugin" )
  return()
elseif( NOT GTK3_FOUND )
  # Currently does not work without GTK3 gui
  message( WARNING "GTK3 not found - skipping exp2imas plugin" )
  return()
endif()

include( plugins )

set( SOURCES
  exp2imas_plugin.c
  exp2imas_mds.c
  exp2imas_xml.c
  exp2imas_ssh_server.c
  exp2imas_ssh.c
  exp2imas_ramCache.c
)

if( GTK3_FOUND )
  link_directories( ${GTK3_LIBRARY_DIRS} )
  add_definitions( -DHAVE_GTK3 )

  set( SOURCES exp2imas_ssh_gui.c ${SOURCES} )
endif()

uda_plugin(
  NAME EXP2IMAS
  ENTRY_FUNC exp2imasPlugin
  DESCRIPTION "Map IMAS element name to experiment static or dynamic data using XML mapping files"
  EXAMPLE "EXP2IMAS::read()"
  LIBNAME exp2imas_plugin
  SOURCES ${SOURCES}
  CONFIG_FILE exp2imas.cfg
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${MDSPLUS_INCLUDES}
    ${LIBSSH_INCLUDE_DIRS}
    ${GTK3_INCLUDE_DIRS}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${MDSPLUS_LIBRARIES}
    ${LIBSSH_LIBRARIES}
    ${GTK3_LIBRARIES}
    pthread
    resolv
)
