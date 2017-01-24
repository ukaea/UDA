cmake_minimum_required( VERSION 2.8 )

########################################
# Names and versions

set( IDC_MAJOR_VERSION 1 )
set( IDC_MINOR_VERSION 8 )

include( modules )

#######################################
# Dependencies

find_package( OpenSSL REQUIRED )
if( FAT_BUILD )
  find_package( LibXml2 REQUIRED )
  find_package( PostgreSQL REQUIRED )
endif()

########################################
# Locations of directories

set( SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/.. )

########################################
# Options

option( SINGLEPACKET "Single packet option" ON )

if( SINGLEPACKET )
  add_definitions( -DSINGLEPACKET )
endif()

option( SECURITYENABLED "Security enabled" OFF )
if( SECURITYENABLED )
  add_definitions( -DSECURITYENABLED )
endif()

option( DEBUG "Debug info" ON )
if( DEBUG )
  add_definitions( -DDEBUG )
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
endif()

option( GENERALSTRUCTS "General structs" ON )
if( GENERALSTRUCTS )
  add_definitions( -DGENERALSTRUCTS )
endif()

########################################
# Target type

if( ${TARGET_TYPE} STREQUAL JET )
  add_definitions( -DJETSERVER )
elseif( ${TARGET_TYPE} STREQUAL MAST )
  add_definitions( -DMASTSERVER )
elseif( ${TARGET_TYPE} STREQUAL OTHER )
  add_definitions( -DOTHERSERVER )
else()
  message( FATAL_ERROR "INVALID TARGET_TYPE: ${TARGET_TYPE}" )
endif()

add_definitions( -DCLEANNAMESPACE25SEP14 -DOSTYPE=linux )
if( NOT FAT_BUILD )
  add_definitions( -DCLIENTBUILD )
endif()

########################################
# Compiler flags

add_definitions( -DFILECACHE -D_GNU_SOURCE -DNO_GSL_LIB )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpic -I." )

# Clientserver
if( NOT FAT_BUILD )
list( APPEND SRC_FILES ${SRC_DIR}/clientserver/expand_path.c )
list( APPEND SRC_FILES ${SRC_DIR}/clientserver/TrimString.c )
list( APPEND SRC_FILES ${SRC_DIR}/clientserver/compressDim.c )
list( APPEND SRC_FILES ${SRC_DIR}/clientserver/printStructs.c )
list( APPEND SRC_FILES ${SRC_DIR}/clientserver/xdrlib.c )
list( APPEND SRC_FILES ${SRC_DIR}/clientserver/readXDRFile.c )
list( APPEND SRC_FILES ${SRC_DIR}/clientserver/freeDataBlock.c )
list( APPEND SRC_FILES ${SRC_DIR}/clientserver/protocol2.c )
list( APPEND SRC_FILES ${SRC_DIR}/clientserver/protocolXML2.c )
list( APPEND SRC_FILES ${SRC_DIR}/clientserver/protocolXML2Put.c )
list( APPEND SRC_FILES ${SRC_DIR}/clientserver/protocol.c )
list( APPEND SRC_FILES ${SRC_DIR}/clientserver/protocolXML.c )
list( APPEND SRC_FILES ${SRC_DIR}/clientserver/allocData.c )
list( APPEND SRC_FILES ${SRC_DIR}/clientserver/initStructs.c)
list( APPEND SRC_FILES ${SRC_DIR}/clientserver/userid.c )
list( APPEND SRC_FILES ${SRC_DIR}/clientserver/manageSockets.c )
list( APPEND SRC_FILES ${SRC_DIR}/clientserver/idamErrorLog.c )
list( APPEND SRC_FILES ${SRC_DIR}/clientserver/xdrlib.c )
list( APPEND SRC_FILES ${SRC_DIR}/clientserver/xdrHData.c )
list( APPEND SRC_FILES ${SRC_DIR}/clientserver/allocXMLData.c )
list( APPEND SRC_FILES ${SRC_DIR}/clientserver/initXMLStructs.c )
endif()
list( APPEND SRC_FILES ${SRC_DIR}/clientserver/copyStructs.c )

# Client
list( APPEND SRC_FILES accAPI_C.c )
list( APPEND SRC_FILES IdamAPI.c )
list( APPEND SRC_FILES ClientAPI.c )
list( APPEND SRC_FILES ClientMDS.c )
list( APPEND SRC_FILES accAPI_C.c  )
list( APPEND SRC_FILES accAPI_CL.c )
list( APPEND SRC_FILES accAPI_F.c )
list( APPEND SRC_FILES accAPI_FL.c)
list( APPEND SRC_FILES accAPI_Java.c )
list( APPEND SRC_FILES accAPI_XML.c )
list( APPEND SRC_FILES accAPI_Gen.c )
list( APPEND SRC_FILES startup.c )
list( APPEND SRC_FILES closedown.c )
list( APPEND SRC_FILES idamCreateConnection.c )
list( APPEND SRC_FILES createClientXDRStream.c )
list( APPEND SRC_FILES UpdateSelectParms.c )
list( APPEND SRC_FILES Writeout.c )
list( APPEND SRC_FILES Readin.c )
list( APPEND SRC_FILES getEnvironment.c)
list( APPEND SRC_FILES generateErrors.c )
list( APPEND SRC_FILES closeClientSockets.c )
list( APPEND SRC_FILES makeClientRequestBlock.c )
list( APPEND SRC_FILES idamPutAPI.c )
if( APPLE )
  list( APPEND SRC_FILES ${SRC_DIR}/clientserver/mac_memstream.c )
endif()
list( APPEND SRC_FILES idam_client.c )# Security & Optimised protocol
#list( APPEND SRC_FILES idam_client_legacy.c )

if( NOT FAT_BUILD )
if( GENERALSTRUCTS )
  list( APPEND SRC_FILES ${SRC_DIR}/structures/accessors.c )
  list( APPEND SRC_FILES ${SRC_DIR}/structures/struct.c )
  list( APPEND SRC_FILES ${SRC_DIR}/structures/xdrUserDefinedData.c )
endif()
endif()

if( SECURITYENABLED )
  list( APPEND SRC_FILES ${SRC_DIR}/security/idamClientAuthentication.c )
  list( APPEND SRC_FILES ${SRC_DIR}/security/idamSecurity.c )
endif()

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpic" )

include_directories(
  ${SRC_DIR}
  ${SRC_DIR}/client
  ${SRC_DIR}/include
  ${SRC_DIR}/clientserver
  ${SRC_DIR}/cache
  ${SRC_DIR}/logging
  ${SRC_DIR}/structures
  ${OPENSSL_INCLUDE_DIR}
)

if( FAT_BUILD )
  include_directories( ${SRC_DIR}/server ${LIBXML2_INCLUDE_DIR} ${PostgreSQL_INCLUDE_DIRS} )
endif()

add_library( idamclient OBJECT ${SRC_FILES} )
if( NOT FAT_BUILD )
  set( CLIENT_SRCS
    $<TARGET_OBJECTS:idamclient>
    $<TARGET_OBJECTS:idamcache>
    $<TARGET_OBJECTS:idamlog>
  )
else()
  set( CLIENT_SRCS
    $<TARGET_OBJECTS:idamclient>
    $<TARGET_OBJECTS:idamcache>
    $<TARGET_OBJECTS:idamserverlog>
  )
endif()

add_library( ${IDC_LIB_NAME} SHARED ${CLIENT_SRCS} )
add_library( ${IDC_LIB_NAME}-static STATIC ${CLIENT_SRCS} )

if( FAT_BUILD )
  find_modules( ${IDC_LIB_NAME} )
endif()

set_target_properties( ${IDC_LIB_NAME}-static 
  PROPERTIES
    OUTPUT_NAME ${IDC_LIB_NAME}
)

set( CLIENT_LIBS
  ${OPENSSL_LIBRARIES}
  ${CACHE_LIBRARIES}
)

target_link_libraries( ${IDC_LIB_NAME} LINK_PUBLIC ${CLIENT_LIBS} )
target_link_libraries( ${IDC_LIB_NAME}-static LINK_PUBLIC ${CLIENT_LIBS} )

if( FAT_BUILD )
  find_package( PostgreSQL REQUIRED )
  set( SERVER_LIB "-Wl,--whole-archive ${CMAKE_BINARY_DIR}/server/lib${IDS_LIB_NAME}.a -Wl,--no-whole-archive" )
  add_dependencies( ${IDC_LIB_NAME} ${IDS_LIB_NAME} )
  target_link_libraries( ${IDC_LIB_NAME} 
    LINK_PUBLIC ${SERVER_LIB}
      idamclientserver
      ${LIBXML2_LIBRARIES}
      ${PostgreSQL_LIBRARY}
      c
      m
      dl
      gcc_s
      ${OPENSSL_LIBRARIES}
      idamclientserver
  )

  target_link_libraries( ${IDC_LIB_NAME}-static LINK_PUBLIC ${IDS_LIB_NAME} idamclientserver )

  link_modules( ${IDC_LIB_NAME} )
endif()

set_target_properties( ${IDC_LIB_NAME}
  PROPERTIES
    VERSION ${IDC_MAJOR_VERSION}.${IDC_MINOR_VERSION}
    SOVERSION ${IDC_MAJOR_VERSION}
)

install( TARGETS ${IDC_LIB_NAME}
  DESTINATION lib
)

install( TARGETS ${IDC_LIB_NAME}-static
  DESTINATION lib
)

#Install header files
#NB Client / Server files are mixed up here
list( APPEND IDAM_INCLUDE_FILES ${SRC_DIR}/include/idamclientconfig.h )
list( APPEND IDAM_INCLUDE_FILES ${SRC_DIR}/include/idamclient.h )
list( APPEND IDAM_INCLUDE_FILES ${SRC_DIR}/include/idamclientprivate.h )
list( APPEND IDAM_INCLUDE_FILES ${SRC_DIR}/include/idamclientpublic.h )
list( APPEND IDAM_INCLUDE_FILES ${SRC_DIR}/include/idamclientserver.h )
list( APPEND IDAM_INCLUDE_FILES ${SRC_DIR}/include/idamclientserverprivate.h )
list( APPEND IDAM_INCLUDE_FILES ${SRC_DIR}/include/idamclientserverpublic.h )
list( APPEND IDAM_INCLUDE_FILES ${SRC_DIR}/include/idamclientserverxml.h )
list( APPEND IDAM_INCLUDE_FILES ${SRC_DIR}/include/idamdlm.h )
list( APPEND IDAM_INCLUDE_FILES ${SRC_DIR}/include/idamgenstruct.h )
list( APPEND IDAM_INCLUDE_FILES ${SRC_DIR}/include/idamgenstructprivate.h )
list( APPEND IDAM_INCLUDE_FILES ${SRC_DIR}/include/idamgenstructpublic.h )
list( APPEND IDAM_INCLUDE_FILES ${SRC_DIR}/include/idamgenstructtest.h )
list( APPEND IDAM_INCLUDE_FILES ${SRC_DIR}/include/idammatlab.h )
list( APPEND IDAM_INCLUDE_FILES ${SRC_DIR}/include/idamplugin.h )
list( APPEND IDAM_INCLUDE_FILES ${SRC_DIR}/include/idamproxy.h )
list( APPEND IDAM_INCLUDE_FILES ${SRC_DIR}/include/idamscheduler.h )
list( APPEND IDAM_INCLUDE_FILES ${SRC_DIR}/include/idamtypes.h )
list( APPEND IDAM_INCLUDE_FILES ${SRC_DIR}/include/mastCPF.h )
list( APPEND IDAM_INCLUDE_FILES ${SRC_DIR}/include/parseEfitXMLPrivate.h )
list( APPEND IDAM_INCLUDE_FILES ${SRC_DIR}/include/parseEfitXMLPublic.h )
list( APPEND IDAM_INCLUDE_FILES ${SRC_DIR}/include/parseEfitXMLPublicString.h )
list( APPEND IDAM_INCLUDE_FILES ${SRC_DIR}/include/random.h )
list( APPEND IDAM_INCLUDE_FILES ${SRC_DIR}/include/read_data.h )

list( APPEND CLIENT_INCLUDE_FILES ${SRC_DIR}/client/accAPI_C.h )
list( APPEND CLIENT_INCLUDE_FILES ${SRC_DIR}/client/IdamAPI.h )
list( APPEND CLIENT_INCLUDE_FILES ${SRC_DIR}/client/idam_client.h )
list( APPEND CLIENT_INCLUDE_FILES ${SRC_DIR}/client/idamPutAPI.h )

install( FILES ${IDAM_INCLUDE_FILES} DESTINATION include/idam )
install( FILES ${CLIENT_INCLUDE_FILES} DESTINATION include/idam/client )
