########################################################################################################################
# Dependencies

find_package( OpenSSL REQUIRED )

########################################################################################################################
# Targets

add_library( authentication-client-objects OBJECT udaClientSSL.cpp )
add_library( authentication-server-objects OBJECT udaServerSSL.cpp )

add_library( uda::authentication-client-objects ALIAS authentication-client-objects )
add_library( uda::authentication-server-objects ALIAS authentication-server-objects )

target_compile_features( authentication-client-objects PRIVATE cxx_std_11 )
target_compile_features( authentication-server-objects PRIVATE cxx_std_11 )

target_link_libraries( authentication-client-objects
  PUBLIC
    OpenSSL::SSL
    OpenSSL::Crypto
)

target_link_libraries( authentication-server-objects
  PUBLIC
    OpenSSL::SSL
    OpenSSL::Crypto
)
