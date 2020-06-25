# - Find TIRPC
#
# To provide the module with a hint about where to find your TIRPC
# installation, you can set the environment variable TIRPC_ROOT. The
# Find module will then look in this path when searching for TIRPC paths
# and libraries.
#
# Find the TIRPC includes and libraries
#
#  TIRPC_INCLUDE_DIR - where to find mdslib.h, etc
#  TIRPC_LIBRARIES   - Link these libraries when using TIRPC
#  TIRPC_FOUND       - True if TIRPC found
#
# Normal usage would be:
#  find_package (TIRPC REQUIRED)
#  target_link_libraries (uses_TIRPC ${TIRPC_LIBRARIES})

if( TIRPC_INCLUDE_DIR AND TIRPC_LIBRARIES )
  # Already in cache, be silent
  set( TIRPC_FIND_QUIETLY TRUE )
endif( TIRPC_INCLUDE_DIR AND TIRPC_LIBRARIES )

find_package(PkgConfig)

pkg_search_module(TIRPC libtirpc QUIET )

if(${TIRPC_FOUND})
  SET(TIRPC_LIBRARIES ${TIRPC_LIBRARIES})      
  SET(TIRPC_INCLUDE_DIR ${TIRPC_INCLUDEDIR}/tirpc)

else()
  find_path(TIRPC_INCLUDE_DIR rpc/xdr.h
        HINTS
          ${TIRPC_ROOT}
        extlib
          ENV TIRPC_ROOT           
          ENV EBROOTLIBTIRPC         
        PATH_SUFFIXES include include/rpc )
  find_library( TIRPC_LIBRARIES NAMES tirpc 
        HINTS
          ${TIRPC_ROOT}
        extlib
          ENV TIRPC_ROOT
          ENV EBROOTLIBTIRPC
        PATH_SUFFIXES lib lib64 )          
endif()

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( TIRPC DEFAULT_MSG TIRPC_LIBRARIES TIRPC_INCLUDE_DIR )

mark_as_advanced( TIRPC_LIBRARIES TIRPC_INCLUDE_DIR )
