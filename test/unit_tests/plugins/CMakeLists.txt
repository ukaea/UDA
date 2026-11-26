if( NOT BUILD_SHARED_LIBS OR NOT TARGET plugins-shared OR NOT TARGET server-shared )
  return()
endif()

add_executable( test_plugin_interface plugin_interface.test.cpp )
target_include_directories( test_plugin_interface PRIVATE ${CMAKE_SOURCE_DIR}/source ${LIBXML2_INCLUDE_DIR} )
target_compile_definitions( test_plugin_interface PRIVATE SERVERBUILD )
target_link_libraries( test_plugin_interface PRIVATE Catch2::Catch2WithMain plugins-shared server-shared client-shared )
add_test( NAME TestPluginInterface COMMAND test_plugin_interface )
