########################################################################################################################
# Dependencies

find_package( Boost REQUIRED )

set( LIBS Boost::boost )

########################################################################################################################
# Targets

add_library( common-objects OBJECT stringUtils.cpp )
target_link_libraries( common-objects PUBLIC ${LIBS} )

target_include_directories( common-objects PRIVATE
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_SOURCE_DIR}/source/include
  ${CMAKE_BINARY_DIR}/include
  ${CMAKE_SOURCE_DIR}/extlib
)
