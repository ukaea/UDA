cmake_minimum_required( VERSION 2.8 )

########################################
# Names and versions

set( CMAKE_EXE_LINKER_FLAGS "" )
set( CMAKE_SHARED_LINKER_FLAGS "" )
set( SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/.. )

#######################################
# Dependencies

find_package( PostgreSQL REQUIRED )
find_package( LibXml2 REQUIRED )
find_package( OpenSSL REQUIRED )

########################################
# Compiler flags

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99" )

########################################
# Options

option( DEBUG "Debug info" ON )
if( DEBUG )
  add_definitions( -DDEBUG )
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0" )
endif( DEBUG )

add_definitions( -D_GNU_SOURCE )
add_definitions( -DCOMPRESSOFF )
add_definitions( -DOSTYPE=linux )
add_definitions( -DISCOMPLIANT )
add_definitions( -D_GNU_SOURCE )
add_definitions( -DFILELISTTEST )
add_definitions( -DSINGLEPACKET )
add_definitions( -DGENERALSTRUCTS )
add_definitions( -DDGMJUL14 )
add_definitions( -DCLEANNAMESPACE25SEP14 )
add_definitions( -DSERVERBUILD )

include_directories(
  ${SRC_DIR}
  ${SRC_DIR}/include
  ${SRC_DIR}/server
)

set( SRC_FILES
  freeDataBlock.c
  idamErrorLog.c
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
  TrimString.c
  parseOperation.c
  parseXML.c
  copyStructs.c )

set( INCLUDE_FILES
  allocData.h
  allocXMLData.h
  compressDim.h
  copyStructs.h
  expand_path.h
  freeDataBlock.h
  idamDefines.h
  idamErrorLog.h
  idamErrors.h
  idamStructs.h
  initStructs.h
  initXMLStructs.h
  mac_memstream.h
  manageSockets.h
  md5Sum.h
  parseOperation.h
  parseXML.h
  printStructs.h
  protocol2.h
  protocol.h
  protocolXML2.h
  protocolXML2Put.h
  protocolXML.h
  readXDRFile.h
  TrimString.h
  userid.h
  xdrHData.h
  xdrlib.h )

include_directories( ${SRC_DIR} ${OPENSSL_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR} )

add_library( idamclientserver ${SRC_FILES} ${INCLUDE_FILES} )

install( FILES ${INCLUDE_FILES} DESTINATION include/idam/clientserver )
