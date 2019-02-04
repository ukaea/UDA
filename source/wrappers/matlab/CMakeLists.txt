set( MATLAB_INCLUDE_DIR /usr/local/depot/octave-3.8.0/include/octave-3.8.0/octave )
set( MATLAB_LIBRARIES /usr/local/depot/octave-3.8.0/lib/octave/3.8.0/liboctave.so )

set( SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../.. )

set( LIB_NAME idammatlab )
set( MAJOR_VERSION 1 )
set( MINOR_VERSION 8 )

set( SRC_FILES getIdamData.c )

include_directories(
  ${SRC_DIR}/include
  ${SRC_DIR}/clientserver
  ${SRC_DIR}/structures
  ${SRC_DIR}/client
  ${MATLAB_INCLUDE_DIR}
)

add_library( ${LIB_NAME} SHARED ${SRC_FILES} )

target_link_libraries( ${LIB_NAME} LINK_PRIVATE ${LIB_DIR}/libidam.so ${MATLAB_LIBRARIES} )

set_target_properties( ${LIB_NAME}
  PROPERTIES
    PREFIX ""
    SUFFIX ".mexglx"
    OUTPUT_NAME getIdamData
    VERSION ${MAJOR_VERSION}.${MINOR_VERSION}
    SOVERSION ${MAJOR_VERSION}
)
