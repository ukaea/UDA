########################################################################################################################
# Dependencies

find_package( CapnProto REQUIRED )

capnp_generate_cpp( CAPNP_SRCS CANPNP_HDRS schema.capnp )

if( WIN32 )
  # Export all symbols from the DLL so MSVC generates an import library without header annotations.
  set( CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON )
endif()

set( _VARIANTS STATIC )
if( BUILD_SHARED_LIBS )
  list( APPEND _VARIANTS SHARED )
endif()
foreach( _VARIANT IN LISTS _VARIANTS )
  string( TOLOWER "serialisation-${_VARIANT}" _TARGET )
  add_library( ${_TARGET} ${_VARIANT} capnp_serialisation.cpp ${CAPNP_SRCS} )
  target_link_libraries( ${_TARGET} PRIVATE CapnProto::capnp )
  target_include_directories( ${_TARGET}
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/source ${CMAKE_SOURCE_DIR}/extlib/GSL-4.0.0/include
  )
  uda_install_library( ${_TARGET} ${PROJECT_NAME}_serialisation )
endforeach()

file( GLOB HEADER_FILES "*.h" )

install( FILES ${HEADER_FILES}
  DESTINATION ${UDA_INSTALL_INCLUDEDIR}/serialisation
  COMPONENT development
)
