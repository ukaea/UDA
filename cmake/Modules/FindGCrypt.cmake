# - Find GCrypt (libgcrypt)
#
# Result variables:
#
#  GCrypt_FOUND         - True if libgcrypt headers and library were found
#  GCRYPT_INCLUDE_DIRS  - Include directory (legacy name, kept for existing callers)
#  GCRYPT_LIBRARIES     - Library to link (legacy name, kept for existing callers)
#
# Imported target:
#
#  GCrypt::gcrypt       - Created when GCrypt is found
#
# Hints: the GCRYPT_ROOT CMake variable or environment variable.
#
# Normal usage:
#  find_package( GCrypt )
#  target_link_libraries( my_target PRIVATE GCrypt::gcrypt )

find_path( GCRYPT_INCLUDE_DIR
  NAMES gcrypt.h
  HINTS ${GCRYPT_ROOT} ENV GCRYPT_ROOT
  PATHS
    "$ENV{PROGRAMFILES}/libgcrypt"
    /usr/local/include
    /opt/local/include
    /sw/include
    /usr/lib/sfw/include
  PATH_SUFFIXES include
)

find_library( GCRYPT_LIBRARY
  NAMES gcrypt gcrypt11 libgcrypt-11
  HINTS ${GCRYPT_ROOT} ENV GCRYPT_ROOT
  PATHS
    /opt/local/lib
    /sw/lib
    /usr/sfw/lib/64
    /usr/sfw/lib
  PATH_SUFFIXES lib lib64
)

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( GCrypt
  REQUIRED_VARS GCRYPT_LIBRARY GCRYPT_INCLUDE_DIR
)

mark_as_advanced( GCRYPT_INCLUDE_DIR GCRYPT_LIBRARY )

if( GCrypt_FOUND )
  set( GCRYPT_INCLUDE_DIRS ${GCRYPT_INCLUDE_DIR} )
  set( GCRYPT_LIBRARIES ${GCRYPT_LIBRARY} )
endif()

if( GCrypt_FOUND AND NOT TARGET GCrypt::gcrypt )
  add_library( GCrypt::gcrypt UNKNOWN IMPORTED )
  set_target_properties( GCrypt::gcrypt PROPERTIES
    IMPORTED_LOCATION "${GCRYPT_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${GCRYPT_INCLUDE_DIR}"
  )
endif()
