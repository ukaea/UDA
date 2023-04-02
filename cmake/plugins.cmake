function( filter_lib_list INPUT OUTPUT GOOD BAD )
  set( LIB_LST ${INPUT} )
  set( USE_LIB YES )
  foreach( ELEMENT IN LISTS LIB_LST )
    if( "${ELEMENT}" STREQUAL "general" OR "${ELEMENT}" STREQUAL "${GOOD}" )
      set( USE_LIB YES )
    elseif( "${ELEMENT}" STREQUAL "${BAD}" )
      set( USE_LIB NO )
    elseif( USE_LIB )
      list( APPEND ${OUTPUT} ${ELEMENT} )
    endif()
  endforeach()
endfunction( filter_lib_list )

function( uda_plugin )

  find_package( OpenSSL REQUIRED )
  if( WIN32 OR MINGW )
    find_package( XDR REQUIRED )
    if( NOT MINGW )
      find_package( dlfcn-win32 CONFIG REQUIRED )
    endif()
  endif()

  include( CMakeParseArguments )

  set( optionArgs )
  set( oneValueArgs NAME LIBNAME ENTRY_FUNC DESCRIPTION EXAMPLE CONFIG_FILE TYPE EXTENSION VERSION )
  set( multiValueArgs SOURCES EXTRA_INCLUDE_DIRS EXTRA_LINK_DIRS EXTRA_LINK_LIBS EXTRA_DEFINITIONS EXTRA_INSTALL_FILES EXTRA_NAMES )

  cmake_parse_arguments(
    PLUGIN
    "${optionArgs}"
    "${oneValueArgs}"
    "${multiValueArgs}"
    "${ARGN}"
  )

  set( BUILT_PLUGINS ${BUILT_PLUGINS} "${PLUGIN_NAME}" PARENT_SCOPE )

  if( NOT PLUGIN_VERSION )
    set( PLUGIN_VERSION "0.0.0" )
  endif()

  if( NOT APPLE AND NOT WIN32 AND NOT MINGW AND CMAKE_COMPILER_IS_GNUCC )
    set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,defs" )
  endif()

  add_library( ${PLUGIN_LIBNAME} SHARED ${PLUGIN_SOURCES} )
  set_target_properties(
    ${PLUGIN_LIBNAME}
    PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
    SOVERSION ${PLUGIN_VERSION}
    VERSION ${PLUGIN_VERSION}
  )

  target_compile_features( ${PLUGIN_LIBNAME} PRIVATE cxx_std_11 )

  # TODO: In UDA 3.0 plugins should only have access to public headers (/include/uda/*) not private headers
  target_include_directories( ${PLUGIN_LIBNAME} PRIVATE ${PROJECT_SOURCE_DIR}/source )

  foreach( INCLUDE_DIR ${PLUGIN_EXTRA_INCLUDE_DIRS} )
    target_include_directories( ${PLUGIN_LIBNAME} PRIVATE SYSTEM ${INCLUDE_DIR} )
  endforeach()

  foreach( LINK_DIR ${PLUGIN_EXTRA_LINK_DIRS} )
    target_link_directories( ${PLUGIN_LIBNAME} PRIVATE ${LINK_DIR} )
  endforeach()

  if( WIN32 )
    set_target_properties(
      ${PLUGIN_LIBNAME}
      PROPERTIES
        COMPILE_FLAGS -DLIBRARY_EXPORTS
        IMPORT_SUFFIX ${IMPLIB_SUFFIX}
    )
  endif()

  if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    target_compile_definitions( ${PLUGIN_LIBNAME} PRIVATE -DA64 )
  endif()

  target_compile_definitions( ${PLUGIN_LIBNAME} PRIVATE -DSERVERBUILD )
  foreach( DEF ${PLUGIN_EXTRA_DEFINITIONS} )
    target_compile_definitions( ${PLUGIN_LIBNAME} PRIVATE ${DEF} )
  endforeach()
  
  set( LIBRARIES client-shared plugins-shared ${OPENSSL_LIBRARIES} )
  if( WIN32 OR MINGW )
    if( MINGW )
      set( LIBRARIES ${LIBRARIES} ${XDR_LIBRARIES} dl stdc++ )
    else()
        set( LIBRARIES ${LIBRARIES} ${XDR_LIBRARIES} dlfcn-win32::dl )
    endif()
  elseif(TIRPC_FOUND)
    set( LINK_LIB ${LINK_LIB} ${TIRPC_LIBRARIES})
  else()
    set( LIBRARIES ${LIBRARIES} dl stdc++ )
  endif()
  
  filter_lib_list( "${PLUGIN_EXTRA_LINK_LIBS}" FILTERED_LINK_LIBS debug optimized ) 
  set( LIBRARIES ${LIBRARIES} ${FILTERED_LINK_LIBS} )
  
  target_link_libraries( ${PLUGIN_LIBNAME} PRIVATE ${LIBRARIES} fmt::fmt uda::server-shared )
  
  install(
    TARGETS ${PLUGIN_LIBNAME}
    DESTINATION lib/plugins
  )

  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/udaPlugins_${PLUGIN_NAME}.conf
    DESTINATION etc/plugins
  )

  foreach( INSTALL_FILE ${PLUGIN_EXTRA_INSTALL_FILES} )
    get_filename_component( INSTALL_DIR ${INSTALL_FILE} DIRECTORY )
    install( FILES ${INSTALL_FILE} DESTINATION etc/plugins/${INSTALL_DIR} )
  endforeach()

  #targetFormat, formatClass="function", librarySymbol, libraryName, methodName, interface, cachePermission, publicUse, description, example
  if( APPLE )
    set( EXT_NAME "dylib" )
  elseif( WIN32 OR MINGW )
    set( EXT_NAME "dll" )
  else()
    set( EXT_NAME "so" )
  endif()

  if( "${PLUGIN_TYPE}" STREQUAL "" )
    set( PLUGIN_TYPE function )
  elseif( "${PLUGIN_TYPE}" STREQUAL "file" )
  elseif( "${PLUGIN_TYPE}" STREQUAL "function" )
  elseif( "${PLUGIN_TYPE}" STREQUAL "server" )
  else()
    message( FATAL_ERROR "unknown plugin type for plugin ${PLUGIN_NAME}: ${PLUGIN_TYPE}" )
  endif()

  if( "${PLUGIN_EXTENSION}" STREQUAL "" )
    set( PLUGIN_EXTENSION "*" )
  endif()

  file( WRITE "${CMAKE_CURRENT_BINARY_DIR}/udaPlugins_${PLUGIN_NAME}.conf"
    "${PLUGIN_NAME}, ${PLUGIN_TYPE}, ${PLUGIN_ENTRY_FUNC}, lib${PLUGIN_LIBNAME}.${EXT_NAME}, ${PLUGIN_EXTENSION}, 1, 1, 1, ${PLUGIN_DESCRIPTION}, ${PLUGIN_EXAMPLE}\n" )
  file( APPEND "${CMAKE_CURRENT_BINARY_DIR}/../udaPlugins.conf"
    "${PLUGIN_NAME}, ${PLUGIN_TYPE}, ${PLUGIN_ENTRY_FUNC}, lib${PLUGIN_LIBNAME}.${EXT_NAME}, ${PLUGIN_EXTENSION}, 1, 1, 1, ${PLUGIN_DESCRIPTION}, ${PLUGIN_EXAMPLE}\n" )

  foreach( EXTRA_NAME ${PLUGIN_EXTRA_NAMES} )
    file( WRITE "${CMAKE_CURRENT_BINARY_DIR}/udaPlugins_${PLUGIN_NAME}.conf"
      "${EXTRA_NAME}, ${PLUGIN_TYPE}, ${PLUGIN_ENTRY_FUNC}, lib${PLUGIN_LIBNAME}.${EXT_NAME}, ${PLUGIN_EXTENSION}, 1, 1, 1, ${PLUGIN_DESCRIPTION}, ${PLUGIN_EXAMPLE}\n" )
    file( APPEND "${CMAKE_CURRENT_BINARY_DIR}/../udaPlugins.conf"
      "${EXTRA_NAME}, ${PLUGIN_TYPE}, ${PLUGIN_ENTRY_FUNC}, lib${PLUGIN_LIBNAME}.${EXT_NAME}, ${PLUGIN_EXTENSION}, 1, 1, 1, ${PLUGIN_DESCRIPTION}, ${PLUGIN_EXAMPLE}\n" )
  endforeach()

  if( NOT EXISTS "${CMAKE_BINARY_DIR}/etc/plugins.d" )
    file( MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/etc/plugins.d" )
  endif()

  if( NOT "${PLUGIN_CONFIG_FILE}" STREQUAL "" )
    configure_file(
      "${CMAKE_CURRENT_LIST_DIR}/${PLUGIN_CONFIG_FILE}.in"
      "${CMAKE_BINARY_DIR}/etc/plugins.d/${PLUGIN_CONFIG_FILE}"
      @ONLY
    )
  endif()

endfunction( uda_plugin )
