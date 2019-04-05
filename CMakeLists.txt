cmake_minimum_required( VERSION 3.0 )

########################################################################################################################
# CMake configuration

cmake_policy( SET CMP0048 NEW )
if( CMAKE_VERSION VERSION_GREATER 3.11 )
  cmake_policy( SET CMP0074 OLD )
  cmake_policy( SET CMP0075 OLD )
endif()

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
  message( "Failed to get git revision: ${GIT_TAG}" )
  set( GIT_TAG "0.0.0" )
endif()

# Strip off the SHA1 tag element if it exists
string( REGEX REPLACE "^([0-9]+\\.[0-9]+\\.[0-9]+).*" "\\1" GIT_VERSION "${GIT_TAG}" )
if( NOT "${GIT_VERSION}" MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+" )
  message( WARNING "invalid git tag for version parsing: ${GIT_VERSION}" )
  set( GIT_VERSION "0.0.0" )
endif()

project( uda VERSION ${GIT_VERSION} )

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

if( "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang" )
  set( CMAKE_MACOSX_RPATH ON )
  set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Qunused-arguments -undefined dynamic_lookup" )
  set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Qunused-arguments" )
endif()

if( CMAKE_VERSION VERSION_GREATER 3.0.9 )
  set( CMAKE_CXX_STANDARD 11 )
  set( CMAKE_C_STANDARD 90 )
else()
  if( NOT WIN32 OR MINGW )
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu89" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11" )
  endif()
endif()

if( NOT WIN32 OR MINGW )
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -pthread" )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -pthread" )
  
  if( GOPTION )
    message( "Compiling with g option to generate and embed debug information" )
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g" )
  endif() 
endif()

add_definitions(
  -DOSTYPE=${CMAKE_SYSTEM_NAME}
)

if( WIN32 OR MINGW )
  add_definitions( -D_CRT_SECURE_NO_WARNINGS )
  include_directories( ${CMAKE_SOURCE_DIR}/extlib/include/rpc )
  link_directories( ${CMAKE_SOURCE_DIR}/extlib/lib )
  link_libraries( -lportablexdr -lws2_32 )
  if ( MINGW )
    set(CMAKE_FIND_LIBRARY_PREFIXES "lib" "")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".dll.a" ".a" ".lib")
  endif()
endif()

if( ITER_CI )
  add_definitions(
    -D_XOPEN_SOURCE=700
    -D_POSIX_C_SOURCE=200809L
  )
endif()

########################################################################################################################
# Build options

option( CLIENT_ONLY "Only build UDA client" OFF )
option( NO_WRAPPERS "Don't build any UDA client wrappers" OFF )
option( NO_MODULES "Don't build any UDA server modules" OFF )
option( FAT_IDL "Build IDL wrapper using fatclient" OFF )

if( SSLAUTHENTICATION )
  message( "Including SSL/RTL X509 User Authentication" )
  add_definitions( -DSSLAUTHENTICATION )
endif() 

if( NOTGENERICENABLED )
  message( "No Generic name mapping service using the internal legacy Postgres plugin" )
  add_definitions( -DNOTGENERICENABLED )
endif()

########################################################################################################################
# Source directories

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

# Portablexdr 4.9.1 libraries
if( WIN32 OR MINGW )
  install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/extlib/lib/
    DESTINATION lib
	FILES_MATCHING PATTERN "libportablexdr*"
  )
endif()



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
