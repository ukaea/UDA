########################################################################################################################
# Dependencies

find_package( LibXml2 REQUIRED )
find_package( OpenSSL REQUIRED )
find_package( fmt REQUIRED )

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
  if( NOT MINGW )
    find_package( dlfcn-win32 CONFIG REQUIRED )
  endif()
else()
  find_package( TIRPC QUIET )
endif()

########################################################################################################################
# Sources

set( SRC_FILES
  applyXML.cpp
  closeServerSockets.cpp
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

########################################################################################################################
# Targets

########################################################################################################################
## Libraries

add_library( server-objects OBJECT )
add_library( fat-server-objects OBJECT )

add_library( uda::server-objects ALIAS server-objects )
add_library( uda::fat-server-objects ALIAS fat-server-objects )

target_sources( server-objects PRIVATE ${SRC_FILES} )
target_sources( fat-server-objects PRIVATE ${SRC_FILES} )

target_sources(
  server-objects
  PRIVATE
  udaServer.cpp
  udaLegacyServer.cpp
  sleepServer.cpp
)
target_sources( fat-server-objects PRIVATE fatServer.cpp )

target_compile_features( server-objects PRIVATE cxx_std_11 )
target_compile_features( fat-server-objects PRIVATE cxx_std_11 )

target_include_directories(
  server-objects
  PUBLIC
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  PRIVATE
  ${PROJECT_SOURCE_DIR}/source
)

target_include_directories(
  fat-server-objects
  PUBLIC
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  PRIVATE
  ${PROJECT_SOURCE_DIR}/source
)

target_compile_definitions( fat-server-objects PRIVATE -DFATCLIENT )

target_link_libraries(
  server-objects
  PUBLIC
  fmt::fmt
  PRIVATE
  uda::server-objects
  uda::shared-objects
  uda::structures-objects
  uda::cache-objects
  uda::logging-server-objects
)

target_link_libraries(
  fat-server-objects
  PUBLIC
  fmt::fmt
)

if( WIN32 AND NOT MINGW )
  target_link_libraries( server-objects PUBLIC dlfcn-win32::dl )
else()
  target_link_libraries( server-objects PUBLIC dl )
endif()

if( ( WIN32 AND MINGW ) OR NOT WIN32 )
  target_link_libraries( server-objects PUBLIC m )
endif()

if( MINGW OR WIN32 )
  target_link_libraries( server-objects PUBLIC XDR::XDR )
  if( MINGW )
    target_link_libraries( server-objects PUBLIC ws2_32 stdc++ )
  else()
    target_link_libraries( server-objects PUBLIC ws2_32 )
  endif()
elseif( TIRPC_FOUND )
  target_link_libraries( server-objects PUBLIC TIRPC::rpc )
  target_compile_definitions( server-objects PUBLIC -D__TIRPC__ )
endif()

add_library( server-static STATIC )
add_library( uda::server-static ALIAS server-static )

target_link_libraries(
  server-static
  PRIVATE
  uda::server-objects
  uda::shared-objects
  uda::structures-objects
  uda::cache-objects
  uda::logging-server-objects
)

target_include_directories(
  server-static
  PUBLIC
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  PRIVATE
  ${PROJECT_SOURCE_DIR}/source
)

if( BUILD_SHARED_LIBS )
  add_library( server-shared SHARED )
  add_library( uda::server-shared ALIAS server-shared )

  target_link_libraries(
    server-shared
    PRIVATE
    uda::server-objects
    uda::structures-objects
    uda::cache-objects
    uda::logging-server-objects
  )

  target_include_directories(
    server-shared
    PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    PRIVATE
    ${PROJECT_SOURCE_DIR}/source
  )
endif()

########################################################################################################################
## Executable

if( WIN32 OR MINGW )
  add_executable( server-exe WIN32 serverMain.cpp )
else()
  add_executable( server-exe serverMain.cpp )
endif()

target_compile_features( server-exe PUBLIC cxx_std_17 )
target_include_directories( server-exe PUBLIC ${PROJECT_SOURCE_DIR}/source )

if( BUILD_SHARED_LIBS )
  target_link_libraries(
    server-exe
    PRIVATE
    uda::client-shared
    uda::server-shared
  )
else()
  target_link_libraries(
    server-exe
    PRIVATE
    uda::client-static
    uda::server-static
  )
endif()

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
  OUTPUT_NAME ${PROJECT_NAME}_server_static
)

install(
  TARGETS server-static
  EXPORT uda-targets
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
    EXPORT uda-targets
    DESTINATION lib
  )
endif()

########################################################################################################################
# Set executable properties

set( EXE_EXT )
if( MINGW )
  set( EXE_EXT ".exe" )
endif()

set_target_properties( server-exe
  PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}_server${EXE_EXT}
)

if( WIN32 )
  set_target_properties( server-exe
    PROPERTIES
     COMPILE_FLAGS -DNOIMPLIB
  )
endif()

install(
  TARGETS server-exe
  EXPORT uda-targets
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

# TODO: move up a level
install(
  DIRECTORY
    ${CMAKE_BINARY_DIR}/lib/pkgconfig
  DESTINATION lib
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

install(
  FILES ${HEADER_FILES}
  DESTINATION include/uda
)
