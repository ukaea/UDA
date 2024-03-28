file( GLOB TEST_FILES "test_*.cpp" )

set( LINK_DL )
if( WIN32 )
  set( LINK_DL dlfcn-win32::dl )
else()
  set( LINK_DL dl )
endif()

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

#add_executable( cpp_testplugin ${EXE_FLAG} ${HELPER_FILES} test2_testplugin.cpp )
#target_include_directories( cpp_testplugin PRIVATE
#  ${LIBXML2_INCLUDE_DIR}
#)
#target_link_libraries( cpp_testplugin PRIVATE
#  ${Boost_LIBRARIES}
#  client2-shared
#  ${OPENSSL_LIBRARIES}
#  ${LIBXML2_LIBRARIES}
#  ${CACHE_LIBRARIES}
#  ${LIBRARIES}
#  ${LINK_M}
#  ${LINK_DL}
#  ${LINK_STD}
#)
#if( NOT WIN32 AND NOT MINGW )
#  target_link_libraries( cpp_testplugin PRIVATE c )
#endif()
#
#set_target_properties( cpp_testplugin
#  PROPERTIES
#  OUTPUT_NAME cpp_testplugin
#)
#
#if( WIN32 )
#  set_target_properties( cpp_testplugin
#    PROPERTIES
#    COMPILE_FLAGS -DNOIMPLIB
#  )
#endif()