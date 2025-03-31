define_test( client )
define_test( connection )
define_test( config )
define_test( logging SERIAL COMPILE_DEFINITIONS SPDLOG_FMT_EXTERNAL )
define_test( host_list )

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
