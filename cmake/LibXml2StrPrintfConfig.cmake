find_package( LibXml2 QUIET )

# Sometimes xmlStrPrintf from LibXml2 takes a "char *" as the format string argument
# and sometimes it takes a "xmlChar *". This causes code to fail with -Wall and -Werror
# flags on. This check determines whether the code with the "char *" argument succeeds
# and adds definition -DLIBXML2_PRINTF_CHAR_ARG if it does.

if( LIBXML2_FOUND )
  include( CheckCSourceCompiles )

  file( READ ${CMAKE_SOURCE_DIR}/cmake/check_xmlstrprintf.c CHECK_XMLPRINTF_SOURCE )

  if( APPLE )
    set( CMAKE_REQUIRED_FLAGS "-Qunused-arguments -Wall -Werror" )
  else()
    set( CMAKE_REQUIRED_FLAGS "-Wall -Werror" )
  endif( APPLE )

  set( CMAKE_REQUIRED_INCLUDES ${LIBXML2_INCLUDE_DIR} )
  set( CMAKE_REQUIRED_LIBRARIES ${LIBXML2_LIBRARIES} )

  check_c_source_compiles(
    "${CHECK_XMLPRINTF_SOURCE}"
    LIBXML2_PRINTF_CHAR_ARG
  )

  if( LIBXML2_PRINTF_CHAR_ARG )
    add_definitions( -DLIBXML2_PRINTF_CHAR_ARG )
  endif()
endif()
