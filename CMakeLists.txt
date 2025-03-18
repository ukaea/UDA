########################################################################################################################
# CMake configuration

cmake_minimum_required( VERSION 3.23 )

if( ${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.25 )
  cmake_policy( SET CMP0140 NEW )
endif()
if( ${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.30 )
  cmake_policy( SET CMP0167 NEW )
endif()

list( APPEND CMAKE_MODULE_PATH
  ${CMAKE_SOURCE_DIR}/cmake
  ${CMAKE_SOURCE_DIR}/cmake/Config
  ${CMAKE_SOURCE_DIR}/cmake/Modules
)

########################################################################################################################
# Version and Machine information

# Set VERSION and FULL_VERSION from `git describe`
# but use git export attribute for release tarballs
set( GIT_ARCHIVE_DESCRIBE [[$Format:%(describe)$]] )
include( DetermineVersion )

project( uda VERSION ${VERSION} )

########################################################################################################################
# Build configuration

set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
# C++ extension = OFF to use STD instead GNU
set( CMAKE_CXX_EXTENSIONS OFF )
set( CMAKE_C_STANDARD 90 )
set( CMAKE_C_STANDARD_REQUIRED ON )

cmake_host_system_information( RESULT _Is64Bit QUERY IS_64BIT )

if( _Is64Bit )
  message( "-- 64 bit system detected" )
  add_definitions( -DA64 )
endif()

message( "-- Including configuration for system '${CMAKE_SYSTEM_NAME}'" )
include( Config${CMAKE_SYSTEM_NAME} OPTIONAL RESULT_VARIABLE _SystemConfig )
if( "${_SystemConfig}" STREQUAL "NOTFOUND" )
  message( "Config${CMAKE_SYSTEM_NAME}.cmake not found" )
endif(  )

message( "-- Including configuration for compiler '${CMAKE_CXX_COMPILER_ID}'" )
include( Config${CMAKE_CXX_COMPILER_ID} OPTIONAL RESULT_VARIABLE _CompilerConfig )
if( "${_CompilerConfig}" STREQUAL "NOTFOUND" )
  message( "Config${CMAKE_CXX_COMPILER_ID}.cmake not found" )
endif(  )

cmake_host_system_information( RESULT _HostName QUERY HOSTNAME )

message( "-- Including configuration for hostname '${_HostName}'" )
include( Config${_HostName} OPTIONAL RESULT_VARIABLE _HostConfig )
if( "${_HostConfig}" STREQUAL "NOTFOUND" )
  message( "Config${_HostName}.cmake not found" )
endif(  )

########################################################################################################################
# Build options

option( ENABLE_CACHE_MEMCACHE "enable memcache caching" OFF )
option( ENABLE_CAPNP "enable capnproto serialisation" ON )
option( ENABLE_SSL "enable ssl authentication" ON )

option( ENABLE_CXX_WRAPPER "Turn off building of C++ wrapper" ON )
option( ENABLE_PYTHON_WRAPPER "Turn off building of Python wrapper" ON )
option( ENABLE_IDL_WRAPPER "Turn off building of IDL wrapper" OFF )

# Defaults for server run scripts
set( UDA_SERVER_PORT 56565 )
set( UDA_SERVER_USER $ENV{USER} )

########################################################################################################################
# Configure files

string( TIMESTAMP BUILD_TIMESTAMP UTC )
configure_file(
  ${CMAKE_SOURCE_DIR}/include/uda/version.h.in
  ${CMAKE_BINARY_DIR}/include/uda/version.h
  @ONLY
)

########################################################################################################################
# enable testing using ctest

enable_testing()

########################################################################################################################
# Source directories

add_subdirectory( source )

########################################################################################################################
# Test directories

add_subdirectory( test )

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
# Installation

include( CMakePackageConfigHelpers )
write_basic_package_version_file(
  udaConfigVersion.cmake
  VERSION ${PACKAGE_VERSION}
  COMPATIBILITY SameMajorVersion
)

set( HEADERS
  include/uda.h
  include/uda/client.h
  include/uda/export.h
  include/uda/legacy.h
  include/uda/plugins.h
  include/uda/structured.h
  include/uda/types.h
  include/uda/uda_plugin_base.hpp
  include/uda/portability.h
  ${CMAKE_BINARY_DIR}/include/uda/version.h
)
if( ENABLE_CAPNP )
  install( FILES include/uda/serialisation/capnp_serialisation.h DESTINATION include/uda/serialisation )
endif()
install( FILES ${HEADERS} DESTINATION include/uda )

install( FILES
  ${CMAKE_CURRENT_BINARY_DIR}/udaConfigVersion.cmake
  ${CMAKE_SOURCE_DIR}/cmake/udaConfig.cmake
  ${CMAKE_SOURCE_DIR}/cmake/udaPlugins.cmake
  DESTINATION lib/cmake/uda
)

install( EXPORT udaClientTargets
  FILE udaClientTargets.cmake
  NAMESPACE
    uda::
  DESTINATION
    lib/cmake/uda
)

install( EXPORT udaPluginTargets
  FILE udaPluginTargets.cmake
  NAMESPACE
    uda::
  DESTINATION
    lib/cmake/uda
)

if( ENABLE_CXX_WRAPPER )
  install( EXPORT udaCxxTargets
    FILE udaCxxTargets.cmake
    NAMESPACE
      uda::
    DESTINATION
      lib/cmake/uda
  )
endif()

########################################################################################################################
# Macro used to convert Windows path to Unix
macro( CONVERT_WIN_PATH VarName )
  if( MINGW AND NOT "${${VarName}}" STREQUAL "" )
    execute_process( COMMAND cygpath.exe -u ${${VarName}} OUTPUT_VARIABLE ${VarName} )
    string( STRIP ${${VarName}} ${VarName} )
    string( REGEX REPLACE "[\r\n]+" ";" ${VarName} ${${VarName}} )
  endif()
endmacro()

# Macro used to convert Unix path to Windows
macro( CONVERT_LIN_PATH VarName )
  if( MINGW AND NOT "${${VarName}}" STREQUAL "" )
    execute_process( COMMAND cygpath.exe -m ${${VarName}} OUTPUT_VARIABLE ${VarName} )
    string( STRIP "${${VarName}}" ${VarName} )
    string( REGEX REPLACE "[\r\n]+" ";" ${VarName} "${${VarName}}" )
  endif()
endmacro()

########################################################################################################################
# Package creations

include( InstallRequiredSystemLibraries )

set( CPACK_PACKAGE_DESCRIPTION_SUMMARY "Universal Data Access library" )
set( CPACK_PACKAGE_VENDOR "ITER" )
set( CPACK_PACKAGE_CONTACT "jonathan.hollocombe@ukaea.uk" )
set( CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}" )
set( CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}" )
set( CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}" )
set( CPACK_PACKAGE_INSTALL_DIRECTORY "uda-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}" )
set( CPACK_PACKAGE_FILE_NAME "uda-install-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}" )
set( CPACK_BUNDLE_NAME uda )
set( CPACK_BUNDLE_PLIST ${CMAKE_SOURCE_DIR}/Info.plist )
set( CPACK_BUNDLE_ICON ${CMAKE_SOURCE_DIR}/cmake/uda.icns )
if( WIN32 OR MINGW )
  set( CMAKE_WIN_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} )
  set( CMAKE_WIN_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} )
  set( CMAKE_WIN_SOURCE_DIR ${CMAKE_SOURCE_DIR} )
  set( CMAKE_WIN_BINARY_DIR  ${CMAKE_BINARY_DIR} )
  if( MINGW )
    convert_lin_path( CMAKE_WIN_CURRENT_SOURCE_DIR )
    convert_lin_path( CMAKE_WIN_INSTALL_PREFIX )
    convert_lin_path( CMAKE_WIN_SOURCE_DIR )
    convert_lin_path( CMAKE_WIN_BINARY_DIR )
    # For MinGW, it is required to use Windows style path for the NSIS command "File"
    # otherwise it is not able to find files
    set( CPACK_NSIS_FULL_INSTALL "File /r \\\"\\\${INST_DIR_WIN}\\\\*.*\\\"" )
  endif()

  set( CPACK_TEMPORARY_INSTALL_DIRECTORY "${CMAKE_BINARY_DIR}/cpack" )
  set( CPACK_PACKAGE_DIRECTORY "${CMAKE_WIN_BINARY_DIR}" )
  set( CPACK_TEMPORARY_DIRECTORY "${CMAKE_BINARY_DIR}/cpack" )
  set( CPACK_WIN_TEMPORARY_DIRECTORY "${CMAKE_WIN_BINARY_DIR}/cpack" )
  set( CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_WIN_CURRENT_SOURCE_DIR}/README.md" )
  set( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_WIN_CURRENT_SOURCE_DIR}/LICENCE.txt" )
  set( CPACK_NSIS_INSTALLED_ICON_NAME "bin/uda_server.exe" )
  set( CPACK_NSIS_DISPLAY_NAME "Universal Data Access v${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}" )
  # Windows trick: Replace last / by \\ otherwise bitmap extraction failed
  set( CPACK_PACKAGE_ICON "${CMAKE_WIN_SOURCE_DIR}/cmake\\\\icon.bmp" )
  set( CPACK_NSIS_HELP_LINK "http://www.iter.org/uda.html" )
  set( CPACK_NSIS_URL_INFO_ABOUT "http://www.iter.org/uda.html" )
  set( CPACK_NSIS_CONTACT "Jonathan.Hollocombe@iter.org" )
  set( CPACK_NSIS_MODIFY_PATH ON )
else()
  set( CPACK_TEMPORARY_INSTALL_DIRECTORY "${CMAKE_BINARY_DIR}/cpack" )
  set( CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}" )
  set( CPACK_TEMPORARY_DIRECTORY "${CMAKE_BINARY_DIR}/cpack" )
  set( CPACK_WIN_TEMPORARY_DIRECTORY "${CMAKE_BINARY_DIR}/cpack" )
  set( CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md" )
  set( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENCE.txt" )
  set( CPACK_STRIP_FILES "bin/uda_server" )
  set( CPACK_SOURCE_STRIP_FILES "" )

  set( CPACK_RPM_PACKAGE_NAME "uda" )
  set( CPACK_RPM_PACKAGE_SUMMARY "UDA RPM package" )
  set( CPACK_RPM_FILE_NAME RPM-DEFAULT )
  set( CPACK_RPM_PACKAGE_VERSION CPACK_PACKAGE_VERSION )
  set( CPACK_RPM_PACKAGE_ARCHITECTURE noarch )
  set( CPACK_RPM_PACKAGE_RELEASE 1 )
  set( CPACK_RPM_PACKAGE_LICENSE "Apache, version 2" )
endif()
set( CPACK_PACKAGE_EXECUTABLES "uda_server" "UDA Server" )

include( CPack )
