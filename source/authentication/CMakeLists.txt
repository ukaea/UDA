find_package( OpenSSL REQUIRED )
find_package( LibXml2 REQUIRED )

# Sources

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${OPENSSL_INCLUDE_DIR}
  ${LIBXML2_INCLUDE_DIR}
)

add_library( authentication-client-objects OBJECT udaClientSSL.cpp udaClientSSL.h udaServerSSL.h )

if( NOT CLIENT_ONLY )
  add_library( authentication-server-objects OBJECT udaServerSSL.cpp udaClientSSL.h )
  target_compile_definitions( authentication-server-objects PRIVATE -DSERVERBUILD )
endif()