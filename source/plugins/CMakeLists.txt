cmake_minimum_required( VERSION 2.8 )

configure_file(
  "${CMAKE_SOURCE_DIR}/etc/idamPlugins.conf.in"
  "${CMAKE_CURRENT_BINARY_DIR}/idamPlugins.conf"
  COPYONLY
)

configure_file(
  "${CMAKE_SOURCE_DIR}/etc/test_plugin.cpp.in"
  "${CMAKE_CURRENT_BINARY_DIR}/test_plugin.cpp"
  @ONLY
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/idamPlugins.conf
    ${CMAKE_CURRENT_BINARY_DIR}/test_plugin.cpp
  DESTINATION
    bin/plugins
)

file( MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/server/plugins.d" )

if( DEFINED BUILD_PLUGINS )
  foreach( PLUGIN IN ITEMS ${BUILD_PLUGINS} )
    add_subdirectory( ${PLUGIN} )
  endforeach( PLUGIN )
else()
  add_subdirectory( meta )
  add_subdirectory( geometry )
  add_subdirectory( hdf5 )
  add_subdirectory( mongodb )
  add_subdirectory( help )
  add_subdirectory( netcdf4 )
  add_subdirectory( imas )
  add_subdirectory( template )
  add_subdirectory( ida3 )
  add_subdirectory( doi )
  add_subdirectory( idam )
  add_subdirectory( ppf )
  add_subdirectory( equimap )
  add_subdirectory( opendata )
  add_subdirectory( provenance )
  add_subdirectory( testplugin )
  add_subdirectory( viewport )
  add_subdirectory( tore_supra )
  add_subdirectory( west )
  add_subdirectory( mast_imas )
  add_subdirectory( livedisplay )
  add_subdirectory( putdata )
  add_subdirectory( efitmagxml )
  add_subdirectory( keyvalue )
endif( DEFINED BUILD_PLUGINS )
