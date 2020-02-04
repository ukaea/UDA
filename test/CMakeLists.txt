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
find_package( PostgreSQL )

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
  set( LIBRARIES portablexdr ws2_32 )
endif()

macro( BUILD_TEST NAME SOURCE )
  add_executable( ${NAME} ${EXE_FLAG} ${HELPER_FILES} ${SOURCE} )
  target_link_libraries( ${NAME} PRIVATE
    ${Boost_LIBRARIES}
    ${PROJECT_NAME}_cpp
    ${OPENSSL_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${CACHE_LIBRARIES}
	${LIBRARIES}
	stdc++
  )

  set_target_properties( ${NAME}
    PROPERTIES
      OUTPUT_NAME ${NAME}${EXE_EXT}
  )
  
  add_executable( fat_${NAME} ${EXE_FLAG} ${HELPER_FILES} ${SOURCE} )
  target_link_libraries( fat_${NAME} PRIVATE
    ${Boost_LIBRARIES}
    fat${PROJECT_NAME}_cpp
    ${OPENSSL_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${CACHE_LIBRARIES}
	${LIBRARIES}
    m dl stdc++
  )
  if( NOT WIN32 AND NOT MINGW )
    target_link_libraries( fat_${NAME} PRIVATE c )
  endif()
  target_compile_definitions( fat_${NAME} PRIVATE -DFATCLIENT )

  set_target_properties( fat_${NAME}
    PROPERTIES
      OUTPUT_NAME fat_${NAME}${EXE_EXT}
  )

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
