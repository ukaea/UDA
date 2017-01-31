# - Find LevelDB
#
# To provide the module with a hint about where to find your LevelDB
# installation, you can set the environment variable LEVELDB_ROOT. The
# Find module will then look in this path when searching for LevelDB paths
# and libraries.
#
# Find the LevelDB includes and libraries
#
#  LEVELDB_INCLUDE_DIR - where to find mdslib.h, etc
#  LEVELDB_LIBRARIES   - Link these libraries when using LevelDB
#  LEVELDB_FOUND       - True if LevelDB found
#
# Normal usage would be:
#  find_package (LevelDB REQUIRED)
#  target_link_libraries (uses_LevelDB ${LEVELDB_LIBRARIES})

if( LEVELDB_INCLUDE_DIR AND LEVELDB_LIBRARIES )
  # Already in cache, be silent
  set( LEVELDB_FIND_QUIETLY TRUE )
endif( LEVELDB_INCLUDE_DIR AND LEVELDB_LIBRARIES )

find_path( LEVELDB_INCLUDE_DIR LEVELDB3.h
  HINTS ${LEVELDB_ROOT}
  ENV LEVELDB_DIR
  PATH_SUFFIXES include )

find_library( LEVELDB_LIBRARIES_LIB NAMES LEVELDB3
  HINTS ${LEVELDB_ROOT}
  ENV LEVELDB_DIR
  PATH_SUFFIXES lib lib64 )

find_library( LEVELDB_C_LIBRARIES_LIB NAMES LEVELDB3c
  HINTS ${LEVELDB_ROOT}
  ENV LEVELDB_DIR
  PATH_SUFFIXES lib lib64 )

set( LEVELDB_LIBRARIES ${LEVELDB_LIBRARIES_LIB} ${LEVELDB_C_LIBRARIES_LIB} )

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( LEVELDB DEFAULT_MSG LEVELDB_LIBRARIES LEVELDB_INCLUDE_DIR )

mark_as_advanced( LEVELDB_LIBRARIES LEVELDB_INCLUDE_DIR )
