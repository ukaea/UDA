########################################################################################################################

if( NO_WRAPPERS )
  message( WARNING "C++ wrapper not built - skipping tests" )
  return()
endif()

enable_testing()

find_package( Boost )
find_package( OpenSSL )
find_package( LibXml2 )
find_package( LibMemcached )

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
  if( NOT MINGW )
    find_package( dlfcn-win32 CONFIG REQUIRED )
  endif()
else()
  find_package( TIRPC QUIET)
  if(TIRPC_FOUND)
    add_definitions(-D__TIRPC__)
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
include_directories( ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_LIST_DIR} )

link_directories( ${CMAKE_BINARY_DIR}/source/wrappers/c++ )

file( GLOB HELPER_FILES "test_helpers.*" )

set( LIBRARIES )
set( EXE_FLAG )
set( EXE_EXT )
if( WIN32 OR MINGW )
  set( EXE_FLAG WIN32 )
  if( MINGW )
    set( EXE_EXT ".exe" )
  endif()
  include_directories( ${XDR_INCLUDE_DIR} )
  set( LIBRARIES ${XDR_LIBRARIES} ws2_32 )
elseif(TIRPC_FOUND)
  include_directories( ${TIRPC_INCLUDE_DIR} )
  set( LINK_LIB ${LINK_LIB} ${TIRPC_LIBRARIES})
endif()

set( LINK_DL )
if( WIN32 AND NOT MINGW )
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
    serialisation-static
  )

  set_target_properties( ${NAME}
    PROPERTIES
      OUTPUT_NAME ${NAME}${EXE_EXT}
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
    serialisation-static
  )
  if( NOT WIN32 AND NOT MINGW )
    target_link_libraries( fat_${NAME} PRIVATE c )
  endif()
  target_compile_definitions( fat_${NAME} PRIVATE -DFATCLIENT )

  set_target_properties( fat_${NAME}
    PROPERTIES
      OUTPUT_NAME fat_${NAME}${EXE_EXT}
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
add_subdirectory( imas )

add_definitions( -D__USE_XOPEN2K8 )

foreach( TEST ${TESTS} )
  BUILD_TEST( ${TEST} ${TEST}.cpp )
endforeach()
