########################################################################################################################
# Dependencies

find_package( OpenSSL REQUIRED )
find_package( fmt REQUIRED )
if( NOT CLIENT_ONLY )
  find_package( LibXml2 REQUIRED )
endif()

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
else()
  find_package( TIRPC QUIET )
  if( TIRPC_FOUND )
    add_definitions( -D__TIRPC__ )
  endif()
endif()

include_directories(
  ${CMAKE_SOURCE_DIR}/source
)

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
#  utils.c
)

set( HIERARCHICAL_SRC_FILES
  allocXMLData.cpp
  xdrHData.cpp
  initXMLStructs.cpp
)

set( HEADER_FILES
  #parseXML.h
  allocData.h
  allocXMLData.h
  compressDim.h
  copyStructs.h
  errorLog.h
  expand_path.h
  initStructs.h
  initXMLStructs.h
  makeRequestBlock.h
  manageSockets.h
  memstream.h
  parseOperation.h
  printStructs.h
  protocol.h
  protocolXML.h
  protocolXML2.h
  protocolXML2Put.h
  readXDRFile.h
  socketStructs.h
  stringUtils.h
  udaDefines.h
  udaErrors.h
  udaStructs.h
  udaTypes.h
  userid.h
  xdrHData.h
  xdrlib.h
  xmlStructs.h
  export.h
  nameValueSubstitution.h
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

include_directories(
  ${SRC_DIR}
  ${OPENSSL_INCLUDE_DIR}
  ${LIBXML2_INCLUDE_DIR}
  ${fmt_SOURCE_DIR}/include
)

if( MINGW OR WIN32 )
  include_directories( ${XDR_INCLUDE_DIR} )
#  add_compile_options( -Wno-error=use-after-free )
elseif( TIRPC_FOUND )
  include_directories( ${TIRPC_INCLUDE_DIR} )
endif()

add_library( clientserver-client-objects OBJECT ${SRC_FILES} ${HEADER_FILES} )
target_link_libraries( clientserver-client-objects PRIVATE fmt::fmt )

if( NOT CLIENT_ONLY )
  add_library( clientserver-server-objects OBJECT ${SRC_FILES} )
  target_compile_definitions( clientserver-server-objects PUBLIC -DSERVERBUILD )
  target_link_libraries( clientserver-server-objects PRIVATE fmt::fmt )

  add_library( fatclientserver-objects OBJECT ${SRC_FILES} )
  target_compile_definitions( fatclientserver-objects PUBLIC -DFATCLIENT )
  target_link_libraries( fatclientserver-objects PRIVATE fmt::fmt )
endif()

file( GLOB HEADER_FILES "*.h" )

install( FILES ${HEADER_FILES}
  DESTINATION include/uda/clientserver
)
