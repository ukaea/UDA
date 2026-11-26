########################################################################################################################
# Dependencies

find_package( LibXml2 REQUIRED )
find_package( OpenSSL REQUIRED )
find_package( spdlog REQUIRED )
find_package( fmt REQUIRED )
find_package( Boost REQUIRED )

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
  find_package( dlfcn-win32 CONFIG REQUIRED )
else()
  find_package( TIRPC QUIET )
endif()

########################################################################################################################
# Sources

set( SRC_FILES
  apply_XML.cpp
  get_data.cpp
  get_plugin_address.cpp
  make_server_request_block.cpp
  plugins.cpp
  server.cpp
  server_environment.cpp
  server_exceptions.cpp
  server_plugin.cpp
  server_processing.cpp
  server_subset_data.cpp
  xdr_protocol.cpp
)

########################################################################################################################
# Targets

add_library( server2-objects OBJECT
  ${SRC_FILES}
)
target_include_directories( server2-objects PRIVATE ${CMAKE_SOURCE_DIR}/source ${CMAKE_CURRENT_SOURCE_DIR} )
target_compile_definitions( server2-objects PRIVATE SERVERBUILD )
target_link_libraries( server2-objects PRIVATE Boost::boost spdlog::spdlog fmt::fmt OpenSSL::SSL LibXml2::LibXml2 )

set( SERVER2_OBJ
  $<TARGET_OBJECTS:server2-objects>
  $<TARGET_OBJECTS:clientserver-server-objects>
  $<TARGET_OBJECTS:logging-server-objects>
  $<TARGET_OBJECTS:structures-server-objects>
  $<TARGET_OBJECTS:authentication-server-objects>
  $<TARGET_OBJECTS:cache-objects>
)

add_library( server2-static STATIC ${SERVER2_OBJ} )
if( BUILD_SHARED_LIBS )
  add_library( server2-shared SHARED ${SERVER2_OBJ} )
endif()

set( LINK_XDR )
set( EXE_FLAG )
if( WIN32 OR MINGW )
  set( EXE_FLAG WIN32 )
  set( LINK_XDR XDR::XDR ws2_32 )
elseif( TIRPC_FOUND )
  set( LINK_XDR TIRPC::TIRPC )
endif()

# The object libraries compile the RPC/XDR code, so they need the same headers and -D__TIRPC__ as the archives.
target_link_libraries( server2-objects PRIVATE ${LINK_XDR} )

set( LINK_MEMCACHED )
if( LIBMEMCACHED_FOUND AND NOT NO_MEMCACHE )
  set( LINK_MEMCACHED LibMemcached::LibMemcached )
endif()

target_link_libraries( server2-static PRIVATE ${LINK_XDR} fmt::fmt )
if( BUILD_SHARED_LIBS )
  target_link_libraries( server2-shared PRIVATE ${LINK_XDR} fmt::fmt )
endif()

add_executable( server2-exe ${EXE_FLAG} server_main.cpp )
target_include_directories( server2-exe PRIVATE ${CMAKE_SOURCE_DIR}/source ${CMAKE_CURRENT_SOURCE_DIR} )
target_compile_definitions( server2-exe PRIVATE SERVERBUILD )

########################################################################################################################
# Add libraries

set( CLIENT_STATIC )
if( BUILD_SHARED_LIBS )
  set( CLIENT_STATIC client-shared )
else()
  set( CLIENT_STATIC client-static )
endif()
if( WIN32 OR MINGW )
  if( MINGW )
    set( CLIENT_STATIC ${CLIENT_STATIC} ws2_32 stdc++ )
  else()
    set( CLIENT_STATIC ${CLIENT_STATIC} ws2_32 )
  endif()
endif()

set( LINK_DL )
if( WIN32 OR MINGW )
  set( LINK_DL dlfcn-win32::dl )
else()
  set( LINK_DL dl )
endif()

set( LINK_M )
if( ( WIN32 AND MINGW ) OR NOT WIN32 )
  set( LINK_M m )
endif()

target_link_libraries( server2-static PRIVATE
  ${CLIENT_STATIC}
  OpenSSL::SSL OpenSSL::Crypto
  LibXml2::LibXml2
  ${LINK_MEMCACHED}
  ${LINK_DL}
  fmt::fmt
)

if( BUILD_SHARED_LIBS )
  set( CLIENT_SHARED client-shared plugins-shared )
  if( WIN32 OR MINGW )
    if( MINGW )
      set( CLIENT_SHARED ${CLIENT_SHARED} ws2_32 stdc++ )
    else()
      set( CLIENT_SHARED ${CLIENT_SHARED} ws2_32 )
    endif()
  endif()

  target_link_libraries( server2-shared PRIVATE
    ${CLIENT_SHARED}
    OpenSSL::SSL OpenSSL::Crypto
    LibXml2::LibXml2
    ${LINK_MEMCACHED}
    ${LINK_DL}
    fmt::fmt
  )
endif()

# Executable
set( SERVER_LINKING )
if( BUILD_SHARED_LIBS )
  set( SERVER_LINKING client-shared server2-shared plugins-shared )
else()
  set( SERVER_LINKING client-static server2-static plugins-static )
endif()

target_link_libraries( server2-exe PRIVATE
  Boost::boost
  ${SERVER_LINKING}
  OpenSSL::SSL OpenSSL::Crypto
  LibXml2::LibXml2
  ${LINK_MEMCACHED}
  ${LINK_M}
  ${LINK_DL}
  ${LINK_XDR}
)

if( NOT WIN32 AND NOT MINGW )
  target_link_libraries( server2-exe PRIVATE c )
endif()

if( NOT APPLE AND ( NOT WIN32 OR ( WIN32 AND MINGW ) ) )
  target_link_libraries( server2-exe PRIVATE gcc_s )
endif()

########################################################################################################################
# Set library properties

set_target_properties( server2-static
  PROPERTIES
  OUTPUT_NAME ${PROJECT_NAME}_server2${UDA_STATIC_LIB_SUFFIX}
)

install(
  TARGETS server2-static
  RUNTIME DESTINATION ${UDA_INSTALL_BINDIR}
  LIBRARY DESTINATION ${UDA_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${UDA_INSTALL_LIBDIR}
  COMPONENT development
)

if( BUILD_SHARED_LIBS )
  set_target_properties( server2-shared
      PROPERTIES
        OUTPUT_NAME ${PROJECT_NAME}_server2
        VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
        SOVERSION ${PROJECT_VERSION_MAJOR}
  )
  if( WIN32 )
    set_target_properties( server2-shared
      PROPERTIES
        COMPILE_FLAGS -DLIBRARY_EXPORTS
        IMPORT_SUFFIX ${IMPLIB_SUFFIX}
    )
  endif()

  install(
    TARGETS server2-shared
    RUNTIME DESTINATION ${UDA_INSTALL_BINDIR}
    LIBRARY DESTINATION ${UDA_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${UDA_INSTALL_LIBDIR}
    COMPONENT server
  )
endif()

########################################################################################################################
# Set executable properties

set_target_properties( server2-exe
  PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}_server2${EXE_EXT}
)

if( WIN32 )
  set_target_properties( server2-exe
    PROPERTIES
     COMPILE_FLAGS -DNOIMPLIB
  )
endif()

install( TARGETS server2-exe
  DESTINATION ${UDA_INSTALL_BINDIR}
  COMPONENT server
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
  launchd.udaserver.plist
)

foreach( ETC_FILE ${ETC_FILES} )
  configure_file(
    "${CMAKE_SOURCE_DIR}/source/etc/${ETC_FILE}.in"
    "${CMAKE_BINARY_DIR}/etc/${ETC_FILE}"
    @ONLY
  )
endforeach()

configure_file(
  "${CMAKE_SOURCE_DIR}/source/etc/udagenstruct.conf"
  "${CMAKE_BINARY_DIR}/etc/udagenstruct.conf"
  COPYONLY
)

install(
  FILES
    ${CMAKE_BINARY_DIR}/etc/udaserver.cfg
    ${CMAKE_BINARY_DIR}/etc/xinetd.conf
    ${CMAKE_BINARY_DIR}/etc/README.md
    ${CMAKE_BINARY_DIR}/etc/launchd.udaserver.plist
    ${CMAKE_BINARY_DIR}/etc/udagenstruct.conf
  DESTINATION ${UDA_INSTALL_CONFIGDIR}
  COMPONENT server
)

install(
  DIRECTORY
    ${CMAKE_SOURCE_DIR}/source/etc/machine.d/
  DESTINATION ${UDA_INSTALL_MACHINE_CONFIGDIR}
  COMPONENT server
)

# The server scripts are installed once, by the server directory.

install(
  DIRECTORY
    ${CMAKE_BINARY_DIR}/etc/plugins.d/
  DESTINATION ${UDA_INSTALL_PLUGIN_FRAGMENTDIR}
  COMPONENT server
)
