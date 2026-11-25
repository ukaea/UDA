cmake_minimum_required( VERSION 3.12 )

if( CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR )
  project( uda_idl LANGUAGES C CXX )

  set( UDA_IDL_STANDALONE ON )
  list( APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake/Modules" )
  include( "${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake/UDAInstallDirs.cmake" )
  find_package( UDA CONFIG REQUIRED COMPONENTS Client )
  set( UDA_IDL_CLIENT_TARGET UDA::client_static )
  string( REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" UDA_IDL_VERSION "${UDA_VERSION}" )
  string( REGEX MATCH "^[0-9]+" UDA_IDL_VERSION_MAJOR "${UDA_VERSION}" )
else()
  set( UDA_IDL_STANDALONE OFF )
  set( UDA_IDL_CLIENT_TARGET client-static )
  set( UDA_IDL_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH} )
  set( UDA_IDL_VERSION_MAJOR ${PROJECT_VERSION_MAJOR} )
endif()

find_package( IDL REQUIRED )

if( UDA_IDL_STANDALONE AND FAT_IDL )
  message( FATAL_ERROR "FAT_IDL requires the full in-tree UDA build" )
endif()

# Preserve the existing GNU/Linux workaround for an older libstdc++ preloaded
# by IDL. Apple Clang uses libc++ and must not search for libstdc++.a.
# Static linkage alone does not guarantee symbol isolation from the IDL host.
if( UNIX AND NOT APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
  set( _UDA_IDL_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES} )
  set( CMAKE_FIND_LIBRARY_SUFFIXES ".a" )
  find_library( STDCXX_STATIC stdc++ HINTS ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES} )
  set( CMAKE_FIND_LIBRARY_SUFFIXES ${_UDA_IDL_LIBRARY_SUFFIXES} )
  unset( _UDA_IDL_LIBRARY_SUFFIXES )
  if( NOT STDCXX_STATIC )
    message( FATAL_ERROR "Install the GNU C++ static runtime or set STDCXX_STATIC to libstdc++.a" )
  endif()
endif()

set( LIB_NAME uda_idl )
set( SRC_FILES idam_dlm.c idamStructs.c )

if( FAT_IDL )
  add_definitions( -DFATCLIENT )

  set( FATCLIENT_OBJS
    $<TARGET_OBJECTS:fatclient-objects>
    $<TARGET_OBJECTS:fatserver-objects>
    $<TARGET_OBJECTS:fatclientserver-objects>
    $<TARGET_OBJECTS:structures-server-objects>
    $<TARGET_OBJECTS:fatmodules-objects>
    $<TARGET_OBJECTS:cache-objects>
    $<TARGET_OBJECTS:logging-server-objects>
    $<TARGET_OBJECTS:plugins-objects>
  )

  add_library( ${LIB_NAME} SHARED ${SRC_FILES} ${FATCLIENT_OBJS} )
else()
  add_library( ${LIB_NAME} SHARED ${SRC_FILES} )
endif()

target_include_directories( ${LIB_NAME} PRIVATE ${IDL_INCLUDES} )
if( UDA_IDL_STANDALONE )
  target_compile_definitions( ${LIB_NAME} PRIVATE OSTYPE=${CMAKE_SYSTEM_NAME} )
  if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    target_compile_definitions( ${LIB_NAME} PRIVATE A64 )
  endif()
endif()
if( NOT UDA_IDL_STANDALONE )
  target_include_directories( ${LIB_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/source )
endif()

if( FAT_IDL )
  include( modules )

  target_link_libraries( ${LIB_NAME} LINK_PRIVATE IDL::IDL ${STDCXX_STATIC} ${EXTRA_IDL_WRAPPER_LINK_ARGS} )
  link_modules( ${LIB_NAME} )
else()
  target_link_libraries( ${LIB_NAME} LINK_PRIVATE ${UDA_IDL_CLIENT_TARGET} IDL::IDL ${STDCXX_STATIC} ${EXTRA_IDL_WRAPPER_LINK_ARGS} )
endif()

set_target_properties( ${LIB_NAME}
  PROPERTIES
    PREFIX ""
    OUTPUT_NAME uda
    VERSION ${UDA_IDL_VERSION}
    SOVERSION ${UDA_IDL_VERSION_MAJOR}
    C_STANDARD 99
)

if( APPLE )
  # IDL's macOS DLM examples and loader use .so rather than .dylib.
  set_target_properties( ${LIB_NAME} PROPERTIES SUFFIX ".so" )
elseif( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
  # A DLM links to IDL::IDL, so references from the wrapper and static UDA
  # client must resolve at link time rather than failing at interpreter load.
  set_property( TARGET ${LIB_NAME} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-z,defs" )
endif()

if( FAT_IDL )
  install( TARGETS ${LIB_NAME}
    DESTINATION ${UDA_INSTALL_FAT_IDL_DLM_DIR}
    COMPONENT idl
  )

  install( FILES uda.dlm
    DESTINATION ${UDA_INSTALL_FAT_IDL_DLM_DIR}
    COMPONENT idl
  )
else()
  install( TARGETS ${LIB_NAME}
    DESTINATION ${UDA_INSTALL_IDL_DLM_DIR}
    COMPONENT idl
  )

  install( FILES uda.dlm
    DESTINATION ${UDA_INSTALL_IDL_DLM_DIR}
    COMPONENT idl
  )
endif()

set( INSTALL_FILES
  getstruct.pro
  getdata.pro
  getgeomdata.pro
  calluda.pro
  getfile.pro
  gettext.pro
)

install( FILES ${INSTALL_FILES}
  DESTINATION ${UDA_INSTALL_IDL_PRO_DIR}
  COMPONENT idl
)
