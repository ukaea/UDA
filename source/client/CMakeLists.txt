########################################################################################################################
# Dependencies

find_package( OpenSSL REQUIRED )
find_package( fmt REQUIRED )
find_package( LibXml2 REQUIRED )

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

set(
  SRC_FILES
  udaGetAPI.cpp
  clientAPI.cpp
  clientMDS.cpp
  accAPI.cpp
  startup.cpp
  closedown.cpp
  connection.cpp
  clientXDRStream.cpp
  updateSelectParms.cpp
  getEnvironment.cpp
  generateErrors.cpp
  makeClientRequestBlock.cpp
  udaPutAPI.cpp
  udaClient.cpp
  udaClientHostList.cpp
)

########################################################################################################################
# Targets

add_library( client-objects OBJECT ${SRC_FILES} )
add_library( uda::client-objects ALIAS client-objects )

target_link_libraries(
  client-objects
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
  client-objects
  PUBLIC
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  PRIVATE
  ${PROJECT_SOURCE_DIR}/source
  ${CMAKE_BINARY_DIR}/include
)

target_compile_definitions( client-objects PRIVATE -DNO_GSL_LIB )

if( MINGW OR WIN32 )
  target_link_libraries( client-objects PUBLIC ws2_32 XDR::XDR )
  if( MINGW )
    target_link_libraries( client-objects PUBLIC dl stdc++ iconv lzma z )
  else()
    target_link_libraries( client-objects PUBLIC dlfcn-win32::dl )
  endif()
elseif( TIRPC_FOUND )
  target_link_libraries( client-objects PUBLIC TIRPC::rpc )
endif()

add_library( client-static STATIC )
add_library( uda::client-static ALIAS client-static )

target_link_libraries(
  client-static
  PRIVATE
  uda::client-objects
  uda::shared-objects
  uda::cache-objects
  uda::logging-client-objects
  uda::structures-objects
  uda::authentication-client-objects
)
if( ENABLE_CAPNP )
  target_link_libraries( client-static PRIVATE uda::serialisation-objects )
endif()

target_include_directories(
  client-static
  PUBLIC
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  PRIVATE
  ${PROJECT_SOURCE_DIR}/source
  ${CMAKE_BINARY_DIR}/include
)

if( BUILD_SHARED_LIBS )
  add_library( client-shared SHARED )
  add_library( uda::client-shared ALIAS client-shared )

  target_link_libraries(
    client-shared
    PRIVATE
    uda::client-objects
    uda::cache-objects
    uda::logging-client-objects
    uda::structures-objects
    uda::authentication-client-objects
  )
  if( ENABLE_CAPNP )
    target_link_libraries( client-shared PRIVATE uda::serialisation-objects )
  endif()

  target_include_directories(
    client-shared
    PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    PRIVATE
    ${PROJECT_SOURCE_DIR}/source
    ${CMAKE_BINARY_DIR}/include
  )
endif()

if( NOT CLIENT_ONLY )
  add_library( fat-client-objects OBJECT ${SRC_FILES} )
  target_compile_definitions( fat-client-objects PRIVATE -DFATCLIENT )

  add_library( uda::fat-client-objects ALIAS fat-client-objects )

  target_link_libraries(
    fat-client-objects
    PRIVATE
    uda::fat-server-objects
    uda::fat-shared-objects
    uda::structures-objects
    uda::cache-objects
    uda::logging-server-objects
  )
  if( MINGW )
    target_link_libraries( fat-client-objects PRIVATE iconv lzma z )
  endif()

  target_include_directories(
    fat-client-objects
    PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    PRIVATE
    ${PROJECT_SOURCE_DIR}/source
    ${CMAKE_BINARY_DIR}/include
  )

  target_compile_definitions( fat-client-objects PRIVATE -DNO_GSL_LIB )

  add_library( fat-client-static STATIC )
  target_link_libraries(
    fat-client-static
    PRIVATE
    uda::fat-client-objects
    uda::fat-server-objects
    uda::structures-objects
    uda::logging-server-objects
    uda::cache-objects
    PUBLIC
    LibXml2::LibXml2
  )

  if( BUILD_SHARED_LIBS )
    add_library( fat-client-shared SHARED )
    target_link_libraries(
      fat-client-shared
      PRIVATE
      uda::fat-client-objects
      uda::fat-server-objects
      uda::structures-objects
      uda::logging-server-objects
      uda::cache-objects
      PUBLIC
      LibXml2::LibXml2
    )
  endif()
endif()

set_target_properties(
  client-static
  PROPERTIES
  OUTPUT_NAME ${PROJECT_NAME}_client_static
)

include( CMakePackageConfigHelpers )
write_basic_package_version_file(
  UDAClientConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
install(
  FILES UDAClientConfig.cmake UDAClientConfigVersion.cmake
  DESTINATION lib/cmake/uda
)

if( BUILD_SHARED_LIBS )
  set_target_properties(
    client-shared
    PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}_client
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    SOVERSION ${PROJECT_VERSION_MAJOR}
  )
  if( WIN32 )
    set_target_properties(
      client-shared
      PROPERTIES
      COMPILE_FLAGS -DLIBRARY_EXPORTS
      IMPORT_SUFFIX ${IMPLIB_SUFFIX}
    )
  endif()

  install(
    TARGETS client-shared
    DESTINATION lib
  )
endif()

install(
  FILES ${HEADER_FILES}
  DESTINATION include/uda/client
)

if( NOT CLIENT_ONLY )
  set_target_properties(
    fat-client-static
    PROPERTIES
    OUTPUT_NAME fat${PROJECT_NAME}_client_static
  )

  install(
    TARGETS fat-client-static
    DESTINATION lib
  )

  if( BUILD_SHARED_LIBS )
    set_target_properties(
      fat-client-shared
      PROPERTIES
      OUTPUT_NAME fat${PROJECT_NAME}_client
      VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
      SOVERSION ${PROJECT_VERSION_MAJOR}
    )
    if( WIN32 )
      set_target_properties(
        fat-client-shared
        PROPERTIES
        COMPILE_FLAGS -DLIBRARY_EXPORTS
        IMPORT_SUFFIX ${IMPLIB_SUFFIX}
      )
    endif()

    install(
      TARGETS fat-client-shared
      DESTINATION lib
    )
  endif()
endif()
