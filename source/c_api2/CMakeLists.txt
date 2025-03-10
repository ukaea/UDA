set( SRCS
  udaGetAPI.cpp
  accAPI.cpp
  udaPutAPI.cpp
  udaStructured.cpp
)

add_library( c_api ${SRCS} )
target_include_directories( c_api PRIVATE ${CMAKE_BINARY_DIR}/include ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/source ${CMAKE_CURRENT_SOURCE_DIR} )
target_link_libraries( c_api PUBLIC
  "$<BUILD_INTERFACE:authentication>"
  "$<BUILD_INTERFACE:cache>"
  "$<BUILD_INTERFACE:client>"
  "$<BUILD_INTERFACE:client_server>"
  "$<BUILD_INTERFACE:common>"
  "$<BUILD_INTERFACE:config>"
  "$<BUILD_INTERFACE:logging>"
  "$<BUILD_INTERFACE:protocol>"
  "$<BUILD_INTERFACE:structures>"
)

if ( ENABLE_CAPNP )
    target_link_libraries( c_api PUBLIC
        "$<BUILD_INTERFACE:serialisation>"
    )
endif()

target_include_directories( c_api
    PUBLIC
    $<INSTALL_INTERFACE:include>
)

set_target_properties( c_api
  PROPERTIES
    OUTPUT_NAME uda_c
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

add_library( uda::c_api ALIAS c_api )

install( TARGETS c_api
  EXPORT udaClientTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
