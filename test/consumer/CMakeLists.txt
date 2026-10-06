########################################################################################################################
# Downstream consumer of an INSTALLED UDA client.
#
# This is a standalone project: it is deliberately NOT part of the UDA build.
# It finds UDA through pkg-config exactly as an external application would, so
# it fails if the installed .pc files do not declare everything needed to
# compile and link against the client libraries.
#
# Point pkg-config at the install and build it:
#
#   PKG_CONFIG_LIBDIR=<prefix>/lib/pkgconfig cmake -S test/consumer -B build-consumer
#   cmake --build build-consumer && ( cd build-consumer && ctest )
#
# check_install.sh drives this, along with raw pkg-config and static-link checks.

cmake_minimum_required( VERSION 3.5 )

project( uda_consumer_test C CXX )

find_package( PkgConfig REQUIRED )

pkg_check_modules( UDA_CLIENT REQUIRED uda-client )

enable_testing()

add_executable( consumer_c consumer_c.c )
target_include_directories( consumer_c PRIVATE ${UDA_CLIENT_INCLUDE_DIRS} )
target_link_libraries( consumer_c ${UDA_CLIENT_LDFLAGS} )
set_target_properties( consumer_c
  PROPERTIES
    INSTALL_RPATH "${UDA_CLIENT_LIBRARY_DIRS}"
    BUILD_WITH_INSTALL_RPATH TRUE
)
add_test( NAME consumer_c COMMAND consumer_c )

pkg_check_modules( UDA_CPP QUIET uda-cpp )

if( UDA_CPP_FOUND )
  add_executable( consumer_cpp consumer_cpp.cpp )
  target_include_directories( consumer_cpp PRIVATE ${UDA_CPP_INCLUDE_DIRS} )
  target_link_libraries( consumer_cpp ${UDA_CPP_LDFLAGS} )
  set_target_properties( consumer_cpp
    PROPERTIES
      INSTALL_RPATH "${UDA_CPP_LIBRARY_DIRS}"
      BUILD_WITH_INSTALL_RPATH TRUE
  )
  add_test( NAME consumer_cpp COMMAND consumer_cpp )
else()
  message( STATUS "uda-cpp.pc not found - only building the C consumer" )
endif()
