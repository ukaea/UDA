find_package( LibXml2 )

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
  add_subdirectory( imas )
  add_subdirectory( keyvalue )
  add_subdirectory( livedisplay )
  add_subdirectory( mast_imas )
  add_subdirectory( netcdf4 )
  add_subdirectory( source )
  add_subdirectory( template )
  add_subdirectory( testplugin )
  add_subdirectory( uda )
  add_subdirectory( viewport )
endif( DEFINED BUILD_PLUGINS )

add_definitions( -DSERVERBUILD )

include_directories(
  ${CMAKE_SOURCE_DIR}/source
)

set( SOURCE_FILES
  managePluginFiles.cpp
  udaPlugin.cpp
)

set( HEADER_FILES
  managePluginFiles.h
  pluginStructs.h
  udaPlugin.h
  udaPluginFiles.h
)

add_library( plugins-objects OBJECT ${SOURCE_FILES} ${HEADER_FILES} )
add_library( plugins-static STATIC $<TARGET_OBJECTS:plugins-objects> )
add_library( plugins-shared SHARED $<TARGET_OBJECTS:plugins-objects> )

target_link_libraries( plugins-static PUBLIC
  #server-static
  ${LIBXML2_LIBRARIES}
  dl
)

target_link_libraries( plugins-shared PUBLIC
  server-static
  ${LIBXML2_LIBRARIES}
  dl
)

set_target_properties( plugins-static
  PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}_plugins
)

set_target_properties( plugins-shared
  PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}_plugins
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

install( FILES ${HEADER_FILES}
  DESTINATION include/uda/plugins
)

install(
  TARGETS plugins-static plugins-shared
  DESTINATION lib
)
