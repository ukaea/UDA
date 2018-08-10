cmake_minimum_required( VERSION 2.8 )

find_package( SWIG 3.0 REQUIRED )
find_package( Boost REQUIRED )

if( WIN32 )
  # pip install not working on Windows so build full wrapper and install manually

  include( ${SWIG_USE_FILE} ) # for swig macros
  find_package( PythonLibs 3.3 REQUIRED )

  include_directories(
    ${CMAKE_SOURCE_DIR}/source
    ${CMAKE_SOURCE_DIR}/source/wrappers/c++
    ${PYTHON_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
  )

  include_directories( SYSTEM "${Boost_INCLUDE_DIR}" )

  set( LIB_NAME c_${PROJECT_NAME} )

  if( MINGW )
    add_definitions( -D_hypot=hypot )
  endif()

  add_definitions( -D__USE_XOPEN2K8 )

  set_source_files_properties( UDA.i PROPERTIES CPLUSPLUS ON )
  if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    set_source_files_properties( UDA.i PROPERTIES SWIG_FLAGS "-c++;-py3;-DA64" )
  else()
    set_source_files_properties( UDA.i PROPERTIES SWIG_FLAGS "-c++;-py3" )
  endif()
  set_source_files_properties( UDA.i PROPERTIES SWIG_MODULE_NAME ${LIB_NAME} )
  swig_add_module( ${LIB_NAME} python UDA.i )
  swig_link_libraries( ${LIB_NAME} ${PROJECT_NAME}_cpp ${PYTHON_LIBRARIES} ${CACHE_LIBRARIES} )

  install( TARGETS ${SWIG_MODULE_${LIB_NAME}_REAL_NAME}
    DESTINATION include/pyuda
  )

  install( FILES ${CMAKE_CURRENT_BINARY_DIR}/${LIB_NAME}.py
    DESTINATION include/pyuda
  )

  file( GLOB INSTALL_FILES pyuda/*.py )

  install( FILES ${INSTALL_FILES}
    DESTINATION include/pyuda
  )
else()
  set( PY_WRAPPER_CODE UDA_wrap.python.cpp )

  set( ARGS
    -python
    -c++
    -DA64
    -I${CMAKE_SOURCE_DIR}/source
    -I${CMAKE_SOURCE_DIR}/source/wrappers/c++
    -outdir ${CMAKE_CURRENT_BINARY_DIR}
    -o "${PY_WRAPPER_CODE}"
    ${CMAKE_CURRENT_SOURCE_DIR}/UDA.i
  )

  add_custom_command(
    OUTPUT "${PY_WRAPPER_CODE}"
    COMMAND "${SWIG_EXECUTABLE}"
    ARGS ${ARGS}
    MAIN_DEPENDENCY UDA.i
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Swig source"
  )

  add_custom_target(
    swig_wrapper ALL DEPENDS ${PY_WRAPPER_CODE}
  )

  install( FILES ${CMAKE_CURRENT_BINARY_DIR}/c_uda.py
    DESTINATION python_installer/pyuda
  )

  file( GLOB INSTALL_FILES pyuda/*.py )

  install( FILES ${INSTALL_FILES}
    DESTINATION python_installer/pyuda
  )

  find_package( OpenSSL REQUIRED )

  get_filename_component( OPENSSL_LIB_DIR ${OPENSSL_SSL_LIBRARY} DIRECTORY )
  configure_file( ${CMAKE_CURRENT_LIST_DIR}/setup.py.in ${CMAKE_CURRENT_BINARY_DIR}/setup.py @ONLY )

  install(
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PY_WRAPPER_CODE}
    ${CMAKE_CURRENT_BINARY_DIR}/setup.py
    DESTINATION python_installer
  )
endif()
