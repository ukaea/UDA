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

find_path( LEVELDB_INCLUDE_DIR db.h
  HINTS ${LEVELDB_ROOT}
    ENV LEVELDB_DIR
  PATHS
    /usr/local
    /opt/local
    /sw
    /usr/lib/sfw
  PATH_SUFFIXES include/leveldb )

find_library( LEVELDB_LIBRARIES NAMES leveldb
  HINTS ${LEVELDB_ROOT}
    ENV LEVELDB_DIR
  PATHS
    /opt/local
    /sw
    /usr
    /usr/local
  PATH_SUFFIXES lib lib64 )

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( LEVELDB DEFAULT_MSG LEVELDB_LIBRARIES LEVELDB_INCLUDE_DIR )

mark_as_advanced( LEVELDB_LIBRARIES LEVELDB_INCLUDE_DIR )
