# - Find MDSplus
# Find the MDSplus includes and libraries
#
# To provide the module with a hint about where to find your MDSPLUS
# installation, you can set the environment variables MDSPLUS_DIR. The
# Find module will then look in this path when searching for MDSPLUS
# paths and libraries.
#
#  MDSPLUS_INCLUDES    - where to find mdslib.h, etc
#  MDSPLUS_LIBRARIES   - Link these libraries when using MDSplus
#  MDSPLUS_FOUND       - True if MDSplus found
#
# Normal usage would be:
#  find_package (MDSplus REQUIRED)
#  target_link_libraries (uses_mdsplus ${MDSPLUS_LIBRARIES})

if( MDSPLUS_INCLUDES AND MDSPLUS_LIBRARIES )
  # Already in cache, be silent
  set( MDSplus_FIND_QUIETLY TRUE )
endif( MDSPLUS_INCLUDES AND MDSPLUS_LIBRARIES )

find_path( MDSPLUS_INCLUDES mdslib.h
  HINTS
    ${MDSPLUS_DIR}
    ENV MDSPLUS_DIR
  PATHS
    /usr/local/mdsplus
  PATH_SUFFIXES include )

set( MDS_LIBS
  MdsLib
  MdsObjectsCppShr
  MdsShr
  MdsIpShr
  TreeShr
  TdiShr
)

set( MDSPLUS_LIBRARIES "" )

foreach( MDS_LIB ${MDS_LIBS} )

  find_library( ${MDS_LIB}-FIND NAMES "${MDS_LIB}"
    HINTS
      ${MDSPLUS_DIR}
      ENV MDSPLUS_DIR
    PATHS
      /usr/local/mdsplus
    PATH_SUFFIXES lib lib64 )

  if( ${MDS_LIB}-FIND )
    list( APPEND MDSPLUS_LIBRARIES "${${MDS_LIB}-FIND}" )
  else()
    if( MDSplus_FIND_REQUIRED )
      message( FATAL_ERROR "Failed to find MDS library: ${MDS_LIB}" )
    endif()
  endif()

endforeach()

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( MDSplus DEFAULT_MSG MDSPLUS_LIBRARIES MDSPLUS_INCLUDES )

mark_as_advanced( MDSPLUS_LIBRARIES MDSPLUS_INCLUDES )
