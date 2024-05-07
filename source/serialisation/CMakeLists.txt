########################################################################################################################
# Dependencies

find_package( CapnProto REQUIRED )

capnp_generate_cpp( CAPNP_SRCS CANPNP_HDRS schema.capnp )

add_library( serialisation-static STATIC capnp_serialisation.cpp ${CAPNP_SRCS} )

target_link_libraries( serialisation-static PRIVATE CapnProto::capnp )
target_include_directories(
  serialisation-static
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/source
    ${CMAKE_SOURCE_DIR}/source/include
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_SOURCE_DIR}/extlib/GSL-4.0.0/include
)

set_target_properties( serialisation-static
  PROPERTIES
  OUTPUT_NAME ${PROJECT_NAME}_serialisation
)

install( TARGETS serialisation-static
  DESTINATION lib
)

install( FILES "capnp_serialisation.h"
  DESTINATION include/uda/serialisation
)