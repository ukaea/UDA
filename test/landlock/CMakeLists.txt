cmake_minimum_required( VERSION 3.12 )

# Also supports a dependency-free build for macOS maintainers using a Linux VM.
if( CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR )
  project( uda_landlock_tests LANGUAGES CXX )
  set( CMAKE_CXX_STANDARD 17 )
  set( CMAKE_CXX_STANDARD_REQUIRED ON )
  enable_testing()
endif()

if( NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" )
  message( FATAL_ERROR "Landlock tests require Linux" )
endif()

add_library( landlock_fixture SHARED fixture.cpp )
# This fixture only returns an integer and needs no startup code or runtime libraries.
# Avoid implicit compiler runtimes (e.g. oneAPI libraries under /opt) that are outside
# the test policy: this test should exercise loading the fixture, not its toolchain.
set_property( TARGET landlock_fixture APPEND_STRING PROPERTY LINK_FLAGS " -nostdlib" )
add_executable( test_landlock
  landlock.test.cpp
  ../../source/server/landlock.cpp
  ../../source/server/landlock_config.cpp
)
target_include_directories( test_landlock PRIVATE ../../source )
target_link_libraries( test_landlock PRIVATE ${CMAKE_DL_LIBS} )
target_compile_options( test_landlock PRIVATE -Wall -Wextra -Werror )
add_dependencies( test_landlock landlock_fixture )

foreach( CASE config readonly readwrite execute files overlap empty cleanup symlink library unavailable )
  add_test( NAME Landlock_${CASE} COMMAND test_landlock ${CASE} $<TARGET_FILE:landlock_fixture> )
  set_tests_properties( Landlock_${CASE} PROPERTIES TIMEOUT 20 )
endforeach()

if( TARGET server-exe AND TARGET client-shared AND TARGET help_plugin )
  find_package( Python3 COMPONENTS Interpreter QUIET )
  if( Python3_Interpreter_FOUND )
    add_test( NAME Landlock_server_plugin
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/server_integration.py
      $<TARGET_FILE:server-exe> $<TARGET_FILE:client-shared> $<TARGET_FILE:help_plugin> )
    set_tests_properties( Landlock_server_plugin PROPERTIES TIMEOUT 30 )
  endif()
endif()
