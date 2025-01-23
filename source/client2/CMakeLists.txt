find_package( OpenSSL REQUIRED )
find_package( fmt REQUIRED )

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

add_library( client ${SRC_FILES} )
target_include_directories( client PRIVATE ${CMAKE_SOURCE_DIR}/source ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( client
  PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_link_libraries( client PRIVATE fmt::fmt cache clientserver structures config logging common )
target_compile_definitions( client PRIVATE -DNO_GSL_LIB )

set_target_properties( client
  PROPERTIES
    OUTPUT_NAME uda_client
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

add_library( uda::client ALIAS client )

install( TARGETS client
  EXPORT udaClientTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)