########################################################################################################################
# Dependencies

find_package( Boost REQUIRED )
find_package( LibXml2 REQUIRED )
find_package( OpenSSL REQUIRED )
find_package( fmt REQUIRED )

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

if( ITER_CI )
  add_definitions( -D__USE_XOPEN2K8 )
endif()

add_library( cpp-objects OBJECT ${SRC_FILES} )
add_library( uda::cpp-objects ALIAS cpp-objects )

target_compile_features( cpp-objects  PRIVATE cxx_std_11 )

target_link_libraries(
  cpp-objects
  PRIVATE
  uda::client-static
  PUBLIC
  Boost::boost
  LibXml2::LibXml2
  OpenSSL::SSL
  OpenSSL::Crypto
  fmt::fmt
)

target_include_directories(
  cpp-objects
  PUBLIC
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  PRIVATE
  ${PROJECT_SOURCE_DIR}/source
)

if( WIN32 OR MINGW )
  if( MINGW )
    target_link_libraries( cpp-objects PRIVATE XDR::XDR ws2_32 dl stdc++ )
  else()
    target_link_libraries( cpp-objects PRIVATE XDR::XDR ws2_32 dlfcn-win32::dl )
  endif()
elseif( TIRPC_FOUND )
  target_link_libraries( cpp-objects PRIVATE TIRPC::rpc )
endif()

add_library( ${LIB_NAME}-static STATIC )
add_library( uda::${LIB_NAME}-static ALIAS ${LIB_NAME}-static )

if( BUILD_SHARED_LIBS )
  add_library( ${LIB_NAME}-shared SHARED )
  add_library( uda::${LIB_NAME}-shared ALIAS ${LIB_NAME}-shared )
endif()

target_link_libraries( ${LIB_NAME}-static PUBLIC uda::cpp-objects )

if( BUILD_SHARED_LIBS )
  target_link_libraries( ${LIB_NAME}-shared PUBLIC uda::cpp-objects )
endif()

if( NOT CLIENT_ONLY )
  add_library( fat${LIB_NAME}-static STATIC )
  add_library( uda::fat${LIB_NAME}-static ALIAS ${LIB_NAME}-static )

  target_compile_definitions( fat${LIB_NAME}-static PRIVATE -DFATCLIENT )

  target_link_libraries(
    fat${LIB_NAME}-static
    PRIVATE
    uda::cpp-objects
    PUBLIC
    LibXml2::LibXml2
  )

  target_include_directories(
    fat${LIB_NAME}-static
    PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    PRIVATE
    ${PROJECT_SOURCE_DIR}/source
  )

  if( BUILD_SHARED_LIBS )
    add_library( fat${LIB_NAME}-shared SHARED )
    add_library( uda::fat${LIB_NAME}-shared ALIAS ${LIB_NAME}-shared )

    target_compile_definitions( fat${LIB_NAME}-shared PRIVATE -DFATCLIENT )
    target_link_libraries(
      fat${LIB_NAME}-shared
      PRIVATE
      uda::cpp-objects
      PUBLIC
      LibXml2::LibXml2
    )

    target_include_directories(
      fat${LIB_NAME}-shared
      PUBLIC
      $<INSTALL_INTERFACE:include>
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
      PRIVATE
      ${PROJECT_SOURCE_DIR}/source
    )
  endif()
endif()

set_target_properties( ${LIB_NAME}-static
  PROPERTIES
    OUTPUT_NAME ${LIB_NAME}_static
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
      OUTPUT_NAME fat${LIB_NAME}_static
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
