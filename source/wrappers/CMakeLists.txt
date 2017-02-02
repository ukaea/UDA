cmake_minimum_required( VERSION 2.8 )

find_package( IDL )
find_package( Java 1.6 )
find_package( JNI )
find_package( PythonLibs 3.3 )
find_package( SWIG 3.0 )
find_package( Boost )

if( BOOST_FOUND )
  add_subdirectory( c++ )

  # Python wrapper can only be build if c++ wrapper is also being built
  if( SWIG_FOUND AND PYTHONLIBS_FOUND AND NOT APPLE )
    add_subdirectory( python )
  else()
    message( WARNING "SWIG or PythonLibs not found - not building Python wrapper" )
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
