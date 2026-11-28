option( BUILD_SHARED_LIBS "Also build a shared C++ wrapper against the shared UDA client" OFF )

find_package( UDA CONFIG REQUIRED COMPONENTS Client )
find_package( Boost 1.60 REQUIRED )

if( BUILD_SHARED_LIBS AND NOT TARGET UDA::client_shared )
  message( FATAL_ERROR
    "BUILD_SHARED_LIBS requires an installed UDA::client_shared target. "
    "Install a UDA client built with BUILD_SHARED_LIBS=ON, or build only the static wrapper." )
endif()

function( uda_add_cpp_wrapper KIND )
  string( TOLOWER "${KIND}" _kind )
  set( _target "uda_cpp-${_kind}" )

  add_library( ${_target} ${KIND} ${SRC_FILES} )
  set_target_properties( ${_target} PROPERTIES EXPORT_NAME "cpp_${_kind}" )
  target_compile_features( ${_target} PUBLIC cxx_std_17 )
  target_include_directories( ${_target} PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${UDA_INSTALL_INCLUDEDIR}/c++>"
  )
  target_link_libraries( ${_target} PUBLIC UDA::client_${_kind} Boost::boost )

  if( KIND STREQUAL "STATIC" )
    set( _export UDACppTargets )
    set( _component development )
    set_target_properties( ${_target} PROPERTIES
      OUTPUT_NAME uda_cpp${UDA_STATIC_LIB_SUFFIX}
      POSITION_INDEPENDENT_CODE ON
    )
    target_compile_definitions( ${_target} PUBLIC UDA_CXX_STATIC )
  else()
    set( _export UDACppSharedTargets )
    set( _component client-runtime )
    set_target_properties( ${_target} PROPERTIES
      OUTPUT_NAME uda_cpp
      VERSION "${UDA_VERSION}"
      SOVERSION "${UDA_VERSION_MAJOR}"
    )
    if( WIN32 )
      set_target_properties( ${_target} PROPERTIES IMPORT_SUFFIX ${IMPLIB_SUFFIX} )
    elseif( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
      set_property( TARGET ${_target} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-z,defs" )
    endif()
    target_compile_definitions( ${_target} PUBLIC UDA_CXX_SHARED PRIVATE UDA_CXX_EXPORT )
  endif()

  install( TARGETS ${_target} EXPORT ${_export}
    ARCHIVE DESTINATION "${UDA_INSTALL_LIBDIR}" COMPONENT development
    LIBRARY DESTINATION "${UDA_INSTALL_LIBDIR}" COMPONENT ${_component} NAMELINK_COMPONENT development
    RUNTIME DESTINATION "${UDA_INSTALL_BINDIR}" COMPONENT ${_component}
  )
endfunction()

uda_add_cpp_wrapper( STATIC )
if( BUILD_SHARED_LIBS )
  uda_add_cpp_wrapper( SHARED )
endif()

include( CMakePackageConfigHelpers )
set( _cpp_package_dir "${UDA_INSTALL_LIBDIR}/cmake/UDACpp" )
configure_package_config_file(
  UDACppConfig.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/UDACppConfig.cmake"
  INSTALL_DESTINATION "${_cpp_package_dir}"
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/UDACppConfigVersion.cmake"
  VERSION "${UDA_VERSION}"
  COMPATIBILITY ExactVersion
)
install( FILES
  "${CMAKE_CURRENT_BINARY_DIR}/UDACppConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/UDACppConfigVersion.cmake"
  DESTINATION "${_cpp_package_dir}"
  COMPONENT development
)
install( EXPORT UDACppTargets
  NAMESPACE UDA::
  DESTINATION "${_cpp_package_dir}"
  COMPONENT development
)
if( BUILD_SHARED_LIBS )
  install( EXPORT UDACppSharedTargets
    NAMESPACE UDA::
    DESTINATION "${_cpp_package_dir}"
    COMPONENT client-runtime
  )
endif()
