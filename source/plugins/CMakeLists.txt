########################################################################################################################
# Dependencies

find_package( LibXml2 REQUIRED )

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
  if( NOT MINGW )
    find_package( dlfcn-win32 CONFIG REQUIRED )
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
  add_subdirectory( netcdf4 )
  add_subdirectory( template )
  add_subdirectory( testplugin )
#  add_subdirectory( text )
  add_subdirectory( uda )
  add_subdirectory( viewport )
endif( DEFINED BUILD_PLUGINS )

add_definitions( -DSERVERBUILD )

include_directories(
  ${CMAKE_SOURCE_DIR}/source
)

if( MINGW OR WIN32 )
  include_directories( ${XDR_INCLUDE_DIR} )
endif()

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
if( BUILD_SHARED_LIBS )
  add_library( plugins-shared SHARED $<TARGET_OBJECTS:plugins-objects> )
endif()

set( LINK_LIB ${LIBXML2_LIBRARIES} )
if( WIN32 OR MINGW )
  if( MINGW )
    set( LINK_LIB ${LINK_LIB} ${XDR_LIBRARIES} ws2_32 dl stdc++ )
  else()
    set( LINK_LIB ${LINK_LIB} ${XDR_LIBRARIES} ws2_32 dlfcn-win32::dl )
  endif()
endif()

target_link_libraries( plugins-static PRIVATE
  #server-static
  ${LINK_LIB}
)

if( BUILD_SHARED_LIBS )
  target_link_libraries( plugins-shared PRIVATE
    server-static
    ${LINK_LIB}
  )
endif()

set_target_properties( plugins-static
  PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}_plugins
)

install(
  TARGETS plugins-static
  DESTINATION lib
)

if( BUILD_SHARED_LIBS )
  set_target_properties( plugins-shared
    PROPERTIES
      OUTPUT_NAME ${PROJECT_NAME}_plugins
      VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
      SOVERSION ${PROJECT_VERSION_MAJOR}
  )
  
  install(
    TARGETS plugins-shared
    DESTINATION lib
  )
endif()

install( FILES ${HEADER_FILES}
  DESTINATION include/uda/plugins
)


