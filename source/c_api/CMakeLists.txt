########################################################################################################################
# Dependencies

#find_package( OpenSSL REQUIRED )
find_package( fmt REQUIRED )
#find_package( LibXml2 REQUIRED )

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

target_link_libraries( c_api-objects PUBLIC fmt::fmt )
target_link_libraries( fatc_api-objects PUBLIC fmt::fmt )

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