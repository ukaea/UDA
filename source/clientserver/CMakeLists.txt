########################################################################################################################
# Dependencies

find_package( OpenSSL REQUIRED )
find_package( fmt REQUIRED )
find_package( Boost REQUIRED COMPONENTS regex )

set( LIBS OpenSSL::SSL OpenSSL::Crypto fmt::fmt Boost::regex )

if( NOT CLIENT_ONLY )
  find_package( LibXml2 REQUIRED )
  list( APPEND LIBS LibXml2::LibXml2 )
endif()

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
  list( APPEND LIBS XDR::XDR )
else()
  find_package( TIRPC QUIET )
  if( TIRPC_FOUND )
    list( APPEND LIBS TIRPC::TIRPC )
  endif()
endif()

set( SRC_FILES
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
  make_request_block.cpp
  version.cpp
  #  utils.c
)

if( APPLE )
  list( APPEND SRC_FILES memstream.cpp )
endif()

if( WIN32 )
  list( APPEND SRC_FILES memstream_win.cpp mkstemp.cpp )
endif()

if( CLIENT_ONLY )
  add_definitions( -DNOXMLPARSER )
endif()

add_library( clientserver-client-objects OBJECT ${SRC_FILES} )
target_link_libraries( clientserver-client-objects PUBLIC ${LIBS} )
target_include_directories( clientserver-client-objects PRIVATE
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_SOURCE_DIR}/source/include
  ${CMAKE_BINARY_DIR}/include
)

if( NOT CLIENT_ONLY )
  add_library( clientserver-server-objects OBJECT ${SRC_FILES} )
  target_compile_definitions( clientserver-server-objects PUBLIC -DSERVERBUILD )
  target_link_libraries( clientserver-server-objects PUBLIC ${LIBS} )
  target_include_directories( clientserver-server-objects PRIVATE
    ${CMAKE_SOURCE_DIR}/source
    ${CMAKE_SOURCE_DIR}/source/include
    ${CMAKE_BINARY_DIR}/include
  )


  add_library( fatclientserver-objects OBJECT ${SRC_FILES} )
  target_compile_definitions( fatclientserver-objects PUBLIC -DFATCLIENT )
  target_link_libraries( fatclientserver-objects PUBLIC ${LIBS} )
  target_include_directories( fatclientserver-objects PRIVATE
    ${CMAKE_SOURCE_DIR}/source
    ${CMAKE_SOURCE_DIR}/source/include
    ${CMAKE_BINARY_DIR}/include
  )
endif()
