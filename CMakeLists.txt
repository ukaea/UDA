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

include( ColourMessages )

########################################################################################################################
# Version and Machine information

include( GetGitRevisionDescription )

git_describe( GIT_TAG --tags )

if( "${GIT_TAG}" MATCHES "^.*NOTFOUND$" )
  message( FATAL_ERROR "Failed to get git revision: ${GIT_TAG}" )
endif()

# Strip off the SHA1 tag element if it exists
string( REGEX REPLACE "^([0-9]+\\.[0-9]+\\.[0-9]+).*" "\\1" GIT_VERSION "${GIT_TAG}" )

project( uda VERSION ${GIT_VERSION} )

cmake_host_system_information( RESULT UDA_HOST QUERY FQDN )

set( USER $ENV{USER} )

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

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++11" )

add_definitions(
  -DOSTYPE=${CMAKE_SYSTEM_NAME}
)

if( NOT APPLE )
  add_definitions(
    -D_XOPEN_SOURCE=700
    -D_POSIX_C_SOURCE=200809L
  )
endif()

########################################################################################################################
# Build options

option( CLIENT_ONLY "Only build IDAM client" OFF )
option( NO_WRAPPERS "Don't build any IDAM client wrappers" OFF )

########################################################################################################################
# Source directories

#add_subdirectory( extlib )
add_subdirectory( source )
if( NOT CLIENT_ONLY )
  add_subdirectory( test )
endif()

########################################################################################################################
# Create module

configure_file(
  ${CMAKE_SOURCE_DIR}/source/etc/module.in
  ${CMAKE_BINARY_DIR}/etc/uda/${PROJECT_VERSION}
  @ONLY
)

install(
  FILES ${CMAKE_BINARY_DIR}/etc/uda/${PROJECT_VERSION}
  DESTINATION modulefiles/uda
)

########################################################################################################################
# Package creations

include( InstallRequiredSystemLibraries )

set( CPACK_PACKAGE_DESCRIPTION_SUMMARY "Universal Data Access library" )
set( CPACK_PACKAGE_VENDOR "ITER" )
set( CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md" )
set( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENCE.txt" )
set( CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}" )
set( CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}" )
set( CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}" )
set( CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMAKE_VERSION_MAJOR}.${CMAKE_VERSION_MINOR}" )
if( WIN32 AND NOT UNIX )
  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backlasshes.
  set( CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/Utilities/Release\\\\InstallIcon.bmp" )
  set( CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\uda_server.exe" )
  set( CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} Universal Data Access" )
  set( CPACK_NSIS_HELP_LINK "http:\\\\\\\\www.iter.org/uda.html" )
  set( CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.iter.org/uda.html" )
  set( CPACK_NSIS_CONTACT "Jonathan.Hollocombe@iter.org" )
  set( CPACK_NSIS_MODIFY_PATH ON )
else( WIN32 AND NOT UNIX )
  set( CPACK_STRIP_FILES "bin/uda_server" )
  set( CPACK_SOURCE_STRIP_FILES "" )
endif( WIN32 AND NOT UNIX )
set( CPACK_PACKAGE_EXECUTABLES "uda_server" "UDA Server" )

include( CPack )
