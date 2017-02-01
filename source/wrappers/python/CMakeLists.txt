cmake_minimum_required( VERSION 2.8 )

find_package( SWIG 3.0 REQUIRED )
include( ${SWIG_USE_FILE} )

find_package( PythonLibs 3.3 REQUIRED )
find_package( Boost REQUIRED )

set( SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../.. )

set( LIB_NAME c_${PROJECT_NAME} )

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_SOURCE_DIR}/source/wrappers/c++
  ${PYTHON_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
)

include_directories( SYSTEM "${Boost_INCLUDE_DIR}" )

add_definitions( -D__USE_XOPEN2K8 )

set_source_files_properties( Idam.i PROPERTIES CPLUSPLUS ON )
if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
  set_source_files_properties( Idam.i PROPERTIES SWIG_FLAGS "-c++;-py3;-DA64" )
else()
  set_source_files_properties( Idam.i PROPERTIES SWIG_FLAGS "-c++;-py3" )
endif()
set_source_files_properties( Idam.i PROPERTIES SWIG_MODULE_NAME ${LIB_NAME} )
swig_add_module( ${LIB_NAME} python Idam.i )
swig_link_libraries( ${LIB_NAME} ${PROJECT_NAME}_cpp ssl ${PYTHON_LIBRARIES} )

set_target_properties( ${SWIG_MODULE_${LIB_NAME}_REAL_NAME}
  PROPERTIES
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

install( TARGETS ${SWIG_MODULE_${LIB_NAME}_REAL_NAME}
  DESTINATION include/pyidam )

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/${LIB_NAME}.py
  DESTINATION include/pyidam )

file( GLOB INSTALL_FILES pyidam/*.py )

install( FILES ${INSTALL_FILES}
  DESTINATION include/pyidam )
