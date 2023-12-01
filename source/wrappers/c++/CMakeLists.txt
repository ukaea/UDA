########################################################################################################################
# Dependencies

find_package( Boost REQUIRED )
find_package( OpenSSL REQUIRED )
find_package( fmt REQUIRED )
find_package( LibXml2 REQUIRED )

set( LIBS Boost::boost OpenSSL::SSL fmt::fmt LibXml2::LibXml2 )

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


add_library( cpp-objects OBJECT ${SRC_FILES} )
if( ITER_CI )
  target_compile_definitions( cpp-objects PRIVATE -D__USE_XOPEN2K8 )
endif()
target_include_directories( cpp-objects PRIVATE
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_SOURCE_DIR}/source/include
)

set( CLIENT_OBJS
  $<TARGET_OBJECTS:client-objects>
  $<TARGET_OBJECTS:c_api-objects>
  $<TARGET_OBJECTS:clientserver-client-objects>
  $<TARGET_OBJECTS:cache-objects>
  $<TARGET_OBJECTS:logging-client-objects>
  $<TARGET_OBJECTS:structures-client-objects>
  $<TARGET_OBJECTS:authentication-client-objects>
)

add_library( ${LIB_NAME}-static STATIC $<TARGET_OBJECTS:cpp-objects> ${CLIENT_OBJS} )
target_link_libraries( ${LIB_NAME}-static PRIVATE ${LIBS} )

if( BUILD_SHARED_LIBS )
  add_library( ${LIB_NAME}-shared SHARED $<TARGET_OBJECTS:cpp-objects> ${CLIENT_OBJS} )
  target_link_libraries( ${LIB_NAME}-shared PRIVATE ${LIBS} )
endif()

if( NOT CLIENT_ONLY )
  add_library( fatcpp-objects OBJECT ${SRC_FILES} )
  target_compile_definitions( fatcpp-objects PRIVATE -DFATCLIENT )
  target_include_directories( fatcpp-objects PRIVATE
    ${CMAKE_SOURCE_DIR}/source
    ${CMAKE_SOURCE_DIR}/source/include
  )

  set( FATCLIENT_OBJS
    $<TARGET_OBJECTS:fatclient-objects>
    $<TARGET_OBJECTS:fatc_api-objects>
    $<TARGET_OBJECTS:fatserver-objects>
    $<TARGET_OBJECTS:fatclientserver-objects>
    $<TARGET_OBJECTS:structures-server-objects>
    $<TARGET_OBJECTS:cache-objects>
    $<TARGET_OBJECTS:logging-server-objects>
    $<TARGET_OBJECTS:plugins-objects>
  )

  add_library( fat${LIB_NAME}-static STATIC $<TARGET_OBJECTS:fatcpp-objects> ${FATCLIENT_OBJS} )
  target_link_libraries( fat${LIB_NAME}-static PRIVATE ${LIBS} plugins-static )
  
  if( BUILD_SHARED_LIBS )
    add_library( fat${LIB_NAME}-shared SHARED $<TARGET_OBJECTS:fatcpp-objects> ${FATCLIENT_OBJS} )
    target_link_libraries( fat${LIB_NAME}-shared PRIVATE ${LIBS} plugins-shared )
  endif()
endif()

set_target_properties( ${LIB_NAME}-static
  PROPERTIES
    OUTPUT_NAME ${LIB_NAME}
)

install( TARGETS ${LIB_NAME}-static
  DESTINATION lib
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
    DESTINATION lib
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
    DESTINATION lib
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
      DESTINATION lib
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
