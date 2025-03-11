add_executable( test_client client.test.cpp )
target_include_directories( test_client PRIVATE ${CMAKE_SOURCE_DIR}/source ${CMAKE_SOURCE_DIR}/include )
target_link_libraries( test_client PRIVATE Catch2::Catch2WithMain c_api )
add_test( NAME TestClient COMMAND test_client )

add_executable( test_connection connection.test.cpp )
target_include_directories( test_connection PRIVATE ${CMAKE_SOURCE_DIR}/source ${CMAKE_SOURCE_DIR}/include )
target_link_libraries( test_connection PRIVATE Catch2::Catch2WithMain c_api ${RPC_LIB} )
add_test( NAME TestConnection COMMAND test_connection )

add_executable( test_config config.test.cpp )
target_include_directories( test_config PRIVATE ${CMAKE_SOURCE_DIR}/source ${CMAKE_SOURCE_DIR}/include )
target_link_libraries( test_config PRIVATE Catch2::Catch2WithMain c_api )
add_test( NAME TestConfig COMMAND test_config )

add_executable( test_logging logging.test.cpp )
target_include_directories( test_logging PRIVATE ${CMAKE_SOURCE_DIR}/source ${CMAKE_SOURCE_DIR}/include )
target_link_libraries( test_logging PRIVATE Catch2::Catch2WithMain c_api )
target_compile_definitions( test_logging PRIVATE SPDLOG_FMT_EXTERNAL)
add_test( NAME TestLogging COMMAND test_logging )
set_tests_properties(TestLogging PROPERTIES RUN_SERIAL TRUE )

add_executable( test_host_list host_list.test.cpp )
target_include_directories( test_host_list PRIVATE ${CMAKE_SOURCE_DIR}/source ${CMAKE_SOURCE_DIR}/include )
target_link_libraries( test_host_list PRIVATE Catch2::Catch2WithMain c_api )
add_test( NAME TestHostList COMMAND test_host_list )

set( TEST_FILES
  test_files/client-config-with-hosts.toml
  test_files/empty.toml
  test_files/host_list.cfg
  test_files/host_list.toml
  test_files/incorrect-field-type.toml
  test_files/test-types.toml
  test_files/uda-client-config.toml
  test_files/unrecognised-field.toml
  test_files/unrecognised-section.toml
)

foreach( TEST_FILE ${TEST_FILES} )
  configure_file( ${TEST_FILE} ${CMAKE_BINARY_DIR}/test/unit_tests/client/${TEST_FILE} COPYONLY )
endforeach(  )
