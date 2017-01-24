cmake_minimum_required( VERSION 2.8 )

########################################
# Names and versions

include( modules )

set( CMAKE_EXE_LINKER_FLAGS "" )
set( CMAKE_SHARED_LINKER_FLAGS "" )
set( IDS_MAJOR_VERSION 1 )
set( IDS_MINOR_VERSION 8 )
set( IDS "${IDAM_NAME}server" )
set( IDS_EXE "${IDAM_NAME}serverprg" )
set( SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/.. )

#######################################
# Dependencies

find_package( PostgreSQL REQUIRED )
find_package( LibXml2 REQUIRED )
find_package( OpenSSL REQUIRED )

find_modules()

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

########################################
# Server
list( APPEND SRC_FILES idamServer.c )
list( APPEND SRC_PRG_FILES idamServerMain.c )
list( APPEND SRC_FILES idamserverGetData.c )
list( APPEND SRC_FILES freeIdamPut.c )
list( APPEND SRC_FILES idamLegacyServer.c )
list( APPEND SRC_FILES idamServerPlugin.c )
list( APPEND SRC_FILES idamServerLegacyPlugin.c )
list( APPEND SRC_FILES makeServerRequestBlock.c )
list( APPEND SRC_FILES idamServer.c )
list( APPEND SRC_FILES sleepServer.c )
list( APPEND SRC_FILES writer.c )
list( APPEND SRC_FILES CreateXDRStream.c )
list( APPEND SRC_FILES mastArchiveFilePath.c )
list( APPEND SRC_FILES closeServerSockets.c )
list( APPEND SRC_FILES dumpFile.c )
list( APPEND SRC_FILES idamserverSubsetData.c )
list( APPEND SRC_FILES applyXML.c )
list( APPEND SRC_FILES manageFiles.c )
list( APPEND SRC_FILES serverProcessing.c )
list( APPEND SRC_FILES sqllib.c )
list( APPEND SRC_FILES managePluginFiles.c )
if( NOT FAT_BUILD )
  list( APPEND SRC_FILES idamServerStartup.c )
  list( APPEND SRC_FILES getServerEnvironment.c )
endif()

# Structures
list( APPEND SRC_FILES ${SRC_DIR}/structures/parseIncludeFile.c )
list( APPEND SRC_FILES ${SRC_DIR}/structures/struct.c )
list( APPEND SRC_FILES ${SRC_DIR}/structures/accessors.c )
list( APPEND SRC_FILES ${SRC_DIR}/structures/xdrUserDefinedData.c )

# Security
list( APPEND SRC_FILES ${SRC_DIR}/security/idamServerAuthentication.c )

# Modules
list( APPEND SRC_FILES modules/ida/readIda.c )
list( APPEND SRC_FILES modules/ida/nameIda.c )
list( APPEND SRC_FILES modules/ida/readIdaItem.c )
list( APPEND SRC_FILES modules/ida/parseIdaPath.c )
list( APPEND SRC_FILES modules/bytes/readBytesNonOptimally.c )
list( APPEND SRC_FILES modules/readNothing/readNothing.c )
list( APPEND SRC_FILES modules/hdf58/readHDF58.c )
list( APPEND SRC_FILES modules/jpf/readjpf.c )
list( APPEND SRC_FILES modules/ufile/readUFile.c )
list( APPEND SRC_FILES modules/idam/readIdam.c )
list( APPEND SRC_FILES modules/netcdf4/readCDF4.c )
if( NETCDF_FOUND )
  list( APPEND SRC_FILES modules/netcdf4/readCDF4SubTree.c )
  list( APPEND SRC_FILES modules/netcdf4/readCDFAtts.c )
  list( APPEND SRC_FILES modules/netcdf4/readCDFMeta.c )
  list( APPEND SRC_FILES modules/netcdf4/readCDFTypes.c )
  list( APPEND SRC_FILES modules/netcdf4/readCDFVar.c )
endif()
list( APPEND SRC_FILES modules/readsql/readSQL.c )
list( APPEND SRC_FILES modules/ppf/readppf.c )
list( APPEND SRC_FILES modules/hdata/readHData.c )
list( APPEND SRC_FILES modules/mdsplus/readMDS.c )
list( APPEND SRC_FILES modules/registration/getPluginAddress.c )

include_directories(
  ${SRC_DIR}
  ${SRC_DIR}/include
  ${SRC_DIR}/clientserver
  ${SRC_DIR}/server
  ${SRC_DIR}/structures
  ${SRC_DIR}/logging
  modules/registration
  modules/ida
  modules/bytes
  modules/mdsplus
  modules/idam
  modules/ida
  modules/netcdf4
  modules/hdf58
  modules/ufile
  modules/ppf
  modules/jpf
  modules/hdata
  modules/readsql
  modules/readNothing
  modules/ufile
  ${LIBXML2_INCLUDE_DIR}
  ${PostgreSQL_INCLUDE_DIRS}
  ${OPENSSL_INCLUDE_DIR}
)

add_definitions( -DCOMPRESSOFF )
add_definitions( -DOSTYPE=linux )
#add_definitions( -DMDSSANDBOX )
add_definitions( -DISCOMPLIANT )
add_definitions( -D_GNU_SOURCE )
#add_definitions( -DIDAMPLUGIN )
add_definitions( -DFILELISTTEST )
add_definitions( -DSINGLEPACKET )
add_definitions( -DGENERALSTRUCTS )
add_definitions( -DDGMJUL14 )
add_definitions( -DCLEANNAMESPACE25SEP14 )
add_definitions( -DSERVERBUILD )

########################################
# Targets

link_directories( ${PostgreSQL_LIBRARY_DIRS} )

if( NOT FAT_BUILD )
  add_library( ${IDS_LIB_NAME} SHARED ${SRC_FILES} $<TARGET_OBJECTS:idamserverlog> )
  add_executable( ${IDS_EXE} ${SRC_PRG_FILES} )
else()
  add_library( ${IDS_LIB_NAME} STATIC ${SRC_FILES} )
endif()

########################################
# Add libraries

# Library
target_link_libraries( ${IDS_LIB_NAME} LINK_PUBLIC
  idamclientserver
  ${LIBXML2_LIBRARIES}
  ${PostgreSQL_LIBRARY}
)

set_source_files_properties(
  modules/idam/readIdam.c
  PROPERTIES COMPILE_FLAGS "-I${SRC_DIR}/client"
)

if( NOT FAT_BUILD )
  target_link_libraries( ${IDS_LIB_NAME} LINK_PUBLIC ${IDC_LIB_NAME} )
  target_link_libraries( ${IDS_LIB_NAME} LINK_PUBLIC ${OPENSSL_LIBRARIES} )

  # Executable
  target_link_libraries( ${IDS_EXE} LINK_PUBLIC ${LIBXML2_LIBRARIES} )
  target_link_libraries( ${IDS_EXE} LINK_PUBLIC ${PostgreSQL_LIBRARY} )
  #target_link_libraries( ${IDS_EXE} LINK_PUBLIC curl ) # commented out to make work on ITER
  target_link_libraries( ${IDS_EXE} LINK_PUBLIC c )
  target_link_libraries( ${IDS_EXE} LINK_PUBLIC m )
  target_link_libraries( ${IDS_EXE} LINK_PUBLIC dl )
  if( NOT APPLE )
    target_link_libraries( ${IDS_EXE} LINK_PUBLIC gcc_s )
  endif()
  target_link_libraries( ${IDS_EXE} LINK_PUBLIC ${IDS_LIB_NAME} )
  target_link_libraries( ${IDS_EXE} LINK_PUBLIC ${IDC_LIB_NAME} )
  target_link_libraries( ${IDS_EXE} LINK_PUBLIC ${OPENSSL_LIBRARIES} )

  link_modules( ${IDS_LIB_NAME} )
endif()

########################################
# Set library properties

set_target_properties( ${IDS_LIB_NAME}
    PROPERTIES
      VERSION ${IDS_MAJOR_VERSION}.${IDS_MINOR_VERSION}
      SOVERSION ${IDS_MAJOR_VERSION}
  )

if( NOT FAT_BUILD )
  install(
    TARGETS ${IDS_LIB_NAME}
    DESTINATION lib
  )

  ########################################
  # Set executable properties

  set_target_properties( ${IDS_EXE}
    PROPERTIES
      OUTPUT_NAME ${IDS}
  )

  install( TARGETS ${IDS_EXE}
    DESTINATION bin
  )
endif()

########################################
# Configure files

string( REPLACE ";" ":" EXTRA_LD_LIBRARY_PATHS "${EXTRA_LD_LIBRARY_PATHS}" )
configure_file(
  "${CMAKE_SOURCE_DIR}/etc/idamserver.sh.in"
  "${CMAKE_CURRENT_BINARY_DIR}/idamserver.sh"
  @ONLY
)

configure_file(
  "${CMAKE_SOURCE_DIR}/etc/idamserver.cfg.in"
  "${CMAKE_CURRENT_BINARY_DIR}/idamserver.cfg"
  @ONLY
)

set( USER $ENV{USER} )
configure_file(
  "${CMAKE_SOURCE_DIR}/etc/xinetd.conf.in"
  "${CMAKE_CURRENT_BINARY_DIR}/xinetd.conf"
  @ONLY
)

configure_file(
  "${CMAKE_SOURCE_DIR}/etc/rc.idam.in"
  "${CMAKE_CURRENT_BINARY_DIR}/rc.idam"
  @ONLY
)

configure_file(
  "${CMAKE_SOURCE_DIR}/etc/README.md.in"
  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
  @ONLY
)

configure_file(
  "${CMAKE_SOURCE_DIR}/etc/idamgenstruct.conf"
  "${CMAKE_CURRENT_BINARY_DIR}/idamgenstruct.conf"
  COPYONLY
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/xinetd.conf
    ${CMAKE_CURRENT_BINARY_DIR}/README.md
    ${CMAKE_CURRENT_BINARY_DIR}/idamserver.cfg
  DESTINATION etc
)

install(
  DIRECTORY
    ${CMAKE_SOURCE_DIR}/etc/machine.d
  DESTINATION etc
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/idamserver.sh
    ${CMAKE_CURRENT_BINARY_DIR}/rc.idam
  DESTINATION etc
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ WORLD_READ
)

install(
  DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/plugins.d
  DESTINATION etc
)

install(
  FILES
  ${CMAKE_CURRENT_BINARY_DIR}/idamgenstruct.conf
  DESTINATION bin/plugins
)
