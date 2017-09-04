find_package( Boost )
find_package( OpenSSL )
find_package( LibXml2 )
find_package( LibMemcached )

file( GLOB TEST_FILES "test_*.cpp" )

foreach( TEST_FILE ${TEST_FILES} )
  string( REGEX MATCH "test_(.*)\\.cpp" OUTVAR "${TEST_FILE}" )
  set( PLUGIN_NAME "${CMAKE_MATCH_1}" )

  if( TARGET ${PLUGIN_NAME}_plugin )
    message( "Target ${PLUGIN_NAME}_plugin exists -> building test for ${PLUGIN_NAME}" )

    set( TEST plugin_test_${PLUGIN_NAME} )

    add_executable( ${TEST} ${TEST_FILE} )
    target_link_libraries( ${TEST} PRIVATE
      ${Boost_LIBRARIES}
      ${PROJECT_NAME}_cpp
      client-shared
      ${OPENSSL_LIBRARIES}
      ${LIBXML2_LIBRARIES}
      ${CACHE_LIBRARIES}
    )

    add_executable( fat_${TEST} ${TEST_FILE} )
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

    add_test( ${TEST} ${TEST} )
  endif()
endforeach()
