set( UDA_AVAILABLE_COMPONENTS server client plugin )

if( NOT UDA_FIND_COMPONENTS )
  set( UDA_FIND_COMPONENTS ${UDA_AVAILABLE_COMPONENTS} )
endif()

add_library( uda::uda INTERFACE )

function( CAPITALIZE STRING OUT_VAR )
  string( SUBSTRING "${STRING}" 0 1 FIRST_LETTER )
  string( TOUPPER "${FIRST_LETTER}" FIRST_LETTER )
  string( SUBSTRING "${STRING}" 1 -1 REMAIN )
  string( TOLOWER "${REMAIN}" REMAIN )
  set( ${OUT_VAR} "${FIRST_LETTER}${REMAIN}" )
  return( PROPAGATE ${OUT_VAR} )
endfunction()

foreach( COMPONENT IN LISTS UDA_FIND_COMPONENTS )
  string( TOLOWER ${COMPONENT} COMPONENT_LOWER )
  capitalize( ${COMPONENT} COMPONENT_CAPITALIZED )

  if( NOT COMPONENT_LOWER IN_LIST UDA_AVAILABLE_COMPONENTS )
    message( FATAL_ERROR "Component '${COMPONENT_LOWER}' is not available in UDA. Available components: '${UDA_AVAILABLE_COMPONENTS}'." )
  endif()
  include( uda${COMPONENT_CAPITALIZED}Targets.cmake )

  target_link_libraries( uda::uda uda::${COMPONENT_LOWER} )

  if( "${COMPONENT_LOWER}" STREQUAL "plugin" )
    include( udaPlugins.cmake )
  endif()
endforeach()
