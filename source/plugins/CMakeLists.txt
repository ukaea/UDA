########################################################################################################################
# Dependencies

find_package( LibXml2 REQUIRED )
find_package( fmt REQUIRED )
find_package( Boost REQUIRED )

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
  find_package( dlfcn-win32 CONFIG REQUIRED )
else()
  find_package( TIRPC QUIET )
endif()


configure_file(
  "${CMAKE_SOURCE_DIR}/source/etc/udaPlugins.conf.in"
  "${CMAKE_CURRENT_BINARY_DIR}/udaPlugins.conf"
  COPYONLY
)

configure_file(
  "${CMAKE_SOURCE_DIR}/source/etc/test_plugin.cpp.in"
  "${CMAKE_CURRENT_BINARY_DIR}/test_plugin.cpp"
  @ONLY
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/udaPlugins.conf
    ${CMAKE_CURRENT_BINARY_DIR}/test_plugin.cpp
  DESTINATION
    ${UDA_INSTALL_PLUGIN_CONFIGDIR}
  COMPONENT plugins
)

file( MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/server/plugins.d" )

if( DEFINED BUILD_PLUGINS )
  foreach( PLUGIN IN ITEMS ${BUILD_PLUGINS} )
    add_subdirectory( ${PLUGIN} )
  endforeach( PLUGIN )
else()
  add_subdirectory( bytes )
  add_subdirectory( help )
  add_subdirectory( template )
  add_subdirectory( testplugin )
  add_subdirectory( uda )
endif( DEFINED BUILD_PLUGINS )

set( SOURCE_FILES
  managePluginFiles.cpp
  udaPlugin.cpp
  utils.cpp
)

set( HEADER_FILES
  managePluginFiles.h
  pluginStructs.h
  udaPlugin.h
  udaPluginFiles.h
  utils.h
)

add_library( plugins-objects OBJECT ${SOURCE_FILES} ${HEADER_FILES} )
target_include_directories( plugins-objects PRIVATE ${CMAKE_SOURCE_DIR}/source )
target_compile_definitions( plugins-objects PRIVATE SERVERBUILD )
add_library( plugins-static STATIC $<TARGET_OBJECTS:plugins-objects> )
if( BUILD_SHARED_LIBS )
  add_library( plugins-shared SHARED $<TARGET_OBJECTS:plugins-objects> )
endif()

set( LINK_LIB LibXml2::LibXml2 fmt::fmt )
if( WIN32 OR MINGW )
  if( MINGW )
    set( LINK_LIB ${LINK_LIB} XDR::XDR ws2_32 dlfcn-win32::dl stdc++ )
  else()
    set( LINK_LIB ${LINK_LIB} XDR::XDR ws2_32 dlfcn-win32::dl )
  endif()
elseif( TIRPC_FOUND )
  set( LINK_LIB ${LINK_LIB} TIRPC::TIRPC )
endif()

# Objects need the same headers and dependencies as the archives that contain them.
target_link_libraries( plugins-objects PRIVATE ${LINK_LIB} Boost::boost )

target_link_libraries( plugins-static PRIVATE
  #server-static
  ${LINK_LIB}
)

if( BUILD_SHARED_LIBS )
  target_link_libraries( plugins-shared PRIVATE
    server-static
    ${LINK_LIB}
    ${UDA_STDFS_LIBRARY}
  )
endif()

uda_install_library( plugins-static ${PROJECT_NAME}_plugins )
if( BUILD_SHARED_LIBS )
  uda_install_library( plugins-shared ${PROJECT_NAME}_plugins RUNTIME_COMPONENT plugins )
endif()

install( FILES ${HEADER_FILES}
  DESTINATION ${UDA_INSTALL_INCLUDEDIR}/plugins
  COMPONENT development
)


