cmake_minimum_required( VERSION 2.8 )

enable_testing()

find_package( Boost REQUIRED )

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_SOURCE_DIR}/source/wrappers
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/setupEnvironment.inc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/setupEnvironment.inc"
  @ONLY
)

set( TESTS test_help test_hdf5 )

include( modules )

include_directories( SYSTEM ${Boost_INCLUDE_DIR} )
include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

foreach( TEST ${TESTS} )
  add_executable( ${TEST} ${TEST}.cpp )
  target_link_libraries( ${TEST} PRIVATE ${Boost_LIBRARIES} fat${PROJECT_NAME}_cpp )
  target_compile_definitions( ${TEST} PRIVATE -DFATCLIENT )
  link_modules( ${TEST} )
  add_test( ${TEST} ${TEST} )
endforeach()