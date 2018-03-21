find_package( LibXml2 QUIET )
find_package( LibSSH QUIET )

if( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping west_tunnel plugin" )
  return()
elseif( NOT LIBSSH_FOUND )
  message( WARNING "Libssh not found - skipping west_tunnel plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME WEST_TUNNEL
  ENTRY_FUNC west_tunnel
  DESCRIPTION "Standardised Plugin Template"
  EXAMPLE "WEST_TUNNEL::read()"
  LIBNAME west_tunnel_plugin
  SOURCES west_tunnel.c west_tunnel_ssh.c west_tunnel_ssh_server.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${LIBSSH_INCLUDE_DIRS}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${LIBSSH_LIBRARIES}
    client-static
    pthread
    resolv
)
