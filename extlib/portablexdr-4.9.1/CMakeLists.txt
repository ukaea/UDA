cmake_minimum_required( VERSION 2.8.11 )

cmake_policy( SET CMP0048 NEW )

project( xdr VERSION 4.9.1 )

if( NOT WIN32 )
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -fno-strict-aliasing" )
endif()

include( TestBigEndian )

if( WIN32 )
  add_definitions( -DBIG_ENDIAN=1 -DLITTLE_ENDIAN=2 )
  test_big_endian( BIG_ENDIAN )

  if( BIG_ENDIAN )
    add_definitions( -DBYTE_ORDER=1 )
  else()
    add_definitions( -DBYTE_ORDER=2 )
  endif()
endif()

set( SOURCES xdr_array.c xdr_float.c xdr_mem.c xdr_reference.c xdr.c xdr_intXX_t.c xdr_rec.c xdr_stdio.c )

configure_file(
  config.h.in
  ${CMAKE_BINARY_DIR}/config.h
)

include_directories( ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR} )

add_library( ${PROJECT_NAME} SHARED ${SOURCES} )

