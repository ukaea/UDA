if( CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR )
  cmake_minimum_required( VERSION 3.12 )
  project( uda_cpp LANGUAGES CXX )
  include( "${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake/UDAInstallDirs.cmake" )
endif()

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

set( INSTALL_FILES
  UDA.hpp
  array.hpp
  client.hpp
  data.hpp
  dim.hpp
  export.hpp
  result.hpp
  scalar.hpp
  signal.hpp
  string.hpp
  structdata.hpp
  treenode.hpp
  vector.hpp
)

install( FILES ${INSTALL_FILES}
  DESTINATION ${UDA_INSTALL_INCLUDEDIR}/c++
  COMPONENT development
)

if( CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR )
  include( "${CMAKE_CURRENT_SOURCE_DIR}/Standalone.cmake" )
  return()
endif()

########################################################################################################################
# Dependencies

find_package( Boost REQUIRED )
find_package( LibXml2 REQUIRED )
find_package( OpenSSL REQUIRED )
find_package( fmt REQUIRED )

if( WIN32 OR MINGW )
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

if( ITER_CI )
  add_definitions( -D__USE_XOPEN2K8 )
endif()

# Dependencies are imported targets, so their include paths and compile definitions reach the wrapper
# objects without directory-wide include_directories() calls.
set( CPP_LINK_LIBS
  Boost::headers
  LibXml2::LibXml2
  OpenSSL::SSL
  OpenSSL::Crypto
  fmt::fmt
)
if( LIBMEMCACHED_FOUND AND NOT NO_MEMCACHE )
  list( APPEND CPP_LINK_LIBS LibMemcached::LibMemcached )
endif()

if( WIN32 OR MINGW )
  list( APPEND CPP_LINK_LIBS XDR::XDR ws2_32 dlfcn-win32::dl crypt32 )
  if( MINGW )
    list( APPEND CPP_LINK_LIBS Iconv::Iconv liblzma::liblzma ZLIB::ZLIB stdc++ )
  endif()
elseif( TIRPC_FOUND )
  list( APPEND CPP_LINK_LIBS TIRPC::TIRPC )
endif()

add_library( cpp-objects OBJECT ${SRC_FILES} )
target_include_directories( cpp-objects PRIVATE ${CMAKE_SOURCE_DIR}/source )
target_link_libraries( cpp-objects PRIVATE ${CPP_LINK_LIBS} )

set( CLIENT_OBJS
  $<TARGET_OBJECTS:client-objects>
  $<TARGET_OBJECTS:clientserver-client-objects>
  $<TARGET_OBJECTS:cache-objects>
  $<TARGET_OBJECTS:logging-client-objects>
  $<TARGET_OBJECTS:structures-client-objects>
  $<TARGET_OBJECTS:authentication-client-objects>
)

add_library( ${LIB_NAME}-static STATIC $<TARGET_OBJECTS:cpp-objects> ${CLIENT_OBJS} )
target_link_libraries( ${LIB_NAME}-static PRIVATE ${CPP_LINK_LIBS} )

if( BUILD_SHARED_LIBS )
  add_library( ${LIB_NAME}-shared SHARED $<TARGET_OBJECTS:cpp-objects> ${CLIENT_OBJS} )
  target_link_libraries( ${LIB_NAME}-shared PRIVATE ${CPP_LINK_LIBS} )
endif()

if( NOT CLIENT_ONLY )
  add_library( fatcpp-objects OBJECT ${SRC_FILES} )
  target_compile_definitions( fatcpp-objects PRIVATE -DFATCLIENT )
  target_include_directories( fatcpp-objects PRIVATE ${CMAKE_SOURCE_DIR}/source )
  target_link_libraries( fatcpp-objects PRIVATE ${CPP_LINK_LIBS} )

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
  target_link_libraries( fat${LIB_NAME}-static PRIVATE ${CPP_LINK_LIBS} plugins-static )
  
  if( BUILD_SHARED_LIBS )
    add_library( fat${LIB_NAME}-shared SHARED $<TARGET_OBJECTS:fatcpp-objects> ${FATCLIENT_OBJS} )
    target_link_libraries( fat${LIB_NAME}-shared PRIVATE ${CPP_LINK_LIBS} plugins-shared )
  endif()
endif()

set_target_properties( ${LIB_NAME}-static
  PROPERTIES
    OUTPUT_NAME ${LIB_NAME}${UDA_STATIC_LIB_SUFFIX}
)

install( TARGETS ${LIB_NAME}-static
  RUNTIME DESTINATION ${UDA_INSTALL_BINDIR}
  LIBRARY DESTINATION ${UDA_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${UDA_INSTALL_LIBDIR}
  COMPONENT development
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
    RUNTIME DESTINATION ${UDA_INSTALL_BINDIR}
    LIBRARY DESTINATION ${UDA_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${UDA_INSTALL_LIBDIR}
    COMPONENT client-runtime
  )
endif()

if( NOT CLIENT_ONLY )
  set_target_properties( fat${LIB_NAME}-static
    PROPERTIES
      OUTPUT_NAME fat${LIB_NAME}${UDA_STATIC_LIB_SUFFIX}
  )
  
  install( TARGETS fat${LIB_NAME}-static
    RUNTIME DESTINATION ${UDA_INSTALL_BINDIR}
    LIBRARY DESTINATION ${UDA_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${UDA_INSTALL_LIBDIR}
    COMPONENT development
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
      RUNTIME DESTINATION ${UDA_INSTALL_BINDIR}
      LIBRARY DESTINATION ${UDA_INSTALL_LIBDIR}
      ARCHIVE DESTINATION ${UDA_INSTALL_LIBDIR}
      COMPONENT client-runtime
    )
  endif()
  
endif()
