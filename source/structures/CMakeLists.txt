########################################################################################################################
# Sources

set( SRC_FILES
  parseIncludeFile.c
  struct.c
  accessors.c
  xdrUserDefinedData.c
  )

include_directories(
  ${CMAKE_SOURCE_DIR}/source
)

########################################################################################################################
# Targets

add_library( structures OBJECT ${SRC_FILES} )