enable_testing()

Include( FetchContent )

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.0 # or a later release
)

FetchContent_MakeAvailable( Catch2 )

add_executable( test_clientserver test_clientserver.cpp )
target_include_directories( test_clientserver PRIVATE ${CMAKE_SOURCE_DIR}/source/clientserver )
target_link_libraries( test_clientserver PRIVATE Catch2::Catch2WithMain
  authentication
  cache
  client_server
  common
  config
  logging
  protocol
  structures
)

add_test( NAME TestClientserver COMMAND test_clientserver CONFIGURATIONS unit )

add_subdirectory( plugins )
add_subdirectory( unit_tests )
