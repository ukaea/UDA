cmake_minimum_required( VERSION 2.8 )

########################################################################################################################
# Dependencies

find_package( OpenSSL REQUIRED )
if( NOT CLIENT_ONLY )
  find_package( PostgreSQL REQUIRED )
  find_package( LibXml2 REQUIRED )
endif()

include_directories(
  ${CMAKE_SOURCE_DIR}/source
)

set( SRC_FILES
  freeDataBlock.c
  errorLog.c
  allocData.c
  protocol.c
  protocol2.c
  protocolXML.c
  protocolXML2.c
  protocolXML2Put.c
  compressDim.c
  initStructs.c
  manageSockets.c
  md5Sum.c
  printStructs.c
  userid.c
  xdrlib.c
  readXDRFile.c
  xdrHData.c
  allocXMLData.c
  initXMLStructs.c
  expand_path.c
  stringUtils.c
  parseOperation.c
  parseXML.c
  copyStructs.c
  udaTypes.c
  )

if( APPLE )
  list( APPEND SRC_FILES
    memstream.c )
endif()

if( CLIENT_ONLY )
  add_definitions( -DNOXMLPARSER )
endif()

include_directories(
  ${SRC_DIR}
  ${OPENSSL_INCLUDE_DIR}
  ${LIBXML2_INCLUDE_DIR}
)

add_library( clientserver-client-objects OBJECT ${SRC_FILES} )

if( NOT CLIENT_ONLY )
  add_library( clientserver-server-objects OBJECT ${SRC_FILES} )
  target_compile_definitions( clientserver-server-objects PRIVATE -DSERVERBUILD )

  add_library( fatclientserver-objects OBJECT ${SRC_FILES} )
  target_compile_definitions( fatclientserver-objects PRIVATE -DFATCLIENT )
endif()
