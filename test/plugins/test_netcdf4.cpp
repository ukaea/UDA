#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/UDA.hpp>

#define ARR_LEN(NAME) sizeof(NAME) / sizeof(NAME[0])
#define ARR2VEC(TYPE, NAME) std::vector<TYPE>(&NAME[0], &NAME[ARR_LEN(NAME)])

TEST_CASE( "Test NEWCDF4::help() function", "[NEWCDF4][plugins]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("NEWCDF4::help()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != NULL );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    auto str = dynamic_cast<uda::String*>(data);

    REQUIRE( str != NULL );

    std::string expected = "\nnetCDF4: get - Read data from a netCDF4 file\n\n";

    REQUIRE( str->str() == expected );
}

TEST_CASE( "Test NEWCDF4::read() with scalar data variable", "[NEWCDF4][plugins]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("NEWCDF4::read(signal=/testGroup/simpleVar, file=" TEST_DATA_DIR "/test_scalar_data.nc)", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != NULL );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(float).name() );

    uda::Scalar* val = dynamic_cast<uda::Scalar*>(data);

    REQUIRE( val != NULL );

    REQUIRE( val->as<float>() == Approx(4.5) );
}

TEST_CASE( "Test NEWCDF4::read() with array data variable", "[NEWCDF4][plugins]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("NEWCDF4::read(signal=testvar, file=" TEST_DATA_DIR "/test_array_data.nc)", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode root = result.tree();

    REQUIRE( root.numChildren() == 1 );

    uda::TreeNode child = root.child(0);

    std::string exp_names[] = { "centreR", "centreZ", "dR", "dZ" };
    bool exp_pointers[] = { false, false, false, false };
    std::string exp_types[] = { "float", "float", "float", "float" };
    size_t exp_ranks[] = { 1, 1, 1, 1 };

    REQUIRE( child.name() == "data" );
    REQUIRE( child.numChildren() == 0 );
    REQUIRE( child.atomicCount() == 4 );
    REQUIRE( child.atomicNames() == ARR2VEC( std::string, exp_names ) );
    REQUIRE( child.atomicPointers() == ARR2VEC( bool, exp_pointers ) );
    REQUIRE( child.atomicTypes() == ARR2VEC( std::string, exp_types ) );
    REQUIRE( child.atomicRank() == ARR2VEC( size_t, exp_ranks ) );

    float exp_centreR[] = { 0.0f, 1.2f, 3.4f, 5.1f };
    float exp_centreZ[] = { 0.1f, 2.3f, 5.1f, 2.5f };
    float exp_dR[] = { 0.2f, 1.2f, 55.2f, 0.6f };
    float exp_dZ[] = { 0.1f, 2.3f, 5.1f, 2.5f };

    uda::Vector centreR = child.atomicVector("centreR");
    REQUIRE( !centreR.isNull() );
    REQUIRE( centreR.type().name() == typeid(float).name() );
    REQUIRE( centreR.as<float>() == ARR2VEC( float, exp_centreR ) );

    uda::Vector centreZ = child.atomicVector("centreZ");
    REQUIRE( !centreZ.isNull() );
    REQUIRE( centreZ.type().name() == typeid(float).name() );
    REQUIRE( centreZ.as<float>() == ARR2VEC( float, exp_centreZ ) );

    uda::Vector dR = child.atomicVector("dR");
    REQUIRE( !dR.isNull() );
    REQUIRE( dR.type().name() == typeid(float).name() );
    REQUIRE( dR.as<float>() == ARR2VEC( float, exp_dR ) );

    uda::Vector dZ = child.atomicVector("dZ");
    REQUIRE( !dZ.isNull() );
    REQUIRE( dZ.type().name() == typeid(float).name() );
    REQUIRE( dZ.as<float>() == ARR2VEC( float, exp_dZ ) );
}

TEST_CASE( "Test NEWCDF4::read() with compound data types", "[NEWCDF4][plugins]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("NEWCDF4::read(signal=/magnetics/pfcoil/d1_upper, file=" TEST_DATA_DIR "/testfile.nc)", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode root = result.tree();

    REQUIRE( root.numChildren() == 1 );

    uda::TreeNode child = root.child(0);

    std::string exp_names[] = { "name", "refFrame", "status", "version", "phi_cut" };
    bool exp_pointers[] = { false, false, false, false, false };
    std::string exp_types[] = { "STRING", "STRING", "STRING", "STRING", "float" };
    size_t exp_ranks[] = { 1, 1, 1, 1, 0 };

    REQUIRE( child.name() == "data" );
    REQUIRE( child.numChildren() == 3 );
    REQUIRE( child.atomicCount() == 5 );
    REQUIRE( child.atomicNames() == ARR2VEC( std::string, exp_names ) );
    REQUIRE( child.atomicPointers() == ARR2VEC( bool, exp_pointers ) );
    REQUIRE( child.atomicTypes() == ARR2VEC( std::string, exp_types ) );
    REQUIRE( child.atomicRank() == ARR2VEC( size_t, exp_ranks ) );

    uda::Scalar name = child.atomicScalar("name");
    REQUIRE( !name.isNull() );
    REQUIRE( name.type().name() == typeid(char*).name() );
    REQUIRE( std::string(name.as<char*>()) == "d1_upper" );

    uda::Scalar refFrame = child.atomicScalar("refFrame");
    REQUIRE( !refFrame.isNull() );
    REQUIRE( refFrame.type().name() == typeid(char*).name() );
    REQUIRE( std::string(refFrame.as<char*>()) == "Machine" );

    uda::Scalar status = child.atomicScalar("status");
    REQUIRE( !status.isNull() );
    REQUIRE( status.type().name() == typeid(char*).name() );
    REQUIRE( std::string(status.as<char*>()) == "NOTCOMMISSIONED" );

    uda::Scalar version = child.atomicScalar("version");
    REQUIRE( !version.isNull() );
    REQUIRE( version.type().name() == typeid(char*).name() );
    REQUIRE( std::string(version.as<char*>()) == "0.1" );

    uda::Scalar phi_cut = child.atomicScalar("phi_cut");
    REQUIRE( !phi_cut.isNull() );
    REQUIRE( phi_cut.type().name() == typeid(float).name() );
    REQUIRE( phi_cut.as<float>() == Approx(299.9696f) );

    std::string exp_child_names[] = { "geom_simplified", "geom_elements", "geom_case" };

    std::string exp_subchild_names[] = { "centreR", "centreZ", "dR", "dZ" };
    bool exp_subchild_pointers[] = { false, false, false, false };
    std::string exp_subchild_types[] = { "float", "float", "float", "float" };

    size_t exp_subchild_ranks[][4] = {
            { 0, 0, 0, 0 },
            { 1, 1, 1, 1 },
            { 1, 1, 1, 1 },
    };

    for (int child_idx = 0; child_idx < 3; ++child_idx) {
        uda::TreeNode subchild = child.child(child_idx);

        REQUIRE( subchild.name() == exp_child_names[child_idx] );
        REQUIRE( subchild.numChildren() == 0 );
        REQUIRE( subchild.atomicCount() == 4 );
        REQUIRE( subchild.atomicNames() == ARR2VEC( std::string, exp_subchild_names ) );
        REQUIRE( subchild.atomicPointers() == ARR2VEC( bool, exp_subchild_pointers ) );
        REQUIRE( subchild.atomicTypes() == ARR2VEC( std::string, exp_subchild_types ) );
        REQUIRE( subchild.atomicRank() == ARR2VEC( size_t, exp_subchild_ranks[child_idx] ) );

        if (child_idx == 0) {
            uda::Scalar centreR = subchild.atomicScalar("centreR");
            REQUIRE( !centreR.isNull() );
            REQUIRE( centreR.type().name() == typeid(float).name() );
            REQUIRE( centreR.as<float>() == Approx(389.4999f) );

            uda::Scalar centreZ = subchild.atomicScalar("centreZ");
            REQUIRE( !centreZ.isNull() );
            REQUIRE( centreZ.type().name() == typeid(float).name() );
            REQUIRE( centreZ.as<float>() == Approx(1578.5f) );

            uda::Scalar dR = subchild.atomicScalar("dR");
            REQUIRE( !dR.isNull() );
            REQUIRE( dR.type().name() == typeid(float).name() );
            REQUIRE( dR.as<float>() == Approx(86.26537f) );

            uda::Scalar dZ = subchild.atomicScalar("dZ");
            REQUIRE( !dZ.isNull() );
            REQUIRE( dZ.type().name() == typeid(float).name() );
            REQUIRE( dZ.as<float>() == Approx(86.26534f) );
        } else if (child_idx == 1) {
            float exp_centreR[] = {352.75f, 367.450134f, 382.150146f, 396.850067f, 352.75f, 352.75f, 352.75f, 352.75f,
                                   352.750397f, 367.450378f, 367.450409f, 367.450409f, 367.450409f, 367.450012f,
                                   382.150024f, 382.150024f, 382.150024f, 382.150024f, 396.850128f, 396.850128f,
                                   396.850128f, 396.850128f, 396.850098f, 411.55014f, 411.55014f, 411.55014f,
                                   411.55014f, 411.55011f, 426.250366f, 426.250061f, 426.250061f, 426.250061f,
                                   426.250061f, 411.550018f, 426.250092f};
            float exp_centreZ[] = {1615.25, 1615.25, 1615.25, 1615.25, 1600.55, 1585.85, 1571.15, 1556.45, 1541.75,
                                   1541.75, 1556.45, 1571.15, 1585.85, 1600.55, 1585.85, 1571.15, 1556.45, 1541.75,
                                   1541.75, 1556.45, 1571.15, 1585.85, 1600.55, 1600.55, 1585.85, 1571.15, 1556.45,
                                   1541.75, 1541.75, 1556.45, 1571.15, 1585.85, 1600.55, 1615.25, 1615.25};
            float exp_dR[] = {12.7653418f, 12.7653465f, 12.7653465f, 12.7653437f, 12.7653427f, 12.7653427f, 12.7653418f,
                              12.7653418f, 12.7653561f, 12.7653551f, 12.7653551f, 12.7653551f, 12.7653551f, 12.7653418f,
                              12.7653427f, 12.7653427f, 12.7653427f, 12.7653427f, 12.7653456f, 12.7653456f, 12.7653456f,
                              12.7653456f, 12.7653446f, 12.7653456f, 12.7653456f, 12.7653456f, 12.7653456f, 12.7653446f,
                              12.7653522f, 12.7653427f, 12.7653427f, 12.7653427f, 12.7653427f, 12.7653418f, 12.7653437f};
            float exp_dZ[] = {12.7653408f, 12.7653408f, 12.7653408f, 12.7653408f, 12.7653408f, 12.7653408f, 12.7653408f,
                              12.7653408f, 12.7653408f, 12.7653408f, 12.7653408f, 12.7653408f, 12.7653408f, 12.7653408f,
                              12.7653408f, 12.7653408f, 12.7653408f, 12.7653408f, 12.7653408f, 12.7653408f, 12.7653408f,
                              12.7653408f, 12.7653408f, 12.7653408f, 12.7653408f, 12.7653408f, 12.7653408f, 12.7653408f,
                              12.7653408f, 12.7653408f, 12.7653408f, 12.7653408f, 12.7653408f, 12.7653408f, 12.7653408f};

            uda::Vector centreR = subchild.atomicVector("centreR");
            REQUIRE( !centreR.isNull() );
            REQUIRE( centreR.type().name() == typeid(float).name() );
            REQUIRE( centreR.as<float>() == ARR2VEC( float, exp_centreR ) );

            uda::Vector centreZ = subchild.atomicVector("centreZ");
            REQUIRE( !centreZ.isNull() );
            REQUIRE( centreZ.type().name() == typeid(float).name() );
            REQUIRE( centreZ.as<float>() == ARR2VEC( float, exp_centreZ ) );

            uda::Vector dR = subchild.atomicVector("dR");
            REQUIRE( !dR.isNull() );
            REQUIRE( dR.type().name() == typeid(float).name() );
            REQUIRE( dR.as<float>() == ARR2VEC( float, exp_dR ) );

            uda::Vector dZ = subchild.atomicVector("dZ");
            REQUIRE( !dZ.isNull() );
            REQUIRE( dZ.type().name() == typeid(float).name() );
            REQUIRE( dZ.as<float>() == ARR2VEC( float, exp_dZ ) );
        } else if (child_idx == 2) {
            float exp_centreR[] = {336.448761f, 391.448761f, 391.448761f, 442.448761f};
            float exp_centreZ[] = {1578.5f, 1525.0f, 1632.0f, 1578.5f};
            float exp_dR[] = {4.0f, 106.0f, 106.0f, 4.0f};
            float exp_dZ[] = {111.0f, 4.0f, 4.0f, 103.0f};

            uda::Vector centreR = subchild.atomicVector("centreR");
            REQUIRE( !centreR.isNull() );
            REQUIRE( centreR.type().name() == typeid(float).name() );
            REQUIRE( centreR.as<float>() == ARR2VEC( float, exp_centreR ) );

            uda::Vector centreZ = subchild.atomicVector("centreZ");
            REQUIRE( !centreZ.isNull() );
            REQUIRE( centreZ.type().name() == typeid(float).name() );
            REQUIRE( centreZ.as<float>() == ARR2VEC( float, exp_centreZ ) );

            uda::Vector dR = subchild.atomicVector("dR");
            REQUIRE( !dR.isNull() );
            REQUIRE( dR.type().name() == typeid(float).name() );
            REQUIRE( dR.as<float>() == ARR2VEC( float, exp_dR ) );

            uda::Vector dZ = subchild.atomicVector("dZ");
            REQUIRE( !dZ.isNull() );
            REQUIRE( dZ.type().name() == typeid(float).name() );
            REQUIRE( dZ.as<float>() == ARR2VEC( float, exp_dZ ) );
        }
    }
}
