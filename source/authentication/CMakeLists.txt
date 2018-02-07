cmake_minimum_required( VERSION 2.8 )

find_package( OpenSSL REQUIRED )

# Sources

set( SRC_FILES
  udaClientSSL.c
  udaServerSSL.c
)

set( HEADER_FILES
  udaSSL.h
)

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${OPENSSL_INCLUDE_DIR}
)

add_library( authentication-client-objects OBJECT ${SRC_FILES} ${HEADER_FILES} )
add_library( authentication-server-objects OBJECT ${SRC_FILES} ${HEADER_FILES} )

target_compile_definitions( authentication-server-objects PRIVATE -DSERVERBUILD )