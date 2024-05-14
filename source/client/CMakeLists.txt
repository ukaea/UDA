########################################################################################################################
# Dependencies

find_package( OpenSSL REQUIRED )
find_package( fmt REQUIRED )
find_package( LibXml2 REQUIRED )

set( LIBS OpenSSL::SSL OpenSSL::Crypto fmt::fmt LibXml2::LibXml2 common-objects )

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
  find_package( dlfcn-win32 CONFIG REQUIRED )
  list( APPEND LIBS XDR::XDR dlfcn-win32::dl ws2_32 )
  if( MINGW )
    find_package( ZLIB REQUIRED )
    find_package( Iconv REQUIRED )
    find_package( liblzma CONFIG REQUIRED )
    list( APPEND LIBS Iconv::Iconv liblzma::liblzma ZLIB::ZLIB crypt32 stdc++ pthread )
  endif()
else()
  find_package( TIRPC QUIET )
  if( TIRPC_FOUND )
    list( APPEND LIBS TIRPC::TIRPC )
  endif()
endif()

find_package( GSL )
if( GSL_FOUND )
  list( APPEND LIBS GSL::gsl )
else()
  add_definitions( -DNO_GSL_LIB )
endif()

if( LIBMEMCACHED_FOUND AND NOT NO_MEMCACHE )
  set( EXTRA_LD_LIBRARY_PATHS "${EXTRA_LD_LIBRARY_PATHS};${LIBMEMCACHED_LIB_DIR}" CACHE INTERNAL "" )
else()
  add_definitions( -DNOLIBMEMCACHED )
endif()

########################################################################################################################
# Sources

set( SRC_FILES
  startup.cpp
  closedown.cpp
  connection.cpp
  clientXDRStream.cpp
  generateErrors.cpp
  makeClientRequestBlock.cpp
  udaClient.cpp
  udaClientHostList.cpp
  client_config.cpp
  client_logging.cpp
)

add_library( client-objects OBJECT ${SRC_FILES} )
target_link_libraries( client-objects PUBLIC ${LIBS} PRIVATE common-objects )
target_include_directories( client-objects PRIVATE
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_SOURCE_DIR}/source/include
  ${CMAKE_BINARY_DIR}/include
)
target_compile_definitions(
  client-objects PRIVATE
)

if( NOT CLIENT_ONLY )
  add_library( fatclient-objects OBJECT ${SRC_FILES} )
  target_link_libraries( fatclient-objects PUBLIC ${LIBS} PRIVATE common-objects )
  target_include_directories( fatclient-objects PRIVATE
    ${CMAKE_SOURCE_DIR}/source
    ${CMAKE_SOURCE_DIR}/source/include
    ${CMAKE_BINARY_DIR}/include
  )
  target_compile_definitions(
    fatclient-objects PRIVATE
  )

  target_compile_definitions( fatclient-objects PUBLIC -DFATCLIENT )
endif()

set( CLIENT_OBJS
  $<TARGET_OBJECTS:client-objects>
  $<TARGET_OBJECTS:c_api-objects>
  $<TARGET_OBJECTS:clientserver-client-objects>
  $<TARGET_OBJECTS:cache-objects>
  $<TARGET_OBJECTS:logging-client-objects>
  $<TARGET_OBJECTS:structures-client-objects>
  $<TARGET_OBJECTS:authentication-client-objects>
  $<TARGET_OBJECTS:config-objects>
)

set( FATCLIENT_OBJS
  $<TARGET_OBJECTS:fatclient-objects>
  $<TARGET_OBJECTS:fatc_api-objects>
  $<TARGET_OBJECTS:fatserver-objects>
  $<TARGET_OBJECTS:fatclientserver-objects>
  $<TARGET_OBJECTS:structures-server-objects>
  $<TARGET_OBJECTS:cache-objects>
  $<TARGET_OBJECTS:logging-server-objects>
  $<TARGET_OBJECTS:authentication-client-objects>
  $<TARGET_OBJECTS:config-objects>
)

add_library( client-static STATIC ${CLIENT_OBJS} )
if( BUILD_SHARED_LIBS )
  add_library( client-shared SHARED ${CLIENT_OBJS} )
endif()

target_link_libraries( client-static PUBLIC ${LIBS} )
if( BUILD_SHARED_LIBS )
  target_link_libraries( client-shared PUBLIC ${LIBS} )
endif()

if( NOT CLIENT_ONLY )
  add_library( fatclient-static STATIC ${FATCLIENT_OBJS} )
  target_link_libraries( fatclient-static PUBLIC plugins-static ${LIBS} )
  
  if( BUILD_SHARED_LIBS )
    add_library( fatclient-shared SHARED ${FATCLIENT_OBJS} )
    target_link_libraries( fatclient-shared PUBLIC plugins-static ${LIBS} )
  endif()
endif()

set_target_properties( client-static
  PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}_client
)

install( TARGETS client-static
  DESTINATION lib
)

if( BUILD_SHARED_LIBS )
  set_target_properties( client-shared
    PROPERTIES
      OUTPUT_NAME ${PROJECT_NAME}_client
      VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
      SOVERSION ${PROJECT_VERSION_MAJOR}
  )
  if( WIN32 )
    set_target_properties( client-shared
      PROPERTIES
        COMPILE_FLAGS -DLIBRARY_EXPORTS
        IMPORT_SUFFIX ${IMPLIB_SUFFIX}
    )
  endif()

  install( TARGETS client-shared
    DESTINATION lib
  )
endif()

if( NOT CLIENT_ONLY )
  set_target_properties( fatclient-static
    PROPERTIES
    OUTPUT_NAME fat${PROJECT_NAME}_client
  )

  install( TARGETS fatclient-static
    DESTINATION lib
  )
  
  if( BUILD_SHARED_LIBS )
    set_target_properties( fatclient-shared
      PROPERTIES
        OUTPUT_NAME fat${PROJECT_NAME}_client
        VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
    if( WIN32 )
      set_target_properties( fatclient-shared
        PROPERTIES
          COMPILE_FLAGS -DLIBRARY_EXPORTS
          IMPORT_SUFFIX ${IMPLIB_SUFFIX}
      )
    endif()

    install( TARGETS fatclient-shared
      DESTINATION lib
  )
  endif()
endif()
