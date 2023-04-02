########################################################################################################################
# Dependencies

find_package( fmt REQUIRED )
find_package( Boost REQUIRED )
find_package( OpenSSL REQUIRED )
if( NOT CLIENT_ONLY )
  find_package( LibXml2 REQUIRED )
endif()

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
else()
  find_package( TIRPC QUIET)
endif()

########################################################################################################################
# Library

set(
  SRC_FILES
  allocData.cpp
  compressDim.cpp
  copyStructs.cpp
  errorLog.cpp
  expand_path.cpp
  initStructs.cpp
  makeRequestBlock.cpp
  manageSockets.cpp
  parseOperation.cpp
  parseXML.cpp
  printStructs.cpp
  protocol.cpp
  protocol2.cpp
  protocolXML.cpp
  protocolXML2.cpp
  protocolXML2Put.cpp
  readXDRFile.cpp
  stringUtils.cpp
  udaTypes.cpp
  udaDefines.cpp
  userid.cpp
  xdrlib.cpp
  nameValueSubstitution.cpp
  udaStructs.cpp
)

add_library( shared-objects OBJECT )
target_sources( shared-objects PUBLIC ${SRC_FILES} )
target_compile_features( shared-objects PRIVATE cxx_std_11 )
target_link_libraries( shared-objects PUBLIC fmt::fmt Boost::boost OpenSSL::SSL OpenSSL::Crypto LibXml2::LibXml2 )

add_library( uda::shared-objects ALIAS shared-objects )

target_include_directories(
  shared-objects
  PUBLIC
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  PRIVATE
  ${PROJECT_SOURCE_DIR}/source
)

# TODO: remove need of this by removing all FATCLIENT specific code in clientserver
if( NOT CLIENT_ONLY )
  add_library( fat-shared-objects OBJECT )
  target_sources( fat-shared-objects PUBLIC ${SRC_FILES} )
  target_compile_definitions( fat-shared-objects PUBLIC -DFATCLIENT )
  target_compile_features( fat-shared-objects PRIVATE cxx_std_11 )
  target_link_libraries( fat-shared-objects PUBLIC fmt::fmt Boost::boost OpenSSL::SSL OpenSSL::Crypto )

  target_include_directories(
    fat-shared-objects
    PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    PRIVATE
    ${PROJECT_SOURCE_DIR}/source
  )

  add_library( uda::fat-shared-objects ALIAS fat-shared-objects )
endif()

#set( HEADER_FILES
#  udaStructs.h
#)

if( APPLE )
  target_sources( shared-objects PUBLIC memstream.cpp )
  if( NOT CLIENT_ONLY )
    target_sources( fat-shared-objects PUBLIC memstream.cpp )
  endif()
endif()

if( WIN32 )
  target_sources( shared-objects PUBLIC memstream_win.cpp mkstemp.cpp )
  if( NOT CLIENT_ONLY )
    target_sources( fat-shared-objects PUBLIC memstream_win.cpp mkstemp.cpp )
  endif()
endif()

if( CLIENT_ONLY )
  target_compile_definitions( shared-objects PUBLIC -DNOXMLPARSER )
  if( NOT CLIENT_ONLY )
    target_compile_definitions( fat-shared-objects PUBLIC -DNOXMLPARSER )
  endif()
endif()

if( MINGW OR WIN32 )
  target_link_libraries( shared-objects PUBLIC XDR::XDR )
  if( NOT CLIENT_ONLY )
    target_link_libraries( fat-shared-objects PUBLIC XDR::XDR )
  endif()
elseif( TIRPC_FOUND )
  target_link_libraries( shared-objects PUBLIC TIRPC::rpc )
  target_compile_definitions( shared-objects PUBLIC -D__TIRPC__ )
  if( NOT CLIENT_ONLY )
    target_link_libraries( fat-shared-objects PUBLIC TIRPC::rpc )
    target_compile_definitions( fat-shared-objects PUBLIC -D__TIRPC__ )
  endif()
endif()

#install( FILES ${HEADER_FILES}
#  DESTINATION include/uda
#)
