set( PYTHON_FILES
  pyuda/__init__.py
  pyuda/_client.py
  pyuda/_data.py
  pyuda/_dim.py
  pyuda/_json.py
  pyuda/_signal.py
  pyuda/_string.py
  pyuda/_structured.py
  pyuda/_tree.py
  pyuda/_utils.py
  pyuda/_video.py
)

set( CYTHON_FILES
  pyuda/cpyuda/client.pyx
  pyuda/cpyuda/cpyuda.pyx
  pyuda/cpyuda/dim.pyx
  pyuda/cpyuda/exceptions.pyx
  pyuda/cpyuda/handle.pxd
  pyuda/cpyuda/handle.pyx
  pyuda/cpyuda/result.pyx
  pyuda/cpyuda/tree_node.pyx
  pyuda/cpyuda/types.pyx
  pyuda/cpyuda/uda.pxd
)

set( CYTHON_SOURCES pyuda/cpyuda/cpyuda.pyx )

if( ENABLE_CAPNP )
  list( APPEND CYTHON_FILES pyuda/cpyuda/capnp.pxd pyuda/cpyuda/capnp_tree.pyx  )
  list( APPEND CYTHON_SOURCES pyuda/cpyuda/capnp_tree.pyx )
endif( )

install( FILES ${CYTHON_FILES} DESTINATION python_installer/pyuda/cpyuda )
install( FILES ${PYTHON_FILES} DESTINATION python_installer/pyuda )

configure_file( ${CMAKE_CURRENT_LIST_DIR}/pyuda/_version.py.in ${CMAKE_CURRENT_BINARY_DIR}/pyuda/_version.py @ONLY )
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/pyuda/_version.py DESTINATION python_installer/pyuda )

find_package( fmt REQUIRED )

if( ENABLE_CACHE_MEMCACHE )
  find_package( LibMemcached REQUIRED )
endif()

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
  find_package( TIRPC REQUIRED )
endif()

find_library( STDCXX_LIBRARY_FILE stdc++ HINTS ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES} )
get_filename_component( STDCXX_LIB_DIR ${STDCXX_LIBRARY_FILE} DIRECTORY )

list( APPEND EXTRA_LIBS fmt )

if( ENABLE_CACHE_MEMCACHE )
  list( APPEND EXTRA_LIBS memcached )
endif()

if( ENABLE_CAPNP )
  find_package( CapnProto REQUIRED )
  list( APPEND EXTRA_LIBS capnp kj )
  set( CAPNP_FLAG "True" )
  get_filename_component( CAPNP_LIB_CMAKE_DIR "${CapnProto_DIR}" DIRECTORY )
  get_filename_component( CAPNP_LIB_DIR "${CAPNP_LIB_CMAKE_DIR}" DIRECTORY )
else()
  set( CAPNP_FLAG "False" )
endif()

find_package( GSL )
if( GSL_FOUND )
  list( APPEND EXTRA_LIBS gsl )
endif()

get_filename_component( FMT_ROOT_DIR "${fmt_CONFIG}/../../../../" ABSOLUTE )
set( FMT_INCLUDE_DIR ${FMT_ROOT_DIR}/include )
find_library( FMT_LIB_FILE NAMES fmt
    HINTS
    ${FMT_ROOT_DIR}
    PATH_SUFFIXES lib lib64 )
get_filename_component( FMT_LIB_DIR ${FMT_LIB_FILE} DIRECTORY )

if( DEFINED TIRPC_INCLUDE_DIR )
  get_filename_component( MAYBE_TIRPC "${TIRPC_INCLUDE_DIR}" NAME )
  if( MAYBE_TIRPC STREQUAL "tirpc" )
     set( TIRPC_PYUDA_INCLUDE_DIR ${TIRPC_INCLUDE_DIR} )
  else()
     set( TIRPC_PYUDA_INCLUDE_DIR ${TIRPC_INCLUDE_DIR}/tirpc )
  endif()
endif()

configure_file( ${CMAKE_CURRENT_LIST_DIR}/setup.py.in ${CMAKE_CURRENT_BINARY_DIR}/setup.py @ONLY )

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/setup.py DESTINATION python_installer )
