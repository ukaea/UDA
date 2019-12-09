########################################################################################################################
# Macro used to convert Windows long path to short (without space)
macro( SHORT_WIN_PATH VarName )
  if( WIN32 AND NOT "${${VarName}}" STREQUAL "" )
    execute_process( COMMAND FOR %%I IN ("${${VarName}}") DO SET ${${VarName}}=%%~sI OUTPUT_VARIABLE ${VarName} )
  endif()
endmacro()


########################################################################################################################
# Dependencies

find_package( IDL )
find_package( Java 1.6 )
find_package( JNI )
find_package( SWIG 3.0 )
find_package( Boost )

# For Windows, shorten the Java path to avoid space in "Program Files"
SHORT_WIN_PATH( JAVA_AWT_INCLUDE_PATH )
SHORT_WIN_PATH( JAVA_AWT_LIBRARY )
SHORT_WIN_PATH( JAVA_INCLUDE_PATH )
SHORT_WIN_PATH( JAVA_INCLUDE_PATH2 )
SHORT_WIN_PATH( JAVA_JVM_LIBRARY )
SHORT_WIN_PATH( Java_IDLJ_EXECUTABLE )
SHORT_WIN_PATH( Java_JARSIGNER_EXECUTABLE )
SHORT_WIN_PATH( Java_JAR_EXECUTABLE )
SHORT_WIN_PATH( Java_JAVAC_EXECUTABLE )
SHORT_WIN_PATH( Java_JAVADOC_EXECUTABLE )
SHORT_WIN_PATH( Java_JAVAH_EXECUTABLE )
SHORT_WIN_PATH( Java_JAVA_EXECUTABLE )


if( Boost_FOUND )
  add_subdirectory( c++ )

  # Python wrapper can only be build if c++ wrapper is also being built
  if( WIN32 OR MINGW )
    find_package( PythonLibs 3.3 )
    if( SWIG_FOUND AND PYTHONLIBS_FOUND )
      add_definitions( -DSWIG )
      add_subdirectory( python )
    else()
      message( WARNING "SWIG or PythonLibs not found - not building Python wrapper" )
    endif()
  else()
    if( SWIG_FOUND )
      add_definitions( -DSWIG )
      add_subdirectory( python )
    else()
      message( WARNING "SWIG not found - not building Python wrapper" )
    endif()
  endif()
else()
  message( WARNING "Boost not found - not building C++ (or Python) wrapper" )
endif()

if( IDL_FOUND )
  add_subdirectory( idl )
else()
  message( WARNING "IDL not found - not building IDL wrapper" )
endif()

if( JAVA_FOUND AND JNI_FOUND )
  add_subdirectory( java )
else()
  message( WARNING "JAVA or JNI not found - not building Java wrapper" )
endif()

#add_subdirectory( matlab )
#add_subdirectory( PHP )
