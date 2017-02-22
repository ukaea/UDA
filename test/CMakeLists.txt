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

find_package( GCrypt REQUIRED )
find_package( KSBA REQUIRED )
find_package( OpenSSL REQUIRED )
find_package( LibXml2 REQUIRED )

add_executable( testSecurity testSecurity.c $<TARGET_OBJECTS:security> )
target_link_libraries( testSecurity PRIVATE
  ${GCRYPT_LIBRARIES}
  ${KSBA_LIBRARIES}
  fatclient-static
  client-shared
  ${OPENSSL_LIBRARIES}
  ${LIBXML2_LIBRARIES} )
link_modules( testSecurity )
add_test( testSecurity testSecurity )