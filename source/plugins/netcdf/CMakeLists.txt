find_package( NetCDF QUIET )
find_package( HDF5 COMPONENTS C HL QUIET )
find_package( fmt QUIET )

if( NOT NETCDF_FOUND )
  message( WARNING "NetCDF not found - skipping netcdf4 plugin" )
  return()
elseif( NOT fmt_FOUND )
  message( WARNING "fmt not found - skipping netcdf4 plugin" )
  return()
endif()

# HDF5 may be used by NetCDF
set( HDF5 )
if( HDF5_FOUND )
  set( HDF5 ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES} )
endif()

include( plugins )

uda_plugin(
  NAME NEW_NETCDF
  EXTRA_NAMES CDF7 NCDF7 HDF57
  ENTRY_FUNC capnp_netcdf
  DESCRIPTION "netCDF4 Data Reader"
  EXAMPLE "NEW_NETCDF::read()"
  LIBNAME new_netcdf_plugin
  SOURCES
    netcdf_plugin.cpp netcdf_plugin.hpp
    netcdf_reader.cpp netcdf_reader.hpp
    capnp_reader.cpp capnp_reader.hpp
    netcdfpp.h
  EXTRA_INCLUDE_DIRS
    ${NETCDF_INCLUDES}
  EXTRA_LINK_LIBS
    ${NETCDF_LIBRARIES}
	  ${HDF5}
    fmt::fmt
)

set ( SOURCE_FILES
    netcdf_reader.cpp netcdf_reader.hpp
    capnp_reader.cpp capnp_reader.hpp
    netcdfpp.h
    test/capnp_json_reader.cpp test/capnp_json_reader.hpp
    test/random_generator.hpp
    test/write_testfile.cpp test/write_testfile.hpp
    )

find_package( nlohmann_json QUIET )
find_package( Catch2 3 QUIET )
find_package( Boost COMPONENTS program_options REQUIRED )

if ( nlohmann_json_FOUND )
    add_executable( test_netcdf_json test/simple_test.cpp 
        ${SOURCE_FILES} 
        ${NETCDF_INCLUDES} 
    )
    target_link_libraries( test_netcdf_json PRIVATE 
        nlohmann_json::nlohmann_json 
        serialisation-static 
        ${NETCDF_LIBRARIES} 
        fmt::fmt 
    )
    target_include_directories(test_netcdf_json PRIVATE 
        ${uda_SOURCE_DIR}/source/plugins/netcdf 
        ${uda_SOURCE_DIR}/source/plugins/netcdf/test 
    )
    set_property( TARGET test_netcdf_json PROPERTY CXX_STANDARD 17 )

    # add_executable( test_epm test/epm_test.cpp ${SOURCE_FILES} ${NETCDF_INCLUDES} )
    # target_link_libraries( test_epm PRIVATE nlohmann_json::nlohmann_json serialisation-static ${NETCDF_LIBRARIES} fmt::fmt )
    # target_include_directories(test_epm PRIVATE ${uda_SOURCE_DIR}/source/plugins/netcdf ${uda_SOURCE_DIR}/source/plugins/netcdf/test )
    # set_property( TARGET test_epm PROPERTY CXX_STANDARD 17 )

    add_executable(nc2json test/nc2json.cpp
        ${SOURCE_FILES}
        ${NETCDF_INCLUDES}
        )
    target_include_directories(nc2json PRIVATE 
        ${uda_SOURCE_DIR}/source/plugins/netcdf 
        ${uda_SOURCE_DIR}/source/plugins/netcdf/test 
    )
    set_property( TARGET nc2json PROPERTY CXX_STANDARD 17 )
    target_link_libraries( nc2json PRIVATE 
        nlohmann_json::nlohmann_json 
        serialisation-static 
        ${NETCDF_LIBRARIES} 
        fmt::fmt 
        Boost::program_options
    )

    if ( Catch2_FOUND)
        add_executable( test_netcdf_reader 
            test/netcdf_reader.test.cpp 
            ${SOURCE_FILES} 
            ${NETCDF_INCLUDES} 
        )
        target_link_libraries( test_netcdf_reader PRIVATE 
            nlohmann_json::nlohmann_json 
            serialisation-static 
            ${NETCDF_LIBRARIES} 
            fmt::fmt 
            Catch2::Catch2WithMain
        )
        target_include_directories( test_netcdf_reader PRIVATE 
            ${uda_SOURCE_DIR}/source/plugins/netcdf 
            ${uda_SOURCE_DIR}/source/plugins/netcdf/test 
        )
        set_property( TARGET test_netcdf_reader PROPERTY CXX_STANDARD 17 )

        include(CTest)
        include(Catch)
        catch_discover_tests(test_netcdf_reader
            TEST_PREFIX netcdf
            )

    else()
        message( WARNING "Catch2 package not found - skipping automated netcdf-capnp tests" )
    endif() 

else()
    message( WARNING "nlohmann_json package not found - skipping netcdf-capnp tests" )
endif()
