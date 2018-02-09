########################################################################################################################
# Sources

set( SRC_FILES
  parseIncludeFile.c
  struct.c
  accessors.c
  xdrUserDefinedData.c
)

include_directories(
  ${CMAKE_SOURCE_DIR}/source
)

########################################################################################################################
# Targets

if( NOT CLIENT_ONLY )
  add_library( structures-server-objects OBJECT ${SRC_FILES} )
  target_compile_definitions( structures-server-objects PRIVATE -DSERVERBUILD )
endif()

add_library( structures-client-objects OBJECT ${SRC_FILES} )

file( GLOB HEADER_FILES "*.h" )

install( FILES ${HEADER_FILES}
  DESTINATION include/uda/structures
)
