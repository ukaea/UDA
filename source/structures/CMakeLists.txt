########################################################################################################################
# Dependencies

find_package( LibXml2 REQUIRED )

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

add_library( structures-objects OBJECT ${SRC_FILES} )
target_compile_features( structures-objects PRIVATE cxx_std_11 )

target_include_directories(
  structures-objects
  PUBLIC
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  PRIVATE
  ${PROJECT_SOURCE_DIR}/source
)

add_library( uda::structures-objects ALIAS structures-objects )