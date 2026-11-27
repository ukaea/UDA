include( GNUInstallDirs )

# Windows library naming. A static archive and the import library of a shared library would both be
# named *.lib, so static archives carry a _static suffix there. Unix archives keep their names
# (libuda_client.a), which pkg-config and existing link lines rely on.
if( WIN32 )
  set( UDA_STATIC_LIB_SUFFIX "_static" )
  if( MINGW )
    set( IMPLIB_SUFFIX ".a" )
  else()
    set( IMPLIB_SUFFIX ".lib" )
  endif()
else()
  set( UDA_STATIC_LIB_SUFFIX "" )
endif()

set( UDA_INSTALL_LAYOUT "LEGACY" CACHE STRING
  "UDA installation layout (LEGACY or SYSTEM)" )
set_property( CACHE UDA_INSTALL_LAYOUT PROPERTY STRINGS LEGACY SYSTEM )
string( TOUPPER "${UDA_INSTALL_LAYOUT}" UDA_INSTALL_LAYOUT )

if( NOT UDA_INSTALL_LAYOUT STREQUAL "LEGACY" AND
    NOT UDA_INSTALL_LAYOUT STREQUAL "SYSTEM" )
  message( FATAL_ERROR
    "Unsupported UDA_INSTALL_LAYOUT='${UDA_INSTALL_LAYOUT}'. "
    "Expected LEGACY or SYSTEM." )
endif()

if( UDA_INSTALL_LAYOUT STREQUAL "LEGACY" )
  set( UDA_INSTALL_BINDIR "bin" )
  set( UDA_INSTALL_TOOLSDIR "bin" )
  set( UDA_INSTALL_LIBDIR "lib" )
  set( UDA_INSTALL_INCLUDEDIR "include/uda" )
  set( UDA_INSTALL_PKGCONFIGDIR "lib/pkgconfig" )
  set( UDA_INSTALL_CMAKEDIR "lib/cmake/UDA" )
  set( UDA_INSTALL_CONFIGDIR "etc" )
  set( UDA_INSTALL_SERVER_SCRIPTDIR "etc" )
  set( UDA_INSTALL_SERVICEDIR "etc" )
  set( UDA_INSTALL_LOGDIR "etc" )
  set( UDA_INSTALL_RUNDIR "etc" )
  set( UDA_INSTALL_PYTHONDIR "python" )
  set( UDA_INSTALL_PLUGINDIR "lib/plugins" )
  set( UDA_INSTALL_PLUGIN_CONFIGDIR "etc/plugins" )
  set( UDA_INSTALL_PLUGIN_FRAGMENTDIR "etc/plugins.d" )
  set( UDA_INSTALL_MACHINE_CONFIGDIR "etc/machine.d" )
  set( UDA_INSTALL_MODULEDIR "modulefiles/uda" )
  set( UDA_INSTALL_WRAPPER_PKGCONFIGDIR "modulefiles/uda" )
  set( UDA_INSTALL_IDL_DLM_DIR "dlm" )
  set( UDA_INSTALL_FAT_IDL_DLM_DIR "fatdlm" )
  set( UDA_INSTALL_IDL_PRO_DIR "idl" )
  set( UDA_INSTALL_PYTHON_SOURCE_DIR "python_installer" )
  set( UDA_INSTALL_JAVA_DIR "java" )
  set( UDA_INSTALL_REST_INCLUDEDIR "include/udarest" )
else()
  set( UDA_INSTALL_BINDIR "${CMAKE_INSTALL_BINDIR}" )
  set( UDA_INSTALL_TOOLSDIR "${CMAKE_INSTALL_DATADIR}/uda/tools" )
  set( UDA_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}" )
  set( UDA_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}/uda" )
  set( UDA_INSTALL_PKGCONFIGDIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig" )
  set( UDA_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/UDA" )
  set( UDA_INSTALL_CONFIGDIR "${CMAKE_INSTALL_FULL_SYSCONFDIR}/uda" )
  set( UDA_INSTALL_SERVER_SCRIPTDIR "${CMAKE_INSTALL_LIBEXECDIR}/uda" )
  set( UDA_INSTALL_SYSTEMD_UNITDIR "lib/systemd/system" CACHE PATH
    "Systemd unit directory, relative to the install prefix or absolute" )
  set( UDA_INSTALL_SERVICEDIR "${UDA_INSTALL_SYSTEMD_UNITDIR}" )
  set( UDA_INSTALL_TMPFILESDIR "lib/tmpfiles.d" CACHE PATH
    "systemd-tmpfiles configuration directory, relative to the install prefix or absolute" )
  set( UDA_INSTALL_LOGDIR "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/log/uda" )
  set( UDA_INSTALL_RUNDIR "${CMAKE_INSTALL_FULL_RUNSTATEDIR}/uda" )
  set( UDA_INSTALL_PYTHONDIR "${CMAKE_INSTALL_DATADIR}/uda/python" )
  set( UDA_INSTALL_PLUGINDIR "${CMAKE_INSTALL_LIBDIR}/uda/plugins" )
  set( UDA_INSTALL_PLUGIN_CONFIGDIR "${CMAKE_INSTALL_FULL_SYSCONFDIR}/uda/plugins" )
  set( UDA_INSTALL_PLUGIN_FRAGMENTDIR "${CMAKE_INSTALL_FULL_SYSCONFDIR}/uda/plugins.d" )
  set( UDA_INSTALL_MACHINE_CONFIGDIR "${CMAKE_INSTALL_FULL_SYSCONFDIR}/uda/machine.d" )
  set( UDA_INSTALL_MODULEDIR "${CMAKE_INSTALL_DATADIR}/modulefiles/uda" )
  set( UDA_INSTALL_WRAPPER_PKGCONFIGDIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig" )
  set( UDA_INSTALL_IDL_DLM_DIR "${CMAKE_INSTALL_LIBDIR}/uda/idl" )
  set( UDA_INSTALL_FAT_IDL_DLM_DIR "${CMAKE_INSTALL_LIBDIR}/uda/fatidl" )
  set( UDA_INSTALL_IDL_PRO_DIR "${CMAKE_INSTALL_DATADIR}/uda/idl" )
  set( UDA_INSTALL_PYTHON_SOURCE_DIR "${CMAKE_INSTALL_DATADIR}/uda/python_installer" )
  set( UDA_INSTALL_JAVA_DIR "${CMAKE_INSTALL_DATADIR}/uda/java" )
  set( UDA_INSTALL_REST_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}/udarest" )
endif()

# Absolute copies of every destination, for templates and install rules. Relative destinations are
# taken from the install prefix, absolute ones are used unchanged.
foreach( _UDA_DIR IN ITEMS
    BINDIR LIBDIR INCLUDEDIR PKGCONFIGDIR CMAKEDIR CONFIGDIR SERVER_SCRIPTDIR
    SERVICEDIR LOGDIR RUNDIR PYTHONDIR PLUGINDIR PLUGIN_CONFIGDIR PLUGIN_FRAGMENTDIR
    MACHINE_CONFIGDIR MODULEDIR WRAPPER_PKGCONFIGDIR IDL_DLM_DIR FAT_IDL_DLM_DIR IDL_PRO_DIR
    PYTHON_SOURCE_DIR JAVA_DIR REST_INCLUDEDIR )
  get_filename_component( UDA_INSTALL_FULL_${_UDA_DIR} "${UDA_INSTALL_${_UDA_DIR}}"
    ABSOLUTE BASE_DIR "${CMAKE_INSTALL_PREFIX}" )
endforeach()
unset( _UDA_DIR )

# pkg-config templates. Relative directories are written relative to the .pc file's ${prefix}
# (or ${exec_prefix} for libdir), which keeps the installed tree relocatable.
if( IS_ABSOLUTE "${UDA_INSTALL_INCLUDEDIR}" )
  set( UDA_PKGCONFIG_INCLUDEDIR "${UDA_INSTALL_INCLUDEDIR}" )
else()
  set( UDA_PKGCONFIG_INCLUDEDIR "\${prefix}/${UDA_INSTALL_INCLUDEDIR}" )
endif()
if( IS_ABSOLUTE "${UDA_INSTALL_LIBDIR}" )
  set( UDA_PKGCONFIG_LIBDIR "${UDA_INSTALL_LIBDIR}" )
else()
  set( UDA_PKGCONFIG_LIBDIR "\${exec_prefix}/${UDA_INSTALL_LIBDIR}" )
endif()
