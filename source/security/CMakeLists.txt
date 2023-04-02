########################################################################################################################
# Dependencies

find_package( GCrypt )
find_package( KSBA )

if( NOT GCRYPT_FOUND OR NOT KSBA_FOUND )
  message( WARNING "GCrypt or KSBA not found - security library being skipped" )
  return()
endif()

########################################################################################################################
# Sources

set( SRC_FILES
  serverAuthentication.cpp
  clientAuthentication.cpp
  security.cpp
  authenticationUtils.cpp
  x509Utils.cpp
)

########################################################################################################################
# Targets

add_library( security-objects OBJECT )
add_library( uda::security-objects ALIAS security-objects )

target_sources( security-objects PRIVATE ${SRC_FILES} )
target_compile_features( security-objects PRIVATE cxx_std_11 )
target_link_libraries( security-objects PUBLIC GCrypt::gcrypt KSBA::KSBA )
target_include_directories( security-objects PRIVATE ${PROJECT_SOURCE_DIR}/source )
