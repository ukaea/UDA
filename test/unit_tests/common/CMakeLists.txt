add_executable( test_uda_env uda_env_options.test.cpp )
target_include_directories( test_uda_env PRIVATE ${CMAKE_SOURCE_DIR}/source )
target_link_libraries( test_uda_env PRIVATE Catch2::Catch2WithMain )
add_test( NAME TestUdaEnvOptions COMMAND test_uda_env )
