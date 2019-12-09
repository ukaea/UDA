function( cat IN_FILE OUT_FILE )
  file( READ ${IN_FILE} CONTENTS )
  file( APPEND ${OUT_FILE} "${CONTENTS}" )
endfunction()

file( GLOB CYTHON_FILES pyuda/cpyuda/*.pyx )
set( CPYTHON_INSTALL_FILE ${CMAKE_CURRENT_BINARY_DIR}/cpyuda.pyx )

file( WRITE ${CPYTHON_INSTALL_FILE} "" )
foreach( CYTHON_FILE ${CYTHON_FILES} )
  cat( ${CYTHON_FILE} ${CPYTHON_INSTALL_FILE} )
endforeach()

install( FILES ${CPYTHON_INSTALL_FILE} pyuda/cpyuda/uda.pxd DESTINATION python_installer/pyuda/cpyuda )

file( GLOB INSTALL_FILES pyuda/*.py )

install( FILES ${INSTALL_FILES} DESTINATION python_installer/pyuda )

find_package( OpenSSL REQUIRED )

get_filename_component( OPENSSL_LIB_DIR ${OPENSSL_SSL_LIBRARY} DIRECTORY )
configure_file( ${CMAKE_CURRENT_LIST_DIR}/setup.py.in ${CMAKE_CURRENT_BINARY_DIR}/setup.py @ONLY )

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/setup.py DESTINATION python_installer )
