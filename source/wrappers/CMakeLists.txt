########################################################################################################################
# Dependencies

find_package( IDL )
find_package( Boost )

if( NOT NO_CXX_WRAPPER )
  if( Boost_FOUND )
    add_subdirectory( c++ )
  else()
    message( WARNING "Boost not found - not building C++ wrapper" )
  endif()
else()
  message( WARNING "C++ wrapper not being built due to CMake option NO_CXX_WRAPPER" )
endif()

if( NOT NO_PYTHON_WRAPPER )
  add_subdirectory( python )
else()
  message( WARNING "Python wrapper not being built due to CMake option NO_PYTHON_WRAPPER" )
endif()

if( NOT NO_IDL_WRAPPER )
  if( IDL_FOUND )
    remove_definitions( -Wall -Werror )
    add_subdirectory( idl )
    add_definitions( -Wall -Werror )
  else()
    message( WARNING "IDL not found - not building IDL wrapper" )
  endif()
else()
  message( WARNING "IDL wrapper not being built due to CMake option NO_IDL_WRAPPER" )
endif()
