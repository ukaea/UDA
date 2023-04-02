########################################################################################################################
# Dependencies

find_package( LibXml2 REQUIRED )
find_package( OpenSSL REQUIRED )
find_package( spdlog REQUIRED )
find_package( fmt REQUIRED )
find_package( Boost REQUIRED )

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
  if( NOT MINGW )
    find_package( dlfcn-win32 CONFIG REQUIRED )
  endif()
else()
  find_package( TIRPC QUIET)
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

########################################################################################################################
## Libraries

add_library( server2-objects OBJECT )
add_library( uda::server2-objects ALIAS server2-objects )

target_sources( server2-objects PRIVATE ${SRC_FILES} )

target_compile_features( server2-objects PUBLIC cxx_std_17 )

target_link_libraries(
  server2-objects
  PRIVATE
  uda::shared-objects
  uda::logging-server-objects
  uda::structures-objects
  uda::authentication-server-objects
  uda::cache-objects
)

target_include_directories(
  server2-objects
  PUBLIC
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  PRIVATE
  ${PROJECT_SOURCE_DIR}/source
)

target_link_libraries( server2-objects PUBLIC fmt::fmt Boost::boost )

if( WIN32 AND NOT MINGW )
  target_link_libraries( server2-objects PUBLIC dlfcn-win32::dl )
else()
  target_link_libraries( server2-objects PUBLIC dl )
endif()

if( ( WIN32 AND MINGW ) OR NOT WIN32 )
  target_link_libraries( server2-objects PUBLIC m )
endif()

if( MINGW OR WIN32 )
  target_link_libraries( server2-objects PUBLIC XDR::XDR )
  if( MINGW )
    target_link_libraries( server2-objects PUBLIC ws2_32 stdc++ )
  else()
    target_link_libraries( server2-objects PUBLIC ws2_32 )
  endif()
elseif( TIRPC_FOUND )
  target_link_libraries( server2-objects PUBLIC TIRPC::rpc )
  target_compile_definitions( server2-objects PUBLIC -D__TIRPC__ )
endif()

add_library( server2-static STATIC )
add_library( uda::server2-static ALIAS server2-static )

target_link_libraries(
  server2-static
  PRIVATE
  uda::server2-objects
  uda::shared-objects
  uda::structures-objects
  uda::cache-objects
  uda::logging-server-objects
)

target_include_directories(
  server2-static
  PUBLIC
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  PRIVATE
  ${PROJECT_SOURCE_DIR}/source
)

if( BUILD_SHARED_LIBS )
  add_library( server2-shared SHARED )
  add_library( uda::server2-shared ALIAS server2-shared )

  target_link_libraries(
    server2-shared
    PRIVATE
    uda::server2-objects
    uda::structures-objects
    uda::cache-objects
    uda::logging-server-objects
  )

  target_include_directories(
    server2-shared
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
  add_executable( server2-exe WIN32 server_main.cpp )
else()
  add_executable( server2-exe server_main.cpp )
endif()

target_compile_features( server2-exe PUBLIC cxx_std_17 )
target_include_directories( server2-exe PUBLIC ${PROJECT_SOURCE_DIR}/source )

if( BUILD_SHARED_LIBS )
  target_link_libraries(
    server2-exe
    PRIVATE
    uda::client-shared
    uda::server2-shared
    Boost::boost
  )
else()
  target_link_libraries(
    server2-exe
    PRIVATE
    uda::client-static
    uda::server2-static
    Boost::boost
  )
endif()

if( NOT WIN32 AND NOT MINGW )
  target_link_libraries( server2-exe LINK_PRIVATE c )
endif()

if( NOT APPLE AND ( NOT WIN32 OR ( WIN32 AND MINGW ) ) )
  target_link_libraries( server2-exe PRIVATE gcc_s )
endif()

########################################################################################################################
# Set library properties

set_target_properties( server2-static
  PROPERTIES
  OUTPUT_NAME ${PROJECT_NAME}_server_static
  )

install(
  TARGETS server2-static
  EXPORT uda-targets
  DESTINATION lib
)

if( BUILD_SHARED_LIBS )
  set_target_properties( server2-shared
      PROPERTIES
        OUTPUT_NAME ${PROJECT_NAME}_server
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

set_target_properties(
  server2-exe
  PROPERTIES
  OUTPUT_NAME ${PROJECT_NAME}_server${EXE_EXT}
)

if( WIN32 )
  set_target_properties(
    server2-exe
    PROPERTIES
    COMPILE_FLAGS -DNOIMPLIB
  )
endif()

install(
  TARGETS server2-exe
  EXPORT uda-targets
  DESTINATION bin
)