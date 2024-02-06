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

file( GLOB INSTALL_FILES pyuda/*.py )

install( FILES ${INSTALL_FILES} DESTINATION python_installer/pyuda )

configure_file( ${CMAKE_CURRENT_LIST_DIR}/pyuda/_version.py.in ${CMAKE_CURRENT_BINARY_DIR}/pyuda/_version.py @ONLY )
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/pyuda/_version.py DESTINATION python_installer/pyuda )

find_package( OpenSSL REQUIRED )
find_package( LibXml2 REQUIRED )
find_package( LibMemcached QUIET )
find_package( fmt REQUIRED )
find_package( TIRPC QUIET )

find_library( STDCXX_LIBRARY_FILE stdc++ HINTS ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES} )
get_filename_component(STDCXX_LIB_DIR ${STDCXX_LIBRARY_FILE} DIRECTORY )

set( UDA_LIBS )
set( EXTRA_LIBS ${EXTRA_LIBS} fmt )

if( LIBMEMCACHED_FOUND )
  set( EXTRA_LIBS ${EXTRA_LIBS} memcached )
endif()
if( ENABLE_CAPNP )
  find_package( CapnProto REQUIRED )
  set( EXTRA_LIBS ${EXTRA_LIBS} capnp kj )
  set( UDA_LIBS ${UDA_LIBS} serialisation )
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
set( FMT_LIB_DIR ${FMT_ROOT_DIR}/lib64 )

get_filename_component( OPENSSL_LIB_DIR "${OPENSSL_SSL_LIBRARY}" DIRECTORY )
get_filename_component( LIBXML_LIB_DIR "${LIBXML2_LIBRARIES}" DIRECTORY )

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
