########################################################################################################################
# Dependencies

find_package( CapnProto REQUIRED )

########################################################################################################################
# Sources

capnp_generate_cpp( CAPNP_SRCS CANPNP_HDRS schema.capnp )
set( SRC_FILES capnp_serialisation.cpp ${CAPNP_SRCS} )

########################################################################################################################
# Targets

add_library( serialisation-objects OBJECT )
add_library( uda::serialisation-objects ALIAS serialisation-objects )

target_sources( serialisation-objects PRIVATE ${SRC_FILES} )
target_compile_features( serialisation-objects PRIVATE cxx_std_11 )
target_link_libraries( serialisation-objects PUBLIC CapnProto::capnp )
target_include_directories( serialisation-objects PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/source )