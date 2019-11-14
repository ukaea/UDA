find_package( IDL REQUIRED )
find_package( LibXml2 )
find_package( NetCDF )

set( LIB_NAME ${PROJECT_NAME}_idl )

set( SRC_FILES idam_dlm.c idamStructs.c )

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${IDL_INCLUDES}
)

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

  target_link_libraries( ${LIB_NAME} LINK_PRIVATE ${IDL_LIBRARIES} ${LIBXML2_LIBRARIES} )
  link_modules( ${LIB_NAME} )
else()
  target_link_libraries( ${LIB_NAME} LINK_PRIVATE client-static ${IDL_LIBRARIES} )
endif()

set_target_properties( ${LIB_NAME}
  PROPERTIES
    PREFIX ""
    OUTPUT_NAME ${PROJECT_NAME}
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    SOVERSION ${PROJECT_VERSION_MAJOR}
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
  calluda.pro
)

install( FILES ${INSTALL_FILES}
  DESTINATION idl
)
