find_package( IDL REQUIRED )
find_package( LibXml2  REQUIRED )
find_package( NetCDF )

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
  if( NOT MINGW )
    find_package( dlfcn-win32 CONFIG REQUIRED )
  endif()
else()
  find_package( TIRPC QUIET )
  if( TIRPC_FOUND )
    add_definitions( -D__TIRPC__ )
  endif()
endif()

set( EXTRA_IDL_WRAPPER_LINK_ARGS "" CACHE STRING "exact string to add to linker arguments, e.g. for setting rpath" )
set( LIB_NAME ${PROJECT_NAME}_idl )
set( SRC_FILES idam_dlm.c idamStructs.c )

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${IDL_INCLUDES}
  ${LIBXML2_INCLUDE_DIR}
)

if( MINGW OR WIN32 )
  include_directories( ${XDR_INCLUDE_DIR} )
elseif( TIRPC_FOUND )
  include_directories( ${TIRPC_INCLUDE_DIR} )
endif()

if( FAT_IDL )
  add_definitions( -DFATCLIENT )

  set( FATCLIENT_OBJS
    $<TARGET_OBJECTS:fatclient-objects>
    $<TARGET_OBJECTS:fatserver-objects>
    $<TARGET_OBJECTS:fatclientserver-objects>
    $<TARGET_OBJECTS:structures-server-objects>
    $<TARGET_OBJECTS:fatmodules-objects>
    $<TARGET_OBJECTS:cache-objects>
    $<TARGET_OBJECTS:logging-server-objects>
    $<TARGET_OBJECTS:plugins-objects>
  )

  add_library( ${LIB_NAME} SHARED ${SRC_FILES} ${FATCLIENT_OBJS} )
else()
  add_library( ${LIB_NAME} SHARED ${SRC_FILES} )
endif()

if( FAT_IDL )
  include( modules )

  target_link_libraries( ${LIB_NAME} LINK_PRIVATE ${IDL_LIBRARIES} ${LIBXML2_LIBRARIES} ${EXTRA_IDL_WRAPPER_LINK_ARGS} )
  link_modules( ${LIB_NAME} )
else()
  target_link_libraries( ${LIB_NAME} LINK_PRIVATE client-static ${IDL_LIBRARIES} ${LIBXML2_LIBRARIES} ${EXTRA_IDL_WRAPPER_LINK_ARGS} )
endif()

set_target_properties( ${LIB_NAME}
  PROPERTIES
    PREFIX ""
    OUTPUT_NAME ${PROJECT_NAME}
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    C_STANDARD 99
    COMPILE_WARNING_AS_ERROR OFF
    LINK_FLAGS ${EXTRA_IDL_WRAPPER_LINK_ARGS}
)

if( FAT_IDL )
  install( TARGETS ${LIB_NAME}
    DESTINATION fatdlm
  )

  install( FILES ${PROJECT_NAME}.dlm
    DESTINATION fatdlm
  )
else()
  install( TARGETS ${LIB_NAME}
    DESTINATION dlm
  )

  install( FILES ${PROJECT_NAME}.dlm
    DESTINATION dlm
  )
endif()

set( INSTALL_FILES
  getstruct.pro
  getdata.pro
  getgeomdata.pro
  calluda.pro
  getfile.pro
  gettext.pro
)

install( FILES ${INSTALL_FILES}
  DESTINATION idl
)
