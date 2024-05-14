########################################################################################################################
# Dependencies

find_package( LibXml2 REQUIRED )
find_package( fmt REQUIRED )
find_package( Boost COMPONENTS filesystem REQUIRED )

set( LIBS LibXml2::LibXml2 fmt::fmt Boost::filesystem server2-shared )

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
  find_package( dlfcn-win32 CONFIG REQUIRED )
  list( APPEND LIBS XDR::XDR )
  list( APPEND LIBS dlfcn-win32::dl )
  list( APPEND LIBS ws2_32 )
  if( MINGW )
    list( APPEND LIBS stdc++ )
  endif()
else()
  find_package( TIRPC QUIET )
  if(TIRPC_FOUND)
    list( APPEND LIBS TIRPC::TIRPC )
  endif()
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
    etc/plugins
)

file( MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/server/plugins.d" )

if( DEFINED BUILD_PLUGINS )
  foreach( PLUGIN IN ITEMS ${BUILD_PLUGINS} )
    add_subdirectory( ${PLUGIN} )
  endforeach( PLUGIN )
else()
  add_subdirectory( bytes )
  add_subdirectory( hdf5 )
  add_subdirectory( help )
  add_subdirectory( keyvalue )
  add_subdirectory( template )
  add_subdirectory( testplugin )
#  add_subdirectory( uda )
  add_subdirectory( viewport )
endif( DEFINED BUILD_PLUGINS )

set( SOURCE_FILES
  managePluginFiles.cpp
  uda_plugin_base.cpp
)

add_library( plugins-static STATIC ${SOURCE_FILES} )

target_include_directories( plugins-static PRIVATE
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_SOURCE_DIR}/source/include
  ${CMAKE_BINARY_DIR}/include
)
target_link_libraries( plugins-static PRIVATE ${LIBS} )

set_target_properties( plugins-static
  PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}_plugins
)

install(
  TARGETS plugins-static
  DESTINATION lib
)
