cmake_minimum_required( VERSION 3.0 )

cmake_policy( SET CMP0048 NEW )

project( idam VERSION 1.8.0 )

if( APPLE )
  set( CMAKE_MACOSX_RPATH ON )
endif()

#Dont inherit the LDFLAGS env variable
set( CMAKE_EXE_LINKER_FLAGS "" )
set( CMAKE_SHARED_LINKER_FLAGS "" )

if( APPLE )
  set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup" )
endif( APPLE )

cmake_host_system_information( RESULT IDAM_HOST QUERY FQDN )

set( TARGET_TYPE "NONE" CACHE STRING "Choose target [JET;MAST;OTHER]" )

# Extra cmake modules to find libraries
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake;${CMAKE_SOURCE_DIR}/cmake/Modules" )

option( DEBUG "Debug info" ON )
if( DEBUG )
  add_definitions( -DDEBUG )
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -Wno-missing-braces" )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Werror" )
  if( CMAKE_C_COMPILER_ID STREQUAL "GNU" )
    execute_process( COMMAND gcc --version OUTPUT_VARIABLE GCC_VERSION_STRING )
    string( REGEX MATCH "[0-9]+\\.[0-9]\\.[0-9]" GCC_VERSION ${GCC_VERSION_STRING} )
    if ( GCC_VERSION VERSION_GREATER 4.4 )
      set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -gdwarf-3" )
      set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gdwarf-3" )
    endif()
  endif()
endif( DEBUG )

option( FAT_BUILD "Build fat client" OFF )
if( FAT_BUILD )
  add_definitions( -DFATCLIENT )
  set( IDAM_NAME idamfat CACHE STRING "Idam library name" )
else()
  set( IDAM_NAME idam CACHE STRING "Idam library name" )
endif()

########################################
# 32/64-bit compiler

if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
  message( "64 bit compiler detected" )
  set( EX_PLATFORM 64 )
  set( EX_PLATFORM_NAME "x64" )
  set( IDC_LIB_NAME "${IDAM_NAME}64" CACHE STRING "Idam client library name" )
  set( IDS_LIB_NAME "${IDAM_NAME}server64" CACHE STRING "Idam server library name" )
  add_definitions( -DA64 )
else()
  message( "32 bit compiler detected" )
  set( EX_PLATFORM 32 )
  set( EX_PLATFORM_NAME "x86" )
  set( IDC_LIB_NAME "${IDAM_NAME}" CACHE STRING "Idam client library name" )
  set( IDS_LIB_NAME "${IDAM_NAME}server" CACHE STRING "Idam server library name" )
endif()

set( EXTRA_LD_LIBRARY_PATHS "" CACHE INTERNAL "" )

option( CLIENT_ONLY "Only build IDAM client" OFF )
option( NO_WRAPPERS "Don't build any IDAM client wrappers" OFF )

add_subdirectory( cache )
add_subdirectory( logging )
add_subdirectory( client )

if( NOT NO_WRAPPERS )
  add_subdirectory( wrappers )
endif()

if( FAT_BUILD OR NOT CLIENT_ONLY )
  add_subdirectory( clientserver )
  add_subdirectory( server )
  add_subdirectory( plugins )
endif()
