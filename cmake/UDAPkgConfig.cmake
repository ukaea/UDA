# Legacy pkg-config metadata for non-CMake Unix consumers. The CMake package is the supported
# cross-platform interface; generating these files does not require pkg-config.
option( UDA_INSTALL_PKGCONFIG "Install legacy pkg-config metadata (Unix only)" ${UNIX} )

# Writes NAME.pc into the build tree and installs it. The prefix is computed relative to the
# .pc file's own location, so the installed tree can be moved.
function( _uda_configure_pkgconfig NAME DESTINATION OUTPUT_DIR )
  if( IS_ABSOLUTE "${DESTINATION}" )
    set( _PC_DIR "${DESTINATION}" )
  else()
    set( _PC_DIR "${CMAKE_INSTALL_PREFIX}/${DESTINATION}" )
  endif()
  file( RELATIVE_PATH _PC_PREFIX "${_PC_DIR}" "${CMAKE_INSTALL_PREFIX}" )
  set( UDA_PKGCONFIG_PREFIX "\${pcfiledir}/${_PC_PREFIX}" )

  configure_file( "${PROJECT_SOURCE_DIR}/source/etc/${NAME}.pc.in" "${OUTPUT_DIR}/${NAME}.pc" @ONLY )
  install( FILES "${OUTPUT_DIR}/${NAME}.pc" DESTINATION "${DESTINATION}" COMPONENT development )
endfunction()

# Generates the metadata for one library, skipping it when that library is not part of this build.
# The C++ wrapper metadata is also copied to the legacy module directory.
function( _uda_pkgconfig NAME TARGET REQUIRES_PRIVATE LIBS_PRIVATE )
  if( NOT TARGET ${TARGET} )
    return()
  endif()

  set( UDA_PC_REQUIRES_PRIVATE "${REQUIRES_PRIVATE}" )
  set( UDA_PC_LIBS_PRIVATE "${LIBS_PRIVATE}" )
  _uda_configure_pkgconfig( ${NAME} "${UDA_INSTALL_PKGCONFIGDIR}" "${PROJECT_BINARY_DIR}/lib/pkgconfig" )

  if( NAME MATCHES "cpp$" AND NOT UDA_INSTALL_WRAPPER_PKGCONFIGDIR STREQUAL UDA_INSTALL_PKGCONFIGDIR )
    _uda_configure_pkgconfig( ${NAME} "${UDA_INSTALL_WRAPPER_PKGCONFIGDIR}" "${PROJECT_BINARY_DIR}/etc/uda" )
  endif()
endfunction()

function( uda_install_pkgconfig )
  if( NOT UDA_INSTALL_PKGCONFIG )
    return()
  endif()
  if( NOT UNIX )
    message( FATAL_ERROR
      "UDA pkg-config metadata supports Unix toolchains only. "
      "Use -DUDA_INSTALL_PKGCONFIG=OFF and find_package(UDA CONFIG) on Windows." )
  endif()

  # Public requirements follow the public headers. Everything else is needed only to link the archives.
  set( UDA_PC_REQUIRES "libxml-2.0" )
  if( UDA_PACKAGE_NEEDS_TIRPC )
    string( APPEND UDA_PC_REQUIRES " libtirpc" )
  endif()

  set( _REQUIRES_PRIVATE "openssl fmt" )
  if( UDA_PACKAGE_NEEDS_LIBMEMCACHED )
    string( APPEND _REQUIRES_PRIVATE " libmemcached" )
  endif()

  string( REPLACE ";" " " _THREAD_LIBS "${CMAKE_THREAD_LIBS_INIT}" )
  set( _LIBS_PRIVATE "${_THREAD_LIBS}" )

  # Archives that load plugins also need the system libraries that dlopen-based code uses.
  set( _SYSTEM_LIBS_PRIVATE "" )
  foreach( _SYSTEM_LIB IN LISTS CMAKE_DL_LIBS )
    string( APPEND _SYSTEM_LIBS_PRIVATE " -l${_SYSTEM_LIB}" )
  endforeach()
  if( NOT APPLE )
    string( APPEND _SYSTEM_LIBS_PRIVATE " -lm" )
  endif()
  if( UDA_STDFS_LIBRARY )
    string( APPEND _SYSTEM_LIBS_PRIVATE " -l${UDA_STDFS_LIBRARY}" )
  endif()
  string( STRIP "${_LIBS_PRIVATE} -luda_plugins${_SYSTEM_LIBS_PRIVATE}" _FAT_LIBS_PRIVATE )
  string( STRIP "${_LIBS_PRIVATE}${_SYSTEM_LIBS_PRIVATE}" _PLUGINS_LIBS_PRIVATE )

  # Each archive is self-contained rather than depending on uda-client, which would pull in a second client copy.
  _uda_pkgconfig( uda-client      client-static        "${_REQUIRES_PRIVATE}"                   "${_LIBS_PRIVATE}" )
  _uda_pkgconfig( uda-cpp         uda_cpp-static       "${_REQUIRES_PRIVATE}"                   "${_LIBS_PRIVATE}" )
  _uda_pkgconfig( uda-fat-client  fatclient-static     "${_REQUIRES_PRIVATE}"                   "${_FAT_LIBS_PRIVATE}" )
  _uda_pkgconfig( uda-fat-cpp     fatuda_cpp-static    "${_REQUIRES_PRIVATE}"                   "${_FAT_LIBS_PRIVATE}" )
  _uda_pkgconfig( uda-plugins     plugins-static       "${_REQUIRES_PRIVATE} uda-client"        "${_PLUGINS_LIBS_PRIVATE}" )
endfunction()
