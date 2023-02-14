cmake_minimum_required( VERSION 3.0 )

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
    string( STRIP ${${VarName}} ${VarName} )
    string( REGEX REPLACE "[\r\n]+" ";" ${VarName} ${${VarName}} )
  endif()
endmacro()

########################################################################################################################
# CMake configuration

cmake_policy( SET CMP0048 NEW )
if( CMAKE_VERSION VERSION_GREATER 3.12 )
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
include_directories( /opt/local/include )

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

set( USER $ENV{USER} )

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
  if( CMAKE_C_COMPILER_ID STREQUAL "Intel" )
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17" )
  else()
    set( CMAKE_CXX_STANDARD 17 )
    set( CMAKE_CXX_STANDARD_REQUIRED ON )
    # C++ extension = OFF to use STD instead GNU
    set( CMAKE_CXX_EXTENSIONS OFF )
    set( CMAKE_C_STANDARD 90 )
    set( CMAKE_C_STANDARD_REQUIRED ON )
  endif()
else()
  if( NOT WIN32 OR MINGW )
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11" )
  endif()
endif()

if( NOT WIN32 OR MINGW )
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -pthread -Wno-implicit-function-declaration" )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -pthread" )
  
  if( MINGW )
    add_definitions( -D_WIN32 )
    add_definitions( -DMINGW )
  endif()
  
  if( GOPTION )
    message( "Compiling with g option to generate and embed debug information" )
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g" )
  endif() 
endif()

if( WIN32 AND NOT MINGW )
  set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO" )
  set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO /SUBSYSTEM:CONSOLE" )
endif()

add_definitions(
  -DOSTYPE=${CMAKE_SYSTEM_NAME}
  -DUDA_EXPORT
)

if( WIN32 OR MINGW )
  add_definitions( -D_CRT_SECURE_NO_WARNINGS )
  set( CMAKE_FIND_LIBRARY_PREFIXES "lib" "" )
  set( CMAKE_FIND_LIBRARY_SUFFIXES ".dll.a" ".dll.lib" ".a" ".lib" )
  if( MINGW )
    set( IMPLIB_SUFFIX ".dll.a" )
  else()
    set( IMPLIB_SUFFIX ".dll.lib" )
  endif()
  find_package( XDR REQUIRED )
  
  # "-Wl,-z,defs" is an ELF instruction, not supported by Windows
  string( REGEX REPLACE "-Wl,-z,defs" "" CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}" )
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
option( SERVER_ONLY "Only build UDA server" OFF )
option( BUILD_SHARED_LIBS "Build shared libraries" OFF )
option( NO_MEMCACHE "Do not attempt to build with libmemcached support" OFF )

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

if( NOT CLIENT_ONLY AND NOT BUILD_SHARED_LIBS )
  message( FATAL_ERROR "cannot compile client AND server when BUILD_SHARED_LIBS is not set" )
endif()

if( SSLAUTHENTICATION )
  message( "Including SSL/RTL X509 User Authentication" )
  add_definitions( -DSSLAUTHENTICATION )
endif() 

if( BUILD_SHARED_LIBS )
  message( "Build shared libraries asked" )
endif()

# Server config options
set (UDA_SERVER_PORT "56565" CACHE string "define port number in server configuration files")
set (UDA_SERVER_HOST "`hostname`" CACHE string "define hostname in server configuration files")
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
