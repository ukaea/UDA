find_package( Boost REQUIRED )
#find_package( OpenSSL REQUIRED )
find_package( fmt REQUIRED )

set( SRC_FILES
  apply_XML.cpp
  get_data.cpp
  make_server_request_block.cpp
  plugins.cpp
  server.cpp
  server_exceptions.cpp
  server_plugin.cpp
  server_processing.cpp
  server_subset_data.cpp
  xdr_protocol.cpp
)

add_library( server ${SRC_FILES} )
target_include_directories( server PRIVATE ${CMAKE_BINARY_DIR}/include ${CMAKE_SOURCE_DIR}/source ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( server
  PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_link_libraries( server PRIVATE clientserver Boost::boost fmt::fmt structures config logging common cache )
target_compile_definitions( server PRIVATE -DSHARED_LIBRARY_SUFFIX="${CMAKE_SHARED_LIBRARY_SUFFIX}" )

set_target_properties( server
  PROPERTIES
  OUTPUT_NAME uda_server
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

add_library( uda::server ALIAS server )

install( TARGETS server
  EXPORT udaServerTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

########################################################################################################################
# Server executable

add_executable( server-exe ${EXE_FLAG} server_main.cpp )
target_include_directories( server-exe PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_BINARY_DIR}/include ${CMAKE_SOURCE_DIR}/source ${CMAKE_CURRENT_SOURCE_DIR} )
target_link_libraries( server-exe PRIVATE Boost::boost fmt::fmt server common config )

install( TARGETS server-exe
  DESTINATION bin
)