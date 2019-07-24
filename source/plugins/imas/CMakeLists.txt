# Macro used to convert Windows path to Unix
macro( CONVERT_WIN_PATH VarName )
  if( MINGW AND NOT "${${VarName}}" STREQUAL "" )
    execute_process( COMMAND cygpath.exe -u ${${VarName}} OUTPUT_VARIABLE ${VarName} )
    string( STRIP ${${VarName}} ${VarName} )
    string( REGEX REPLACE "[\r\n]+" ";" ${VarName} ${${VarName}} )
  endif()
endmacro()

########################################################################################################################
# Dependencies
find_package( LibXml2 QUIET )
find_package( MDSplus QUIET )
find_package( HDF5 COMPONENTS C HL QUIET )
find_package( OpenSSL QUIET )
if( MINGW )
  CONVERT_WIN_PATH( LIBXML2_INCLUDE_DIR )
  CONVERT_WIN_PATH( LIBXML2_LIBRARIES )
endif()

if( NOT MDSPLUS_FOUND )
  message( WARNING "mdsplus not found - skipping imas plugin" )
  return()
elseif( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping imas plugin" )
  return()
elseif( NOT HDF5_FOUND )
  message( WARNING "HDF5 not found - skipping imas plugin" )
  return()
elseif( NOT OPENSSL_FOUND )
  message( WARNING "OpenSSL not found - skipping imas plugin" )
  return()
endif()

# libgnurx is required under Windows for std::regex
if( WIN32 OR MINGW )
  set( WIN_LIB "gnurx" )
else()
  set( WIN_LIB "" )
endif()

add_definitions( -Wno-error -Wno-all )

include( plugins )

set( IMAS_SOURCES 
  ual_low_level_mdsplus.c
  ual_low_level_remote.c
  ual_low_level.c
  ual_low_level_meta.cpp
  ual_low_level_mdsobjects.cpp
  ual_low_level_idam_dummy.c
)

uda_plugin(
  NAME IMAS
  ENTRY_FUNC imas_mds
  DESCRIPTION "ITER IMAS put & get plugin"
  EXAMPLE "IMAS::put()"
  LIBNAME imas_plugin
  SOURCES
    imas_common.c
    imas_mds.c
    imas_hdf5.c
    extract_indices.c
    common.c
    ${IMAS_SOURCES}
  CONFIG_FILE imas.cfg
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${MDSPLUS_INCLUDES}
    ${CMAKE_SOURCE_DIR}/client
    ${HDF5_INCLUDE_DIRS}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${MDSPLUS_LIBRARIES}
    MdsObjectsCppShr
    ${HDF5_LIBRARIES}
    ${HDF5_HL_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    client-static
	stdc++
	${WIN_LIB}
)
