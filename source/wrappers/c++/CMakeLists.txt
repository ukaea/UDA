########################################################################################################################
# Dependencies

find_package( Boost REQUIRED )
find_package( LibXml2 REQUIRED )
find_package( OpenSSL REQUIRED )
find_package( fmt REQUIRED )

if( WIN32 OR MINGW )
  find_package( OpenSSL REQUIRED )
  find_package( XDR REQUIRED )
  find_package( dlfcn-win32 CONFIG REQUIRED )
  if( MINGW )
    find_package( ZLIB REQUIRED )
    find_package( Iconv REQUIRED )
    find_package( liblzma CONFIG REQUIRED )
  endif()
else()
  find_package( TIRPC QUIET )
  if( TIRPC_FOUND )
    add_definitions( -D__TIRPC__ )
  endif()
endif()

set( SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../.. )

set( LIB_NAME ${PROJECT_NAME}_cpp )

set( SRC_FILES
  array.cpp
  client.cpp
  data.cpp
  dim.cpp
  result.cpp
  scalar.cpp
  string.cpp
  structdata.cpp
  treenode.cpp
  vector.cpp
  signal.cpp
)

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${Boost_INCLUDE_DIR}
  ${fmt_SOURCE_DIR}/include
)

include_directories( ${LIBXML2_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR} )

if( MINGW OR WIN32 )
  include_directories( ${XDR_INCLUDE_DIR} )
elseif( TIRPC_FOUND )
  include_directories( ${TIRPC_INCLUDE_DIR} )
endif()

if( ITER_CI )
  add_definitions( -D__USE_XOPEN2K8 )
endif()

include_directories( SYSTEM "${Boost_INCLUDE_DIRS}" )

add_library( cpp-objects OBJECT ${SRC_FILES} )

set( CLIENT_OBJS
  $<TARGET_OBJECTS:client-objects>
  $<TARGET_OBJECTS:clientserver-client-objects>
  $<TARGET_OBJECTS:cache-objects>
  $<TARGET_OBJECTS:logging-client-objects>
  $<TARGET_OBJECTS:structures-client-objects>
  $<TARGET_OBJECTS:authentication-client-objects>
)

set( LINK_LIB ${Boost_LIBRARIES} ${LIBXML2_LIBRARIES} ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY} ${CACHE_LIBRARIES} fmt::fmt )
if( WIN32 OR MINGW )
  if( MINGW )
    set( LINK_LIB ${LINK_LIB} ${XDR_LIBRARIES} ws2_32 dlfcn-win32::dl Iconv::Iconv liblzma::liblzma ZLIB::ZLIB crypt32 stdc++ )
  else()
    set( LINK_LIB ${LINK_LIB} ${XDR_LIBRARIES} ws2_32 crypt32 dlfcn-win32::dl )
  endif()
elseif( TIRPC_FOUND )
  set( LINK_LIB ${LINK_LIB} ${TIRPC_LIBRARIES} )
endif()

add_library( ${LIB_NAME}-static STATIC $<TARGET_OBJECTS:cpp-objects> ${CLIENT_OBJS} )
target_link_libraries( ${LIB_NAME}-static PRIVATE ${LINK_LIB} )

if( BUILD_SHARED_LIBS )
  add_library( ${LIB_NAME}-shared SHARED $<TARGET_OBJECTS:cpp-objects> ${CLIENT_OBJS} )
  target_link_libraries( ${LIB_NAME}-shared PRIVATE ${LINK_LIB} )
endif()

if( NOT CLIENT_ONLY )
  add_library( fatcpp-objects OBJECT ${SRC_FILES} )
  target_compile_definitions( fatcpp-objects PRIVATE -DFATCLIENT )

  set( FATCLIENT_OBJS
    $<TARGET_OBJECTS:fatclient-objects>
    $<TARGET_OBJECTS:fatserver-objects>
    $<TARGET_OBJECTS:fatclientserver-objects>
    $<TARGET_OBJECTS:structures-server-objects>
    $<TARGET_OBJECTS:cache-objects>
    $<TARGET_OBJECTS:logging-server-objects>
    $<TARGET_OBJECTS:plugins-objects>
  )

  add_library( fat${LIB_NAME}-static STATIC $<TARGET_OBJECTS:fatcpp-objects> ${FATCLIENT_OBJS} )
  target_link_libraries( fat${LIB_NAME}-static PRIVATE ${LINK_LIB} plugins-static )

  if( BUILD_SHARED_LIBS )
    add_library( fat${LIB_NAME}-shared SHARED $<TARGET_OBJECTS:fatcpp-objects> ${FATCLIENT_OBJS} )
    target_link_libraries( fat${LIB_NAME}-shared PRIVATE ${LINK_LIB} plugins-shared )
  endif()
endif()

set_target_properties( ${LIB_NAME}-static
  PROPERTIES
    OUTPUT_NAME ${LIB_NAME}_static
)

install( TARGETS ${LIB_NAME}-static
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if( BUILD_SHARED_LIBS )
  set_target_properties( ${LIB_NAME}-shared
    PROPERTIES
      OUTPUT_NAME ${LIB_NAME}
      VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
      SOVERSION ${PROJECT_VERSION_MAJOR}
  )
  if( WIN32 )
    set_target_properties( ${LIB_NAME}-shared
      PROPERTIES
        COMPILE_FLAGS -DLIBRARY_EXPORTS
        IMPORT_SUFFIX ${IMPLIB_SUFFIX}
    )
  endif()

  install( TARGETS ${LIB_NAME}-shared
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
endif()

# pkg-config file
configure_file(
  ${CMAKE_SOURCE_DIR}/source/etc/uda-cpp.pc.in
  ${CMAKE_BINARY_DIR}/etc/uda/uda-cpp.pc
  @ONLY
)

install( FILES ${CMAKE_BINARY_DIR}/etc/uda/uda-cpp.pc
  DESTINATION modulefiles/uda
)


if( NOT CLIENT_ONLY )
  set_target_properties( fat${LIB_NAME}-static
    PROPERTIES
      OUTPUT_NAME fat${LIB_NAME}
  )

  install( TARGETS fat${LIB_NAME}-static
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

  if( BUILD_SHARED_LIBS )
    set_target_properties( fat${LIB_NAME}-shared
      PROPERTIES
        OUTPUT_NAME fat${LIB_NAME}
        VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
    if( WIN32 )
      set_target_properties( fat${LIB_NAME}-shared
        PROPERTIES
          COMPILE_FLAGS -DLIBRARY_EXPORTS
          IMPORT_SUFFIX ${IMPLIB_SUFFIX}
      )
    endif()

    install( TARGETS fat${LIB_NAME}-shared
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
  endif()

  # pkg-config file
  configure_file(
    ${CMAKE_SOURCE_DIR}/source/etc/uda-fat-cpp.pc.in
    ${CMAKE_BINARY_DIR}/etc/uda/uda-fat-cpp.pc
    @ONLY
  )

  install( FILES ${CMAKE_BINARY_DIR}/etc/uda/uda-fat-cpp.pc
    DESTINATION modulefiles/uda
  )
endif()

set( INSTALL_FILES
  UDA.hpp
  array.hpp
  client.hpp
  data.hpp
  dim.hpp
  result.hpp
  scalar.hpp
  string.hpp
  structdata.hpp
  treenode.hpp
  vector.hpp
)

install( FILES ${INSTALL_FILES}
  DESTINATION include/uda/c++
)
