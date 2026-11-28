# Naming, versioning and installation shared by every UDA library variant.
#
#   uda_install_library( <target> <base-name> [EXPORT <export-set>] [RUNTIME_COMPONENT <component>] )
#
# Static archives are named <base-name> plus UDA_STATIC_LIB_SUFFIX and installed with the
# development component. Shared libraries are named <base-name>, carry the project version and
# SOVERSION, use the Windows import-library suffix, and are installed with RUNTIME_COMPONENT
# (default client-runtime). EXPORT adds the installed target to an export set.
function( uda_install_library TARGET NAME )
  cmake_parse_arguments( PARSE_ARGV 2 ARG "" "EXPORT;RUNTIME_COMPONENT" "" )

  get_target_property( _type ${TARGET} TYPE )
  if( _type STREQUAL "STATIC_LIBRARY" )
    set_target_properties( ${TARGET} PROPERTIES OUTPUT_NAME ${NAME}${UDA_STATIC_LIB_SUFFIX} )
    set( _component development )
  else()
    set_target_properties( ${TARGET} PROPERTIES
      OUTPUT_NAME ${NAME}
      VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
      SOVERSION ${PROJECT_VERSION_MAJOR}
    )
    if( WIN32 )
      set_target_properties( ${TARGET} PROPERTIES IMPORT_SUFFIX ${IMPLIB_SUFFIX} )
    endif()
    set( _component client-runtime )
    if( ARG_RUNTIME_COMPONENT )
      set( _component ${ARG_RUNTIME_COMPONENT} )
    endif()
  endif()

  set( _export )
  if( ARG_EXPORT )
    set( _export EXPORT ${ARG_EXPORT} )
  endif()
  install( TARGETS ${TARGET} ${_export}
    RUNTIME DESTINATION ${UDA_INSTALL_BINDIR}
    LIBRARY DESTINATION ${UDA_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${UDA_INSTALL_LIBDIR}
    COMPONENT ${_component}
  )
endfunction()
