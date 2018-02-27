cmake_minimum_required( VERSION 2.8 )

enable_testing()

find_package( Boost )
find_package( OpenSSL )
find_package( LibXml2 )
find_package( LibMemcached )

set( FAT_TESTS OFF CACHE BOOL "Build tests using fat-client" )

add_definitions( -D__USE_XOPEN2K8 )

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
  "${CMAKE_CURRENT_SOURCE_DIR}/setup.inc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/setup.inc"
  @ONLY
)

set( TESTS
  #  test_putcpp
  test_mast
)

include( modules )

include_directories( SYSTEM ${Boost_INCLUDE_DIR} )
include_directories( ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_LIST_DIR} )

add_subdirectory( plugins )
add_subdirectory( imas )

add_definitions( -D__USE_XOPEN2K8 )

foreach( TEST ${TESTS} )
  add_executable( ${TEST} test_helpers.h ${TEST}.cpp )
  target_link_libraries( ${TEST} PRIVATE
    ${Boost_LIBRARIES}
    ${PROJECT_NAME}_cpp
    ${OPENSSL_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${CACHE_LIBRARIES}
    )
  link_modules( ${TEST} )

  add_executable( fat_${TEST} test_helpers.h ${TEST}.cpp )
  target_link_libraries( fat_${TEST} PRIVATE
    ${Boost_LIBRARIES}
    fat${PROJECT_NAME}_cpp
    ${OPENSSL_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${CACHE_LIBRARIES}
    c m dl
    )
  target_compile_definitions( fat_${TEST} PRIVATE -DFATCLIENT )
  link_modules( fat_${TEST} )

  if( FAT_TESTS )
    add_test( ${TEST} fat_${TEST} )
  else()
    add_test( ${TEST} ${TEST} )
  endif()
endforeach()
