# Compatibility metadata for non-CMake Unix consumers. CMake exports are the
# supported cross-platform interface; generation does not require pkg-config.
option( UDA_INSTALL_PKGCONFIG "Install legacy pkg-config metadata (Unix only)" ${UNIX} )

function( _uda_configure_pkgconfig NAME DESTINATION OUTPUT_DIR )
  if( IS_ABSOLUTE "${DESTINATION}" )
    set( _PC_DIR "${DESTINATION}" )
  else()
    set( _PC_DIR "${CMAKE_INSTALL_PREFIX}/${DESTINATION}" )
  endif()
  file( RELATIVE_PATH _PC_PREFIX "${_PC_DIR}" "${CMAKE_INSTALL_PREFIX}" )
  set( UDA_PKGCONFIG_PREFIX "\${pcfiledir}/${_PC_PREFIX}" )
  configure_file(
    "${PROJECT_SOURCE_DIR}/source/etc/${NAME}.pc.in"
    "${OUTPUT_DIR}/${NAME}.pc"
    @ONLY
  )
  install( FILES "${OUTPUT_DIR}/${NAME}.pc"
    DESTINATION "${DESTINATION}"
    COMPONENT development
  )
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

  set( UDA_PC_REQUIRES "libxml-2.0" )
  if( UDA_PACKAGE_NEEDS_TIRPC )
    string( APPEND UDA_PC_REQUIRES " libtirpc" )
  endif()
  set( _PRIVATE_REQUIRES "openssl fmt" )
  if( UDA_PACKAGE_NEEDS_LIBMEMCACHED )
    string( APPEND _PRIVATE_REQUIRES " libmemcached" )
  endif()
  set( _PRIVATE_LIBS "${CMAKE_THREAD_LIBS_INIT}" )
  string( REPLACE ";" " " _PRIVATE_LIBS "${_PRIVATE_LIBS}" )

  # Each legacy name describes a self-contained client archive, rather than
  # depending on uda-client and accidentally pulling in a second client copy.
  foreach( _PAIR IN ITEMS
      "uda-client|client-static"
      "uda-fat-client|fatclient-static"
      "uda-cpp|uda_cpp-static"
      "uda-fat-cpp|fatuda_cpp-static"
      "uda-plugins|plugins-static" )
    string( REPLACE "|" ";" _PAIR "${_PAIR}" )
    list( GET _PAIR 0 _NAME )
    list( GET _PAIR 1 _TARGET )
    if( NOT TARGET ${_TARGET} )
      continue()
    endif()

    set( UDA_PC_REQUIRES_PRIVATE "${_PRIVATE_REQUIRES}" )
    set( UDA_PC_LIBS_PRIVATE "${_PRIVATE_LIBS}" )
    if( _NAME MATCHES "^uda-fat-" )
      string( APPEND UDA_PC_LIBS_PRIVATE " -luda_plugins" )
    elseif( _NAME STREQUAL "uda-plugins" )
      string( APPEND UDA_PC_REQUIRES_PRIVATE " uda-client" )
    endif()
    if( _NAME MATCHES "^uda-fat-" OR _NAME STREQUAL "uda-plugins" )
      foreach( _SYSTEM_LIB IN LISTS CMAKE_DL_LIBS )
        string( APPEND UDA_PC_LIBS_PRIVATE " -l${_SYSTEM_LIB}" )
      endforeach()
      if( NOT APPLE )
        string( APPEND UDA_PC_LIBS_PRIVATE " -lm" )
      endif()
      if( TARGET plugins-shared )
        get_target_property( _PLUGIN_LIBS plugins-shared LINK_LIBRARIES )
        foreach( _FS_LIB IN ITEMS stdc++fs c++fs )
          if( _FS_LIB IN_LIST _PLUGIN_LIBS )
            string( APPEND UDA_PC_LIBS_PRIVATE " -l${_FS_LIB}" )
          endif()
        endforeach()
      endif()
    endif()
    string( STRIP "${UDA_PC_LIBS_PRIVATE}" UDA_PC_LIBS_PRIVATE )

    _uda_configure_pkgconfig( "${_NAME}" "${UDA_INSTALL_PKGCONFIGDIR}"
      "${PROJECT_BINARY_DIR}/lib/pkgconfig" )

    # Preserve historical C++ metadata copies, but compute their prefix for
    # their actual location rather than copying a lib/pkgconfig-relative file.
    if( _NAME MATCHES "cpp$" AND
        NOT UDA_INSTALL_WRAPPER_PKGCONFIGDIR STREQUAL UDA_INSTALL_PKGCONFIGDIR )
      _uda_configure_pkgconfig( "${_NAME}" "${UDA_INSTALL_WRAPPER_PKGCONFIGDIR}"
        "${PROJECT_BINARY_DIR}/etc/uda" )
    endif()
  endforeach()
endfunction()
