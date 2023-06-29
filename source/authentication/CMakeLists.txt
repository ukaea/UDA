cmake_minimum_required( VERSION 2.8 )

#find_package( OpenSSL REQUIRED )

# Sources

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${OPENSSL_INCLUDE_DIR}
)

add_library( authentication-client-objects OBJECT udaClientSSL.cpp udaClientSSL.h udaServerSSL.h )

if( NOT CLIENT_ONLY )
  add_library( authentication-server-objects OBJECT udaServerSSL.cpp udaClientSSL.h )
  target_compile_definitions( authentication-server-objects PRIVATE -DSERVERBUILD )
endif()
