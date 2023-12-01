########################################################################################################################
# Dependencies

find_package( LibXml2 REQUIRED )

set( LIBS LibXml2::LibXml2 )

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
  list( APPEND LIBS XDR::XDR )
else()
  find_package( TIRPC QUIET )
  list( APPEND TIRPC::TIRPC )
endif()

########################################################################################################################
# Sources

set( SRC_FILES
  parseIncludeFile.cpp
  struct.cpp
  accessors.cpp
  xdrUserDefinedData.cpp
)

########################################################################################################################
# Targets

add_library( structures-client-objects OBJECT ${SRC_FILES} )
target_include_directories( structures-client-objects PRIVATE
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_SOURCE_DIR}/source/include
)

if( NOT CLIENT_ONLY )
  add_library( structures-server-objects OBJECT ${SRC_FILES} )
  target_compile_definitions( structures-server-objects PRIVATE -DSERVERBUILD )
  target_include_directories( structures-server-objects PRIVATE
    ${CMAKE_SOURCE_DIR}/source
    ${CMAKE_SOURCE_DIR}/source/include
  )
endif()
