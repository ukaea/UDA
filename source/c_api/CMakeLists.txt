########################################################################################################################
# Dependencies

#find_package( OpenSSL REQUIRED )
find_package( fmt REQUIRED )
#find_package( LibXml2 REQUIRED )

set( LIBS fmt::fmt )

if( WIN32 OR MINGW )

else()
  find_package( TIRPC QUIET )
  if( TIRPC_FOUND )
    list( APPEND LIBS TIRPC::TIRPC )
  endif()
endif()

########################################################################################################################
# Sources

set( SRC_FILES
  udaGetAPI.cpp
  clientAPI.cpp
  clientMDS.cpp
  accAPI.cpp
  udaPutAPI.cpp
)

add_library( c_api-objects OBJECT ${SRC_FILES} )
add_library( fatc_api-objects OBJECT ${SRC_FILES} )

target_link_libraries( c_api-objects PUBLIC ${LIBS} )
target_link_libraries( fatc_api-objects PUBLIC ${LIBS} )

target_compile_definitions( fatc_api-objects PUBLIC -DFATCLIENT )

target_include_directories(
  c_api-objects PRIVATE
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_SOURCE_DIR}/source/include
)
target_include_directories(
  fatc_api-objects PRIVATE
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_SOURCE_DIR}/source/include
)
