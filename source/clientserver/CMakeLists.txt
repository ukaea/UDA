find_package( Boost COMPONENTS headers REQUIRED )
find_package( LibXml2 REQUIRED )
find_package( OpenSSL REQUIRED )

set( SRCS
  allocData.cpp
  compressDim.cpp
  copyStructs.cpp
  errorLog.cpp
  expand_path.cpp
  initStructs.cpp
  makeRequestBlock.cpp
  make_request_block.cpp
  manageSockets.cpp
  nameValueSubstitution.cpp
  name_value_list.cpp
  parseOperation.cpp
  parseXML.cpp
  printStructs.cpp
  protocol.cpp
  protocol2.cpp
  protocolXML.cpp
  protocolXML2.cpp
  protocolXML2Put.cpp
  readXDRFile.cpp
  udaDefines.cpp
  udaStructs.cpp
  udaTypes.cpp
  userid.cpp
  version.cpp
  xdrlib.cpp
)

add_library( client_server OBJECT ${SRCS} )
target_include_directories( client_server PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/source ${CMAKE_CURRENT_SOURCE_DIR} )
target_link_libraries( client_server PRIVATE Boost::boost LibXml2::LibXml2 OpenSSL::SSL OpenSSL::Crypto common ${RPC_LIB} )