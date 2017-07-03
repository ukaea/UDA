cmake_minimum_required( VERSION 2.8 )

find_package( GCrypt )
find_package( KSBA )

if( NOT GCRYPT_FOUND OR NOT KSBA_FOUND )
  message( WARNING "GCrypt or KSBA not found - security library being skipped" )
  return()
endif()

set( SRC_FILES
  serverAuthentication.c
  clientAuthentication.c
  security.c
  authenticationUtils.c
  x509Utils.c
)

include_directories( ${CMAKE_SOURCE_DIR}/source ${GCRYPT_INCLUDE_DIRS} ${KSBA_INCLUDE_DIRS} )

add_library( security-objects OBJECT ${SRC_FILES} )

# Authentication code without any passing of objects between the client and server to use in local test executables.
add_library( security-test-objects OBJECT ${SRC_FILES} )
target_compile_definitions( security-test-objects PRIVATE -DTESTIDAMSECURITY )
