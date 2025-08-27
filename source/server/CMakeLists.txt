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
  if( TIRPC_FOUND )
    add_definitions( -D__TIRPC__ )
  endif()
endif()

set ( STDFS_VAR "" )
if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 9.1 )
    set ( STDFS_VAR "stdc++fs" )
elseif ( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 9.0 )
    set ( STDFS_VAR "c++fs" )
endif()

########################################################################################################################
# Sources

set( SRC_FILES
  applyXML.cpp
  closeServerSockets.cpp
  filesystemUtils.cpp
  getPluginAddress.cpp
  getServerEnvironment.cpp
  initPluginList.cpp
  makeServerRequestBlock.cpp
  serverGetData.cpp
  serverLegacyPlugin.cpp
  serverPlugin.cpp
  serverProcessing.cpp
  serverStartup.cpp
  serverSubsetData.cpp
  writer.cpp
  createXDRStream.cpp
)

set( HEADER_FILES
  applyXML.h
  closeServerSockets.h
  createXDRStream.h
  filesystemUtils.h
  getPluginAddress.h
  getServerEnvironment.h
  initPluginList.h
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

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${LIBXML2_INCLUDE_DIR}
  ${OPENSSL_INCLUDE_DIR}
  ${fmt_SOURCE_DIR}/include
  ${Boost_INCLUDE_DIRS}
)

add_definitions( -DSERVERBUILD )

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
target_link_libraries( server-objects PRIVATE fmt::fmt )
target_link_libraries( server-objects PUBLIC ${STDFS_VAR} )

add_library( fatserver-objects OBJECT
  fatServer.cpp
  ${SRC_FILES}
  ${HEADER_FILES}
)
target_link_libraries( fatserver-objects PRIVATE fmt::fmt )
target_link_libraries( fatserver-objects PUBLIC ${STDFS_VAR} )

target_compile_definitions( fatserver-objects PRIVATE -DFATCLIENT )
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
  include_directories( ${XDR_INCLUDE_DIR} )
  set( LINK_XDR ${XDR_LIBRARIES} ws2_32 )
elseif(TIRPC_FOUND )
  include_directories( ${TIRPC_INCLUDE_DIR} )
  set( LINK_XDR ${LINK_XDR} ${TIRPC_LIBRARIES} )
endif()

target_link_libraries( server-static PRIVATE ${LINK_XDR} fmt::fmt )
target_link_libraries( server-static PUBLIC ${STDFS_VAR} )
if( BUILD_SHARED_LIBS )
  target_link_libraries( server-shared PRIVATE ${LINK_XDR} fmt::fmt )
  target_link_libraries( server-shared PUBLIC ${STDFS_VAR} )
endif()

add_executable( server-exe ${EXE_FLAG} serverMain.cpp )

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
  ${OPENSSL_LIBRARIES}
  ${LIBXML2_LIBRARIES}
  ${CACHE_LIBRARIES}
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
    ${OPENSSL_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${CACHE_LIBRARIES}
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
  ${OPENSSL_LIBRARIES}
  ${LIBXML2_LIBRARIES}
  ${CACHE_LIBRARIES}
  ${LINK_M}
  ${LINK_DL}
  ${LINK_XDR}
)

if( NOT WIN32 AND NOT MINGW )
  target_link_libraries( server-exe LINK_PRIVATE c )
endif()

if( NOT APPLE AND ( NOT WIN32 OR ( WIN32 AND MINGW ) ) )
  target_link_libraries( server-exe PRIVATE gcc_s )
endif()

########################################################################################################################
# Set library properties

set_target_properties( server-static
  PROPERTIES
  OUTPUT_NAME ${PROJECT_NAME}_server
)

install(
  TARGETS server-static
  DESTINATION lib
)

if( BUILD_SHARED_LIBS )
  set_target_properties( server-shared
      PROPERTIES
        OUTPUT_NAME ${PROJECT_NAME}_server
        VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
        SOVERSION ${PROJECT_VERSION_MAJOR}
  )
  if( WIN32 )
    set_target_properties( server-shared
      PROPERTIES
        COMPILE_FLAGS -DLIBRARY_EXPORTS
        IMPORT_SUFFIX ${IMPLIB_SUFFIX}
    )
  endif()

  install(
    TARGETS server-shared
    DESTINATION lib
  )
endif()

########################################################################################################################
# Set executable properties

set_target_properties( server-exe
  PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}_server
)

if( WIN32 )
  set_target_properties( server-exe
    PROPERTIES
     COMPILE_FLAGS -DNOIMPLIB
  )
endif()

install( TARGETS server-exe
  DESTINATION bin
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
  FILES
    ${GEN_ETC_FILES}
  DESTINATION etc
)

install(
  DIRECTORY
    ${CMAKE_SOURCE_DIR}/source/etc/machine.d
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
  DIRECTORY
    ${CMAKE_BINARY_DIR}/etc/plugins.d
  DESTINATION etc
)

install(
  FILES
    ${CMAKE_BINARY_DIR}/etc/udagenstruct.conf
  DESTINATION etc
)

install( FILES ${HEADER_FILES}
  DESTINATION include/uda/server
)
