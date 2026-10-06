if (NOT CLIENT_ONLY)
include( LibXml2StrPrintfConfig )
endif()
find_package( Boost REQUIRED )
include_directories( SYSTEM ${Boost_INCLUDE_DIR} )

find_package( LibXml2 REQUIRED )


if( WIN32 AND NOT MINGW )
  add_definitions( /W2 /WX )
else()
  add_definitions( -Wall -Werror -Wextra -Wno-unused-parameter -Wno-missing-field-initializers )
  if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER 5.0 )
    add_definitions( -Wno-format-truncation -Wno-format-overflow )
  endif()
endif()

set( LOGGING_BACKEND "FILE" CACHE STRING "Choose logging backend [FILE;SYSLOG]" )

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

add_subdirectory( cache )
add_subdirectory( logging )
add_subdirectory( authentication )
add_subdirectory( structures )
add_subdirectory( clientserver )
add_subdirectory( client )

if( NOT NO_WRAPPERS )
  add_subdirectory( wrappers )
endif()

if( NOT CLIENT_ONLY )
  add_subdirectory( server )
  add_subdirectory( plugins )
endif()

########################################################################################################################
# Configure files

string( TIMESTAMP BUILD_TIMESTAMP UTC )
configure_file(
  ${CMAKE_SOURCE_DIR}/source/version.h.in
  ${CMAKE_BINARY_DIR}/source/version.h
  @ONLY
)

install( FILES uda.h ${CMAKE_BINARY_DIR}/source/version.h DESTINATION include/uda )

set( ENV_PATH )
if( WIN32 AND NOT MINGW )
  set( ENV_PATH "set" )
else()
  execute_process( COMMAND which env OUTPUT_VARIABLE ENV_PATH )
endif()
string( STRIP ${ENV_PATH} ENV_PATH )

find_program( PYTHON_EXE NAMES python python3 PATHS /usr/bin/ )

configure_file(
  ${CMAKE_SOURCE_DIR}/source/bin/install_plugin.in
  ${CMAKE_BINARY_DIR}/bin/install_plugin
  @ONLY
)

install(
  FILES ${CMAKE_BINARY_DIR}/bin/install_plugin
  DESTINATION bin
  PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
)

# Only install pkg-config files for the libraries this configuration builds
set( BUILD_CXX_WRAPPER OFF )
if( NOT NO_WRAPPERS AND NOT NO_CXX_WRAPPER AND Boost_FOUND )
  set( BUILD_CXX_WRAPPER ON )
endif()

set( PKGCONFIG_FILES uda-client.pc )
if( BUILD_CXX_WRAPPER )
  list( APPEND PKGCONFIG_FILES uda-cpp.pc )
endif()
if( NOT CLIENT_ONLY )
  list( APPEND PKGCONFIG_FILES uda-fat-client.pc uda-plugins.pc )
  if( BUILD_CXX_WRAPPER )
    list( APPEND PKGCONFIG_FILES uda-fat-cpp.pc )
  endif()
endif()

get_filename_component( LIBXML_LIB_DIR "${LIBXML2_LIBRARIES}" DIRECTORY )
set( PKGCONFIG_LIBRARIES "-L${LIBXML_LIB_DIR} -lxml2" )
set( PKGCONFIG_INCLUDES "-I${LIBXML2_INCLUDE_DIR}" )
set( PKGCONFIG_REQUIRES "" )

find_package( TIRPC QUIET )
if( TIRPC_FOUND )
  set( PKGCONFIG_REQUIRES "${PKGCONFIG_REQUIRES} libtirpc" )
endif()

if( ENABLE_CAPNP )
  set( PKGCONFIG_REQUIRES "${PKGCONFIG_REQUIRES} capnp" )
  set( PKGCONFIG_LIBRARIES "${PKGCONFIG_LIBRARIES} -luda_serialisation" )
endif()

find_package( LibMemcached QUIET )

if( LIBMEMCACHED_FOUND AND NOT NO_MEMCACHE )
  set( PKGCONFIG_LIBRARIES "${PKGCONFIG_LIBRARIES} ${LIBMEMCACHED_LIBRARIES}" )
endif()

# The installed C++ wrapper headers include boost
set( PKGCONFIG_CPP_INCLUDES "" )
if( Boost_INCLUDE_DIR )
  list( FIND CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES "${Boost_INCLUDE_DIR}" BOOST_IS_IMPLICIT )
  if( BOOST_IS_IMPLICIT EQUAL -1 AND NOT "${Boost_INCLUDE_DIR}" STREQUAL "/usr/include" )
    set( PKGCONFIG_CPP_INCLUDES "-I${Boost_INCLUDE_DIR}" )
  endif()
endif()

# Dependencies of the (privately linked) client libraries, needed when linking statically
find_package( OpenSSL QUIET )
set( PKGCONFIG_LIBS_PRIVATE "" )
if( OPENSSL_FOUND )
  get_filename_component( OPENSSL_LIB_DIR "${OPENSSL_SSL_LIBRARY}" DIRECTORY )
  set( PKGCONFIG_LIBS_PRIVATE "-L${OPENSSL_LIB_DIR} -lssl -lcrypto" )
endif()
if( APPLE )
  set( PKGCONFIG_LIBS_PRIVATE "${PKGCONFIG_LIBS_PRIVATE} -lc++" )
elseif( NOT WIN32 )
  set( PKGCONFIG_LIBS_PRIVATE "${PKGCONFIG_LIBS_PRIVATE} -lstdc++ -ldl -lpthread" )
endif()

set( PKGCONFIG_INSTALL_FILES )
foreach( PKGCONFIG_FILE ${PKGCONFIG_FILES} )
  configure_file(
    "${CMAKE_SOURCE_DIR}/source/etc/${PKGCONFIG_FILE}.in"
    "${CMAKE_BINARY_DIR}/lib/pkgconfig/${PKGCONFIG_FILE}"
    @ONLY
  )
  list( APPEND PKGCONFIG_INSTALL_FILES "${CMAKE_BINARY_DIR}/lib/pkgconfig/${PKGCONFIG_FILE}" )
endforeach()

install(
  FILES ${PKGCONFIG_INSTALL_FILES}
  DESTINATION lib/pkgconfig
)

find_program( XINETD_EXE xinetd PATHS /usr/sbin/ /usr/local/sbin/ )

configure_file(
 ${CMAKE_SOURCE_DIR}/source/etc/rc.uda.in
 ${CMAKE_BINARY_DIR}/etc/rc.uda
 @ONLY
)

install(
  FILES ${CMAKE_BINARY_DIR}/etc/rc.uda
  DESTINATION etc
  PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
)
