file( GLOB CYTHON_FILES pyuda/cpyuda/*.pyx )

install( FILES ${CYTHON_FILES} pyuda/cpyuda/uda.pxd DESTINATION python_installer/pyuda/cpyuda )

file( GLOB INSTALL_FILES pyuda/*.py )

install( FILES ${INSTALL_FILES} DESTINATION python_installer/pyuda )

configure_file( ${CMAKE_CURRENT_LIST_DIR}/pyuda/_version.py.in ${CMAKE_CURRENT_BINARY_DIR}/pyuda/_version.py @ONLY )
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/pyuda/_version.py DESTINATION python_installer/pyuda )

find_package( OpenSSL REQUIRED )
find_package( LibXml2 REQUIRED )
find_package( LibMemcached QUIET )

set( UDA_LIBS )

if( LIBMEMCACHED_FOUND )
  set( EXTRA_LIBS "memcached" )
endif()
if( ENABLE_CAPNP )
  set( EXTRA_LIBS ${EXTRA_LIBS} capnp kj )
  set( UDA_LIBS ${UDA_LIBS} serialisation )
  set( CAPNP_FLAG "True" )
else()
  set( CAPNP_FLAG "False" )
endif()

get_filename_component( OPENSSL_LIB_DIR ${OPENSSL_SSL_LIBRARY} DIRECTORY )
get_filename_component( LIBXML_LIB_DIR ${LIBXML2_LIBRARIES} DIRECTORY )
configure_file( ${CMAKE_CURRENT_LIST_DIR}/setup.py.in ${CMAKE_CURRENT_BINARY_DIR}/setup.py @ONLY )

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/setup.py DESTINATION python_installer )