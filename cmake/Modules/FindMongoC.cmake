# - Try to find libmongoc
#
# To provide the module with a hint about where to find your libmongoc
# installation, you can set the environment variable MONGOC_ROOT. The Find
# module will then look in this path when searching for libmongoc paths and
# libraries.
#
# Once done this will define
#  LIBMONGOC_FOUND - System has libmongoc
#  LIBMONGOC_INCLUDE_DIRS - The libmongoc include directories
#  LIBMONGOC_LIBRARIES - The libraries needed to use libmongoc

find_library( LIBMONGOC_LIBRARY
  NAMES mongoc-1.0
  HINTS ${MONGOC_ROOT}
    ENV MONGOC_ROOT
  PATHS
    /usr
    /usr/local
    /opt/local
  PATH_SUFFIXES lib lib64
  )

find_library( LIBBSON_LIBRARY
  NAMES bson-1.0
  HINTS ${MONGOC_ROOT}
    ENV MONGOC_ROOT
  PATHS
    /usr
    /usr/local
    /opt/local
  PATH_SUFFIXES lib lib64
  )

find_path( LIBMONGOC_INCLUDE_DIR
  mongoc.h
  HINTS ${MONGOC_ROOT}
    ENV MONGOC_ROOT
  PATHS
    /usr
    /usr/local
    /opt/local
  PATH_SUFFIXES include/libmongoc-1.0
  )

find_path( LIBBSON_INCLUDE_DIR
  bson.h
  HINTS ${MONGOC_ROOT}
    ENV MONGOC_ROOT
  PATH
    /usr/
    /usr/local
    /opt/local
  PATH_SUFFIXES include/libbson-1.0
  )

set( LIBMONGOC_LIBRARIES ${LIBMONGOC_LIBRARY} ${LIBBSON_LIBRARY} )
set( LIBMONGOC_INCLUDE_DIRS ${LIBMONGOC_INCLUDE_DIR} ${LIBBSON_INCLUDE_DIR} )

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( LIBMONGOC DEFAULT_MSG LIBMONGOC_LIBRARY LIBMONGOC_INCLUDE_DIR LIBBSON_INCLUDE_DIR )
