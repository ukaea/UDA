########################################################################################################################

if( ENABLE_LANDLOCK )
  enable_testing()
  add_subdirectory( landlock )
elseif( NOT WIN32 AND TARGET server-exe AND TARGET client-shared AND TARGET help_plugin )
  # Without Landlock support, a configured policy must make the server refuse to start.
  find_package( Python3 COMPONENTS Interpreter QUIET )
  if( Python3_Interpreter_FOUND )
    enable_testing()
    add_test( NAME Landlock_server_plugin_disabled
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/landlock/server_integration.py
      $<TARGET_FILE:server-exe> $<TARGET_FILE:client-shared> $<TARGET_FILE:help_plugin> --disabled )
    set_tests_properties( Landlock_server_plugin_disabled PROPERTIES TIMEOUT 30 )
  endif()
endif()

if( NO_WRAPPERS )
  message( WARNING "C++ wrapper not built - skipping wrapper tests" )
  return()
endif()

enable_testing()

# Plugin support / plugin libraries must not embed the client or server logger and error-stack state, and only plugins
# that opt in with LINK_CLIENT may depend on the client library (see cmake/checkPluginSymbols.cmake)
if( BUILD_SHARED_LIBS AND NOT WIN32 AND NOT MINGW AND TARGET plugins-shared )
  if( APPLE )
    find_program( LINK_GUARD_DEPS_TOOL otool )
  else()
    find_program( LINK_GUARD_DEPS_TOOL readelf )
  endif()
  set( LINK_GUARD_STATELESS_LIBS $<TARGET_FILE:plugins-shared> )
  foreach( GUARD_PLUGIN bytes_plugin help_plugin template_plugin testplugin_plugin )
    if( TARGET ${GUARD_PLUGIN} )
      list( APPEND LINK_GUARD_STATELESS_LIBS $<TARGET_FILE:${GUARD_PLUGIN}> )
    endif()
  endforeach()
  set( LINK_GUARD_LIBS ${LINK_GUARD_STATELESS_LIBS} )
  if( TARGET uda_plugin )
    list( APPEND LINK_GUARD_LIBS $<TARGET_FILE:uda_plugin> )   # uses the client API (LINK_CLIENT)
  endif()
  add_test( NAME plugin_link_guard
    COMMAND ${CMAKE_COMMAND} -DNM=${CMAKE_NM} -DDEPS_TOOL=${LINK_GUARD_DEPS_TOOL} "-DLIBS=${LINK_GUARD_LIBS}"
      "-DSTATELESS_LIBS=${LINK_GUARD_STATELESS_LIBS}" -P ${CMAKE_SOURCE_DIR}/cmake/checkPluginSymbols.cmake )
endif()

find_package( Boost )
find_package( OpenSSL )
find_package( LibXml2 )
find_package( LibMemcached )

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
  find_package( dlfcn-win32 CONFIG REQUIRED )
else()
  find_package( TIRPC QUIET )
  if( TIRPC_FOUND )
    add_definitions( -D__TIRPC__ )
  endif()
endif()

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

include_directories( SYSTEM ${Boost_INCLUDE_DIR} )
include_directories( SYSTEM ${Boost_INCLUDE_DIRS} )
include_directories( SYSTEM ${LIBXML2_INCLUDE_DIR} )
include_directories( ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_LIST_DIR} )

link_directories( ${CMAKE_BINARY_DIR}/source/wrappers/c++ )

file( GLOB HELPER_FILES "test_helpers.*" )

set( LIBRARIES )
set( EXE_FLAG )
if( WIN32 OR MINGW )
  set( EXE_FLAG WIN32 )
  include_directories( ${XDR_INCLUDE_DIR} )
  set( LIBRARIES ${XDR_LIBRARIES} ws2_32 )
elseif( TIRPC_FOUND )
  include_directories( ${TIRPC_INCLUDE_DIR} )
  set( LINK_LIB ${LINK_LIB} ${TIRPC_LIBRARIES} )
endif()

if( ENABLE_CAPNP )
  set( LIBRARIES ${LIBRARIES} serialisation-static )
endif()

set( LINK_DL )
if( WIN32 )
  set( LINK_DL dlfcn-win32::dl )
else()
  set( LINK_DL dl )
endif()

set( LINK_STD )
if( ( WIN32 AND MINGW ) OR NOT WIN32 )
  set( LINK_STD stdc++ )
endif()

set( LINK_M )
if( ( WIN32 AND MINGW ) OR NOT WIN32 )
  set( LINK_M m )
endif()

macro( BUILD_TEST NAME SOURCE )
  set( LINK_PROJECT ${PROJECT_NAME}_cpp-static )
  if( BUILD_SHARED_LIBS )
    set( LINK_PROJECT ${PROJECT_NAME}_cpp-shared )
  endif()

  add_executable( ${NAME} ${EXE_FLAG} ${HELPER_FILES} ${SOURCE} )
  target_link_libraries( ${NAME} PRIVATE
    ${Boost_LIBRARIES}
    ${LINK_PROJECT}
    ${OPENSSL_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${CACHE_LIBRARIES}
    ${LIBRARIES}
    ${LINK_STD}
  )

  set_target_properties( ${NAME}
    PROPERTIES
      OUTPUT_NAME ${NAME}
  )
  
  if( WIN32 )
    set_target_properties( ${NAME}
      PROPERTIES
       COMPILE_FLAGS -DNOIMPLIB
    )
  endif()
  
  add_executable( fat_${NAME} ${EXE_FLAG} ${HELPER_FILES} ${SOURCE} )
  target_link_libraries( fat_${NAME} PRIVATE
    ${Boost_LIBRARIES}
    fat${LINK_PROJECT}
    ${OPENSSL_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${CACHE_LIBRARIES}
    ${LIBRARIES}
    ${LINK_M}
    ${LINK_DL}
    ${LINK_STD}
  )
  if( NOT WIN32 AND NOT MINGW )
    target_link_libraries( fat_${NAME} PRIVATE c )
  endif()
  target_compile_definitions( fat_${NAME} PRIVATE -DFATCLIENT )

  set_target_properties( fat_${NAME}
    PROPERTIES
      OUTPUT_NAME fat_${NAME}
  )

  if( WIN32 )
    set_target_properties( fat_${NAME}
      PROPERTIES
       COMPILE_FLAGS -DNOIMPLIB
    )
  endif()

  if( FAT_TESTS )
    add_test( ${NAME} fat_${NAME} -r junit -o ${NAME}_out.xml )
  else()
    add_test( ${NAME} ${NAME} -r junit -o ${NAME}_out.xml )
  endif()
endmacro( BUILD_TEST )

add_subdirectory( plugins )
# add_subdirectory( imas )
add_subdirectory( unit_tests )

add_definitions( -D__USE_XOPEN2K8 )

foreach( TEST ${TESTS} )
  BUILD_TEST( ${TEST} ${TEST}.cpp )
endforeach()
