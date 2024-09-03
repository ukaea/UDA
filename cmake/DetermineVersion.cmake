# Determine the version of the current AL component, based on git describe

if( NOT GIT_ARCHIVE_DESCRIBE )
  message( FATAL_ERROR "GIT_ARCHIVE_DESCRIBE should be set before including ALDetermineVersion" )
endif()

if( NOT GIT_ARCHIVE_DESCRIBE MATCHES "^.Format:%.describe" )
  # We are part of an exported tarball and git-archive set the describe content:
  set( _GIT_DESCRIBE_ERROR_CODE 0 )
  set( _GIT_DESCRIBE_OUTPUT "${GIT_ARCHIVE_DESCRIBE}" )
else()
  # Ask git for a describe:
  find_package( Git )
  if( GIT_EXECUTABLE )
  # Generate a git-describe version string from Git repository tags
    execute_process(
      COMMAND ${GIT_EXECUTABLE} describe --tags --dirty
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      OUTPUT_VARIABLE _GIT_DESCRIBE_OUTPUT
      RESULT_VARIABLE _GIT_DESCRIBE_ERROR_CODE
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  endif()
endif()

# Process git describe output:
if( _GIT_DESCRIBE_OUTPUT AND NOT _GIT_DESCRIBE_ERROR_CODE )
  # Git describe should return the version (MAJOR.MINOR.PATCH) and potentially
  # a suffix "-<ncommits>-g<hash>[-dirty]"
  # Use a regex to extract all parts:
  if( _GIT_DESCRIBE_OUTPUT MATCHES "([0-9]+)[.]([0-9]+)[.]*([0-9]+)(.*)" )
    set( VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}" )
    if( CMAKE_MATCH_4 MATCHES "-([0-9]+)-(.*)" )
      # Use ncommits as fourth version component for the CMAKE project version
      set( VERSION "${VERSION}.${CMAKE_MATCH_1}" )
    endif()
  else()
    message( FATAL_ERROR "Unexpected output of git describe: '${_GIT_DESCRIBE_OUTPUT}'")
  endif()

  # Generate a version string that conforms to the Python standards
  # e.g. 5.1.0-3-g7c620eb5-dirty becomes 5.1.0+3-g7c620eb5-dirty
  string( REGEX REPLACE "-(.*)$" "+\\1" FULL_VERSION ${_GIT_DESCRIBE_OUTPUT} )
  message( VERBOSE "Determined project version: ${VERSION}" )
endif()

# Fallback: git not found, or git describe fails
# Set version to 0.0.0 and report a warning
if( NOT DEFINED VERSION )
  set( VERSION "0.0.0" )
  set( FULL_VERSION "0.0.0+unknown" )
  message( WARNING "Failed to determine VERSION from git tags. Falling back to default version '${VERSION}'" )
endif()
