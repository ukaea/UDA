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

add_executable( cpp_testplugin ${EXE_FLAG} ${HELPER_FILES} test2_testplugin.cpp )
target_compile_features( cpp_testplugin PRIVATE cxx_std_11 )
target_include_directories( cpp_testplugin PRIVATE
  ${PROJECT_SOURCE_DIR}/test
  ${PROJECT_BINARY_DIR}/test
)
target_link_libraries( cpp_testplugin PRIVATE
  uda::client2-shared
  Boost::boost
  OpenSSL::SSL
  OpenSSL::Crypto
  LibXml2::LibXml2
)
if( NOT WIN32 AND NOT MINGW )
  target_link_libraries( cpp_testplugin PRIVATE c )
endif()

set_target_properties( cpp_testplugin
  PROPERTIES
  OUTPUT_NAME cpp_testplugin${EXE_EXT}
)

if( WIN32 )
  set_target_properties( cpp_testplugin
    PROPERTIES
    COMPILE_FLAGS -DNOIMPLIB
  )
endif()