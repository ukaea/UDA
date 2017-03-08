cmake_minimum_required( VERSION 2.8 )

find_package( GCrypt )
find_package( KSBA )

if( NOT GCRYPT_FOUND OR NOT KSBA_FOUND )
  message( WARNING "GCrypt or KSBA not found - security library being skipped" )
  add_library( security OBJECT )
  return()
endif()

set( SRC_FILES
  serverAuthentication.c
  clientAuthentication.c
  security.c
  authenticationUtils.c
  )

include_directories( ${CMAKE_SOURCE_DIR}/source ${GCRYPT_INCLUDE_DIRS} ${KSBA_INCLUDE_DIRS} )

add_definitions( -DTESTIDAMSECURITY )

add_library( security OBJECT ${SRC_FILES} )
