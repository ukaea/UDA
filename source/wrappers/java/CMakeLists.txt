# Macro used to convert Windows path to Unix
macro( CONVERT_WIN_PATH VarName )
  if( MINGW AND NOT "${${VarName}}" STREQUAL "" 	)
    execute_process( COMMAND cygpath.exe -u ${${VarName}} OUTPUT_VARIABLE ${VarName} )
    string( STRIP ${${VarName}} ${VarName} )
  endif()
endmacro()

########################################################################################################################
set( SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../.. )

set( JAR_NAME UDA )
set( LIB_NAME ${PROJECT_NAME}_jni )

# Dependencies
find_package( Java REQUIRED )
find_package( JNI REQUIRED )
if( MINGW )
  CONVERT_WIN_PATH( JAVA_HOME )
  CONVERT_WIN_PATH( JAVA_INCLUDE_PATH )
  CONVERT_WIN_PATH( JAVA_INCLUDE_PATH2 )
  CONVERT_WIN_PATH( JAVA_JVM_LIBRARY )
  CONVERT_WIN_PATH( JNI_INCLUDE_DIRS )
  CONVERT_WIN_PATH( JNI_LIBRARIES )
endif()

include( UseJava )

set( CMAKE_JAVA_COMPILE_FLAGS "-source" "1.6" "-target" "1.6" )

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${JNI_INCLUDE_DIRS}
)

add_library( ${LIB_NAME} SHARED idam_jni.c )

target_link_libraries( ${LIB_NAME} LINK_PRIVATE client-static )

set( CMAKE_JNI_TARGET TRUE )
add_jar( ${JAR_NAME} jIdam/Idam.java
  VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
)
install_jar( ${JAR_NAME} java )
install_jni_symlink( ${JAR_NAME} java )

set_target_properties( ${LIB_NAME}
  PROPERTIES
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

install( TARGETS ${LIB_NAME}
  DESTINATION lib
)
