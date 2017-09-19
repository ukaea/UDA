
include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_SOURCE_DIR}/source/wrappers
)

set( TESTS
  test_imas_jet_magnetics
  test_imas_jet_tf
  test_imas_jet_pf_active
  test_imas_jet_all
)

include( modules )

include_directories( SYSTEM ${Boost_INCLUDE_DIR} )
include_directories( ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_LIST_DIR} )

add_definitions( -D__USE_XOPEN2K8 )

foreach( TEST ${TESTS} )
  add_executable( ${TEST} ${TEST}.cpp )
  target_link_libraries( ${TEST} PRIVATE
    ${Boost_LIBRARIES}
    ${PROJECT_NAME}_cpp
    client-shared
    ${OPENSSL_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${CACHE_LIBRARIES}
  )
  link_modules( ${TEST} )

  add_executable( fat_${TEST} ${TEST}.cpp )
  target_link_libraries( fat_${TEST} PRIVATE
    ${Boost_LIBRARIES}
    fat${PROJECT_NAME}_cpp
    fatclient-shared
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
