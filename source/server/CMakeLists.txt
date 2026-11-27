########################################################################################################################
# Dependencies

find_package( LibXml2 REQUIRED )
find_package( OpenSSL REQUIRED )
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
  applyXML.cpp
  closeServerSockets.cpp
  createXDRStream.cpp
  getPluginAddress.cpp
  getServerEnvironment.cpp
  initPluginList.cpp
  landlock_config.cpp
  makeServerRequestBlock.cpp
  serverGetData.cpp
  serverLegacyPlugin.cpp
  serverPlugin.cpp
  serverProcessing.cpp
  serverStartup.cpp
  serverSubsetData.cpp
  writer.cpp
)

set( HEADER_FILES
  applyXML.h
  closeServerSockets.h
  createXDRStream.h
  getPluginAddress.h
  getServerEnvironment.h
  initPluginList.h
  landlock.h
  makeServerRequestBlock.h
  serverGetData.h
  serverLegacyPlugin.h
  serverPlugin.h
  serverProcessing.h
  serverStartup.h
  serverSubsetData.h
  sleepServer.h
  udaLegacyServer.h
  udaServer.h
  writer.h
)

########################################################################################################################
# Targets

add_library( server-objects OBJECT
  udaServer.cpp
  udaLegacyServer.cpp
  createXDRStream.cpp
  sleepServer.cpp
  ${SRC_FILES}
  ${HEADER_FILES}
)
target_include_directories( server-objects PRIVATE ${CMAKE_SOURCE_DIR}/source ${CMAKE_CURRENT_SOURCE_DIR} )
target_compile_definitions( server-objects PRIVATE SERVERBUILD )
target_link_libraries( server-objects PRIVATE fmt::fmt Boost::boost OpenSSL::SSL LibXml2::LibXml2 )

add_library( fatserver-objects OBJECT
  fatServer.cpp
  ${SRC_FILES}
  ${HEADER_FILES}
)
target_include_directories( fatserver-objects PRIVATE ${CMAKE_SOURCE_DIR}/source ${CMAKE_CURRENT_SOURCE_DIR} )
target_compile_definitions( fatserver-objects PRIVATE SERVERBUILD )
target_link_libraries( fatserver-objects PRIVATE fmt::fmt Boost::boost OpenSSL::SSL LibXml2::LibXml2 )

target_compile_definitions( fatserver-objects PRIVATE -DFATCLIENT )

if( ENABLE_LANDLOCK )
  foreach( target server-objects fatserver-objects )
    target_sources( ${target} PRIVATE landlock.cpp )
    target_compile_definitions( ${target} PRIVATE USE_LANDLOCK )
  endforeach()
endif()

set( SERVER_OBJ
  $<TARGET_OBJECTS:server-objects>
  $<TARGET_OBJECTS:clientserver-server-objects>
  $<TARGET_OBJECTS:logging-server-objects>
  $<TARGET_OBJECTS:structures-server-objects>
  $<TARGET_OBJECTS:authentication-server-objects>
  $<TARGET_OBJECTS:cache-objects>
)

add_library( server-static STATIC ${SERVER_OBJ} )
if( BUILD_SHARED_LIBS )
  add_library( server-shared SHARED ${SERVER_OBJ} )
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
target_link_libraries( server-objects PRIVATE ${LINK_XDR} )
target_link_libraries( fatserver-objects PRIVATE ${LINK_XDR} )

set( LINK_MEMCACHED )
if( TARGET LibMemcached::LibMemcached )
  set( LINK_MEMCACHED LibMemcached::LibMemcached )
endif()

target_link_libraries( server-static PRIVATE ${LINK_XDR} fmt::fmt )
if( BUILD_SHARED_LIBS )
  target_link_libraries( server-shared PRIVATE ${LINK_XDR} fmt::fmt )
endif()

add_executable( server-exe ${EXE_FLAG} serverMain.cpp )
target_include_directories( server-exe PRIVATE ${CMAKE_SOURCE_DIR}/source ${CMAKE_CURRENT_SOURCE_DIR} )
target_compile_definitions( server-exe PRIVATE SERVERBUILD )

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

target_link_libraries( server-static PRIVATE
  ${CLIENT_STATIC}
  OpenSSL::SSL OpenSSL::Crypto
  LibXml2::LibXml2
  ${LINK_MEMCACHED}
  ${LINK_DL}
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

  target_link_libraries( server-shared PRIVATE
    ${CLIENT_SHARED}
    OpenSSL::SSL OpenSSL::Crypto
    LibXml2::LibXml2
    ${LINK_MEMCACHED}
    ${LINK_DL}
  )
endif()

# Executable
set( SERVER_LINKING )
if( BUILD_SHARED_LIBS )
  set( SERVER_LINKING client-shared server-shared plugins-shared )
else()
  set( SERVER_LINKING client-static server-static plugins-static )
endif()

target_link_libraries( server-exe PRIVATE
  ${SERVER_LINKING}
  OpenSSL::SSL OpenSSL::Crypto
  LibXml2::LibXml2
  ${LINK_MEMCACHED}
  ${LINK_M}
  ${LINK_DL}
  ${LINK_XDR}
)

if( NOT WIN32 AND NOT MINGW )
  target_link_libraries( server-exe PRIVATE c )
endif()

if( NOT APPLE AND ( NOT WIN32 OR ( WIN32 AND MINGW ) ) )
  target_link_libraries( server-exe PRIVATE gcc_s )
endif()

########################################################################################################################
# Set library properties

uda_install_library( server-static ${PROJECT_NAME}_server )
if( BUILD_SHARED_LIBS )
  uda_install_library( server-shared ${PROJECT_NAME}_server RUNTIME_COMPONENT server )
endif()

########################################################################################################################
# Set executable properties

set_target_properties( server-exe PROPERTIES OUTPUT_NAME ${PROJECT_NAME}_server )

install( TARGETS server-exe
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

install(
  PROGRAMS
    ${CMAKE_BINARY_DIR}/etc/udaserver.sh
  DESTINATION ${UDA_INSTALL_SERVER_SCRIPTDIR}
  COMPONENT server
)

install(
  DIRECTORY
    ${CMAKE_BINARY_DIR}/etc/plugins.d/
  DESTINATION ${UDA_INSTALL_PLUGIN_FRAGMENTDIR}
  COMPONENT server
)

install( FILES ${HEADER_FILES}
  DESTINATION ${UDA_INSTALL_INCLUDEDIR}/server
  COMPONENT development
)
