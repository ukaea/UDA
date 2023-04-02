########################################################################################################################
# Dependencies

find_package( OpenSSL REQUIRED )
find_package( LibXml2 REQUIRED )
find_package( fmt REQUIRED )

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
  if( NOT MINGW )
    find_package( dlfcn-win32 CONFIG REQUIRED )
  endif()
else()
  find_package( TIRPC QUIET)
endif()

if( LIBMEMCACHED_FOUND AND NOT NO_MEMCACHE )
  set( EXTRA_LD_LIBRARY_PATHS "${EXTRA_LD_LIBRARY_PATHS};${LIBMEMCACHED_LIB_DIR}" CACHE INTERNAL "" )
else()
  add_definitions( -DNOLIBMEMCACHED )
endif()

add_definitions( -DNO_GSL_LIB )

########################################################################################################################
# Sources

set( SRC_FILES
  udaGetAPI.cpp
  udaPutAPI.cpp
  accAPI.cpp
  client.cpp
  thread_client.cpp
  client_environment.cpp
  make_request_block.cpp
  client_xdr_stream.cpp
  connection.cpp
  closedown.cpp
  host_list.cpp
  generate_errors.cpp
  handle.cpp
  exceptions.cpp
  updateSelectParms.cpp
)

########################################################################################################################
# Targets

add_library( client2-objects OBJECT ${SRC_FILES} )
add_library( uda::client2-objects ALIAS client2-objects )

target_link_libraries(
  client2-objects
  PRIVATE
  uda::shared-objects
  uda::cache-objects
  uda::logging-client-objects
  uda::structures-objects
  uda::authentication-client-objects
  PUBLIC
  OpenSSL::SSL
  OpenSSL::Crypto
  LibXml2::LibXml2
  fmt::fmt
)

target_include_directories(
  client2-objects
  PUBLIC
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  PRIVATE
  ${PROJECT_SOURCE_DIR}/source
  ${CMAKE_BINARY_DIR}/include
)

if( MINGW OR WIN32 )
  target_link_libraries( client2-objects PUBLIC ws2_32 XDR::XDR )
  if ( MINGW )
    target_link_libraries( client2-objects PUBLIC dl stdc++ iconv lzma z )
  else()
    target_link_libraries( client2-objects PUBLIC dlfcn-win32::dl )
  endif()
elseif( TIRPC_FOUND )
  target_link_libraries( client2-objects PUBLIC TIRPC::rpc )
endif()

add_library( client2-static STATIC )
add_library( uda::client2-static ALIAS client2-static )

target_link_libraries(
  client2-static
  PRIVATE
  uda::client2-objects
  uda::shared-objects
  uda::cache-objects
  uda::logging-client-objects
  uda::structures-objects
  uda::authentication-client-objects
)
if( ENABLE_CAPNP )
  target_link_libraries( client2-static PRIVATE uda::serialisation-objects )
endif()

target_include_directories(
  client2-static
  PUBLIC
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  PRIVATE
  ${PROJECT_SOURCE_DIR}/source
  ${CMAKE_BINARY_DIR}/include
)

if( BUILD_SHARED_LIBS )
  add_library( client2-shared SHARED )
  add_library( uda::client2-shared ALIAS client2-shared )

  target_link_libraries(
    client2-shared
    PRIVATE
    uda::client2-objects
    uda::cache-objects
    uda::logging-client-objects
    uda::structures-objects
    uda::authentication-client-objects
  )
  if( ENABLE_CAPNP )
    target_link_libraries( client2-shared PRIVATE uda::serialisation-objects )
  endif()

  target_include_directories(
    client2-shared
    PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    PRIVATE
    ${PROJECT_SOURCE_DIR}/source
    ${CMAKE_BINARY_DIR}/include
  )
endif()

if( NOT CLIENT_ONLY )
  add_library( fat-client2-objects OBJECT ${SRC_FILES} )
  add_library( uda::fat-client2-objects ALIAS fat-client2-objects )

  target_compile_definitions( fat-client2-objects PRIVATE -DFATCLIENT )

  target_link_libraries( fat-client2-objects
    PRIVATE
    uda::fat-client2-objects
    uda::fat-server-objects
    uda::fat-shared-objects
    uda::structures-objects
    uda::cache-objects
    uda::logging-server-objects
    PUBLIC
    OpenSSL::SSL
    OpenSSL::Crypto
    fmt::fmt
    LibXml2::LibXml2
  )
  if( MINGW )
    target_link_libraries( fat-client2-objects PRIVATE iconv lzma z )
  endif()

  target_include_directories(
    fat-client2-objects
    PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    PRIVATE
    ${PROJECT_SOURCE_DIR}/source
    ${CMAKE_BINARY_DIR}/include
  )

  add_library( fat-client2-static STATIC )

  target_link_libraries(
    fat-client2-static
    PRIVATE
    uda::fat-client2-objects
    uda::cache-objects
    uda::logging-client-objects
    uda::structures-objects
    uda::authentication-client-objects
  )
  if( ENABLE_CAPNP )
    target_link_libraries( fat-client2-static PRIVATE uda::serialisation-objects )
  endif()

  if( BUILD_SHARED_LIBS )
    add_library( fat-client2-shared SHARED )

    target_link_libraries(
      fat-client2-shared
      PRIVATE
      uda::fat-client2-objects
      uda::cache-objects
      uda::logging-client-objects
      uda::structures-objects
      uda::authentication-client-objects
    )
    if( ENABLE_CAPNP )
      target_link_libraries( fat-client2-shared PRIVATE uda::serialisation-objects )
    endif()
  endif()
endif()

set_target_properties( client2-static
  PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}_client2_static
)

install( TARGETS client2-static
  DESTINATION lib
)

if( BUILD_SHARED_LIBS )
  set_target_properties( client2-shared
    PROPERTIES
      OUTPUT_NAME ${PROJECT_NAME}_client2
      VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
      SOVERSION ${PROJECT_VERSION_MAJOR}
  )
  if( WIN32 )
    set_target_properties( client2-shared
      PROPERTIES
        COMPILE_FLAGS -DLIBRARY_EXPORTS
        IMPORT_SUFFIX ${IMPLIB_SUFFIX}
    )
  endif()

  install( TARGETS client2-shared
    DESTINATION lib
  )
endif()

install( FILES ${HEADER_FILES}
  DESTINATION include/uda/client2
)

if( NOT CLIENT_ONLY )
  set_target_properties( fat-client2-static
    PROPERTIES
    OUTPUT_NAME fat${PROJECT_NAME}_client2_static
  )

  install( TARGETS fat-client2-static
    DESTINATION lib
  )
  
  if( BUILD_SHARED_LIBS )
    set_target_properties( fat-client2-shared
      PROPERTIES
        OUTPUT_NAME fat${PROJECT_NAME}_client2
        VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
    if( WIN32 )
      set_target_properties( fat-client2-shared
        PROPERTIES
          COMPILE_FLAGS -DLIBRARY_EXPORTS
          IMPORT_SUFFIX ${IMPLIB_SUFFIX}
      )
    endif()

    install( TARGETS fat-client2-shared
      DESTINATION lib
    )
  endif()
endif()
