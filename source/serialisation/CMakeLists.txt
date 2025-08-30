########################################################################################################################
# Dependencies

find_package( CapnProto REQUIRED )

if( WIN32 )
  # Use variable-based auto-export so MSVC generates import libs without header annotations
  set( CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON )
endif()

capnp_generate_cpp( CAPNP_SRCS CANPNP_HDRS schema.capnp )

add_library( serialisation-static STATIC capnp_serialisation.cpp ${CAPNP_SRCS} )

target_link_libraries( serialisation-static PRIVATE CapnProto::capnp )
target_include_directories(
  serialisation-static
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/source ${CMAKE_SOURCE_DIR}/extlib/GSL-4.0.0/include
)

set_target_properties( serialisation-static
  PROPERTIES
  OUTPUT_NAME ${PROJECT_NAME}_serialisation_static
)

# Optionally build a shared library variant as well
if( BUILD_SHARED_LIBS )
  add_library( serialisation-shared SHARED capnp_serialisation.cpp ${CAPNP_SRCS} )
  target_link_libraries( serialisation-shared PRIVATE CapnProto::capnp )
  target_include_directories(
    serialisation-shared
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/source ${CMAKE_SOURCE_DIR}/extlib/GSL-4.0.0/include
  )

  set_target_properties( serialisation-shared
    PROPERTIES
      OUTPUT_NAME ${PROJECT_NAME}_serialisation
      VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
      SOVERSION ${PROJECT_VERSION_MAJOR}
  )

  if( WIN32 )
    set_target_properties( serialisation-shared
      PROPERTIES
        COMPILE_FLAGS -DLIBRARY_EXPORTS
        IMPORT_SUFFIX ${IMPLIB_SUFFIX}
    )
  endif()
endif()

install( TARGETS serialisation-static
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if( BUILD_SHARED_LIBS )
  install( TARGETS serialisation-shared
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
endif()

file( GLOB HEADER_FILES "*.h" )

install( FILES ${HEADER_FILES}
  DESTINATION include/uda/serialisation
)
