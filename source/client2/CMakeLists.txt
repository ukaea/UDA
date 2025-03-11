find_package( OpenSSL REQUIRED )
find_package( fmt REQUIRED )

set ( STDFS_VAR "" )
if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 9.1 )
    set ( STDFS_VAR "stdc++fs" )
elseif ( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 9.0 )
    set ( STDFS_VAR "c++fs" )
endif()

set( SRC_FILES
  client.cpp
  thread_client.cpp
  make_request_block.cpp
  client_xdr_stream.cpp
  connection.cpp
  closedown.cpp
  host_list.cpp
  generate_errors.cpp
  handle.cpp
  exceptions.cpp
)

add_library( client OBJECT ${SRC_FILES} )
target_include_directories( client PRIVATE ${CMAKE_BINARY_DIR}/include ${CMAKE_SOURCE_DIR}/source ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( client
  PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_link_libraries( client PRIVATE fmt::fmt ${STDFS_VAR} )
target_link_libraries( client PRIVATE client_server structures config logging common cache )
target_compile_definitions( client PRIVATE -DNO_GSL_LIB )
if( ENABLE_SSL )
  target_compile_definitions( client PRIVATE SSLAUTHENTICATION )
endif(  )
