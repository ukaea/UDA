cmake_minimum_required( VERSION 2.8 )

enable_testing()

find_package( Boost )
find_package( OpenSSL )
find_package( LibXml2 )
find_package( LibMemcached )

if( NOT Boost_FOUND OR NOT OPENSSL_FOUND OR NOT LIBXML2_FOUND )
  message( WARNING "Not building tests" )
  return()
endif()

if( LIBMEMCACHED_FOUND )
  set( CACHE_LIBRARIES ${LIBMEMCACHED_LIBRARIES} )
endif()

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_SOURCE_DIR}/source/wrappers
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/setupEnvironment.inc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/setupEnvironment.inc"
  @ONLY
)

set( TESTS test_help test_hdf5 test_putcpp test_putdata )

include( modules )

include_directories( SYSTEM ${Boost_INCLUDE_DIR} )
include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

add_definitions( -D__USE_XOPEN2K8 )

foreach( TEST ${TESTS} )
  add_executable( ${TEST} ${TEST}.cpp )
  target_link_libraries( ${TEST} PRIVATE
    ${Boost_LIBRARIES} 
    fat${PROJECT_NAME}_cpp
    fatclient-shared
    ${OPENSSL_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${CACHE_LIBRARIES}
    c
    m
    dl
  )
  target_compile_definitions( ${TEST} PRIVATE -DFATCLIENT )
  link_modules( ${TEST} )
  add_test( ${TEST} ${TEST} )
endforeach()

find_package( GCrypt )
find_package( KSBA )

add_definitions( -g -O0 )

if( GCRYPT_FOUND AND KSBA_FOUND AND OPENSSL_FOUND AND LIBXML2_FOUND )
  add_executable( testSecurity testSecurity.c $<TARGET_OBJECTS:security-test-objects> )
  target_link_libraries( testSecurity PRIVATE
    ${GCRYPT_LIBRARIES}
    ${KSBA_LIBRARIES}
    client-shared
    ${OPENSSL_LIBRARIES}
    ${LIBXML2_LIBRARIES} )
  add_test( testSecurity testSecurity )
endif()
