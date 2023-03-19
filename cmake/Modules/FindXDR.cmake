# - Find XDR
#
# To provide the module with a hint about where to find your XDR
# installation, you can set the environment variable XDR_ROOT. The
# Find module will then look in this path when searching for XDR paths
# and libraries.
#
# Find the XDR includes and libraries
#
#  XDR_INCLUDE_DIR - where to find mdslib.h, etc
#  XDR_LIBRARIES   - Link these libraries when using XDR
#  XDR_FOUND       - True if XDR found
#
# Normal usage would be:
#  find_package (XDR REQUIRED)
#  target_link_libraries (uses_XDR ${XDR_LIBRARIES})

if( XDR_INCLUDE_DIR AND XDR_LIBRARIES )
  # Already in cache, be silent
  set( XDR_FIND_QUIETLY TRUE )
endif( XDR_INCLUDE_DIR AND XDR_LIBRARIES )

find_path( XDR_INCLUDE_DIR rpc/xdr.h
  HINTS
    ${XDR_ROOT}
    extlib
    ENV XDR_ROOT
  PATH_SUFFIXES include include/rpc )

find_library( XDR_LIBRARIES NAMES xdr portablexdr
  HINTS
    ${XDR_ROOT}
    extlib
    ENV XDR_ROOT
  PATH_SUFFIXES lib lib64 )

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( XDR DEFAULT_MSG XDR_LIBRARIES XDR_INCLUDE_DIR )

mark_as_advanced( XDR_LIBRARIES XDR_INCLUDE_DIR )
