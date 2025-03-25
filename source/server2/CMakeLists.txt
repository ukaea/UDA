find_package( Boost REQUIRED )
find_package( fmt REQUIRED )

set ( STDFS_VAR "" )
if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 9.1 )
    set ( STDFS_VAR "stdc++fs" )
elseif ( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 9.0 )
    set ( STDFS_VAR "c++fs" )
endif()

set( SRC_FILES
  get_data.cpp
  make_server_request_block.cpp
  plugins.cpp
  server.cpp
  server_exceptions.cpp
  server_plugin.cpp
  server_processing.cpp
  server_subset_data.cpp
)

add_library( server OBJECT ${SRC_FILES} )
target_include_directories( server PRIVATE ${CMAKE_BINARY_DIR}/include ${CMAKE_SOURCE_DIR}/source ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( server
  PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_link_libraries( server PRIVATE Boost::boost fmt::fmt dl ${STDFS_VAR} )
target_link_libraries( server PRIVATE
  "$<BUILD_INTERFACE:client_server>"
  "$<BUILD_INTERFACE:structures>"
  "$<BUILD_INTERFACE:config>"
  "$<BUILD_INTERFACE:logging>"
  "$<BUILD_INTERFACE:common>"
  "$<BUILD_INTERFACE:cache>"
)
target_compile_definitions( server PRIVATE -DSHARED_LIBRARY_SUFFIX="${CMAKE_SHARED_LIBRARY_SUFFIX}" )
if( ENABLE_SSL )
  target_compile_definitions( server PRIVATE SSLAUTHENTICATION )
endif(  )

########################################################################################################################
# Server executable

add_executable( server-exe ${EXE_FLAG} server_main.cpp )
target_include_directories( server-exe PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_BINARY_DIR}/include ${CMAKE_SOURCE_DIR}/source ${CMAKE_CURRENT_SOURCE_DIR} )
target_link_libraries( server-exe PRIVATE Boost::boost fmt::fmt ${RPC_LIB} )
target_link_libraries( server-exe PRIVATE server authentication common config structures logging client_server cache protocol )

set_target_properties( server-exe
  PROPERTIES
  OUTPUT_NAME uda_server
)
install( TARGETS server-exe
  DESTINATION bin
)

########################################################################################################################
# Configure server files

string( REPLACE ";" ":" EXTRA_LD_LIBRARY_PATHS "${EXTRA_LD_LIBRARY_PATHS}" )

set( ETC_FILES
  udaserver.sh
  udaserver.cfg
  uda_server.toml
  xinetd.conf
  rc.uda
  README.md
  uda.socket
  uda@.service
  uda.plist
)

set( GEN_ETC_FILES ${CMAKE_BINARY_DIR}/etc/udagenstruct.conf )
set( INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR} )

include( GNUInstallDirs )
set( INSTALL_LIBDIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}" )

foreach( ETC_FILE ${ETC_FILES} )
  configure_file(
    "${CMAKE_SOURCE_DIR}/source/etc/${ETC_FILE}.in"
    "${CMAKE_BINARY_DIR}/etc/${ETC_FILE}"
    @ONLY
  )
  list( APPEND GEN_ETC_FILES ${CMAKE_BINARY_DIR}/etc/${ETC_FILE} )
endforeach()

configure_file(
  "${CMAKE_SOURCE_DIR}/source/etc/udagenstruct.conf"
  "${CMAKE_BINARY_DIR}/etc/udagenstruct.conf"
  COPYONLY
)

install(
  FILES ${GEN_ETC_FILES}
  DESTINATION etc
)

set( PC_FILES
  uda-client.pc
  uda-cpp.pc
  uda-plugins.pc
)

foreach( PC_FILE ${PC_FILES} )
  configure_file(
    "${CMAKE_SOURCE_DIR}/source/etc/${PC_FILE}.in"
    "${CMAKE_BINARY_DIR}/lib/pkgconfig/${PC_FILE}"
    @ONLY
  )
endforeach()

install(
  DIRECTORY ${CMAKE_BINARY_DIR}/lib/pkgconfig
  DESTINATION lib
)

install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/source/etc/machine.d
  DESTINATION etc
)

install(
  FILES
    ${CMAKE_BINARY_DIR}/etc/udaserver.sh
    ${CMAKE_BINARY_DIR}/etc/rc.uda
  DESTINATION etc
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ WORLD_READ
)

install(
  DIRECTORY ${CMAKE_BINARY_DIR}/etc/plugins.d
  DESTINATION etc
)

install(
  FILES ${CMAKE_BINARY_DIR}/etc/udagenstruct.conf
  DESTINATION etc
)

install( FILES ${HEADER_FILES}
  DESTINATION include/uda/server
)

########################################################################################################################
# Configure server files

string( REPLACE ";" ":" EXTRA_LD_LIBRARY_PATHS "${EXTRA_LD_LIBRARY_PATHS}" )

set( ETC_FILES
  udaserver.sh
  udaserver.cfg
  xinetd.conf
  rc.uda
  README.md
  uda.socket
  uda@.service
  uda.plist
)

set( GEN_ETC_FILES ${CMAKE_BINARY_DIR}/etc/udagenstruct.conf )

foreach( ETC_FILE ${ETC_FILES} )
  configure_file(
    "${CMAKE_SOURCE_DIR}/source/etc/${ETC_FILE}.in"
    "${CMAKE_BINARY_DIR}/etc/${ETC_FILE}"
    @ONLY
  )
  list( APPEND GEN_ETC_FILES ${CMAKE_BINARY_DIR}/etc/${ETC_FILE} )
endforeach()

configure_file(
  "${CMAKE_SOURCE_DIR}/source/etc/udagenstruct.conf"
  "${CMAKE_BINARY_DIR}/etc/udagenstruct.conf"
  COPYONLY
)

install(
  FILES ${GEN_ETC_FILES}
  DESTINATION etc
)

set( PC_FILES
  uda-client.pc
  uda-cpp.pc
  uda-plugins.pc
)

foreach( PC_FILE ${PC_FILES} )
  configure_file(
    "${CMAKE_SOURCE_DIR}/source/etc/${PC_FILE}.in"
    "${CMAKE_BINARY_DIR}/lib/pkgconfig/${PC_FILE}"
    @ONLY
  )
endforeach()

install(
  DIRECTORY ${CMAKE_BINARY_DIR}/lib/pkgconfig
  DESTINATION lib
)

install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/source/etc/machine.d
  DESTINATION etc
)

install(
  FILES
    ${CMAKE_BINARY_DIR}/etc/udaserver.sh
    ${CMAKE_BINARY_DIR}/etc/rc.uda
  DESTINATION etc
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ WORLD_READ
)

install(
  DIRECTORY ${CMAKE_BINARY_DIR}/etc/plugins.d
  DESTINATION etc
)

install(
  FILES ${CMAKE_BINARY_DIR}/etc/udagenstruct.conf
  DESTINATION etc
)

install( FILES ${HEADER_FILES}
  DESTINATION include/uda/server
)
