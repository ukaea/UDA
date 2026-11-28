# - Find KSBA (libksba)
#
# Result variables:
#
#  KSBA_FOUND          - True if libksba and libgpg-error were found
#  KSBA_INCLUDE_DIRS   - Include directory (legacy name, kept for existing callers)
#  KSBA_LIBRARIES      - Libraries to link (legacy name, kept for existing callers)
#
# Imported target:
#
#  KSBA::KSBA          - Created when KSBA is found; depends on gpg-error
#
# Hints: the KSBA_ROOT CMake variable or environment variable.
#
# Normal usage:
#  find_package( KSBA )
#  target_link_libraries( my_target PRIVATE KSBA::KSBA )

find_path( KSBA_INCLUDE_DIR
  NAMES ksba.h
  HINTS ${KSBA_ROOT} ENV KSBA_ROOT
  PATHS
    "$ENV{PROGRAMFILES}/libKSBA"
    /usr/local/include
    /opt/local/include
    /sw/include
    /usr/lib/sfw/include
  PATH_SUFFIXES include
)

find_library( KSBA_LIBRARY
  NAMES ksba
  HINTS ${KSBA_ROOT} ENV KSBA_ROOT
  PATHS /opt/local/lib /sw/lib /usr/lib /usr/local/lib
  PATH_SUFFIXES lib lib64
)

find_library( GPG_ERROR_LIBRARY
  NAMES gpg-error
  HINTS ${KSBA_ROOT} ENV KSBA_ROOT
  PATHS /opt/local/lib /sw/lib /usr/lib /usr/local/lib
  PATH_SUFFIXES lib lib64
)

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( KSBA
  REQUIRED_VARS KSBA_LIBRARY KSBA_INCLUDE_DIR GPG_ERROR_LIBRARY
)

mark_as_advanced( KSBA_INCLUDE_DIR KSBA_LIBRARY GPG_ERROR_LIBRARY )

if( KSBA_FOUND )
  set( KSBA_INCLUDE_DIRS ${KSBA_INCLUDE_DIR} )
  set( KSBA_LIBRARIES ${KSBA_LIBRARY} ${GPG_ERROR_LIBRARY} )
endif()

if( KSBA_FOUND AND NOT TARGET KSBA::KSBA )
  add_library( KSBA::KSBA UNKNOWN IMPORTED )
  set_target_properties( KSBA::KSBA PROPERTIES
    IMPORTED_LOCATION "${KSBA_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${KSBA_INCLUDE_DIR}"
    INTERFACE_LINK_LIBRARIES "${GPG_ERROR_LIBRARY}"
  )
endif()
