find_package( Boost REQUIRED )

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

set( HDR_FILES
  include/uda/c++/UDA.hpp
  include/uda/c++/array.hpp
  include/uda/c++/client.hpp
  include/uda/c++/data.hpp
  include/uda/c++/dim.hpp
  include/uda/c++/result.hpp
  include/uda/c++/scalar.hpp
  include/uda/c++/string.hpp
  include/uda/c++/structdata.hpp
  include/uda/c++/treenode.hpp
  include/uda/c++/vector.hpp
)

add_library( uda_cpp ${SRC_FILES} )
target_include_directories( uda_cpp PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/source ${CMAKE_CURRENT_SOURCE_DIR}/include/uda/c++ )
target_link_libraries( uda_cpp PRIVATE c_api Boost::boost )
target_link_libraries( uda_cpp PRIVATE
  "$<BUILD_INTERFACE:client>"
)

set_target_properties( uda_cpp
  PROPERTIES
    OUTPUT_NAME uda_cpp
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

add_library( uda::cpp ALIAS uda_cpp )

target_sources( uda_cpp PUBLIC
  FILE_SET public_headers
  TYPE HEADERS
  BASE_DIRS include
  FILES ${HDR_FILES}
)

include( GNUInstallDirs )

install( TARGETS uda_cpp
  EXPORT udaCxxTargets
  FILE_SET public_headers
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)