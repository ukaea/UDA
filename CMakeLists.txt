cmake_minimum_required( VERSION 3.12 )

########################################################################################################################
# CMake configuration

cmake_policy( SET CMP0048 NEW )
if( CMAKE_VERSION VERSION_GREATER 3.12 )
  cmake_policy( SET CMP0074 OLD )
  cmake_policy( SET CMP0075 OLD )
endif()

if( CMAKE_VERSION VERSION_GREATER 3.30 )
    cmake_policy( SET CMP0167 NEW )
    cmake_policy( SET CMP0057 NEW )
endif()

# Extra cmake modules to find libraries
set( CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${CMAKE_SOURCE_DIR}/cmake
  ${CMAKE_SOURCE_DIR}/cmake/Modules
  ${CMAKE_SOURCE_DIR}/cmake/Config
)
list( APPEND CMAKE_PREFIX_PATH ${_VCPKG_INSTALLED_DIR}/x64-mingw-static )

#include( ColourMessages )
include_directories( /opt/local/include )

set( Boost_USE_MULTITHREADED OFF )

########################################################################################################################
# Version and Machine information

# Set VERSION and FULL_VERSION from `git describe`
# but use git export attribute for release tarballs 
set( GIT_ARCHIVE_DESCRIBE [[$Format:%(describe)$]] )
include( DetermineVersion )

project( uda VERSION ${VERSION} )

# Discover the thread flags before the legacy compiler flags add -pthread.
# The installed CMake package and pkg-config consumers have their own flags.
find_package( Threads REQUIRED )

set( USER $ENV{USER} )

include( UDAInstallDirs )
include( UDALibrary )

# GCC before 9.1 and Clang before 9.0 keep std::filesystem in a separate library.
set( UDA_STDFS_LIBRARY "" )
if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 9.1 )
  set( UDA_STDFS_LIBRARY "stdc++fs" )
elseif( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 9.0 )
  set( UDA_STDFS_LIBRARY "c++fs" )
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
  if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "IntelLLVM" )
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17" )
  else()
    if( WIN32 AND MSVC )
        set( CMAKE_CXX_STANDARD 20 )
    else()
        set( CMAKE_CXX_STANDARD 17 )
    endif()
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

    if( NOT APPLE AND NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "IntelLLVM") )
      set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-use-after-free" )
    endif()
  endif()
endif()

# Static archives are linked into shared libraries and Python extensions, so build everything as
# position-independent code. (No effect on Windows, where code is always relocatable.)
set( CMAKE_POSITION_INDEPENDENT_CODE ON )

if( NOT WIN32 )
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -Wno-implicit-function-declaration" )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread" )
  
  if( MINGW )
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
  add_definitions( -D_CRT_SECURE_NO_WARNINGS -DNOMINMAX )
  set( CMAKE_FIND_LIBRARY_PREFIXES "lib" "" )
  set( CMAKE_FIND_LIBRARY_SUFFIXES ".dll.a" ".dll.lib" ".a" ".lib" )
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
option( NO_MEMCACHE "Do not attempt to build with libmemcached support" ON )
option( ENABLE_CAPNP "Enable Cap’n Proto serialisation" OFF )
option( ENABLE_LANDLOCK "Enable landlock file restriction" OFF )
# ON/OFF spelling of ENABLE_LANDLOCK for templates, which compare strings: CMake keeps whatever
# truthy value was passed (1, TRUE, YES, ...).
if( ENABLE_LANDLOCK )
  set( UDA_LANDLOCK_ENABLED "ON" )
else()
  set( UDA_LANDLOCK_ENABLED "OFF" )
endif()
if(ENABLE_LANDLOCK)
    if( NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" )
        message( FATAL_ERROR "Landlock requires Linux" )
    endif()
    include( CheckCXXSourceCompiles )
    check_cxx_source_compiles( "
      #include <fcntl.h>
      #include <linux/landlock.h>
      #include <sys/syscall.h>
      int main() {
        landlock_ruleset_attr attr{};
        attr.handled_access_fs = LANDLOCK_ACCESS_FS_REFER | LANDLOCK_ACCESS_FS_TRUNCATE;
        (void)attr;
        return O_PATH + SYS_landlock_create_ruleset + SYS_landlock_add_rule + SYS_landlock_restrict_self;
      }" UDA_HAVE_LANDLOCK_ABI3_HEADERS )
    if( NOT UDA_HAVE_LANDLOCK_ABI3_HEADERS )
        message( FATAL_ERROR "Landlock requires Linux headers with ABI 3 support and Landlock syscall definitions" )
    endif()
endif()

# Wrapper configuration
option( NO_WRAPPERS "Don't build any UDA client wrappers" OFF )
option( NO_CXX_WRAPPER "Don't build C++ wrapper" OFF )
option( NO_PYTHON_WRAPPER "Don't build Python wrapper" OFF )
option( NO_JAVA_WRAPPER "Don't build Java wrapper" OFF )
option( NO_IDL_WRAPPER "Don't build IDL wrapper" OFF )
option( FAT_IDL "Build IDL wrapper using fat-client" OFF )
option( NO_CLI "Don't build UDA CLI" OFF )

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
set( UDA_SERVER_PORT "56565" CACHE STRING "define port number in server configuration files" )
set( UDA_SERVER_HOST "`hostname`" CACHE STRING "define hostname in server configuration files" )

########################################################################################################################
# Source directories

# Optional libmemcached support. Found here so the imported target and the settings below reach
# every subdirectory, which then only test if( TARGET LibMemcached::LibMemcached ).
if( NOT NO_MEMCACHE )
  find_package( LibMemcached QUIET )
endif()
set( EXTRA_LD_LIBRARY_PATHS "" )
if( TARGET LibMemcached::LibMemcached )
  set( EXTRA_LD_LIBRARY_PATHS "${LIBMEMCACHED_LIB_DIR}" )
else()
  add_compile_definitions( NOLIBMEMCACHED )
endif()

add_subdirectory( source )
include( UDAPkgConfig )
uda_install_pkgconfig()
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
  DESTINATION ${UDA_INSTALL_MODULEDIR}
  COMPONENT tools
)

########################################################################################################################
# Installed CMake package for client consumers

include( CMakePackageConfigHelpers )

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/UDAConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/UDAConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/UDAConfig.cmake
  INSTALL_DESTINATION ${UDA_INSTALL_CMAKEDIR}
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/UDAConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/UDAConfigVersion.cmake
  DESTINATION ${UDA_INSTALL_CMAKEDIR}
  COMPONENT development
)

install(
  FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/FindTIRPC.cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/FindXDR.cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/FindLibMemcached.cmake
  DESTINATION ${UDA_INSTALL_CMAKEDIR}/Modules
  COMPONENT development
)

# Static client targets are usable from the development component alone. The shared client
# targets reference the runtime library, so they are exported with the runtime component.
install(
  EXPORT UDAClientTargets
  FILE UDAClientTargets.cmake
  NAMESPACE UDA::
  DESTINATION ${UDA_INSTALL_CMAKEDIR}
  COMPONENT development
)

if( BUILD_SHARED_LIBS )
  install(
    EXPORT UDAClientSharedTargets
    FILE UDAClientSharedTargets.cmake
    NAMESPACE UDA::
    DESTINATION ${UDA_INSTALL_CMAKEDIR}
    COMPONENT client-runtime
  )
endif()

# Portablexdr 4.9.1 runtime. The DLL is built next to its import library (lib/libxdr.dll.a) and
# must sit in the bin directory with the other DLLs, or the programs that load it cannot start.
if( WIN32 OR MINGW )
  get_filename_component( _XDR_LIBDIR "${XDR_LIBRARIES}" DIRECTORY )
  find_file( UDA_XDR_RUNTIME NAMES libxdr.dll xdr.dll
    HINTS "${_XDR_LIBDIR}/../bin" "${XDR_ROOT}/bin"
    NO_DEFAULT_PATH )
  if( UDA_XDR_RUNTIME )
    install(
      FILES "${UDA_XDR_RUNTIME}"
      DESTINATION ${UDA_INSTALL_BINDIR}
      COMPONENT client-runtime
    )
  elseif( XDR_LIBRARIES MATCHES "\\.dll\\.(a|lib)$" )
    message( FATAL_ERROR "The portablexdr runtime DLL was not found next to ${XDR_LIBRARIES}" )
  else()
    message( STATUS "portablexdr is linked statically, so there is no runtime DLL to install" )
  endif()
  return()
endif()

########################################################################################################################
# Packages (Windows builds returned above)

include( packaging )
