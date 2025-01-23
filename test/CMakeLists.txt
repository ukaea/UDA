enable_testing()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/setup.inc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/setup.inc"
  @ONLY
)

function( BUILD_TEST NAME SOURCE )
  set( LINK_PROJECT ${PROJECT_NAME}_cpp-static )
  if( BUILD_SHARED_LIBS )
    set( LINK_PROJECT ${PROJECT_NAME}_cpp-shared )
  endif()

  add_executable( ${NAME} ${EXE_FLAG} ${HELPER_FILES} ${SOURCE} )
  target_link_libraries( ${NAME} PRIVATE )
  target_include_directories( ${NAME} PRIVATE ${CMAKE_SOURCE_DIR}/test uda-cpp )
  target_compile_definitions( ${NAME} PRIVATE -DCAPNP_ENABLED=1 )

  set_target_properties( ${NAME}
    PROPERTIES
    OUTPUT_NAME ${NAME}
  )

  if( WIN32 )
    set_target_properties( ${NAME}
      PROPERTIES
      COMPILE_FLAGS -DNOIMPLIB
    )
  endif()

  add_test( ${NAME} ${NAME} -r junit -o ${NAME}_out.xml )
endfunction( BUILD_TEST )

#add_subdirectory( plugins )
