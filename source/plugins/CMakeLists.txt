########################################################################################################################
# Dependencies

find_package( LibXml2 REQUIRED )
find_package( fmt REQUIRED )

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
  if( NOT MINGW )
    find_package( dlfcn-win32 CONFIG REQUIRED )
  endif()
else()
  find_package( TIRPC QUIET )
endif()

########################################################################################################################
# Sources

set( SRC_FILES managePluginFiles.cpp udaPlugin.cpp )
set( HEADER_FILES pluginStructs.h )

########################################################################################################################
# Targets

add_library( plugins-objects OBJECT ${SRC_FILES} )
target_compile_features( plugins-objects PRIVATE cxx_std_11 )
target_include_directories( plugins-objects PRIVATE ${PROJECT_SOURCE_DIR}/source )

add_library( uda::plugins-objects ALIAS plugins-objects )

add_library( plugins-static STATIC )
add_library( uda::plugins-static ALIAS plugins-static )

target_link_libraries( plugins-static PUBLIC uda::plugins-objects uda::server-shared )

if( BUILD_SHARED_LIBS )
  add_library( plugins-shared SHARED )
  add_library( uda::plugins-shared ALIAS plugins-shared )

  target_link_libraries( plugins-shared PUBLIC uda::plugins-objects uda::server-shared )
endif()

set_target_properties(
  plugins-static
  PROPERTIES
  OUTPUT_NAME ${PROJECT_NAME}_plugins_static
)

if( BUILD_SHARED_LIBS )
  set_target_properties(
    plugins-shared
    PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}_plugins
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    SOVERSION ${PROJECT_VERSION_MAJOR}
  )
  if( WIN32 )
    set_target_properties(
      plugins-shared
      PROPERTIES
      COMPILE_FLAGS -DLIBRARY_EXPORTS
      IMPORT_SUFFIX ${IMPLIB_SUFFIX}
    )
  endif()
endif()

########################################################################################################################
# Configuration

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

file( MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/server/plugins.d" )

########################################################################################################################
# Subdirectories

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
  add_subdirectory( uda )
  add_subdirectory( viewport )
endif( DEFINED BUILD_PLUGINS )

########################################################################################################################
# Install

install(
  FILES
  ${CMAKE_CURRENT_BINARY_DIR}/udaPlugins.conf
  ${CMAKE_CURRENT_BINARY_DIR}/test_plugin.cpp
  DESTINATION
  etc/plugins
)

install(
  TARGETS plugins-static
  EXPORT uda-targets
  LIBRARY DESTINATION lib
)

if( BUILD_SHARED_LIBS )
  install(
    TARGETS plugins-shared
    EXPORT uda-targets
    LIBRARY DESTINATION lib
  )
endif()

install( FILES ${HEADER_FILES}
  DESTINATION include/uda
  )
