########################################################################################################################
set( SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../.. )

set( JAR_NAME UDA )
set( LIB_NAME ${PROJECT_NAME}_jni )

# Dependencies
find_package( Java REQUIRED )
find_package( JNI REQUIRED )
if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
else()
  find_package( TIRPC QUIET)
  if(TIRPC_FOUND)
    add_definitions(-D__TIRPC__)
    
      endif()
endif()

include( UseJava )

set( CMAKE_JAVA_COMPILE_FLAGS "-source" "1.6" "-target" "1.6" )

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${JNI_INCLUDE_DIRS}
)

if( MINGW OR WIN32 )
  include_directories( ${XDR_INCLUDE_DIR} )
elseif(TIRPC_FOUND)
  include_directories( ${TIRPC_INCLUDE_DIR} )
endif()

add_library( ${LIB_NAME} SHARED idam_jni.c )

target_link_libraries( ${LIB_NAME} LINK_PRIVATE client-static )

set( CMAKE_JNI_TARGET TRUE )
if( WIN32 )
  # For Windows, no need to specify a version because it generates an symlink, which is not supported
  add_jar( ${JAR_NAME} jIdam/Idam.java )
else()
  add_jar( ${JAR_NAME} jIdam/Idam.java
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
  )
  install_jni_symlink( ${JAR_NAME} java )
endif()

install_jar( ${JAR_NAME} java )

set_target_properties( ${LIB_NAME}
  PROPERTIES
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    C_STANDARD 99
)
if( WIN32 )
  set_target_properties( ${LIB_NAME}
    PROPERTIES
      COMPILE_FLAGS -DLIBRARY_EXPORTS
      IMPORT_SUFFIX ${IMPLIB_SUFFIX}
  )
endif()

install( TARGETS ${LIB_NAME}
  DESTINATION lib
)
