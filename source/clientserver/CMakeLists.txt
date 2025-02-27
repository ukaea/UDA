find_package( Boost COMPONENTS headers REQUIRED )
find_package( OpenSSL REQUIRED )
find_package( fmt REQUIRED )

set( SRCS
  compress_dim.cpp
  copy_structs.cpp
  error_log.cpp
  expand_path.cpp
  init_structs.cpp
  makeRequestBlock.cpp
  make_request_block.cpp
  manage_sockets.cpp
  name_value_substitution.cpp
  name_value_list.cpp
  parse_operation.cpp
  print_structs.cpp
  uda_defines.cpp
  uda_structs.cpp
  uda_types.cpp
  userid.cpp
  version.cpp
)

add_library( client_server OBJECT ${SRCS} )
target_include_directories( client_server PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/source ${CMAKE_CURRENT_SOURCE_DIR} )
target_link_libraries( client_server PUBLIC Boost::boost OpenSSL::SSL OpenSSL::Crypto fmt::fmt ${RPC_LIB} )
target_link_libraries( client_server PRIVATE common )