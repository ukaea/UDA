cmake_minimum_required( VERSION 2.8 )

find_package( SWIG 3.0 REQUIRED )
find_package( PythonLibs 3.3 REQUIRED )
find_package( Boost REQUIRED )

set( ARGS
  -python
  -c++
  -DA64
  -I${CMAKE_SOURCE_DIR}/source
  -I${CMAKE_SOURCE_DIR}/source/wrappers/c++
  -outdir ${CMAKE_CURRENT_BINARY_DIR}
)

set( PY_WRAPPER_CODE UDA_wrap.python.cpp )
#set( PY3_WRAPPER_CODE UDA_wrap.python3.cpp )

add_custom_command(
  OUTPUT "${PY_WRAPPER_CODE}"
  COMMAND "${SWIG_EXECUTABLE}"
  ARGS
    ${ARGS}
    -o "${PY_WRAPPER_CODE}"
    ${CMAKE_CURRENT_SOURCE_DIR}/UDA.i
  MAIN_DEPENDENCY UDA.i
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Swig source"
)

#add_custom_command(
#  OUTPUT "${PY3_WRAPPER_CODE}"
#  COMMAND "${SWIG_EXECUTABLE}"
#  ARGS
#    ${ARGS}
#    -modern
#    -o "${PY3_WRAPPER_CODE}"
#    ${CMAKE_CURRENT_SOURCE_DIR}/UDA.i
#  MAIN_DEPENDENCY UDA.i
#  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
#  COMMENT "Swig source"
#)

add_custom_target(
  swig_wrapper ALL DEPENDS ${PY_WRAPPER_CODE}
)

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/c_uda.py
  DESTINATION python/pyuda )

file( GLOB INSTALL_FILES pyuda/*.py )

install( FILES ${INSTALL_FILES}
  DESTINATION python/pyuda )

configure_file( ${CMAKE_CURRENT_LIST_DIR}/setup.py.in ${CMAKE_CURRENT_BINARY_DIR}/setup.py @ONLY )

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PY_WRAPPER_CODE}
#    ${CMAKE_CURRENT_BINARY_DIR}/${PY3_WRAPPER_CODE}
    ${CMAKE_CURRENT_BINARY_DIR}/setup.py
  DESTINATION python )