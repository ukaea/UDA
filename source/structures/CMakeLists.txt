find_package( spdlog REQUIRED )

set( SRCS
  parseIncludeFile.cpp
  struct.cpp
  accessors.cpp
  xdrUserDefinedData.cpp
)

add_library( structures OBJECT ${SRCS} )
target_include_directories( structures PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/source ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( structures PRIVATE ${CMAKE_SOURCE_DIR}/extlib/GSL-4.0.0/include )
target_link_libraries( structures PRIVATE spdlog::spdlog_header_only )
target_link_libraries( structures PUBLIC ${RPC_LIB} )
