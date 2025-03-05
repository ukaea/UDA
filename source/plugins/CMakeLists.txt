set( BUILD_PLUGINS "" CACHE STRING "Selection of plugins to build" )

if( DEFINED BUILD_PLUGINS AND NOT ${BUILD_PLUGINS} STREQUAL "" )
  foreach( PLUGIN IN ITEMS ${BUILD_PLUGINS} )
    add_subdirectory( ${PLUGIN} )
  endforeach( PLUGIN )
else()
  add_subdirectory( bytes )
  add_subdirectory( hdf5 )
  add_subdirectory( help )
  add_subdirectory( template )
  add_subdirectory( testplugin )
#  add_subdirectory( uda ) # this still needs updating to remove dependency on server internals
endif()

find_package( Boost COMPONENTS filesystem REQUIRED )

set( SRCS
  managePluginFiles.cpp
  uda_plugin_base.cpp
  plugin_api.cpp
)

add_library( plugins ${SRCS} )
target_include_directories( plugins PRIVATE ${CMAKE_SOURCE_DIR}/source ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( plugins
  PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_link_libraries( plugins PRIVATE Boost::filesystem )
target_link_libraries( plugins PRIVATE
  "$<BUILD_INTERFACE:authentication>"
  "$<BUILD_INTERFACE:cache>"
  "$<BUILD_INTERFACE:client_server>"
  "$<BUILD_INTERFACE:common>"
  "$<BUILD_INTERFACE:config>"
  "$<BUILD_INTERFACE:logging>"
  "$<BUILD_INTERFACE:protocol>"
  "$<BUILD_INTERFACE:server>"
  "$<BUILD_INTERFACE:structures>"
)

set_target_properties( plugins
  PROPERTIES
    OUTPUT_NAME uda_plugins
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

add_library( uda::plugins ALIAS plugins )

install( TARGETS plugins
  EXPORT udaPluginTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)