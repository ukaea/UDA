find_package( Boost REQUIRED )
find_package( OpenSSL REQUIRED )
find_package( fmt REQUIRED )
find_package( LibXml2 REQUIRED )

set( SRC_FILES
  array.cpp
  client.cpp
  data.cpp
  dim.cpp
  result.cpp
  scalar.cpp
  string.cpp
  structdata.cpp
  treenode.cpp
  vector.cpp
  signal.cpp
)

add_library( uda-cpp ${SRC_FILES} )
target_include_directories( uda-cpp PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/source ${CMAKE_CURRENT_SOURCE_DIR} )
target_link_libraries( uda-cpp PRIVATE c_api client Boost::boost )