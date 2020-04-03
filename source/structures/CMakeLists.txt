########################################################################################################################
# Dependencies

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
endif()

########################################################################################################################
# Sources

set( SRC_FILES
  parseIncludeFile.cpp
  struct.cpp
  accessors.cpp
  xdrUserDefinedData.cpp
)

include_directories(
  ${CMAKE_SOURCE_DIR}/source
)

if( MINGW OR WIN32 )
  include_directories( ${XDR_INCLUDE_DIR} )
endif()

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
