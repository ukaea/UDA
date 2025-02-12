Include( FetchContent )

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.0 # or a later release
)

FetchContent_MakeAvailable( Catch2 )

add_executable( test_client client.test.cpp )
target_include_directories( test_client PRIVATE ${CMAKE_SOURCE_DIR}/source ${CMAKE_SOURCE_DIR}/include )
target_link_libraries( test_client PRIVATE Catch2::Catch2WithMain c_api )
add_test( NAME TestClient COMMAND test_client)

add_executable( test_connection connection.test.cpp )
target_include_directories( test_connection PRIVATE ${CMAKE_SOURCE_DIR}/source ${CMAKE_SOURCE_DIR}/include )
target_link_libraries( test_connection PRIVATE Catch2::Catch2WithMain c_api ${RPC_LIB} )
add_test( NAME TestConnection COMMAND test_connection)

add_executable( test_config config.test.cpp )
target_include_directories( test_config PRIVATE ${CMAKE_SOURCE_DIR}/source ${CMAKE_SOURCE_DIR}/include )
target_link_libraries( test_config PRIVATE Catch2::Catch2WithMain c_api )
add_test( NAME TestConfig COMMAND test_config)

add_executable( test_logging logging.test.cpp )
target_include_directories( test_logging PRIVATE ${CMAKE_SOURCE_DIR}/source ${CMAKE_SOURCE_DIR}/include )
target_link_libraries( test_logging PRIVATE Catch2::Catch2WithMain c_api )
add_test( NAME TestLogging COMMAND test_logging RUN_SERIAL TRUE )

install(
  DIRECTORY
  test_files
  DESTINATION 
  ${CMAKE_BINARY_DIR}/test/unit_tests
)
