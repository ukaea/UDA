find_package( Boost COMPONENTS headers REQUIRED )
find_package( OpenSSL REQUIRED )
find_package( fmt REQUIRED )

set( SRCS
  alloc_data.cpp
  protocol.cpp
  protocol2.cpp
  protocolXML.cpp
  protocolXML2.cpp
  protocolXML2Put.cpp
  readXDRFile.cpp
  xdrlib.cpp
)

add_library( protocol OBJECT ${SRCS} )
target_include_directories( protocol PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/source ${CMAKE_CURRENT_SOURCE_DIR} )
target_link_libraries( protocol PUBLIC Boost::boost OpenSSL::SSL OpenSSL::Crypto fmt::fmt ${RPC_LIB} )
target_link_libraries( protocol PRIVATE common )