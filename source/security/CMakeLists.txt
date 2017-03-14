cmake_minimum_required( VERSION 2.8 )

if( NOT ENABLE_AUTH )
  return()
endif()

find_package( GCrypt REQUIRED )
find_package( KSBA REQUIRED )

set( SRC_FILES
  security.c
  authenticationUtils.c
  x509Utils.c
)

include_directories( ${CMAKE_SOURCE_DIR}/source ${GCRYPT_INCLUDE_DIRS} ${KSBA_INCLUDE_DIRS} )

add_library( security-client-objects OBJECT ${SRC_FILES} clientAuthentication.c )
add_library( security-server-objects OBJECT ${SRC_FILES} serverAuthentication.c )

# Authentication code without any passing of objects between the client and server to use in local test executables.
add_library( security-test-objects OBJECT ${SRC_FILES} clientAuthentication.c serverAuthentication.c )
target_compile_definitions( security-test-objects PRIVATE -DTESTIDAMSECURITY )
