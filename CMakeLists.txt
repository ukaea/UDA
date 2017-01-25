cmake_minimum_required( VERSION 3.0 )

########################################################################################################################
# CMake configuration

cmake_policy( SET CMP0048 NEW )

# Extra cmake modules to find libraries
set( CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${CMAKE_SOURCE_DIR}/cmake
  ${CMAKE_SOURCE_DIR}/cmake/Modules
  ${CMAKE_SOURCE_DIR}/cmake/Config
)

########################################################################################################################
# Version and Machine information

include( GetGitRevisionDescription )

git_describe( GIT_TAG --tags )

# Strip off the SHA1 tag element if it exists
string( REGEX REPLACE "^([0-9]+\\.[0-9]+\\.[0-9]+).*" "\\1" GIT_VERSION "${GIT_TAG}" )

project( uda VERSION ${GIT_VERSION} )

cmake_host_system_information( RESULT UDA_HOST QUERY FQDN )

########################################################################################################################
# Target type

set( TARGET_TYPE "NONE" CACHE STRING "Choose target [JET;MAST;OTHER]" )

if( NOT ${TARGET_TYPE} STREQUAL "NONE" )
  include( "target-${TARGET_TYPE}" )
else()
  message( FATAL_ERROR "INVALID TARGET_TYPE: ${TARGET_TYPE}" )
endif()

########################################################################################################################
# Compiler configuration

if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
  message( "64 bit compiler detected" )
  set( EX_PLATFORM 64 )
  set( EX_PLATFORM_NAME "x64" )
  add_definitions( -DA64 )
else()
  message( "32 bit compiler detected" )
  set( EX_PLATFORM 32 )
  set( EX_PLATFORM_NAME "x86" )
endif()

if( APPLE )
  set( CMAKE_MACOSX_RPATH ON )
  set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup" )
endif( APPLE )

add_definitions(
  -Wall
  -Werror
)

########################################################################################################################
# Build options

option( FAT_BUILD "Build fat client" OFF )
if( FAT_BUILD )
  add_definitions( -DFATCLIENT )
  set( IDAM_NAME idamfat CACHE STRING "Idam library name" )
else()
  set( IDAM_NAME idam CACHE STRING "Idam library name" )
endif()

option( CLIENT_ONLY "Only build IDAM client" OFF )
option( NO_WRAPPERS "Don't build any IDAM client wrappers" OFF )

########################################################################################################################
# Source directories

add_subdirectory( source )
#add_subdirectory( test )
