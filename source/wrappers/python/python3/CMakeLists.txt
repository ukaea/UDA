cmake_minimum_required( VERSION 2.8 )

find_package( SWIG 3.0 REQUIRED )
include( ${SWIG_USE_FILE} )

# Reset cache variables to force lookup of Python3 libs
unset( PYTHON_DEBUG_LIBRARY CACHE )
unset( PYTHON_LIBRARY CACHE )
unset( PYTHON_INCLUDE_DIR CACHE )

find_package( PythonLibs 3.3 REQUIRED )
find_package( Boost REQUIRED )
find_package( LibXml2 REQUIRED)

set( SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../.. )

set( LIB_NAME ${PROJECT_NAME}_python3 )

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_SOURCE_DIR}/source/wrappers/c++
  ${PYTHON_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
)

include_directories( SYSTEM "${Boost_INCLUDE_DIR}" )

add_definitions( -D__USE_XOPEN2K8 )

set_source_files_properties( ../UDA.i PROPERTIES CPLUSPLUS ON )
if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
  set_source_files_properties( ../UDA.i PROPERTIES SWIG_FLAGS "-c++;-py3;-DSWIGPYTHON3;-DA64" )
else()
  set_source_files_properties( ../UDA.i PROPERTIES SWIG_FLAGS "-c++;-py3;-DSWIGPYTHON3" )
endif()
set_source_files_properties( ../UDA.i PROPERTIES SWIG_MODULE_NAME ${LIB_NAME} )
swig_add_library( ${LIB_NAME} LANGUAGE python SOURCES ../UDA.i TYPE MODULE )
swig_link_libraries( ${LIB_NAME} cpp-static ssl ${PYTHON_LIBRARIES} ${LIBXML2_LIBRARIES} ${CACHE_LIBRARIES} )

set( INSTALL_DIR python/python${PYTHONLIBS_VERSION_STRING}/site-packages/pyuda )

install( TARGETS ${SWIG_MODULE_${LIB_NAME}_REAL_NAME}
  DESTINATION ${INSTALL_DIR} )

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/${LIB_NAME}.py
  DESTINATION ${INSTALL_DIR}
  RENAME ${PROJECT_NAME}_swig.py )

file( GLOB INSTALL_FILES ../pyuda/*.py )

install( FILES ${INSTALL_FILES}
  DESTINATION ${INSTALL_DIR} )
