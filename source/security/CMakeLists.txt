find_package( GCrypt )
find_package( KSBA )

if( NOT GCrypt_FOUND OR NOT KSBA_FOUND )
  message( WARNING "GCrypt or KSBA not found - security library being skipped" )
  return()
endif()

set( SRC_FILES
  serverAuthentication.cpp
  clientAuthentication.cpp
  security.cpp
  authenticationUtils.cpp
  x509Utils.cpp
)

add_library( security-objects OBJECT ${SRC_FILES} )
target_include_directories( security-objects PRIVATE ${CMAKE_SOURCE_DIR}/source )
target_link_libraries( security-objects PRIVATE GCrypt::gcrypt KSBA::KSBA )

# Authentication code without any passing of objects between the client and server to use in local test executables.
add_library( security-test-objects OBJECT ${SRC_FILES} )
target_compile_definitions( security-test-objects PRIVATE -DTESTIDAMSECURITY )
target_include_directories( security-test-objects PRIVATE ${CMAKE_SOURCE_DIR}/source )
target_link_libraries( security-test-objects PRIVATE GCrypt::gcrypt KSBA::KSBA )
