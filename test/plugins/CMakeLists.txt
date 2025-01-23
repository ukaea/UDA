file( GLOB TEST_FILES "test_*.cpp" )

foreach( TEST_FILE ${TEST_FILES} )
  string( REGEX MATCH "test_(.*)\\.cpp" OUTVAR "${TEST_FILE}" )
  set( PLUGIN_NAME "${CMAKE_MATCH_1}" )

  if( TARGET ${PLUGIN_NAME}_plugin )
    message( "Target ${PLUGIN_NAME}_plugin exists -> building test for ${PLUGIN_NAME}" )
    BUILD_TEST( plugin_test_${PLUGIN_NAME} ${TEST_FILE} )
  else()
    message( "Target ${PLUGIN_NAME}_plugin does not exist -> skipping test for ${PLUGIN_NAME}" )
  endif()
endforeach()