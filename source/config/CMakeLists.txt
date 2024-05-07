########################################################################################################################
# Dependencies

find_package( Boost REQUIRED )

set( LIBS Boost::boost )

########################################################################################################################
# Targets

add_library( config-objects OBJECT config.cpp )
target_link_libraries( config-objects PUBLIC ${LIBS} )

target_include_directories( config-objects PRIVATE
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_SOURCE_DIR}/source/include
  ${CMAKE_BINARY_DIR}/include
  ${CMAKE_SOURCE_DIR}/extlib
)
