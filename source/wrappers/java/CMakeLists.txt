cmake_minimum_required( VERSION 2.8 )

set( SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../.. )

set( JAR_NAME UDA )
set( LIB_NAME ${PROJECT_NAME}_jni )

find_package( Java REQUIRED )
find_package( JNI REQUIRED )
include( UseJava )

set( CMAKE_JAVA_COMPILE_FLAGS "-source" "1.6" "-target" "1.6" )

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${JNI_INCLUDE_DIRS}
)

add_library( ${LIB_NAME} SHARED idam_jni.c )

target_link_libraries( ${LIB_NAME} LINK_PUBLIC client-static )

add_jar( ${JAR_NAME} Idam.java )

set_target_properties( ${LIB_NAME}
  PROPERTIES
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

install( TARGETS ${LIB_NAME}
  DESTINATION lib
)
