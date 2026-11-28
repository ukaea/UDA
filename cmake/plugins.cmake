macro( filter_lib_list INPUT OUTPUT GOOD BAD )
  set( LIB_LST ${INPUT} )
  set( USE_LIB YES )
  foreach( ELEMENT IN LISTS LIB_LST )
    # Expected sequence [<build_type>] <lib> [[<build_type>] <lib>] [...]
    if( "${ELEMENT}" STREQUAL "general" OR "${ELEMENT}" STREQUAL "${GOOD}" )
      set( USE_LIB YES )
    elseif( "${ELEMENT}" STREQUAL "${BAD}" )
      set( USE_LIB NO )
    elseif( USE_LIB )
      list( APPEND ${OUTPUT} ${ELEMENT} )
    else()
      set( USE_LIB YES )
    endif()
  endforeach()
endmacro( filter_lib_list )

# Builds, installs and registers one plugin. A function, so its settings stay out of the calling
# directory's scope.
function( uda_plugin )

  find_package( OpenSSL REQUIRED )
  if( WIN32 OR MINGW )
    find_package( XDR REQUIRED )
    find_package( dlfcn-win32 CONFIG REQUIRED )
  endif()

  set( oneValueArgs NAME LIBNAME ENTRY_FUNC DESCRIPTION EXAMPLE CONFIG_FILE TYPE EXTENSION VERSION )
  set( multiValueArgs SOURCES EXTRA_INCLUDE_DIRS EXTRA_LINK_DIRS EXTRA_LINK_LIBS EXTRA_DEFINITIONS EXTRA_INSTALL_FILES EXTRA_NAMES )
  cmake_parse_arguments( PLUGIN "" "${oneValueArgs}" "${multiValueArgs}" "${ARGN}" )

  if( NOT PLUGIN_VERSION )
    set( PLUGIN_VERSION "0.0.0" )
  endif()
  if( "${PLUGIN_TYPE}" STREQUAL "" )
    set( PLUGIN_TYPE function )
  elseif( NOT PLUGIN_TYPE MATCHES "^(file|function|server)$" )
    message( FATAL_ERROR "unknown plugin type for plugin ${PLUGIN_NAME}: ${PLUGIN_TYPE}" )
  endif()
  if( "${PLUGIN_EXTENSION}" STREQUAL "" )
    set( PLUGIN_EXTENSION "*" )
  endif()

  # link_directories() only affects targets created after it, in this directory.
  foreach( LINK_DIR ${PLUGIN_EXTRA_LINK_DIRS} )
    link_directories( ${LINK_DIR} )
  endforeach()

  add_library( ${PLUGIN_LIBNAME} SHARED ${PLUGIN_SOURCES} )
  set_target_properties( ${PLUGIN_LIBNAME}
    PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
    SOVERSION ${PLUGIN_VERSION}
    VERSION ${PLUGIN_VERSION}
  )
  if( WIN32 )
    set_target_properties( ${PLUGIN_LIBNAME} PROPERTIES IMPORT_SUFFIX ${IMPLIB_SUFFIX} )
  endif()
  # Report unresolved symbols when the plugin is linked, not when the server loads it.
  if( NOT APPLE AND NOT WIN32 AND NOT MINGW AND CMAKE_COMPILER_IS_GNUCC )
    set_property( TARGET ${PLUGIN_LIBNAME} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-z,defs" )
  endif()

  target_include_directories( ${PLUGIN_LIBNAME} PRIVATE ${CMAKE_SOURCE_DIR}/source )
  target_include_directories( ${PLUGIN_LIBNAME} SYSTEM PRIVATE /opt/local/include ${PLUGIN_EXTRA_INCLUDE_DIRS} )

  target_compile_definitions( ${PLUGIN_LIBNAME} PRIVATE SERVERBUILD ${PLUGIN_EXTRA_DEFINITIONS} )
  if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    target_compile_definitions( ${PLUGIN_LIBNAME} PRIVATE A64 )
  endif()

  set( LIBRARIES client-shared plugins-shared OpenSSL::SSL OpenSSL::Crypto )
  if( ENABLE_CAPNP )
    list( APPEND LIBRARIES serialisation-static )
  endif()
  if( WIN32 OR MINGW )
    list( APPEND LIBRARIES XDR::XDR dlfcn-win32::dl )
    if( MINGW )
      list( APPEND LIBRARIES stdc++ )
    endif()
  elseif( TIRPC_FOUND )
    # TIRPC::TIRPC also supplies the -D__TIRPC__ definition.
    list( APPEND LIBRARIES TIRPC::TIRPC )
  else()
    list( APPEND LIBRARIES dl stdc++ )
  endif()
  filter_lib_list( "${PLUGIN_EXTRA_LINK_LIBS}" FILTERED_LINK_LIBS debug optimized )
  target_link_libraries( ${PLUGIN_LIBNAME} PRIVATE ${LIBRARIES} ${FILTERED_LINK_LIBS} )

  install(
    TARGETS ${PLUGIN_LIBNAME}
    DESTINATION ${UDA_INSTALL_PLUGINDIR}
    COMPONENT plugins
  )
  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/udaPlugins_${PLUGIN_NAME}.conf
    DESTINATION ${UDA_INSTALL_PLUGIN_CONFIGDIR}
    COMPONENT plugins
  )
  foreach( INSTALL_FILE ${PLUGIN_EXTRA_INSTALL_FILES} )
    get_filename_component( INSTALL_DIR ${INSTALL_FILE} DIRECTORY )
    install(
      FILES ${INSTALL_FILE}
      DESTINATION ${UDA_INSTALL_PLUGIN_CONFIGDIR}/${INSTALL_DIR}
      COMPONENT plugins
    )
  endforeach()

  # Plugin registration, one line per name, written to the plugin's own file and the combined udaPlugins.conf:
  #targetFormat, formatClass="function", librarySymbol, libraryName, methodName, interface, cachePermission, publicUse, description, example
  if( APPLE )
    set( EXT_NAME "dylib" )
  elseif( WIN32 OR MINGW )
    set( EXT_NAME "dll" )
  else()
    set( EXT_NAME "so" )
  endif()
  set( _CONF "" )
  foreach( _NAME IN ITEMS ${PLUGIN_NAME} ${PLUGIN_EXTRA_NAMES} )
    string( APPEND _CONF "${_NAME}, ${PLUGIN_TYPE}, ${PLUGIN_ENTRY_FUNC}, lib${PLUGIN_LIBNAME}.${EXT_NAME}, "
      "${PLUGIN_EXTENSION}, 1, 1, 1, ${PLUGIN_DESCRIPTION}, ${PLUGIN_EXAMPLE}\n" )
  endforeach()
  file( WRITE "${CMAKE_CURRENT_BINARY_DIR}/udaPlugins_${PLUGIN_NAME}.conf" "${_CONF}" )
  file( APPEND "${CMAKE_CURRENT_BINARY_DIR}/../udaPlugins.conf" "${_CONF}" )

  file( MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/etc/plugins.d" )
  if( NOT "${PLUGIN_CONFIG_FILE}" STREQUAL "" )
    configure_file(
      "${CMAKE_CURRENT_LIST_DIR}/${PLUGIN_CONFIG_FILE}.in"
      "${CMAKE_BINARY_DIR}/etc/plugins.d/${PLUGIN_CONFIG_FILE}"
      @ONLY
    )
  endif()

endfunction()
