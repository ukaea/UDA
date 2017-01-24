cmake_minimum_required( VERSION 2.8 )

set( SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../.. )

set( JAR_NAME Idam )
set( LIB_NAME idamjni )
set( MAJOR_VERSION 1 )
set( MINOR_VERSION 8 )

find_package( Java REQUIRED )
find_package( JNI REQUIRED )
include( UseJava )

set( CMAKE_JAVA_COMPILE_FLAGS "-source" "1.6" "-target" "1.6" )

include_directories(
  ${SRC_DIR}
  ${SRC_DIR}/include
  ${JNI_INCLUDE_DIRS}
)

add_library( ${LIB_NAME} SHARED idam_jni.c )

target_link_libraries( ${LIB_NAME} LINK_PUBLIC ${IDC_LIB_NAME} )

add_jar( ${JAR_NAME} Idam.java )

set_target_properties( ${LIB_NAME}
  PROPERTIES
    VERSION ${MAJOR_VERSION}.${MINOR_VERSION}
    SOVERSION ${MAJOR_VERSION}
)

install( TARGETS ${LIB_NAME}
  DESTINATION lib
)
