find_package( OpenSSL REQUIRED )
find_package( LibXml2 REQUIRED )

set( LIBS OpenSSL::SSL OpenSSL::Crypto LibXml2::LibXml2 )

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
  list( APPEND LIBS XDR::XDR )
else()
  find_package( TIRPC QUIET)
  if( TIRPC_FOUND )
    list( APPEND LIBS TIRPC::TIRPC )
  endif()
endif()

# Sources

add_library( authentication-client-objects OBJECT udaClientSSL.cpp updateSelectParms.cpp )
target_link_libraries( authentication-client-objects ${LIBS} )

target_include_directories( authentication-client-objects PRIVATE
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_SOURCE_DIR}/source/include
  ${CMAKE_BINARY_DIR}/include
)

if( NOT CLIENT_ONLY )
  add_library( authentication-server-objects OBJECT udaServerSSL.cpp updateSelectParms.cpp )
  target_compile_definitions( authentication-server-objects PRIVATE -DSERVERBUILD )
  target_link_libraries( authentication-server-objects ${LIBS} )
  target_include_directories( authentication-server-objects PRIVATE
    ${CMAKE_SOURCE_DIR}/source
    ${CMAKE_SOURCE_DIR}/source/include
    ${CMAKE_BINARY_DIR}/include
  )
endif()