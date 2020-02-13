########################################################################################################################
# Dependencies

find_package( IDL )
find_package( Java 1.6 )
find_package( JNI )
find_package( Boost )

if( Boost_FOUND )
  add_subdirectory( c++ )
else()
  message( WARNING "Boost not found - not building C++ wrapper" )
endif()

if( NOT NO_PYTHON_WRAPPER )
  add_subdirectory( python )
else()
  message( WARNING "Python wrapper ignored by command" )
endif()

if( IDL_FOUND )
  add_subdirectory( idl )
else()
  message( WARNING "IDL not found - not building IDL wrapper" )
endif()

if( NOT NO_JAVA_WRAPPER )
  if( JAVA_FOUND AND JNI_FOUND )
    add_subdirectory( java )
  else()
    message( WARNING "JAVA or JNI not found - not building Java wrapper" )
  endif()
else()
  message( WARNING "Java wrapper ignored by command" )
endif()

#add_subdirectory( matlab )
#add_subdirectory( PHP )
