cmake_minimum_required( VERSION 3.13 )

########################################################################################################################
# Cmake policies

cmake_policy( SET CMP0048 NEW )
cmake_policy( SET CMP0074 NEW )
cmake_policy( SET CMP0075 NEW )
cmake_policy( SET CMP0076 NEW )

########################################################################################################################
# Cmake configuration

# Extra cmake modules to find libraries
set(
  CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${CMAKE_SOURCE_DIR}/cmake
  ${CMAKE_SOURCE_DIR}/cmake/Modules
)

include( ColourMessages )

########################################################################################################################
# Version and Machine information

include( GetGitRevisionDescription )

git_describe( GIT_TAG --tags )

if( "${GIT_TAG}" MATCHES "^.*NOTFOUND$" )
  message( WARNING "Failed to get git revision: ${GIT_TAG}" )
  set( GIT_VERSION "0.0.0" )
elseif( "${GIT_TAG}" MATCHES "^([0-9]+\\.[0-9]+\\.[0-9]+)$" )
  set( GIT_VERSION "${GIT_TAG}" )
elseif( "${GIT_TAG}" MATCHES "^([0-9]+\\.[0-9]+\\.[0-9]+)-([0-9]+)-.*$" )
  set( GIT_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}" )
else()
  message( WARNING "invalid git tag for version parsing: ${GIT_TAG}" )
  set( GIT_VERSION "0.0.0" )
endif()

project( uda VERSION ${GIT_VERSION} )

########################################################################################################################
# Build options

option( CLIENT_ONLY "Only build UDA client" OFF )
option( SERVER_ONLY "Only build UDA server" OFF )
option( BUILD_SHARED_LIBS "Build shared libraries" OFF )
option( NO_MEMCACHE "Do not attempt to build with libmemcached support" OFF )
option( ENABLE_CAPNP "Enable Cap’n Proto serialisation" OFF )
option( SSLAUTHENTICATION "Enable SSL/RTL X509 User Authentication" OFF )

# Wrapper configuration
option( NO_WRAPPERS "Don't build any UDA client wrappers" OFF )
option( NO_CXX_WRAPPER "Don't build C++ wrapper" OFF )
option( NO_PYTHON_WRAPPER "Don't build Python wrapper" OFF )
option( NO_JAVA_WRAPPER "Don't build Java wrapper" OFF )
option( NO_IDL_WRAPPER "Don't build IDL wrapper" OFF )
option( FAT_IDL "Build IDL wrapper using fat-client" OFF )

if( SERVER_ONLY )
  if( CLIENT_ONLY OR NO_WRAPPERS OR FAT_IDL )
    message( FATAL_ERROR "cannot use CLIENT_ONLY, NO_WRAPPERS or FAT_IDL when configuring with SERVER_ONLY" )
  endif()
endif()

# Server config options
set( LOGGING_BACKEND "FILE" CACHE STRING "Server logging backend" )
set( UDA_SERVER_PORT "56565" CACHE STRING "define port number in server configuration files" )
set( UDA_SERVER_HOST "`hostname`" CACHE STRING "define hostname in server configuration files" )

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
    FILES_MATCHING PATTERN "*xdr*"
  )
endif()

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
    CONVERT_LIN_PATH( CMAKE_WIN_CURRENT_SOURCE_DIR )
    CONVERT_LIN_PATH( CMAKE_WIN_INSTALL_PREFIX )
    CONVERT_LIN_PATH( CMAKE_WIN_SOURCE_DIR )
    CONVERT_LIN_PATH( CMAKE_WIN_BINARY_DIR )
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
