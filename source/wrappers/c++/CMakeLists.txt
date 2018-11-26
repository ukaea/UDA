cmake_minimum_required( VERSION 2.8 )

find_package( Boost REQUIRED )

set( SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../.. )

set( LIB_NAME ${PROJECT_NAME}_cpp )

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

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${Boost_INCLUDE_DIR}
)

if( ITER_CI )
  add_definitions( -D__USE_XOPEN2K8 )
endif()

include_directories( SYSTEM "${Boost_INCLUDE_DIR}" )

add_library( cpp-objects OBJECT ${SRC_FILES} )

set( CLIENT_OBJS
  $<TARGET_OBJECTS:client-objects>
  $<TARGET_OBJECTS:clientserver-client-objects>
  $<TARGET_OBJECTS:cache-objects>
  $<TARGET_OBJECTS:logging-client-objects>
  $<TARGET_OBJECTS:structures-client-objects>
  $<TARGET_OBJECTS:authentication-client-objects>
)

add_library( ${LIB_NAME} STATIC $<TARGET_OBJECTS:cpp-objects> ${CLIENT_OBJS} )
target_link_libraries( ${LIB_NAME} LINK_PRIVATE ${Boost_LIBRARIES} )
if( WIN32 OR MINGW )
  target_link_libraries( ${LIB_NAME} LINK_PRIVATE portablexdr ws2_32 )
endif()

if( NOT CLIENT_ONLY )
  add_library( fatcpp-objects OBJECT ${SRC_FILES} )
  target_compile_definitions( fatcpp-objects PRIVATE -DFATCLIENT )

  set( FATCLIENT_OBJS
    $<TARGET_OBJECTS:fatclient-objects>
    $<TARGET_OBJECTS:fatserver-objects>
    $<TARGET_OBJECTS:fatclientserver-objects>
    $<TARGET_OBJECTS:structures-server-objects>
    $<TARGET_OBJECTS:fatmodules-objects>
    $<TARGET_OBJECTS:cache-objects>
    $<TARGET_OBJECTS:logging-server-objects>
    $<TARGET_OBJECTS:plugins-objects>
  )

  add_library( fat${LIB_NAME} STATIC $<TARGET_OBJECTS:fatcpp-objects> ${FATCLIENT_OBJS} )
  target_link_libraries( fat${LIB_NAME} LINK_PRIVATE ${Boost_LIBRARIES} plugins-static )
  if( WIN32 OR MINGW )
    target_link_libraries( fat${LIB_NAME} LINK_PRIVATE portablexdr ws2_32 )
  endif()
endif()

set_target_properties( ${LIB_NAME}
  PROPERTIES
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

install( TARGETS ${LIB_NAME}
  DESTINATION lib
)

# pkg-config file
configure_file(
  ${CMAKE_SOURCE_DIR}/source/etc/uda-cpp.pc.in
  ${CMAKE_BINARY_DIR}/etc/uda/uda-cpp.pc
  @ONLY
)

install( FILES ${CMAKE_BINARY_DIR}/etc/uda/uda-cpp.pc
  DESTINATION modulefiles/uda
)


if( NOT CLIENT_ONLY )
  set_target_properties( fat${LIB_NAME}
    PROPERTIES
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    SOVERSION ${PROJECT_VERSION_MAJOR}
  )

  install( TARGETS fat${LIB_NAME}
    DESTINATION lib
  )
  
  # pkg-config file
  configure_file(
    ${CMAKE_SOURCE_DIR}/source/etc/uda-fat-cpp.pc.in
    ${CMAKE_BINARY_DIR}/etc/uda/uda-fat-cpp.pc
    @ONLY
  )
  
  install( FILES ${CMAKE_BINARY_DIR}/etc/uda/uda-fat-cpp.pc
    DESTINATION modulefiles/uda
  )
endif()

set( INSTALL_FILES
  UDA.hpp
  array.hpp
  client.hpp
  data.hpp
  dim.hpp
  result.hpp
  scalar.hpp
  string.hpp
  structdata.hpp
  treenode.hpp
  vector.hpp
)

install( FILES ${INSTALL_FILES}
  DESTINATION include/uda/c++
)
