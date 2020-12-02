# - Find LevelDB
#
# To provide the module with a hint about where to find your LevelDB
# installation, you can set the environment variable LevelDB_ROOT. The
# Find module will then look in this path when searching for LevelDB paths
# and libraries.
#
# Find the LevelDB includes and libraries
#
#  LevelDB_INCLUDE_DIR - where to find mdslib.h, etc
#  LevelDB_LIBRARIES   - Link these libraries when using LevelDB
#  LevelDB_FOUND       - True if LevelDB found
#
# Normal usage would be:
#  find_package (LevelDB REQUIRED)
#  target_link_libraries (uses_LevelDB ${LevelDB_LIBRARIES})

if( LevelDB_INCLUDE_DIR AND LevelDB_LIBRARIES )
  # Already in cache, be silent
  set( LevelDB_FIND_QUIETLY TRUE )
endif( LevelDB_INCLUDE_DIR AND LevelDB_LIBRARIES )

find_path( LevelDB_INCLUDE_DIR LevelDB/db.h
  HINTS ${LevelDB_ROOT}
    ENV LevelDB_DIR
  PATHS
    /usr/local
    /opt/local
    /sw
    /usr/lib/sfw
  PATH_SUFFIXES include )

find_library( LevelDB_LIBRARIES NAMES LevelDB
  HINTS ${LevelDB_ROOT}
    ENV LevelDB_DIR
  PATHS
    /opt/local
    /sw
    /usr
    /usr/local
  PATH_SUFFIXES lib lib64 )

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( LevelDB DEFAULT_MSG LevelDB_LIBRARIES LevelDB_INCLUDE_DIR )

mark_as_advanced( LevelDB_LIBRARIES LevelDB_INCLUDE_DIR )
