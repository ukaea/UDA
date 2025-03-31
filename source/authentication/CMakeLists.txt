find_package( OpenSSL REQUIRED )
find_package( fmt REQUIRED )

set( SRCS
  client_ssl.cpp
  server_ssl.cpp
  update_select_params.cpp
)

add_library( authentication OBJECT ${SRCS} )
target_include_directories( authentication PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/source ${CMAKE_CURRENT_SOURCE_DIR} )
target_link_libraries( authentication PRIVATE OpenSSL::SSL OpenSSL::Crypto fmt::fmt )
if( ENABLE_SSL )
target_compile_definitions( authentication PRIVATE SSLAUTHENTICATION )
endif(  )