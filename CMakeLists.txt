cmake_minimum_required( VERSION 3.0 )

cmake_policy( SET CMP0048 NEW )

# Extra cmake modules to find libraries
set( CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${CMAKE_SOURCE_DIR}/cmake
  ${CMAKE_SOURCE_DIR}/cmake/Modules )

include( GetGitRevisionDescription )

git_describe( GIT_TAG --tags )

# Strip off the SHA1 tag element if it exists
string( REGEX REPLACE "^([0-9]+\\.[0-9]+\\.[0-9]+).*" "\\1" GIT_VERSION "${GIT_TAG}" )

project( uda VERSION ${GIT_VERSION} )

if( APPLE )
  set( CMAKE_MACOSX_RPATH ON )
endif()

# TODO: replace with machine specific configuration input
add_definitions(
  -Wall
  -Werror
  -DIDAM_SERVER_HOST="fuslw"
  -DIDAM_SERVER_HOST2="fuslw"
  -DIDAM_SERVER_PORT=56565
  -DIDAM_SERVER_PORT2=56565
  -DAPI_DEVICE="MAST"
  -DAPI_ARCHIVE="MAST"
  -DAPI_PARSE_STRING="::"
  -DAPI_FILE_FORMAT="IDA3"
  -DDEFAULT_ARCHIVE_DIR="/net/fuslsa/data/MAST_Data/"
  -DLOCAL_MDSPLUS_SERVER="idam0"
  -DIDAM_STARTPULSE=12270
  -DFORMAT_LEGACY="IDA3"
  -DFORMAT_MODERN="CDF"
  -DSQL_PORT=56566
  -DSQL_DBNAME="idam"
  -DSQL_USER="readonly"
  -DSQL_HOST="idam1"
)

add_subdirectory( source )
#add_subdirectory( test )
