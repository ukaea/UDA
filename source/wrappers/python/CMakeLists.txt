file( GLOB CYTHON_FILES pyuda/cpyuda/*.pyx )

install( FILES ${CYTHON_FILES} pyuda/cpyuda/uda.pxd DESTINATION python_installer/pyuda/cpyuda )

file( GLOB INSTALL_FILES pyuda/*.py )

install( FILES ${INSTALL_FILES} DESTINATION python_installer/pyuda )

configure_file( ${CMAKE_CURRENT_LIST_DIR}/pyuda/_version.py.in ${CMAKE_CURRENT_BINARY_DIR}/pyuda/_version.py @ONLY )
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/pyuda/_version.py DESTINATION python_installer/pyuda )

find_package( OpenSSL REQUIRED )
find_package( LibXml2 REQUIRED )
find_package( LibMemcached QUIET )
find_package( fmt REQUIRED )

set( UDA_LIBS ) 
set( EXTRA_LIBS ${EXTRA_LIBS} )

if( LIBMEMCACHED_FOUND )
  set( EXTRA_LIBS "memcached" )
endif()
if( ENABLE_CAPNP )
  find_package( CapnProto REQUIRED )
  set( EXTRA_LIBS ${EXTRA_LIBS} capnp kj )
  set( UDA_LIBS ${UDA_LIBS} serialisation )
  set( CAPNP_FLAG "True" )
  get_filename_component( CAPNP_LIB_CMAKE_DIR ${CapnProto_DIR} DIRECTORY )
  get_filename_component( CAPNP_LIB_DIR ${CAPNP_LIB_CMAKE_DIR} DIRECTORY )
else()
  set( CAPNP_FLAG "False" )
endif()

get_filename_component( FMT_ROOT_DIR "${fmt_CONFIG}/../../../../" ABSOLUTE)
set( FMT_INCLUDE_DIR ${FMT_ROOT_DIR}/include )
set( FMT_LIB_DIR ${FMT_ROOT_DIR}/lib64 )

get_filename_component( OPENSSL_LIB_DIR ${OPENSSL_SSL_LIBRARY} DIRECTORY )
get_filename_component( LIBXML_LIB_DIR ${LIBXML2_LIBRARIES} DIRECTORY )
configure_file( ${CMAKE_CURRENT_LIST_DIR}/setup.py.in ${CMAKE_CURRENT_BINARY_DIR}/setup.py @ONLY )

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/setup.py DESTINATION python_installer )