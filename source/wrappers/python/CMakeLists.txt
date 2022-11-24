file( GLOB CYTHON_FILES pyuda/cpyuda/*.pyx )
set( CPYTHON_INSTALL_FILE ${CMAKE_CURRENT_BINARY_DIR}/cpyuda.pyx )

add_custom_target( cpyuda ALL DEPENDS ${CPYTHON_INSTALL_FILE} )
add_custom_command(
  OUTPUT ${CPYTHON_INSTALL_FILE}
  DEPENDS ${CYTHON_FILES}
  COMMAND cat ${CYTHON_FILES} > ${CPYTHON_INSTALL_FILE}
  COMMAND ${CMAKE_COMMAND} -E touch ${CPYTHON_INSTALL_FILE}
)

install( FILES ${CPYTHON_INSTALL_FILE} pyuda/cpyuda/uda.pxd DESTINATION python_installer/pyuda/cpyuda )

file( GLOB INSTALL_FILES pyuda/*.py )

install( FILES ${INSTALL_FILES} DESTINATION python_installer/pyuda )

configure_file( ${CMAKE_CURRENT_LIST_DIR}/pyuda/_version.py.in ${CMAKE_CURRENT_BINARY_DIR}/pyuda/_version.py @ONLY )
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/pyuda/_version.py DESTINATION python_installer/pyuda )

find_package( OpenSSL REQUIRED )
find_package( LibXml2 REQUIRED )
find_package( LibMemcached QUIET )

if( LIBMEMCACHED_FOUND )
  set( EXTRA_LIBS "memcached" )
endif()

get_filename_component( OPENSSL_LIB_DIR ${OPENSSL_SSL_LIBRARY} DIRECTORY )
get_filename_component( LIBXML_LIB_DIR ${LIBXML2_LIBRARIES} DIRECTORY )
configure_file( ${CMAKE_CURRENT_LIST_DIR}/setup.py.in ${CMAKE_CURRENT_BINARY_DIR}/setup.py @ONLY )

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/setup.py DESTINATION python_installer )