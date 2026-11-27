# Metadata for the Linux packages (DEB and RPM) built by CPack.
#
# Included after every install() rule that belongs in a package, so the configuration files
# generated during configuration are known. DEB and RPM packages install under
# CMAKE_INSTALL_PREFIX (see CPackProjectConfig.cmake.in), which is why these lists are built
# from the UDA_INSTALL_FULL_* variables.

# Debian copyright file (DEP-5). The licence text itself comes from the system.
configure_file(
  "${CMAKE_CURRENT_LIST_DIR}/packaging/copyright.in"
  "${CMAKE_BINARY_DIR}/packaging/copyright"
  @ONLY
)
install( FILES "${CMAKE_BINARY_DIR}/packaging/copyright"
  DESTINATION "${CMAKE_INSTALL_DOCDIR}"
)

configure_file(
  "${CMAKE_CURRENT_LIST_DIR}/packaging/CPackProjectConfig.cmake.in"
  "${CMAKE_BINARY_DIR}/packaging/CPackProjectConfig.cmake"
  @ONLY
)
set( CPACK_PROJECT_CONFIG_FILE "${CMAKE_BINARY_DIR}/packaging/CPackProjectConfig.cmake" )

# Conffiles: anything an administrator may edit, so that package upgrades keep local changes.
# The server configuration, machine.d and plugin files are only installed by server builds.
set( _UDA_CONFFILES )

if( NOT CLIENT_ONLY )
  foreach( _NAME IN ITEMS udaserver.cfg xinetd.conf udagenstruct.conf README.md launchd.udaserver.plist )
    if( EXISTS "${CMAKE_BINARY_DIR}/etc/${_NAME}" )
      list( APPEND _UDA_CONFFILES "${UDA_INSTALL_FULL_CONFIGDIR}/${_NAME}" )
    endif()
  endforeach()

  file( GLOB _MACHINE_FILES "${CMAKE_SOURCE_DIR}/source/etc/machine.d/*" )
  foreach( _FILE IN LISTS _MACHINE_FILES )
    get_filename_component( _NAME "${_FILE}" NAME )
    list( APPEND _UDA_CONFFILES "${UDA_INSTALL_FULL_MACHINE_CONFIGDIR}/${_NAME}" )
  endforeach()

  file( GLOB _PLUGIN_CONF_FILES
    "${CMAKE_BINARY_DIR}/source/plugins/udaPlugins.conf"
    "${CMAKE_BINARY_DIR}/source/plugins/test_plugin.cpp"
    "${CMAKE_BINARY_DIR}/source/plugins/*/udaPlugins_*.conf" )
  foreach( _FILE IN LISTS _PLUGIN_CONF_FILES )
    get_filename_component( _NAME "${_FILE}" NAME )
    list( APPEND _UDA_CONFFILES "${UDA_INSTALL_FULL_PLUGIN_CONFIGDIR}/${_NAME}" )
  endforeach()

  file( GLOB _FRAGMENT_FILES "${CMAKE_BINARY_DIR}/etc/plugins.d/*" )
  foreach( _FILE IN LISTS _FRAGMENT_FILES )
    get_filename_component( _NAME "${_FILE}" NAME )
    list( APPEND _UDA_CONFFILES "${UDA_INSTALL_FULL_PLUGIN_FRAGMENTDIR}/${_NAME}" )
  endforeach()
endif()

list( JOIN _UDA_CONFFILES "\n" _UDA_CONFFILES_TEXT )
file( REMOVE "${CMAKE_BINARY_DIR}/packaging/conffiles" )
set( CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA )
if( _UDA_CONFFILES )
  file( WRITE "${CMAKE_BINARY_DIR}/packaging/conffiles" "${_UDA_CONFFILES_TEXT}\n" )
  list( APPEND CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_BINARY_DIR}/packaging/conffiles" )
endif()

# Debian maintainer scripts: shared libraries are registered through the ldconfig trigger, and
# the shlibs file is generated from the installed libraries.
set( CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON )
# install_plugin and the log parser are Python scripts; the RPM gets the same requirement from the shebang.
set( CPACK_DEBIAN_PACKAGE_DEPENDS "python3" )
list( APPEND CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
  "${CMAKE_CURRENT_LIST_DIR}/packaging/triggers"
  "${CMAKE_CURRENT_LIST_DIR}/packaging/postinst"
  "${CMAKE_CURRENT_LIST_DIR}/packaging/postrm"
)

set( CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_LIST_DIR}/packaging/rpm-post" )
