########################################################################################################################
# Dependencies

find_package( LibXml2 REQUIRED )
find_package( OpenSSL REQUIRED )
find_package( spdlog REQUIRED )

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
  if( NOT MINGW )
    find_package( dlfcn-win32 CONFIG REQUIRED )
  endif()
else()
  find_package( TIRPC QUIET)
  if(TIRPC_FOUND)
    add_definitions(-D__TIRPC__)    
  endif()
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

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${LIBXML2_INCLUDE_DIR}
  ${OPENSSL_INCLUDE_DIR}
)

add_definitions( -DSERVERBUILD )

########################################################################################################################
# Targets

add_library( server2-objects OBJECT
  ${SRC_FILES}
)

set( SERVER2_OBJ
  $<TARGET_OBJECTS:server2-objects>
  $<TARGET_OBJECTS:clientserver-server-objects>
  $<TARGET_OBJECTS:logging-server-objects>
  $<TARGET_OBJECTS:structures-server-objects>
  $<TARGET_OBJECTS:authentication-server-objects>
)

add_library( server2-static STATIC ${SERVER2_OBJ} )
if( BUILD_SHARED_LIBS )
  add_library( server2-shared SHARED ${SERVER2_OBJ} )
endif()

set( LINK_XDR )
set( EXE_FLAG )
set( EXE_EXT )
if( WIN32 OR MINGW )
  set( EXE_FLAG WIN32 )
  if( MINGW )
    set( EXE_EXT ".exe" )
  endif()
  include_directories( ${XDR_INCLUDE_DIR} )
  set( LINK_XDR ${XDR_LIBRARIES} ws2_32 )
elseif( TIRPC_FOUND )
  include_directories( ${TIRPC_INCLUDE_DIR} )
  set( LINK_XDR ${LINK_XDR} ${TIRPC_LIBRARIES} )
endif()

target_link_libraries( server2-static PRIVATE ${LINK_XDR} )
if( BUILD_SHARED_LIBS )
  target_link_libraries( server2-shared PRIVATE ${LINK_XDR} )
endif()

add_executable( server2-exe ${EXE_FLAG} server_main.cpp)

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
if( WIN32 AND NOT MINGW )
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

  target_link_libraries( server2-shared PRIVATE
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
  set( SERVER_LINKING client-shared server2-shared plugins-shared )
else()
  set( SERVER_LINKING client-static server2-static plugins-static )
endif()

target_link_libraries( server2-exe PRIVATE
  ${SERVER_LINKING}
  ${OPENSSL_LIBRARIES}
  ${LIBXML2_LIBRARIES}
  ${CACHE_LIBRARIES}
  ${LINK_M}
  ${LINK_DL}
  ${LINK_XDR}
)

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
  OUTPUT_NAME ${PROJECT_NAME}_server2
)

install(
  TARGETS server2-static
  DESTINATION lib
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
    DESTINATION lib
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
