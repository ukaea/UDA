# CPack configuration: a tarball everywhere, plus the native DEB or RPM package on Linux.
#
# Included after every install() rule that belongs in a package, so the configuration files
# generated during configuration are known. DEB and RPM packages install under
# CMAKE_INSTALL_PREFIX (see CPackProjectConfig.cmake.in), which is why the conffiles list is built
# from the UDA_INSTALL_FULL_* variables.

include( InstallRequiredSystemLibraries )

# The development component's shared client targets (UDA::client_shared) load the runtime library,
# so the development package must declare its dependency on the runtime package.
if( BUILD_SHARED_LIBS )
  set( CPACK_COMPONENT_DEVELOPMENT_DEPENDS client-runtime )
endif()

set( CPACK_PACKAGE_DESCRIPTION_SUMMARY "Universal Data Access library" )
set( CPACK_PACKAGE_VENDOR "UKAEA" )
set( CPACK_PACKAGE_CONTACT "jonathan.hollocombe@ukaea.uk" )
set( CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}" )
set( CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}" )
set( CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}" )
set( CPACK_PACKAGE_INSTALL_DIRECTORY "uda-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}" )
set( CPACK_PACKAGE_FILE_NAME "uda-install-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}" )
set( CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}" )
set( CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_LIST_DIR}/packaging/description.txt" )
set( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENCE.txt" )
set( CPACK_STRIP_FILES "bin/uda_server" )
set( CPACK_SOURCE_STRIP_FILES "" )

# Package the tarball always, and the native package for the host distribution.
set( CPACK_GENERATOR TGZ )
if( EXISTS "/etc/redhat-release" )
  list( APPEND CPACK_GENERATOR RPM )
elseif( EXISTS "/etc/debian_version" )
  list( APPEND CPACK_GENERATOR DEB )
endif()

if( CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64" )
  set( CPACK_DEBIAN_PACKAGE_ARCHITECTURE amd64 )
  set( CPACK_RPM_PACKAGE_ARCHITECTURE x86_64 )
elseif( CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64" )
  set( CPACK_DEBIAN_PACKAGE_ARCHITECTURE arm64 )
  set( CPACK_RPM_PACKAGE_ARCHITECTURE aarch64 )
else()
  set( CPACK_RPM_PACKAGE_ARCHITECTURE noarch )
endif()

set( CPACK_DEBIAN_PACKAGE_NAME "uda" )
set( CPACK_DEBIAN_FILE_NAME DEB-DEFAULT )
set( CPACK_DEBIAN_PACKAGE_MAINTAINER "Jonathan Hollocombe <jonathan.hollocombe@ukaea.uk>" )
set( CPACK_DEBIAN_PACKAGE_PRIORITY "optional" )
set( CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON )
set( CPACK_DEBIAN_PACKAGE_LICENSE "Apache-2.0" )
set( CPACK_DEBIAN_PACKAGE_DESCRIPTION "Universal Data Access (UDA)" )

set( CPACK_RPM_PACKAGE_NAME "uda" )
set( CPACK_RPM_PACKAGE_SUMMARY "UDA RPM package" )
set( CPACK_RPM_FILE_NAME RPM-DEFAULT )
set( CPACK_RPM_PACKAGE_VERSION "${PROJECT_VERSION}" )
set( CPACK_RPM_PACKAGE_RELEASE 1 )
set( CPACK_RPM_PACKAGE_LICENSE "Apache, version 2" )
set( CPACK_RPM_PACKAGE_RELOCATABLE TRUE )
set( CPACK_RPM_PACKAGE_AUTOREQ 1 )
set( CPACK_RPM_PACKAGE_AUTOPROV 1 )

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
set( CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${CMAKE_CURRENT_LIST_DIR}/packaging/rpm-postun" )

include( CPack )
