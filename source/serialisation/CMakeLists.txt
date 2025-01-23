find_package( CapnProto REQUIRED )

set( SRCS
  access_log.cpp
  logging.cpp
)

capnp_generate_cpp( CAPNP_SRCS CANPNP_HDRS schema.capnp )

add_library( serialisation OBJECT capnp_serialisation.cpp ${CAPNP_SRCS} )
target_include_directories( serialisation PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/source ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( serialisation PRIVATE ${CMAKE_SOURCE_DIR}/include/uda/serialisation )
target_include_directories( serialisation PRIVATE ${CMAKE_CURRENT_BINARY_DIR} )
target_include_directories( serialisation PRIVATE ${CMAKE_SOURCE_DIR}/extlib/GSL-4.0.0/include )
target_link_libraries( serialisation PRIVATE CapnProto::capnp )