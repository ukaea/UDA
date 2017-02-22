cmake_minimum_required( VERSION 2.8 )

find_package( GCrypt REQUIRED )
find_package( KSBA REQUIRED )

set( SRC_FILES
  serverAuthentication.c
  clientAuthentication.c
  security.c
  )

include_directories( ${CMAKE_SOURCE_DIR}/source ${GCRYPT_INCLUDE_DIRS} ${KSBA_INCLUDE_DIRS} )

add_library( security OBJECT ${SRC_FILES} )