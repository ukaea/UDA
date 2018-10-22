#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "test_helpers.h"

#include <c++/UDA.hpp>

TEST_CASE( "Test read first frame from IPX version 1", "[IMAS][JET][TF]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("NEWIPX::read(filename='" TEST_DATA_DIR "/rgb030420.ipx', frame=0)", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode video = tree.child(0);

    REQUIRE( video.name() == "data" );
    REQUIRE( video.numChildren() == 1 );
    REQUIRE( video.atomicCount() == 22 );

    std::vector<std::string> exp_names = { "datetime", "shot", "lens", "filter", "view", "camera", "is_color",
                                           "width", "height", "depth", "taps", "left", "top", "hbin", "vbin",
                                           "offset", "gain", "preexp", "exposure", "board_temp", "ccd_temp",
                                           "n_frames" };
    REQUIRE( video.atomicNames() == exp_names );

    std::vector<bool> exp_ptrs = { false, false, false, false, false, false, false, false, false, false, false, false,
                                   false, false, false, false, false, false, false, false, false, false };
    REQUIRE( video.atomicPointers() == exp_ptrs );

    std::vector<std::string> exp_types = { "STRING", "int", "STRING", "STRING", "STRING", "STRING", "int", "int",
                                           "int", "int", "int", "int", "int", "int", "int", "double", "double",
                                           "double", "double", "double", "double", "int" };
    REQUIRE( video.atomicTypes() == exp_types );

    std::vector<size_t> exp_rank = { 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0 };
    REQUIRE( video.atomicRank() == exp_rank );

    std::vector<std::vector<size_t>> exp_shapes = { {256}, {1}, {256}, {256}, {256}, {256}, {1}, {1}, {1}, {1}, {1},
                                                    {1}, {1}, {1}, {1}, {2}, {2}, {1}, {1}, {1}, {1}, {1} };
    REQUIRE( video.atomicShape() == exp_shapes );

    std::vector<const char*> exp_string_values = { "2013-10-25T12:37:18Z", "Fujinon f/1.4", "SW/B/B", "Sector9U",
                                                   "IPX-VGA210LCFN      ASSY-0074-0002-RE04 SW v1.63 BL v1.38 CUST " };
    std::vector<int> exp_int_values = { 30420, 0, 640, 480, 12, 1, 1, 1, 0, 0, 1 };
    std::vector<double> exp_double_values = { 9090.0, 9090.0, 39.25, 0.0 };

    int string_i = 0;
    int int_i = 0;
    int double_i = 0;

    for (const auto& name : exp_names) {
        uda::Scalar value = video.atomicScalar(name);
        if (name != "offset" && name != "gain") {
            REQUIRE( !value.isNull() );
        } else {
            continue;
        }

        if (value.type() == typeid(char*)) {
            REQUIRE( std::string(value.as<char*>()) == exp_string_values[string_i] );
            ++string_i;
        } else if (value.type() == typeid(int)) {
            REQUIRE( value.as<int>() == exp_int_values[int_i] );
            ++int_i;
        } else if (value.type() == typeid(double)) {
            REQUIRE( value.as<double >() == Approx(exp_double_values[double_i]) );
            ++double_i;
        }
    }

    uda::Vector offset = video.atomicVector("offset");
    REQUIRE( !offset.isNull() );
    std::vector<double> exp_offset = { 202.0, 208.0 };
    REQUIRE( offset.as<double>() == ApproxVector(exp_offset) );

    uda::Vector gain = video.atomicVector("gain");
    REQUIRE( !gain.isNull() );
    std::vector<double> exp_gain = { 6.0, 0.0 };
    REQUIRE( gain.as<double>() == ApproxVector(exp_gain) );

    uda::TreeNode frame = video.child(0);

    REQUIRE( frame.name() == "frames" );
    REQUIRE( frame.numChildren() == 0 );
    REQUIRE( frame.atomicCount() == 3 );

    exp_names = { "number", "time", "k" };
    REQUIRE( frame.atomicNames() == exp_names );

    exp_ptrs = { false, false, true };
    REQUIRE( frame.atomicPointers() == exp_ptrs );

    exp_types = { "int", "double", "unsigned short *" };
    REQUIRE( frame.atomicTypes() == exp_types );

    exp_rank = { 0, 0, 0 };
    REQUIRE( frame.atomicRank() == exp_rank );

    exp_shapes = { {1}, {1}, {307200} };
    REQUIRE( frame.atomicShape() == exp_shapes );

    uda::Scalar number = frame.atomicScalar("number");
    REQUIRE( number.as<int>() == 0 );

    uda::Scalar time = frame.atomicScalar("time");
    REQUIRE( time.as<double>() == Approx(-0.0089800863) );

    uda::Vector vec = frame.atomicVector( "k" );
    REQUIRE( vec.size() == 307200 );

    long sum = 0;
    auto vals = vec.as<unsigned short>();
    for (auto val : vals) {
        sum += val;
    }

    REQUIRE( sum == 2744207 );
}

TEST_CASE( "Test read single frame from IPX version 1", "[IMAS][JET][TF]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("NEWIPX::read(filename='" TEST_DATA_DIR "/rgb030420.ipx', frame=25)", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode video = tree.child(0);

    REQUIRE( video.name() == "data" );
    REQUIRE( video.numChildren() == 1 );
    REQUIRE( video.atomicCount() == 22 );

    std::vector<std::string> exp_names = { "datetime", "shot", "lens", "filter", "view", "camera", "is_color",
                                           "width", "height", "depth", "taps", "left", "top", "hbin", "vbin",
                                           "offset", "gain", "preexp", "exposure", "board_temp", "ccd_temp",
                                           "n_frames" };
    REQUIRE( video.atomicNames() == exp_names );

    std::vector<bool> exp_ptrs = { false, false, false, false, false, false, false, false, false, false, false, false,
                                   false, false, false, false, false, false, false, false, false, false };
    REQUIRE( video.atomicPointers() == exp_ptrs );

    std::vector<std::string> exp_types = { "STRING", "int", "STRING", "STRING", "STRING", "STRING", "int", "int",
                                           "int", "int", "int", "int", "int", "int", "int", "double", "double",
                                           "double", "double", "double", "double", "int" };
    REQUIRE( video.atomicTypes() == exp_types );

    std::vector<size_t> exp_rank = { 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0 };
    REQUIRE( video.atomicRank() == exp_rank );

    std::vector<std::vector<size_t>> exp_shapes = { {256}, {1}, {256}, {256}, {256}, {256}, {1}, {1}, {1}, {1}, {1},
                                                    {1}, {1}, {1}, {1}, {2}, {2}, {1}, {1}, {1}, {1}, {1} };
    REQUIRE( video.atomicShape() == exp_shapes );

    std::vector<const char*> exp_string_values = { "2013-10-25T12:37:18Z", "Fujinon f/1.4", "SW/B/B", "Sector9U",
                                                   "IPX-VGA210LCFN      ASSY-0074-0002-RE04 SW v1.63 BL v1.38 CUST " };
    std::vector<int> exp_int_values = { 30420, 0, 640, 480, 12, 1, 1, 1, 0, 0, 1 };
    std::vector<double> exp_double_values = { 9090.0, 9090.0, 39.25, 0.0 };

    int string_i = 0;
    int int_i = 0;
    int double_i = 0;

    for (const auto& name : exp_names) {
        uda::Scalar value = video.atomicScalar(name);
        if (name != "offset" && name != "gain") {
            REQUIRE( !value.isNull() );
        } else {
            continue;
        }

        if (value.type() == typeid(char*)) {
            REQUIRE( std::string(value.as<char*>()) == exp_string_values[string_i] );
            ++string_i;
        } else if (value.type() == typeid(int)) {
            REQUIRE( value.as<int>() == exp_int_values[int_i] );
            ++int_i;
        } else if (value.type() == typeid(double)) {
            REQUIRE( value.as<double >() == Approx(exp_double_values[double_i]) );
            ++double_i;
        }
    }

    uda::Vector offset = video.atomicVector("offset");
    REQUIRE( !offset.isNull() );
    std::vector<double> exp_offset = { 202.0, 208.0 };
    REQUIRE( offset.as<double>() == ApproxVector(exp_offset) );

    uda::Vector gain = video.atomicVector("gain");
    REQUIRE( !gain.isNull() );
    std::vector<double> exp_gain = { 6.0, 0.0 };
    REQUIRE( gain.as<double>() == ApproxVector(exp_gain) );

    uda::TreeNode frame = video.child(0);

    REQUIRE( frame.name() == "frames" );
    REQUIRE( frame.numChildren() == 0 );
    REQUIRE( frame.atomicCount() == 3 );

    exp_names = { "number", "time", "k" };
    REQUIRE( frame.atomicNames() == exp_names );

    exp_ptrs = { false, false, true };
    REQUIRE( frame.atomicPointers() == exp_ptrs );

    exp_types = { "int", "double", "unsigned short *" };
    REQUIRE( frame.atomicTypes() == exp_types );

    exp_rank = { 0, 0, 0 };
    REQUIRE( frame.atomicRank() == exp_rank );

    exp_shapes = { {1}, {1}, {307200} };
    REQUIRE( frame.atomicShape() == exp_shapes );

    uda::Scalar number = frame.atomicScalar("number");
    REQUIRE( number.as<int>() == 25 );

    uda::Scalar time = frame.atomicScalar("time");
    REQUIRE( time.as<double>() == Approx(0.2185909137) );

    uda::Vector vec = frame.atomicVector("k");
    REQUIRE( vec.size() == 307200 );

    long sum = 0;
    auto vals = vec.as<unsigned short>();
    for (auto val : vals) {
        sum += val;
    }

    REQUIRE( sum == 32038259 );

    std::vector<unsigned short> exp_k = { 7, 22, 10, 7, 4, 12, 6, 8, 0, 13 };

    auto k = vec.as<unsigned short>();
    k.resize(10);
    REQUIRE( k == exp_k );

    exp_k = { 1, 0, 1, 0, 14, 4, 7, 1, 2, 5 };

    k = vec.as<unsigned short>();
    auto kk = std::vector<unsigned short>(k.begin() + 307190, k.end());

    REQUIRE( kk == exp_k );
}

TEST_CASE( "Test read all frames from IPX version 1", "[IMAS][JET][TF]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("NEWIPX::read(filename='" TEST_DATA_DIR "/rgb030420.ipx')", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode video = tree.child(0);

    REQUIRE( video.name() == "data" );
    REQUIRE( video.numChildren() == 99 );
    REQUIRE( video.atomicCount() == 22 );

    std::vector<std::string> exp_names = { "datetime", "shot", "lens", "filter", "view", "camera", "is_color",
                                           "width", "height", "depth", "taps", "left", "top", "hbin", "vbin",
                                           "offset", "gain", "preexp", "exposure", "board_temp", "ccd_temp",
                                           "n_frames" };
    REQUIRE( video.atomicNames() == exp_names );

    std::vector<bool> exp_ptrs = { false, false, false, false, false, false, false, false, false, false, false, false,
                                   false, false, false, false, false, false, false, false, false, false };
    REQUIRE( video.atomicPointers() == exp_ptrs );

    std::vector<std::string> exp_types = { "STRING", "int", "STRING", "STRING", "STRING", "STRING", "int", "int",
                                           "int", "int", "int", "int", "int", "int", "int", "double", "double",
                                           "double", "double", "double", "double", "int" };
    REQUIRE( video.atomicTypes() == exp_types );

    std::vector<size_t> exp_rank = { 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0 };
    REQUIRE( video.atomicRank() == exp_rank );

    std::vector<std::vector<size_t>> exp_shapes = { {256}, {1}, {256}, {256}, {256}, {256}, {1}, {1}, {1}, {1}, {1},
                                                    {1}, {1}, {1}, {1}, {2}, {2}, {1}, {1}, {1}, {1}, {1} };
    REQUIRE( video.atomicShape() == exp_shapes );

    std::vector<const char*> exp_string_values = { "2013-10-25T12:37:18Z", "Fujinon f/1.4", "SW/B/B", "Sector9U",
                                                   "IPX-VGA210LCFN      ASSY-0074-0002-RE04 SW v1.63 BL v1.38 CUST " };
    std::vector<int> exp_int_values = { 30420, 0, 640, 480, 12, 1, 1, 1, 0, 0, 99 };
    std::vector<double> exp_double_values = { 9090.0, 9090.0, 39.25, 0.0 };

    int string_i = 0;
    int int_i = 0;
    int double_i = 0;

    for (const auto& name : exp_names) {
        uda::Scalar value = video.atomicScalar(name);
        if (name != "offset" && name != "gain") {
            REQUIRE( !value.isNull() );
        } else {
            continue;
        }

        if (value.type() == typeid(char*)) {
            REQUIRE( std::string(value.as<char*>()) == exp_string_values[string_i] );
            ++string_i;
        } else if (value.type() == typeid(int)) {
            REQUIRE( value.as<int>() == exp_int_values[int_i] );
            ++int_i;
        } else if (value.type() == typeid(double)) {
            REQUIRE( value.as<double >() == Approx(exp_double_values[double_i]) );
            ++double_i;
        }
    }

    uda::Vector offset = video.atomicVector("offset");
    REQUIRE( !offset.isNull() );
    std::vector<double> exp_offset = { 202.0, 208.0 };
    REQUIRE( offset.as<double>() == ApproxVector(exp_offset) );

    uda::Vector gain = video.atomicVector("gain");
    REQUIRE( !gain.isNull() );
    std::vector<double> exp_gain = { 6.0, 0.0 };
    REQUIRE( gain.as<double>() == ApproxVector(exp_gain) );

    std::vector<int> exp_sums = {
            2744207,
            2753578,
            10472617,
            10119079,
            9049764,
            9259192,
            10630244,
            12553347,
            14098187,
            15308968,
            15641697,
            16424349,
            18355640,
            19549280,
            18944664,
            20318935,
            21209104,
            22434386,
            23655941,
            24005361,
            25664982,
            26567714,
            27851750,
            29218804,
            30504494,
            32038259,
            33700757,
            35301984,
            37013401,
            38467104,
            35648042,
            30445127,
            26991551,
            23475395,
            21854605,
            53662658,
            3295277,
            2868991,
            2681948,
            2689057,
            2725826,
            2725546,
            2730518,
            2730236,
            2614803,
            2643570,
            2696321,
            2744784,
            2742681,
            2694582,
            2685452,
            2678515,
            2678980,
            2663877,
            2715718,
            2674938,
            2689971,
            2676795,
            2688600,
            2697473,
            2683250,
            2678413,
            2677472,
            2671100,
            2691833,
            2650072,
            2661535,
            2674949,
            2679492,
            2691941,
            2699661,
            2668503,
            2643482,
            2661289,
            2666871,
            2668836,
            2677627,
            2651329,
            2611731,
            2585793,
            2606677,
            2584177,
            2574413,
            2597892,
            2587676,
            2580751,
            2579697,
            2599836,
            2578467,
            2543731,
            2808747,
            2569292,
            2584069,
            2581517,
            2582914,
            2540754,
            2558420,
            2569188,
            2569565,
    };

    for (int frame_i = 0; frame_i < 99; ++frame_i) {
        uda::TreeNode frame = video.child(frame_i);

        REQUIRE( frame.name() == "frames" );
        REQUIRE( frame.numChildren() == 0 );
        REQUIRE( frame.atomicCount() == 3 );

        exp_names = { "number", "time", "k" };
        REQUIRE( frame.atomicNames() == exp_names );

        exp_ptrs = { false, false, true };
        REQUIRE( frame.atomicPointers() == exp_ptrs );

        exp_types = { "int", "double", "unsigned short *" };
        REQUIRE( frame.atomicTypes() == exp_types );

        exp_rank = { 0, 0, 0 };
        REQUIRE( frame.atomicRank() == exp_rank );

        exp_shapes = { {1}, {1}, {307200} };
        REQUIRE( frame.atomicShape() == exp_shapes );

        uda::Scalar number = frame.atomicScalar("number");
        REQUIRE( number.as<int>() == frame_i );

//        uda::Scalar time = frame.atomicScalar("time");
//        REQUIRE( time.as<double>() == Approx(0.2185909137) );

        uda::Vector vec = frame.atomicVector("k");
        REQUIRE( vec.size() == 307200 );

        long sum = 0;
        auto vals = vec.as<unsigned short>();
        for (auto val : vals) {
            sum += val;
        }

        REQUIRE( sum == exp_sums[frame_i] );
    }
}

TEST_CASE( "Test read range of frames from IPX version 1", "[IMAS][JET][TF]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("NEWIPX::read(filename='" TEST_DATA_DIR "/rgb030420.ipx', first=10, last=30)", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode video = tree.child(0);

    REQUIRE( video.name() == "data" );
    REQUIRE( video.numChildren() == 20 );
    REQUIRE( video.atomicCount() == 22 );

    std::vector<std::string> exp_names = { "datetime", "shot", "lens", "filter", "view", "camera", "is_color",
                                           "width", "height", "depth", "taps", "left", "top", "hbin", "vbin",
                                           "offset", "gain", "preexp", "exposure", "board_temp", "ccd_temp",
                                           "n_frames" };
    REQUIRE( video.atomicNames() == exp_names );

    std::vector<bool> exp_ptrs = { false, false, false, false, false, false, false, false, false, false, false, false,
                                   false, false, false, false, false, false, false, false, false, false };
    REQUIRE( video.atomicPointers() == exp_ptrs );

    std::vector<std::string> exp_types = { "STRING", "int", "STRING", "STRING", "STRING", "STRING", "int", "int",
                                           "int", "int", "int", "int", "int", "int", "int", "double", "double",
                                           "double", "double", "double", "double", "int" };
    REQUIRE( video.atomicTypes() == exp_types );

    std::vector<size_t> exp_rank = { 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0 };
    REQUIRE( video.atomicRank() == exp_rank );

    std::vector<std::vector<size_t>> exp_shapes = { {256}, {1}, {256}, {256}, {256}, {256}, {1}, {1}, {1}, {1}, {1},
                                                    {1}, {1}, {1}, {1}, {2}, {2}, {1}, {1}, {1}, {1}, {1} };
    REQUIRE( video.atomicShape() == exp_shapes );

    std::vector<const char*> exp_string_values = { "2013-10-25T12:37:18Z", "Fujinon f/1.4", "SW/B/B", "Sector9U",
                                                   "IPX-VGA210LCFN      ASSY-0074-0002-RE04 SW v1.63 BL v1.38 CUST " };
    std::vector<int> exp_int_values = { 30420, 0, 640, 480, 12, 1, 1, 1, 0, 0, 20 };
    std::vector<double> exp_double_values = { 9090.0, 9090.0, 39.25, 0.0 };

    int string_i = 0;
    int int_i = 0;
    int double_i = 0;

    for (const auto& name : exp_names) {
        uda::Scalar value = video.atomicScalar(name);
        if (name != "offset" && name != "gain") {
            REQUIRE( !value.isNull() );
        } else {
            continue;
        }

        if (value.type() == typeid(char*)) {
            REQUIRE( std::string(value.as<char*>()) == exp_string_values[string_i] );
            ++string_i;
        } else if (value.type() == typeid(int)) {
            REQUIRE( value.as<int>() == exp_int_values[int_i] );
            ++int_i;
        } else if (value.type() == typeid(double)) {
            REQUIRE( value.as<double >() == Approx(exp_double_values[double_i]) );
            ++double_i;
        }
    }

    uda::Vector offset = video.atomicVector("offset");
    REQUIRE( !offset.isNull() );
    std::vector<double> exp_offset = { 202.0, 208.0 };
    REQUIRE( offset.as<double>() == ApproxVector(exp_offset) );

    uda::Vector gain = video.atomicVector("gain");
    REQUIRE( !gain.isNull() );
    std::vector<double> exp_gain = { 6.0, 0.0 };
    REQUIRE( gain.as<double>() == ApproxVector(exp_gain) );

    std::vector<int> exp_sums = {
            15641697,
            16424349,
            18355640,
            19549280,
            18944664,
            20318935,
            21209104,
            22434386,
            23655941,
            24005361,
            25664982,
            26567714,
            27851750,
            29218804,
            30504494,
            32038259,
            33700757,
            35301984,
            37013401,
            38467104,
            35648042,
    };

    for (int frame_i = 0; frame_i < 20; ++frame_i) {
        uda::TreeNode frame = video.child(frame_i);

        REQUIRE( frame.name() == "frames" );
        REQUIRE( frame.numChildren() == 0 );
        REQUIRE( frame.atomicCount() == 3 );

        exp_names = { "number", "time", "k" };
        REQUIRE( frame.atomicNames() == exp_names );

        exp_ptrs = { false, false, true };
        REQUIRE( frame.atomicPointers() == exp_ptrs );

        exp_types = { "int", "double", "unsigned short *" };
        REQUIRE( frame.atomicTypes() == exp_types );

        exp_rank = { 0, 0, 0 };
        REQUIRE( frame.atomicRank() == exp_rank );

        exp_shapes = { {1}, {1}, {307200} };
        REQUIRE( frame.atomicShape() == exp_shapes );

        uda::Scalar number = frame.atomicScalar("number");
        REQUIRE( number.as<int>() == frame_i + 10 );

        uda::Vector vec = frame.atomicVector("k");
        REQUIRE( vec.size() == 307200 );

        long sum = 0;
        auto vals = vec.as<unsigned short>();
        for (auto val : vals) {
            sum += val;
        }

        REQUIRE( sum == exp_sums[frame_i] );
    }
}

TEST_CASE( "Test read single frame from colour IPX version 1", "[IMAS][JET][TF]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("NEWIPX::read(filename='" TEST_DATA_DIR "/rco030420.ipx', frame=100)", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode video = tree.child(0);

    REQUIRE( video.name() == "data" );
    REQUIRE( video.numChildren() == 1 );
    REQUIRE( video.atomicCount() == 22 );

    std::vector<std::string> exp_names = { "datetime", "shot", "lens", "filter", "view", "camera", "is_color",
                                           "width", "height", "depth", "taps", "left", "top", "hbin", "vbin",
                                           "offset", "gain", "preexp", "exposure", "board_temp", "ccd_temp",
                                           "n_frames" };
    REQUIRE( video.atomicNames() == exp_names );

    std::vector<bool> exp_ptrs = { false, false, false, false, false, false, false, false, false, false, false, false,
                                   false, false, false, false, false, false, false, false, false, false };
    REQUIRE( video.atomicPointers() == exp_ptrs );

    std::vector<std::string> exp_types = { "STRING", "int", "STRING", "STRING", "STRING", "STRING", "int", "int",
                                           "int", "int", "int", "int", "int", "int", "int", "double", "double",
                                           "double", "double", "double", "double", "int" };
    REQUIRE( video.atomicTypes() == exp_types );

    std::vector<size_t> exp_rank = { 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0 };
    REQUIRE( video.atomicRank() == exp_rank );

    std::vector<std::vector<size_t>> exp_shapes = { {256}, {1}, {256}, {256}, {256}, {256}, {1}, {1}, {1}, {1}, {1},
                                                    {1}, {1}, {1}, {1}, {2}, {2}, {1}, {1}, {1}, {1}, {1} };
    REQUIRE( video.atomicShape() == exp_shapes );

    std::vector<const char*> exp_string_values = { "2013-10-25T12:17:19Z", "", "", "HM11 - normal ",
                                                   "MAST colour; ser 1135; cam 4; firm 195" };
    std::vector<int> exp_int_values = { 30420, 1, 512, 364, 8, 0, 1, 149, 0, 0, 1 };
    std::vector<double> exp_double_values = { 0.0, 990.0, 0.0, 0.0 };

    int string_i = 0;
    int int_i = 0;
    int double_i = 0;

    for (const auto& name : exp_names) {
        uda::Scalar value = video.atomicScalar(name);
        if (name != "offset" && name != "gain") {
            REQUIRE( !value.isNull() );
        } else {
            continue;
        }

        if (value.type() == typeid(char*)) {
            REQUIRE( std::string(value.as<char*>()) == exp_string_values[string_i] );
            ++string_i;
        } else if (value.type() == typeid(int)) {
            REQUIRE( value.as<int>() == exp_int_values[int_i] );
            ++int_i;
        } else if (value.type() == typeid(double)) {
            REQUIRE( value.as<double >() == Approx(exp_double_values[double_i]) );
            ++double_i;
        }
    }

    uda::Vector offset = video.atomicVector("offset");
    REQUIRE( !offset.isNull() );
    std::vector<double> exp_offset = { 0.0, 0.0 };
    REQUIRE( offset.as<double>() == ApproxVector(exp_offset) );

    uda::Vector gain = video.atomicVector("gain");
    REQUIRE( !gain.isNull() );
    std::vector<double> exp_gain = { 0.0, 0.0 };
    REQUIRE( gain.as<double>() == ApproxVector(exp_gain) );

    uda::TreeNode frame = video.child(0);

    REQUIRE( frame.name() == "frames" );
    REQUIRE( frame.numChildren() == 0 );
    REQUIRE( frame.atomicCount() == 6 );

    exp_names = { "number", "time", "r", "g", "b", "raw" };
    REQUIRE( frame.atomicNames() == exp_names );

    exp_ptrs = { false, false, true, true, true, true };
    REQUIRE( frame.atomicPointers() == exp_ptrs );

    exp_types = { "int", "double", "unsigned char *", "unsigned char *", "unsigned char *", "unsigned char *" };
    REQUIRE( frame.atomicTypes() == exp_types );

    exp_rank = { 0, 0, 0, 0, 0, 0 };
    REQUIRE( frame.atomicRank() == exp_rank );

    exp_shapes = { {1}, {1}, {186368}, {186368}, {186368}, {186368} };
    REQUIRE( frame.atomicShape() == exp_shapes );

    uda::Scalar number = frame.atomicScalar("number");
    REQUIRE( number.as<int>() == 100 );

    uda::Scalar time = frame.atomicScalar("time");
    REQUIRE( time.as<double>() == Approx(0.09) );

    std::vector<int> exp_vals = { 3041149, 2277875, 2625531, 2598690 };

    int i = 0;
    for (const auto& name : exp_names) {
        if (name == "number" || name == "time") {
            continue;
        }

        uda::Vector vec = frame.atomicVector(name);
        REQUIRE( vec.size() == 186368 );

        long sum = 0;
        auto vals = vec.as<unsigned char>();
        for (auto val : vals) {
            sum += val;
        }

        REQUIRE( sum == exp_vals[i] );
        ++i;
    }
}

TEST_CASE( "Test read single frame from IPX version 2", "[IMAS][JET][TF]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("NEWIPX::read(filename='" TEST_DATA_DIR "/rit030420.ipx', frame=10)", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode video = tree.child(0);

    REQUIRE( video.name() == "data" );
    REQUIRE( video.numChildren() == 1 );
    REQUIRE( video.atomicCount() == 22 );

    std::vector<std::string> exp_names = { "datetime", "shot", "lens", "filter", "view", "camera", "is_color",
                                           "width", "height", "depth", "taps", "left", "top", "hbin", "vbin",
                                           "offset", "gain", "preexp", "exposure", "board_temp", "ccd_temp",
                                           "n_frames" };
    REQUIRE( video.atomicNames() == exp_names );

    std::vector<bool> exp_ptrs = { false, false, false, false, false, false, false, false, false, false, false, false,
                                   false, false, false, false, false, false, false, false, false, false };
    REQUIRE( video.atomicPointers() == exp_ptrs );

    std::vector<std::string> exp_types = { "STRING", "int", "STRING", "STRING", "STRING", "STRING", "int", "int",
                                           "int", "int", "int", "int", "int", "int", "int", "double", "double",
                                           "double", "double", "double", "double", "int" };
    REQUIRE( video.atomicTypes() == exp_types );

    std::vector<size_t> exp_rank = { 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0 };
    REQUIRE( video.atomicRank() == exp_rank );

    std::vector<std::vector<size_t>> exp_shapes = { {256}, {1}, {256}, {256}, {256}, {256}, {1}, {1}, {1}, {1}, {1},
                                                    {1}, {1}, {1}, {1}, {2}, {2}, {1}, {1}, {1}, {1}, {1} };
    REQUIRE( video.atomicShape() == exp_shapes );

    std::vector<const char*> exp_string_values = { "2013-10-25T12:45:29Z", "", "", "HL01 Upper divertor view#1",
                                                   "Thermosensorik CMT 256 SM HS" };
    std::vector<int> exp_int_values = { 30420, 0, 256, 32, 14, 0, 0, 153, 0, 0, 1 };
    std::vector<double> exp_double_values = { 0.0, 50.0, 0.0, 59.0 };

    int string_i = 0;
    int int_i = 0;
    int double_i = 0;

    for (const auto& name : exp_names) {
        uda::Scalar value = video.atomicScalar(name);
        if (name != "offset" && name != "gain") {
            REQUIRE( !value.isNull() );
        } else {
            continue;
        }

        if (value.type() == typeid(char*)) {
            REQUIRE( std::string(value.as<char*>()) == exp_string_values[string_i] );
            ++string_i;
        } else if (value.type() == typeid(int)) {
            REQUIRE( value.as<int>() == exp_int_values[int_i] );
            ++int_i;
        } else if (value.type() == typeid(double)) {
            REQUIRE( value.as<double >() == Approx(exp_double_values[double_i]) );
            ++double_i;
        }
    }

    uda::Vector offset = video.atomicVector("offset");
    REQUIRE( !offset.isNull() );
    std::vector<double> exp_offset = { 0.0, 0.0 };
    REQUIRE( offset.as<double>() == ApproxVector(exp_offset) );

    uda::Vector gain = video.atomicVector("gain");
    REQUIRE( !gain.isNull() );
    std::vector<double> exp_gain = { 0.0, 0.0 };
    REQUIRE( gain.as<double>() == ApproxVector(exp_gain) );

    uda::TreeNode frame = video.child(0);

    REQUIRE( frame.name() == "frames" );
    REQUIRE( frame.numChildren() == 0 );
    REQUIRE( frame.atomicCount() == 3 );

    exp_names = { "number", "time", "k" };
    REQUIRE( frame.atomicNames() == exp_names );

    exp_ptrs = { false, false, true };
    REQUIRE( frame.atomicPointers() == exp_ptrs );

    exp_types = { "int", "double", "unsigned short *" };
    REQUIRE( frame.atomicTypes() == exp_types );

    exp_rank = { 0, 0, 0 };
    REQUIRE( frame.atomicRank() == exp_rank );

    exp_shapes = { {1}, {1}, {8192} };
    REQUIRE( frame.atomicShape() == exp_shapes );

    uda::Scalar number = frame.atomicScalar("number");
    REQUIRE( number.as<int>() == 10 );

    uda::Scalar time = frame.atomicScalar("time");
    REQUIRE( time.as<double>() == Approx(-0.047948) );

    uda::Vector vec = frame.atomicVector("k");
    REQUIRE( vec.size() == 8192 );

    long sum = 0;
    auto vals = vec.as<unsigned short>();
    for (auto val : vals) {
        sum += val;
    }

    REQUIRE( sum == 52925314 );

    std::vector<unsigned short> exp_k = { 6357, 6241, 6383, 6505, 6306, 6432, 6449, 6322, 6384, 6521 };

    auto k = vec.as<unsigned short>();
    k.resize(10);
    REQUIRE( k == exp_k );
}