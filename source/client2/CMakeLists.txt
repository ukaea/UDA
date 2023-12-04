########################################################################################################################
# Dependencies

find_package( OpenSSL REQUIRED )
find_package( LibXml2 REQUIRED )
find_package( fmt REQUIRED )

set( LIBS OpenSSL::SSL LibXml2::LibXml2 fmt::fmt )

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

add_definitions( -DNOTGENERICENABLED )
add_definitions( -DNO_GSL_LIB )

if( LIBMEMCACHED_FOUND AND NOT NO_MEMCACHE )
  set( EXTRA_LD_LIBRARY_PATHS "${EXTRA_LD_LIBRARY_PATHS};${LIBMEMCACHED_LIB_DIR}" CACHE INTERNAL "" )
else()
  add_definitions( -DNOLIBMEMCACHED )
endif()

########################################################################################################################
# Sources

set( SRC_FILES
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

add_library( client2-objects OBJECT ${SRC_FILES} )
target_link_libraries( client2-objects PRIVATE ${LIBS} )
target_include_directories( client2-objects PRIVATE
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_SOURCE_DIR}/source/include
)

if( NOT CLIENT_ONLY )
  add_library( fatclient2-objects OBJECT ${SRC_FILES} )
  target_link_libraries( fatclient2-objects PRIVATE ${LIBS} )
  target_include_directories( fatclient2-objects PRIVATE
    ${CMAKE_SOURCE_DIR}/source
    ${CMAKE_SOURCE_DIR}/source/include
  )

  target_compile_definitions( fatclient2-objects PRIVATE -DFATCLIENT )
endif()

set( CLIENT_OBJS
  $<TARGET_OBJECTS:client2-objects>
  $<TARGET_OBJECTS:c_api-objects>
  $<TARGET_OBJECTS:clientserver-client-objects>
  $<TARGET_OBJECTS:cache-objects>
  $<TARGET_OBJECTS:logging-client-objects>
  $<TARGET_OBJECTS:structures-client-objects>
  $<TARGET_OBJECTS:authentication-client-objects>
)

set( FATCLIENT_OBJS
  $<TARGET_OBJECTS:fatclient2-objects>
  $<TARGET_OBJECTS:fatc_api-objects>
  $<TARGET_OBJECTS:fatserver-objects>
  $<TARGET_OBJECTS:fatclientserver-objects>
  $<TARGET_OBJECTS:structures-server-objects>
  $<TARGET_OBJECTS:cache-objects>
  $<TARGET_OBJECTS:logging-server-objects>
)

add_library( client2-static STATIC ${CLIENT_OBJS} )
if( BUILD_SHARED_LIBS )
  add_library( client2-shared SHARED ${CLIENT_OBJS} )
endif()

target_link_libraries( client2-static PRIVATE ${LIBS} )
if( BUILD_SHARED_LIBS )
  target_link_libraries( client2-shared PRIVATE ${LIBS} )
endif()

if( NOT CLIENT_ONLY )
  add_library( fatclient2-static STATIC ${FATCLIENT_OBJS} )
  target_link_libraries( fatclient2-static PRIVATE plugins-static ${LIBS} )
  
  if( BUILD_SHARED_LIBS )
    add_library( fatclient2-shared SHARED ${FATCLIENT_OBJS} )
    target_link_libraries( fatclient2-shared PRIVATE plugins-shared ${LIBS} )
  endif()
endif()

set_target_properties( client2-static
  PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}_client2
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

if( NOT CLIENT_ONLY )
  set_target_properties( fatclient2-static
    PROPERTIES
    OUTPUT_NAME fat${PROJECT_NAME}_client2
  )

  install( TARGETS fatclient2-static
    DESTINATION lib
  )
  
  if( BUILD_SHARED_LIBS )
    set_target_properties( fatclient2-shared
      PROPERTIES
        OUTPUT_NAME fat${PROJECT_NAME}_client2
        VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
    if( WIN32 )
      set_target_properties( fatclient2-shared
        PROPERTIES
          COMPILE_FLAGS -DLIBRARY_EXPORTS
          IMPORT_SUFFIX ${IMPLIB_SUFFIX}
      )
    endif()

    install( TARGETS fatclient2-shared
      DESTINATION lib
  )
  endif()
endif()
