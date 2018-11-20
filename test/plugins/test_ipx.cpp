#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "test_helpers.h"

#include <c++/UDA.hpp>

TEST_CASE( "Test read first frame from IPX version 1", "[IPX][V1]" )
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
    REQUIRE( video.atomicCount() == 23 );

    std::vector<std::string> exp_names = { "datetime", "shot", "lens", "filter", "view", "camera", "is_color",
                                           "width", "height", "depth", "taps", "left", "top", "hbin", "vbin",
                                           "offset", "gain", "preexp", "exposure", "board_temp", "ccd_temp",
                                           "n_frames", "frame_times" };
    REQUIRE( video.atomicNames() == exp_names );

    std::vector<bool> exp_ptrs = { false, false, false, false, false, false, false, false, false, false, false, false,
                                   false, false, false, false, false, false, false, false, false, false, true };
    REQUIRE( video.atomicPointers() == exp_ptrs );

    std::vector<std::string> exp_types = { "STRING", "int", "STRING", "STRING", "STRING", "STRING", "int", "int",
                                           "int", "int", "int", "int", "int", "int", "int", "double", "double",
                                           "double", "double", "double", "double", "int", "double *" };
    REQUIRE( video.atomicTypes() == exp_types );

    std::vector<size_t> exp_rank = { 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0 };
    REQUIRE( video.atomicRank() == exp_rank );

    std::vector<std::vector<size_t>> exp_shapes = { {256}, {1}, {256}, {256}, {256}, {256}, {1}, {1}, {1}, {1}, {1},
                                                    {1}, {1}, {1}, {1}, {2}, {2}, {1}, {1}, {1}, {1}, {1}, {99} };
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
        if (name != "offset" && name != "gain" && name != "frame_times") {
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

    uda::Vector frame_times = video.atomicVector("frame_times");
    REQUIRE( !frame_times.isNull() );
    std::vector<double> exp_frame_times = { -0.0089800863, 0.0001199137, 0.0092269137, 0.0183259137, 0.0274289137,
                                            0.0365319137, 0.0456349137, 0.0547379137, 0.0638409137, 0.0729429137,
                                            0.0820489137, 0.0911489137, 0.1002569137, 0.1093549137, 0.1184589137,
                                            0.1275609137, 0.1366639137, 0.1457669137, 0.1548689137, 0.1639729137,
                                            0.1730749137, 0.1821789137, 0.1912819137, 0.2003849137, 0.2094869137,
                                            0.2185909137, 0.2276929137, 0.2367969137, 0.2458989137, 0.2550269137,
                                            0.2641049137, 0.2732059137, 0.2823109137, 0.2914139137, 0.3005169137,
                                            0.3096249137, 0.3187229137, 0.3278259137, 0.3369279137, 0.3460319137,
                                            0.3551339137, 0.3642379137, 0.3733399137, 0.3824439137, 0.3915459137,
                                            0.4006499137, 0.4097509137, 0.4188549137, 0.4279569137, 0.4370609137,
                                            0.4461719137, 0.4552669137, 0.4643689137, 0.4734729137, 0.4825749137,
                                            0.4916789137, 0.5007799137, 0.5098849137, 0.5189869137, 0.5280909137,
                                            0.5371999137, 0.5462969137, 0.5553989137, 0.5645019137, 0.5736039137,
                                            0.5827079137, 0.5918109137, 0.6009139137, 0.6100159137, 0.6191269137,
                                            0.6282219137, 0.6373259137, 0.6464279137, 0.6555319137, 0.6646339137,
                                            0.6737369137, 0.6828399137, 0.6919429137, 0.7010449137, 0.7101499137,
                                            0.7192519137, 0.7283559137, 0.7374569137, 0.7465599137, 0.7556629137,
                                            0.7647659137, 0.7738689137, 0.7829739137, 0.7920749137, 0.8011799137,
                                            0.8102809137, 0.8193839137, 0.8284869137, 0.8375899137, 0.8466919137,
                                            0.8557959137, 0.8648979137, 0.8740009137, 0.8831019137 };
    REQUIRE( frame_times.as<double>() == ApproxVector(exp_frame_times) );

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

TEST_CASE( "Test read single frame from IPX version 1", "[IPX][V1]" )
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
    REQUIRE( video.atomicCount() == 23 );

    std::vector<std::string> exp_names = { "datetime", "shot", "lens", "filter", "view", "camera", "is_color",
                                           "width", "height", "depth", "taps", "left", "top", "hbin", "vbin",
                                           "offset", "gain", "preexp", "exposure", "board_temp", "ccd_temp",
                                           "n_frames", "frame_times" };
    REQUIRE( video.atomicNames() == exp_names );

    std::vector<bool> exp_ptrs = { false, false, false, false, false, false, false, false, false, false, false, false,
                                   false, false, false, false, false, false, false, false, false, false, true };
    REQUIRE( video.atomicPointers() == exp_ptrs );

    std::vector<std::string> exp_types = { "STRING", "int", "STRING", "STRING", "STRING", "STRING", "int", "int",
                                           "int", "int", "int", "int", "int", "int", "int", "double", "double",
                                           "double", "double", "double", "double", "int", "double *" };
    REQUIRE( video.atomicTypes() == exp_types );

    std::vector<size_t> exp_rank = { 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0 };
    REQUIRE( video.atomicRank() == exp_rank );

    std::vector<std::vector<size_t>> exp_shapes = { {256}, {1}, {256}, {256}, {256}, {256}, {1}, {1}, {1}, {1}, {1},
                                                    {1}, {1}, {1}, {1}, {2}, {2}, {1}, {1}, {1}, {1}, {1}, {99} };
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
        if (name != "offset" && name != "gain" && name != "frame_times") {
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

    uda::Vector frame_times = video.atomicVector("frame_times");
    REQUIRE( !frame_times.isNull() );
    std::vector<double> exp_frame_times = { -0.0089800863, 0.0001199137, 0.0092269137, 0.0183259137, 0.0274289137,
                                            0.0365319137, 0.0456349137, 0.0547379137, 0.0638409137, 0.0729429137,
                                            0.0820489137, 0.0911489137, 0.1002569137, 0.1093549137, 0.1184589137,
                                            0.1275609137, 0.1366639137, 0.1457669137, 0.1548689137, 0.1639729137,
                                            0.1730749137, 0.1821789137, 0.1912819137, 0.2003849137, 0.2094869137,
                                            0.2185909137, 0.2276929137, 0.2367969137, 0.2458989137, 0.2550269137,
                                            0.2641049137, 0.2732059137, 0.2823109137, 0.2914139137, 0.3005169137,
                                            0.3096249137, 0.3187229137, 0.3278259137, 0.3369279137, 0.3460319137,
                                            0.3551339137, 0.3642379137, 0.3733399137, 0.3824439137, 0.3915459137,
                                            0.4006499137, 0.4097509137, 0.4188549137, 0.4279569137, 0.4370609137,
                                            0.4461719137, 0.4552669137, 0.4643689137, 0.4734729137, 0.4825749137,
                                            0.4916789137, 0.5007799137, 0.5098849137, 0.5189869137, 0.5280909137,
                                            0.5371999137, 0.5462969137, 0.5553989137, 0.5645019137, 0.5736039137,
                                            0.5827079137, 0.5918109137, 0.6009139137, 0.6100159137, 0.6191269137,
                                            0.6282219137, 0.6373259137, 0.6464279137, 0.6555319137, 0.6646339137,
                                            0.6737369137, 0.6828399137, 0.6919429137, 0.7010449137, 0.7101499137,
                                            0.7192519137, 0.7283559137, 0.7374569137, 0.7465599137, 0.7556629137,
                                            0.7647659137, 0.7738689137, 0.7829739137, 0.7920749137, 0.8011799137,
                                            0.8102809137, 0.8193839137, 0.8284869137, 0.8375899137, 0.8466919137,
                                            0.8557959137, 0.8648979137, 0.8740009137, 0.8831019137 };
    REQUIRE( frame_times.as<double>() == ApproxVector(exp_frame_times) );

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

TEST_CASE( "Test read all frames from IPX version 1", "[IPX][V1]" )
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
    REQUIRE( video.atomicCount() == 23 );

    std::vector<std::string> exp_names = { "datetime", "shot", "lens", "filter", "view", "camera", "is_color",
                                           "width", "height", "depth", "taps", "left", "top", "hbin", "vbin",
                                           "offset", "gain", "preexp", "exposure", "board_temp", "ccd_temp",
                                           "n_frames", "frame_times" };
    REQUIRE( video.atomicNames() == exp_names );

    std::vector<bool> exp_ptrs = { false, false, false, false, false, false, false, false, false, false, false, false,
                                   false, false, false, false, false, false, false, false, false, false, true };
    REQUIRE( video.atomicPointers() == exp_ptrs );

    std::vector<std::string> exp_types = { "STRING", "int", "STRING", "STRING", "STRING", "STRING", "int", "int",
                                           "int", "int", "int", "int", "int", "int", "int", "double", "double",
                                           "double", "double", "double", "double", "int", "double *" };
    REQUIRE( video.atomicTypes() == exp_types );

    std::vector<size_t> exp_rank = { 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0 };
    REQUIRE( video.atomicRank() == exp_rank );

    std::vector<std::vector<size_t>> exp_shapes = { {256}, {1}, {256}, {256}, {256}, {256}, {1}, {1}, {1}, {1}, {1},
                                                    {1}, {1}, {1}, {1}, {2}, {2}, {1}, {1}, {1}, {1}, {1}, {99} };
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
        if (name != "offset" && name != "gain" && name != "frame_times") {
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

    uda::Vector frame_times = video.atomicVector("frame_times");
    REQUIRE( !frame_times.isNull() );
    std::vector<double> exp_frame_times = { -0.0089800863, 0.0001199137, 0.0092269137, 0.0183259137, 0.0274289137,
                                            0.0365319137, 0.0456349137, 0.0547379137, 0.0638409137, 0.0729429137,
                                            0.0820489137, 0.0911489137, 0.1002569137, 0.1093549137, 0.1184589137,
                                            0.1275609137, 0.1366639137, 0.1457669137, 0.1548689137, 0.1639729137,
                                            0.1730749137, 0.1821789137, 0.1912819137, 0.2003849137, 0.2094869137,
                                            0.2185909137, 0.2276929137, 0.2367969137, 0.2458989137, 0.2550269137,
                                            0.2641049137, 0.2732059137, 0.2823109137, 0.2914139137, 0.3005169137,
                                            0.3096249137, 0.3187229137, 0.3278259137, 0.3369279137, 0.3460319137,
                                            0.3551339137, 0.3642379137, 0.3733399137, 0.3824439137, 0.3915459137,
                                            0.4006499137, 0.4097509137, 0.4188549137, 0.4279569137, 0.4370609137,
                                            0.4461719137, 0.4552669137, 0.4643689137, 0.4734729137, 0.4825749137,
                                            0.4916789137, 0.5007799137, 0.5098849137, 0.5189869137, 0.5280909137,
                                            0.5371999137, 0.5462969137, 0.5553989137, 0.5645019137, 0.5736039137,
                                            0.5827079137, 0.5918109137, 0.6009139137, 0.6100159137, 0.6191269137,
                                            0.6282219137, 0.6373259137, 0.6464279137, 0.6555319137, 0.6646339137,
                                            0.6737369137, 0.6828399137, 0.6919429137, 0.7010449137, 0.7101499137,
                                            0.7192519137, 0.7283559137, 0.7374569137, 0.7465599137, 0.7556629137,
                                            0.7647659137, 0.7738689137, 0.7829739137, 0.7920749137, 0.8011799137,
                                            0.8102809137, 0.8193839137, 0.8284869137, 0.8375899137, 0.8466919137,
                                            0.8557959137, 0.8648979137, 0.8740009137, 0.8831019137 };
    REQUIRE( frame_times.as<double>() == ApproxVector(exp_frame_times) );

    std::vector<int> exp_sums = {
            2744207, 2753578, 10472617, 10119079, 9049764, 9259192, 10630244, 12553347, 14098187, 15308968,
            15641697, 16424349, 18355640, 19549280, 18944664, 20318935, 21209104, 22434386, 23655941,
            24005361, 25664982, 26567714, 27851750, 29218804, 30504494, 32038259, 33700757, 35301984,
            37013401, 38467104, 35648042, 30445127, 26991551, 23475395, 21854605, 53662658, 3295277,
            2868991, 2681948, 2689057, 2725826, 2725546, 2730518, 2730236, 2614803, 2643570, 2696321,
            2744784, 2742681, 2694582, 2685452, 2678515, 2678980, 2663877, 2715718, 2674938, 2689971,
            2676795, 2688600, 2697473, 2683250, 2678413, 2677472, 2671100, 2691833, 2650072, 2661535,
            2674949, 2679492, 2691941, 2699661, 2668503, 2643482, 2661289, 2666871, 2668836, 2677627,
            2651329, 2611731, 2585793, 2606677, 2584177, 2574413, 2597892, 2587676, 2580751, 2579697,
            2599836, 2578467, 2543731, 2808747, 2569292, 2584069, 2581517, 2582914, 2540754, 2558420, 2569188, 2569565,
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

TEST_CASE( "Test read range of frames from IPX version 1", "[IPX][V1]" )
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
    REQUIRE( video.numChildren() == 21 );
    REQUIRE( video.atomicCount() == 23 );

    std::vector<std::string> exp_names = { "datetime", "shot", "lens", "filter", "view", "camera", "is_color",
                                           "width", "height", "depth", "taps", "left", "top", "hbin", "vbin",
                                           "offset", "gain", "preexp", "exposure", "board_temp", "ccd_temp",
                                           "n_frames", "frame_times" };
    REQUIRE( video.atomicNames() == exp_names );

    std::vector<bool> exp_ptrs = { false, false, false, false, false, false, false, false, false, false, false, false,
                                   false, false, false, false, false, false, false, false, false, false, true };
    REQUIRE( video.atomicPointers() == exp_ptrs );

    std::vector<std::string> exp_types = { "STRING", "int", "STRING", "STRING", "STRING", "STRING", "int", "int",
                                           "int", "int", "int", "int", "int", "int", "int", "double", "double",
                                           "double", "double", "double", "double", "int", "double *" };
    REQUIRE( video.atomicTypes() == exp_types );

    std::vector<size_t> exp_rank = { 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0 };
    REQUIRE( video.atomicRank() == exp_rank );

    std::vector<std::vector<size_t>> exp_shapes = { {256}, {1}, {256}, {256}, {256}, {256}, {1}, {1}, {1}, {1}, {1},
                                                    {1}, {1}, {1}, {1}, {2}, {2}, {1}, {1}, {1}, {1}, {1}, {99} };
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
        if (name != "offset" && name != "gain" && name != "frame_times") {
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

    uda::Vector frame_times = video.atomicVector("frame_times");
    REQUIRE( !frame_times.isNull() );
    std::vector<double> exp_frame_times = { -0.0089800863, 0.0001199137, 0.0092269137, 0.0183259137, 0.0274289137,
                                            0.0365319137, 0.0456349137, 0.0547379137, 0.0638409137, 0.0729429137,
                                            0.0820489137, 0.0911489137, 0.1002569137, 0.1093549137, 0.1184589137,
                                            0.1275609137, 0.1366639137, 0.1457669137, 0.1548689137, 0.1639729137,
                                            0.1730749137, 0.1821789137, 0.1912819137, 0.2003849137, 0.2094869137,
                                            0.2185909137, 0.2276929137, 0.2367969137, 0.2458989137, 0.2550269137,
                                            0.2641049137, 0.2732059137, 0.2823109137, 0.2914139137, 0.3005169137,
                                            0.3096249137, 0.3187229137, 0.3278259137, 0.3369279137, 0.3460319137,
                                            0.3551339137, 0.3642379137, 0.3733399137, 0.3824439137, 0.3915459137,
                                            0.4006499137, 0.4097509137, 0.4188549137, 0.4279569137, 0.4370609137,
                                            0.4461719137, 0.4552669137, 0.4643689137, 0.4734729137, 0.4825749137,
                                            0.4916789137, 0.5007799137, 0.5098849137, 0.5189869137, 0.5280909137,
                                            0.5371999137, 0.5462969137, 0.5553989137, 0.5645019137, 0.5736039137,
                                            0.5827079137, 0.5918109137, 0.6009139137, 0.6100159137, 0.6191269137,
                                            0.6282219137, 0.6373259137, 0.6464279137, 0.6555319137, 0.6646339137,
                                            0.6737369137, 0.6828399137, 0.6919429137, 0.7010449137, 0.7101499137,
                                            0.7192519137, 0.7283559137, 0.7374569137, 0.7465599137, 0.7556629137,
                                            0.7647659137, 0.7738689137, 0.7829739137, 0.7920749137, 0.8011799137,
                                            0.8102809137, 0.8193839137, 0.8284869137, 0.8375899137, 0.8466919137,
                                            0.8557959137, 0.8648979137, 0.8740009137, 0.8831019137 };
    REQUIRE( frame_times.as<double>() == ApproxVector(exp_frame_times) );

    std::vector<int> exp_sums = {
            15641697, 16424349, 18355640, 19549280, 18944664, 20318935, 21209104, 22434386, 23655941, 24005361,
            25664982, 26567714, 27851750, 29218804, 30504494, 32038259, 33700757, 35301984, 37013401, 38467104,
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

TEST_CASE( "Test read range of frames with stride from IPX version 1", "[IPX][V1]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("NEWIPX::read(filename='" TEST_DATA_DIR "/rgb030420.ipx', first=0, last=90, stride=7)", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode video = tree.child(0);

    REQUIRE( video.name() == "data" );
    REQUIRE( video.numChildren() == 13 );
    REQUIRE( video.atomicCount() == 23 );

    std::vector<std::string> exp_names = { "datetime", "shot", "lens", "filter", "view", "camera", "is_color",
                                           "width", "height", "depth", "taps", "left", "top", "hbin", "vbin",
                                           "offset", "gain", "preexp", "exposure", "board_temp", "ccd_temp",
                                           "n_frames", "frame_times" };
    REQUIRE( video.atomicNames() == exp_names );

    std::vector<bool> exp_ptrs = { false, false, false, false, false, false, false, false, false, false, false, false,
                                   false, false, false, false, false, false, false, false, false, false, true };
    REQUIRE( video.atomicPointers() == exp_ptrs );

    std::vector<std::string> exp_types = { "STRING", "int", "STRING", "STRING", "STRING", "STRING", "int", "int",
                                           "int", "int", "int", "int", "int", "int", "int", "double", "double",
                                           "double", "double", "double", "double", "int", "double *" };
    REQUIRE( video.atomicTypes() == exp_types );

    std::vector<size_t> exp_rank = { 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0 };
    REQUIRE( video.atomicRank() == exp_rank );

    std::vector<std::vector<size_t>> exp_shapes = { {256}, {1}, {256}, {256}, {256}, {256}, {1}, {1}, {1}, {1}, {1},
                                                    {1}, {1}, {1}, {1}, {2}, {2}, {1}, {1}, {1}, {1}, {1}, {99} };
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
        if (name != "offset" && name != "gain" && name != "frame_times") {
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

    uda::Vector frame_times = video.atomicVector("frame_times");
    REQUIRE( !frame_times.isNull() );
    std::vector<double> exp_frame_times = { -0.0089800863, 0.0001199137, 0.0092269137, 0.0183259137, 0.0274289137,
                                            0.0365319137, 0.0456349137, 0.0547379137, 0.0638409137, 0.0729429137,
                                            0.0820489137, 0.0911489137, 0.1002569137, 0.1093549137, 0.1184589137,
                                            0.1275609137, 0.1366639137, 0.1457669137, 0.1548689137, 0.1639729137,
                                            0.1730749137, 0.1821789137, 0.1912819137, 0.2003849137, 0.2094869137,
                                            0.2185909137, 0.2276929137, 0.2367969137, 0.2458989137, 0.2550269137,
                                            0.2641049137, 0.2732059137, 0.2823109137, 0.2914139137, 0.3005169137,
                                            0.3096249137, 0.3187229137, 0.3278259137, 0.3369279137, 0.3460319137,
                                            0.3551339137, 0.3642379137, 0.3733399137, 0.3824439137, 0.3915459137,
                                            0.4006499137, 0.4097509137, 0.4188549137, 0.4279569137, 0.4370609137,
                                            0.4461719137, 0.4552669137, 0.4643689137, 0.4734729137, 0.4825749137,
                                            0.4916789137, 0.5007799137, 0.5098849137, 0.5189869137, 0.5280909137,
                                            0.5371999137, 0.5462969137, 0.5553989137, 0.5645019137, 0.5736039137,
                                            0.5827079137, 0.5918109137, 0.6009139137, 0.6100159137, 0.6191269137,
                                            0.6282219137, 0.6373259137, 0.6464279137, 0.6555319137, 0.6646339137,
                                            0.6737369137, 0.6828399137, 0.6919429137, 0.7010449137, 0.7101499137,
                                            0.7192519137, 0.7283559137, 0.7374569137, 0.7465599137, 0.7556629137,
                                            0.7647659137, 0.7738689137, 0.7829739137, 0.7920749137, 0.8011799137,
                                            0.8102809137, 0.8193839137, 0.8284869137, 0.8375899137, 0.8466919137,
                                            0.8557959137, 0.8648979137, 0.8740009137, 0.8831019137 };
    REQUIRE( frame_times.as<double>() == ApproxVector(exp_frame_times) );

    std::vector<int> exp_sums = {
            2744207, 12553347, 18944664, 26567714, 37013401, 53662658, 2730518,
            2694582, 2689971, 2671100, 2699661, 2651329, 2587676
    };

    for (int frame_i = 0; frame_i < 12; ++frame_i) {
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
        REQUIRE( number.as<int>() == frame_i * 7 );

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

TEST_CASE( "Test read single frame from colour IPX version 1", "[IPX][V1]" )
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
    REQUIRE( video.atomicCount() == 23 );

    std::vector<std::string> exp_names = { "datetime", "shot", "lens", "filter", "view", "camera", "is_color",
                                           "width", "height", "depth", "taps", "left", "top", "hbin", "vbin",
                                           "offset", "gain", "preexp", "exposure", "board_temp", "ccd_temp",
                                           "n_frames", "frame_times" };
    REQUIRE( video.atomicNames() == exp_names );

    std::vector<bool> exp_ptrs = { false, false, false, false, false, false, false, false, false, false, false, false,
                                   false, false, false, false, false, false, false, false, false, false, true };
    REQUIRE( video.atomicPointers() == exp_ptrs );

    std::vector<std::string> exp_types = { "STRING", "int", "STRING", "STRING", "STRING", "STRING", "int", "int",
                                           "int", "int", "int", "int", "int", "int", "int", "double", "double",
                                           "double", "double", "double", "double", "int", "double *" };
    REQUIRE( video.atomicTypes() == exp_types );

    std::vector<size_t> exp_rank = { 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0 };
    REQUIRE( video.atomicRank() == exp_rank );

    std::vector<std::vector<size_t>> exp_shapes = { {256}, {1}, {256}, {256}, {256}, {256}, {1}, {1}, {1}, {1}, {1},
                                                    {1}, {1}, {1}, {1}, {2}, {2}, {1}, {1}, {1}, {1}, {1}, {750} };
    REQUIRE( video.atomicShape() == exp_shapes );

    std::vector<const char*> exp_string_values = { "2013-10-25T12:17:19Z", "", "", "HM11 - normal ",
                                                   "MAST colour; ser 1135; cam 4; firm 195" };
    std::vector<int> exp_int_values = { 30420, 1, 512, 364, 8, 0, 1, 149, 0, 0, 750 };
    std::vector<double> exp_double_values = { 0.0, 990.0, 0.0, 0.0 };

    int string_i = 0;
    int int_i = 0;
    int double_i = 0;

    for (const auto& name : exp_names) {
        uda::Scalar value = video.atomicScalar(name);
        if (name != "offset" && name != "gain" && name != "frame_times") {
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

    uda::Vector frame_times = video.atomicVector("frame_times");
    REQUIRE( !frame_times.isNull() );
    std::vector<double> exp_frame_times = {
            -0.009999, -0.008999, -0.007999, -0.006999, -0.005999, -0.004999, -0.003999, -0.002999, -0.001999,
            -0.000999, 0.0, 0.001, 0.002, 0.003, 0.004, 0.005, 0.006, 0.007, 0.008, 0.009, 0.01, 0.011, 0.012,
            0.013, 0.014, 0.015, 0.016, 0.017, 0.018, 0.019, 0.02, 0.021, 0.022, 0.023, 0.024, 0.025, 0.026,
            0.027, 0.028, 0.029, 0.03, 0.031, 0.032, 0.033, 0.034, 0.035, 0.036, 0.037, 0.038, 0.039, 0.04,
            0.041, 0.042, 0.043, 0.044, 0.045, 0.046, 0.047, 0.048, 0.049, 0.05, 0.051, 0.052, 0.053, 0.054,
            0.055, 0.056, 0.057, 0.058, 0.059, 0.06, 0.061, 0.062, 0.063, 0.064, 0.065, 0.066, 0.067, 0.068,
            0.069, 0.07, 0.071, 0.072, 0.073, 0.074, 0.075, 0.076, 0.077, 0.078, 0.079, 0.08, 0.081, 0.082,
            0.083, 0.084, 0.085, 0.086, 0.087, 0.088, 0.089, 0.09, 0.091, 0.092, 0.093, 0.094, 0.095, 0.096,
            0.097, 0.098, 0.099, 0.1, 0.101, 0.102, 0.103, 0.104, 0.105, 0.106, 0.107, 0.108, 0.109, 0.11,
            0.111, 0.112, 0.113, 0.114, 0.115, 0.116, 0.117, 0.118, 0.119, 0.12, 0.121, 0.122, 0.123, 0.124,
            0.125, 0.126, 0.127, 0.128, 0.129, 0.13, 0.131, 0.132, 0.133, 0.134, 0.135, 0.136, 0.137, 0.138,
            0.139, 0.14, 0.141, 0.142, 0.143, 0.144, 0.145, 0.146, 0.147, 0.148, 0.149, 0.15, 0.151, 0.152,
            0.153, 0.154, 0.155, 0.156, 0.157, 0.158, 0.159, 0.16, 0.161, 0.162, 0.163, 0.164, 0.165, 0.166,
            0.167, 0.168, 0.169, 0.17, 0.171, 0.172, 0.173, 0.174, 0.175, 0.176, 0.177, 0.178, 0.179, 0.18,
            0.181, 0.182, 0.183, 0.184, 0.185, 0.186, 0.187, 0.188, 0.189, 0.19, 0.191, 0.192, 0.193, 0.194,
            0.195, 0.196, 0.197, 0.198, 0.199, 0.2, 0.201, 0.202, 0.203, 0.204, 0.205, 0.206, 0.207, 0.208,
            0.209, 0.21, 0.211, 0.212, 0.213, 0.214, 0.215, 0.216, 0.217, 0.218, 0.219, 0.22, 0.221, 0.222,
            0.223, 0.224, 0.225, 0.226, 0.227, 0.228, 0.229, 0.23, 0.231, 0.232, 0.233, 0.234, 0.235, 0.236,
            0.237, 0.238, 0.239, 0.24, 0.241, 0.242, 0.243, 0.244, 0.245, 0.246, 0.247, 0.248, 0.249, 0.25,
            0.251, 0.252, 0.253, 0.254, 0.255, 0.256, 0.257, 0.258, 0.259, 0.26, 0.261, 0.262, 0.263, 0.264,
            0.265, 0.266, 0.267, 0.268, 0.269, 0.27, 0.271, 0.272, 0.273, 0.274, 0.275, 0.276, 0.277, 0.278,
            0.279, 0.28, 0.281, 0.282, 0.283, 0.284, 0.285, 0.286, 0.287, 0.288, 0.289, 0.29, 0.291, 0.292,
            0.293, 0.294, 0.295, 0.296, 0.297, 0.298, 0.299, 0.3, 0.301, 0.302, 0.303, 0.304, 0.305, 0.306,
            0.307, 0.308, 0.309, 0.31, 0.311, 0.312, 0.313, 0.314, 0.315, 0.316, 0.317, 0.318, 0.319, 0.32,
            0.321, 0.322, 0.323, 0.324, 0.325, 0.326, 0.327, 0.328, 0.329, 0.33, 0.331, 0.332, 0.333, 0.334,
            0.335, 0.336, 0.337, 0.338, 0.339, 0.34, 0.341, 0.342, 0.343, 0.344, 0.345, 0.346, 0.347, 0.348,
            0.349, 0.35, 0.351, 0.352, 0.353, 0.354, 0.355, 0.356, 0.357, 0.358, 0.359, 0.36, 0.361, 0.362,
            0.363, 0.364, 0.365, 0.366, 0.367, 0.368, 0.369, 0.37, 0.371, 0.372, 0.373, 0.374, 0.375, 0.376,
            0.377, 0.378, 0.379, 0.38, 0.381, 0.382, 0.383, 0.384, 0.385, 0.386, 0.387, 0.388, 0.389, 0.39,
            0.391, 0.392, 0.393, 0.394, 0.395, 0.396, 0.397, 0.398, 0.399, 0.4, 0.401, 0.402, 0.403, 0.404,
            0.405, 0.406, 0.407, 0.408, 0.409, 0.41, 0.411, 0.412, 0.413, 0.414, 0.415, 0.416, 0.417, 0.418,
            0.419, 0.42, 0.421, 0.422, 0.423, 0.424, 0.425, 0.426, 0.427, 0.428, 0.429, 0.43, 0.431, 0.432,
            0.433, 0.434, 0.435, 0.436, 0.437, 0.438, 0.439, 0.44, 0.441, 0.442, 0.443, 0.444, 0.445, 0.446,
            0.447, 0.448, 0.449, 0.45, 0.451, 0.452, 0.453, 0.454, 0.455, 0.456, 0.457, 0.458, 0.459, 0.46,
            0.461, 0.462, 0.463, 0.464, 0.465, 0.466, 0.467, 0.468, 0.469, 0.47, 0.471, 0.472, 0.473, 0.474,
            0.475, 0.476, 0.477, 0.478, 0.479, 0.48, 0.481, 0.482, 0.483, 0.484, 0.485, 0.486, 0.487, 0.488,
            0.489, 0.49, 0.491, 0.492, 0.493, 0.494, 0.495, 0.496, 0.497, 0.498, 0.499, 0.5, 0.501, 0.502,
            0.503, 0.504, 0.505, 0.506, 0.507, 0.508, 0.509, 0.51, 0.511, 0.512, 0.513, 0.514, 0.515, 0.516,
            0.517, 0.518, 0.519, 0.52, 0.521, 0.522, 0.523, 0.524, 0.525, 0.526, 0.527, 0.528, 0.529, 0.53,
            0.531, 0.532, 0.533, 0.534, 0.535, 0.536, 0.537, 0.538, 0.539, 0.54, 0.541, 0.542, 0.543, 0.544,
            0.545, 0.546, 0.547, 0.548, 0.549, 0.55, 0.551, 0.552, 0.553, 0.554, 0.555, 0.556, 0.557, 0.558,
            0.559, 0.56, 0.561, 0.562, 0.563, 0.564, 0.565, 0.566, 0.567, 0.568, 0.569, 0.57, 0.571, 0.572,
            0.573, 0.574, 0.575, 0.576, 0.577, 0.578, 0.579, 0.58, 0.581, 0.582, 0.583, 0.584, 0.585, 0.586,
            0.587, 0.588, 0.589, 0.59, 0.591, 0.592, 0.593, 0.594, 0.595, 0.596, 0.597, 0.598, 0.599, 0.6,
            0.601, 0.602, 0.603, 0.604, 0.605, 0.606, 0.607, 0.608, 0.609, 0.61, 0.611, 0.612, 0.613, 0.614,
            0.615, 0.616, 0.617, 0.618, 0.619, 0.62, 0.621, 0.622, 0.623, 0.624, 0.625, 0.626, 0.627, 0.628,
            0.629, 0.63, 0.631, 0.632, 0.633, 0.634, 0.635, 0.636, 0.637, 0.638, 0.639, 0.64, 0.641, 0.642,
            0.643, 0.644, 0.645, 0.646, 0.647, 0.648, 0.649, 0.65, 0.651, 0.652, 0.653, 0.654, 0.655, 0.656,
            0.657, 0.658, 0.659, 0.66, 0.661, 0.662, 0.663, 0.664, 0.665, 0.666, 0.667, 0.668, 0.669, 0.67,
            0.671, 0.672, 0.673, 0.674, 0.675, 0.676, 0.677, 0.678, 0.679, 0.68, 0.681, 0.682, 0.683, 0.684,
            0.685, 0.686, 0.687, 0.688, 0.689, 0.69, 0.691, 0.692, 0.693, 0.694, 0.695, 0.696, 0.697, 0.698,
            0.699, 0.7, 0.701, 0.702, 0.703, 0.704, 0.705, 0.706, 0.707, 0.708, 0.709, 0.71, 0.711, 0.712,
            0.713, 0.714, 0.715, 0.716, 0.717, 0.718, 0.719, 0.72, 0.721, 0.722, 0.723, 0.724, 0.725, 0.726,
            0.727, 0.728, 0.729, 0.73, 0.731, 0.732, 0.733, 0.734, 0.735, 0.736, 0.737, 0.738, 0.739 };
    REQUIRE( frame_times.as<double>() == ApproxVector(exp_frame_times) );

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

TEST_CASE( "Test read single frame from IPX version 2", "[IPX][V2]" )
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
    REQUIRE( video.atomicCount() == 23 );

    std::vector<std::string> exp_names = { "datetime", "shot", "lens", "filter", "view", "camera", "is_color",
                                           "width", "height", "depth", "taps", "left", "top", "hbin", "vbin",
                                           "offset", "gain", "preexp", "exposure", "board_temp", "ccd_temp",
                                           "n_frames", "frame_times" };
    REQUIRE( video.atomicNames() == exp_names );

    std::vector<bool> exp_ptrs = { false, false, false, false, false, false, false, false, false, false, false, false,
                                   false, false, false, false, false, false, false, false, false, false, true };
    REQUIRE( video.atomicPointers() == exp_ptrs );

    std::vector<std::string> exp_types = { "STRING", "int", "STRING", "STRING", "STRING", "STRING", "int", "int",
                                           "int", "int", "int", "int", "int", "int", "int", "double", "double",
                                           "double", "double", "double", "double", "int", "double *" };
    REQUIRE( video.atomicTypes() == exp_types );

    std::vector<size_t> exp_rank = { 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0 };
    REQUIRE( video.atomicRank() == exp_rank );

    std::vector<std::vector<size_t>> exp_shapes = { {256}, {1}, {256}, {256}, {256}, {256}, {1}, {1}, {1}, {1}, {1},
                                                    {1}, {1}, {1}, {1}, {2}, {2}, {1}, {1}, {1}, {1}, {1}, {2296} };
    REQUIRE( video.atomicShape() == exp_shapes );

    std::vector<const char*> exp_string_values = { "2013-10-25T12:45:29Z", "", "", "HL01 Upper divertor view#1",
                                                   "Thermosensorik CMT 256 SM HS" };
    std::vector<int> exp_int_values = { 30420, 0, 256, 32, 14, 0, 0, 153, 0, 0, 2296 };
    std::vector<double> exp_double_values = { 0.0, 50.0, 0.0, 59.0 };

    int string_i = 0;
    int int_i = 0;
    int double_i = 0;

    for (const auto& name : exp_names) {
        uda::Scalar value = video.atomicScalar(name);
        if (name != "offset" && name != "gain" && name != "frame_times") {
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

    uda::Vector frame_times = video.atomicVector("frame_times");
    REQUIRE( !frame_times.isNull() );
    std::vector<double> exp_frame_times = {
            -0.049948, -0.049748, -0.049548, -0.049348, -0.049148, -0.048948, -0.048748, -0.048548, -0.048349,
            -0.048148, -0.047948, -0.047748, -0.047548, -0.047348, -0.047149, -0.046948, -0.046748, -0.046548,
            -0.046348, -0.046148, -0.045948, -0.045748, -0.045548, -0.045348, -0.045148, -0.044949, -0.044749,
            -0.044548, -0.044348, -0.044148, -0.043948, -0.043749, -0.043548, -0.043348, -0.043148, -0.042948,
            -0.042748, -0.042548, -0.042348, -0.042148, -0.041948, -0.041748, -0.041548, -0.041349, -0.041148,
            -0.040948, -0.040748, -0.040548, -0.040349, -0.040148, -0.039948, -0.039748, -0.039548, -0.039348,
            -0.039148, -0.038949, -0.038748, -0.038548, -0.038348, -0.038148, -0.037949, -0.037748, -0.037548,
            -0.037348, -0.037148, -0.036948, -0.036748, -0.036548, -0.036348, -0.036148, -0.035948, -0.035748,
            -0.035549, -0.035348, -0.035148, -0.034948, -0.034748, -0.034549, -0.034348, -0.034148, -0.033948,
            -0.033748, -0.033548, -0.033348, -0.033149, -0.032948, -0.032748, -0.032548, -0.032348, -0.032148,
            -0.031948, -0.031748, -0.031548, -0.031348, -0.031148, -0.030948, -0.030748, -0.030548, -0.030348,
            -0.030148, -0.029948, -0.029748, -0.029548, -0.029348, -0.029148, -0.028948, -0.028748, -0.028548,
            -0.028348, -0.028148, -0.027948, -0.027748, -0.027548, -0.027348, -0.027148, -0.026948, -0.026748,
            -0.026548, -0.026348, -0.026148, -0.025948, -0.025748, -0.025548, -0.025348, -0.025148, -0.024948,
            -0.024748, -0.024548, -0.024348, -0.024148, -0.023948, -0.023748, -0.023548, -0.023348, -0.023148,
            -0.022948, -0.022748, -0.022548, -0.022348, -0.022148, -0.021948, -0.021748, -0.021548, -0.021348,
            -0.021148, -0.020948, -0.020748, -0.020548, -0.020348, -0.020148, -0.019948, -0.019748, -0.019548,
            -0.019348, -0.019148, -0.018948, -0.018748, -0.018548, -0.018348, -0.018148, -0.017948, -0.017748,
            -0.017548, -0.017348, -0.017148, -0.016948, -0.016748, -0.016548, -0.016348, -0.016148, -0.015948,
            -0.015748, -0.015548, -0.015348, -0.015148, -0.014948, -0.014748, -0.014548, -0.014348, -0.014148,
            -0.013948, -0.013748, -0.013548, -0.013348, -0.013148, -0.012948, -0.012748, -0.012548, -0.012348,
            -0.012148, -0.011948, -0.011748, -0.011548, -0.011348, -0.011148, -0.010948, -0.010748, -0.010548,
            -0.010348, -0.010148, -0.009948, -0.009748, -0.009548, -0.009348, -0.009148, -0.008948, -0.008748,
            -0.008548, -0.008348, -0.008148, -0.007948, -0.007748, -0.007548, -0.007348, -0.007148, -0.006948,
            -0.006748, -0.006548, -0.006348, -0.006148, -0.005948, -0.005748, -0.005548, -0.005348, -0.005148,
            -0.004948, -0.004748, -0.004548, -0.004348, -0.004148, -0.003948, -0.003748, -0.003548, -0.003348,
            -0.003148, -0.002948, -0.002748, -0.002548, -0.002348, -0.002148, -0.001948, -0.001748, -0.001548,
            -0.001348, -0.001148, -0.000948, -0.000748, -0.000548, -0.000348, -0.000148, 0.000051, 0.000251,
            0.000451, 0.000651, 0.000851, 0.001051, 0.001251, 0.001451, 0.001651, 0.001851, 0.002051, 0.002251,
            0.002451, 0.002651, 0.002851, 0.003051, 0.003251, 0.003451, 0.003651, 0.003851, 0.004051, 0.004251,
            0.004451, 0.004651, 0.004851, 0.005051, 0.005251, 0.005451, 0.005651, 0.005851, 0.006051, 0.006251,
            0.006451, 0.006651, 0.006851, 0.007051, 0.007251, 0.007451, 0.007651, 0.007851, 0.008051, 0.008251,
            0.008451, 0.008651, 0.008851, 0.009051, 0.009251, 0.009451, 0.009651, 0.009851, 0.010051, 0.010251,
            0.010451, 0.010651, 0.010851, 0.011051, 0.011251, 0.011451, 0.011651, 0.011851, 0.012051, 0.012251,
            0.012451, 0.012651, 0.012851, 0.013051, 0.013251, 0.013451, 0.013651, 0.013851, 0.014051, 0.014251,
            0.014451, 0.014651, 0.014851, 0.015051, 0.015251, 0.015451, 0.015651, 0.015851, 0.016051, 0.016251,
            0.016451, 0.016651, 0.016851, 0.017051, 0.017251, 0.017451, 0.017651, 0.017851, 0.018051, 0.018251,
            0.018451, 0.018651, 0.018851, 0.019051, 0.019251, 0.019451, 0.019651, 0.019851, 0.020051, 0.020251,
            0.020451, 0.020651, 0.020851, 0.021051, 0.021251, 0.021451, 0.021651, 0.021851, 0.022051, 0.022251,
            0.022451, 0.022651, 0.022851, 0.023051, 0.023251, 0.023451, 0.023651, 0.023851, 0.024051, 0.024251,
            0.024451, 0.024651, 0.024851, 0.025051, 0.025251, 0.025451, 0.025651, 0.025851, 0.026051, 0.026251,
            0.026451, 0.026651, 0.026851, 0.027051, 0.027251, 0.027451, 0.027651, 0.027851, 0.028051, 0.028251,
            0.028451, 0.028651, 0.028851, 0.029051, 0.029251, 0.029451, 0.029651, 0.029851, 0.030051, 0.030251,
            0.030451, 0.030651, 0.030851, 0.031051, 0.031251, 0.031451, 0.031651, 0.031851, 0.032051, 0.032251,
            0.032451, 0.032651, 0.032851, 0.033051, 0.033251, 0.033451, 0.033651, 0.033851, 0.034051, 0.034251,
            0.034451, 0.034651, 0.034851, 0.035051, 0.035251, 0.035451, 0.035651, 0.035851, 0.036051, 0.036251,
            0.036451, 0.036651, 0.036851, 0.037051, 0.037251, 0.037451, 0.037651, 0.037851, 0.038051, 0.038251,
            0.038451, 0.038651, 0.038851, 0.039051, 0.039251, 0.039451, 0.039651, 0.039851, 0.040051, 0.040251,
            0.040451, 0.040651, 0.040851, 0.041051, 0.041251, 0.041451, 0.041651, 0.041851, 0.042051, 0.042251,
            0.042451, 0.042651, 0.042851, 0.043051, 0.043251, 0.043451, 0.043651, 0.043851, 0.044051, 0.044251,
            0.044451, 0.044651, 0.044851, 0.045051, 0.045251, 0.045451, 0.045651, 0.045851, 0.046051, 0.046251,
            0.046451, 0.046651, 0.046851, 0.047051, 0.047251, 0.047451, 0.047651, 0.047851, 0.048051, 0.048251,
            0.048451, 0.048651, 0.048851, 0.049051, 0.049251, 0.049451, 0.049651, 0.049851, 0.050051, 0.050251,
            0.050451, 0.050651, 0.050851, 0.051051, 0.051251, 0.051451, 0.051651, 0.051851, 0.052051, 0.052251,
            0.052451, 0.052651, 0.052851, 0.053051, 0.053251, 0.053451, 0.053651, 0.053851, 0.054051, 0.054251,
            0.054451, 0.054651, 0.054851, 0.055051, 0.055251, 0.055451, 0.055651, 0.055851, 0.056051, 0.056251,
            0.056451, 0.056651, 0.056851, 0.057051, 0.057251, 0.057451, 0.057651, 0.057851, 0.058051, 0.058251,
            0.058451, 0.058651, 0.058851, 0.059051, 0.059251, 0.059451, 0.059651, 0.059851, 0.060051, 0.060251,
            0.060451, 0.060651, 0.060851, 0.061051, 0.061251, 0.061451, 0.061651, 0.061851, 0.062051, 0.062251,
            0.062451, 0.062651, 0.062851, 0.063051, 0.063251, 0.063451, 0.063651, 0.063851, 0.064051, 0.064251,
            0.064451, 0.064651, 0.064851, 0.065051, 0.065251, 0.065451, 0.065651, 0.065851, 0.066051, 0.066251,
            0.066451, 0.066651, 0.066851, 0.067051, 0.067251, 0.067451, 0.067651, 0.067851, 0.068051, 0.068251,
            0.068451, 0.068651, 0.068851, 0.069051, 0.069251, 0.069451, 0.069651, 0.069851, 0.070051, 0.070251,
            0.070451, 0.070651, 0.070851, 0.071051, 0.071251, 0.071451, 0.071651, 0.071851, 0.072051, 0.072251,
            0.072451, 0.072651, 0.072851, 0.073051, 0.073251, 0.073451, 0.073651, 0.073851, 0.074051, 0.074251,
            0.074451, 0.074651, 0.074851, 0.075051, 0.075251, 0.075451, 0.075651, 0.075851, 0.076051, 0.076251,
            0.076451, 0.076651, 0.076851, 0.077051, 0.077251, 0.077451, 0.077651, 0.077851, 0.078051, 0.078251,
            0.07845, 0.078651, 0.078851, 0.079051, 0.079251, 0.079451, 0.079651, 0.079851, 0.080051, 0.080251,
            0.080451, 0.080651, 0.080851, 0.081051, 0.081251, 0.081451, 0.081651, 0.081851, 0.082051, 0.082251,
            0.082451, 0.082651, 0.082851, 0.083051, 0.083251, 0.083451, 0.083651, 0.083851, 0.084051, 0.084251,
            0.084451, 0.084651, 0.084851, 0.085051, 0.085251, 0.085451, 0.085651, 0.085851, 0.086051, 0.086251,
            0.086451, 0.086651, 0.086851, 0.087051, 0.087251, 0.087451, 0.087651, 0.087851, 0.088051, 0.088251,
            0.088451, 0.088651, 0.088851, 0.089051, 0.089251, 0.089451, 0.089651, 0.089851, 0.090051, 0.090251,
            0.090451, 0.090651, 0.090851, 0.09105, 0.091251, 0.091451, 0.091651, 0.091851, 0.092051, 0.092251,
            0.092451, 0.092651, 0.092851, 0.093051, 0.093251, 0.093451, 0.093651, 0.093851, 0.094051, 0.094251,
            0.094451, 0.094651, 0.094851, 0.095051, 0.095251, 0.095451, 0.095651, 0.095851, 0.096051, 0.096251,
            0.096451, 0.096651, 0.096851, 0.097051, 0.097251, 0.097451, 0.097651, 0.097851, 0.098051, 0.098251,
            0.098451, 0.098651, 0.098851, 0.099051, 0.099251, 0.099451, 0.099651, 0.099851, 0.100051, 0.100251,
            0.100451, 0.100651, 0.100851, 0.101051, 0.101251, 0.101451, 0.101651, 0.101851, 0.102051, 0.102251,
            0.102451, 0.102651, 0.102851, 0.103051, 0.103251, 0.103451, 0.10365, 0.103851, 0.104051, 0.104251,
            0.104451, 0.104651, 0.104851, 0.105051, 0.105251, 0.105451, 0.105651, 0.105851, 0.106051, 0.106251,
            0.106451, 0.106651, 0.106851, 0.107051, 0.107251, 0.107451, 0.107651, 0.107851, 0.108051, 0.108251,
            0.108451, 0.108651, 0.108851, 0.109051, 0.109251, 0.10945, 0.109651, 0.109851, 0.110051, 0.110251,
            0.110451, 0.110651, 0.110851, 0.111051, 0.111251, 0.111451, 0.111651, 0.111851, 0.112051, 0.112251,
            0.112451, 0.112651, 0.112851, 0.113051, 0.113251, 0.113451, 0.113651, 0.113851, 0.114051, 0.114251,
            0.114451, 0.114651, 0.114851, 0.115051, 0.115251, 0.115451, 0.115651, 0.115851, 0.116051, 0.116251,
            0.116451, 0.116651, 0.116851, 0.117051, 0.117251, 0.117451, 0.117651, 0.117851, 0.118051, 0.118251,
            0.118451, 0.118651, 0.118851, 0.119051, 0.119251, 0.119451, 0.119651, 0.119851, 0.120051, 0.120251,
            0.120451, 0.120651, 0.120851, 0.121051, 0.121251, 0.121451, 0.121651, 0.121851, 0.12205, 0.122251,
            0.122451, 0.122651, 0.122851, 0.123051, 0.123251, 0.123451, 0.123651, 0.123851, 0.124051, 0.124251,
            0.124451, 0.124651, 0.124851, 0.125051, 0.12525, 0.125451, 0.125651, 0.125851, 0.126051, 0.12625,
            0.126451, 0.126651, 0.126851, 0.127051, 0.12725, 0.127451, 0.127651, 0.127851, 0.128051, 0.12825,
            0.128451, 0.128651, 0.12885, 0.129051, 0.12925, 0.129451, 0.129651, 0.12985, 0.130051, 0.13025,
            0.130451, 0.130651, 0.13085, 0.131051, 0.131251, 0.131451, 0.131651, 0.13185, 0.132051, 0.132251,
            0.132451, 0.132651, 0.13285, 0.133051, 0.133251, 0.133451, 0.133651, 0.13385, 0.134051, 0.134251,
            0.134451, 0.134651, 0.13485, 0.135051, 0.135251, 0.135451, 0.135651, 0.13585, 0.136051, 0.136251,
            0.136451, 0.136651, 0.136851, 0.137051, 0.137251, 0.137451, 0.137651, 0.137851, 0.138051, 0.138251,
            0.138451, 0.138651, 0.138851, 0.139051, 0.139251, 0.139451, 0.139651, 0.139851, 0.140051, 0.140251,
            0.140451, 0.140651, 0.140851, 0.141051, 0.141251, 0.14145, 0.141651, 0.141851, 0.142051, 0.142251,
            0.14245, 0.142651, 0.142851, 0.143051, 0.143251, 0.14345, 0.143651, 0.143851, 0.144051, 0.144251,
            0.14445, 0.144651, 0.144851, 0.145051, 0.145251, 0.14545, 0.145651, 0.145851, 0.146051, 0.146251,
            0.14645, 0.146651, 0.146851, 0.147051, 0.147251, 0.14745, 0.147651, 0.147851, 0.148051, 0.148251,
            0.14845, 0.148651, 0.148851, 0.149051, 0.149251, 0.149451, 0.149651, 0.149851, 0.150051, 0.150251,
            0.150451, 0.150651, 0.150851, 0.151051, 0.151251, 0.151451, 0.151651, 0.151851, 0.152051, 0.152251,
            0.152451, 0.152651, 0.152851, 0.153051, 0.153251, 0.153451, 0.153651, 0.153851, 0.15405, 0.154251,
            0.154451, 0.154651, 0.154851, 0.15505, 0.155251, 0.155451, 0.155651, 0.155851, 0.15605, 0.156251,
            0.156451, 0.156651, 0.156851, 0.15705, 0.157251, 0.157451, 0.157651, 0.157851, 0.15805, 0.158251, 0.158451,
            0.158651, 0.158851, 0.15905, 0.159251, 0.159451, 0.159651, 0.159851, 0.16005, 0.160251, 0.160451, 0.160651,
            0.160851, 0.16105, 0.161251, 0.161451, 0.161651, 0.161851, 0.162051, 0.162251, 0.162451, 0.162651, 0.162851,
            0.163051, 0.163251, 0.163451, 0.163651, 0.163851, 0.164051, 0.164251, 0.164451, 0.164651, 0.164851,
            0.165051, 0.165251, 0.165451, 0.165651, 0.165851, 0.166051, 0.166251, 0.166451, 0.16665, 0.166851, 0.167051,
            0.167251, 0.167451, 0.16765, 0.167851, 0.168051, 0.168251, 0.168451, 0.16865, 0.168851, 0.169051, 0.169251,
            0.169451, 0.16965, 0.169851, 0.170051, 0.170251, 0.170451, 0.17065, 0.170851, 0.171051, 0.171251, 0.171451,
            0.17165, 0.171851, 0.172051, 0.172251, 0.172451, 0.17265, 0.172851, 0.173051, 0.173251, 0.173451, 0.17365,
            0.173851, 0.174051, 0.174251, 0.174451, 0.174651, 0.174851, 0.175051, 0.175251, 0.175451, 0.175651,
            0.175851, 0.176051, 0.176251, 0.176451, 0.176651, 0.176851, 0.177051, 0.177251, 0.177451, 0.177651,
            0.177851, 0.178051, 0.17825, 0.178451, 0.178651, 0.178851, 0.179051, 0.17925, 0.179451, 0.179651, 0.179851,
            0.180051, 0.18025, 0.180451, 0.180651, 0.180851, 0.181051, 0.18125, 0.181451, 0.181651, 0.181851, 0.182051,
            0.18225, 0.182451, 0.182651, 0.182851, 0.183051, 0.18325, 0.183451, 0.183651, 0.183851, 0.184051, 0.18425,
            0.184451, 0.184651, 0.184851, 0.185051, 0.18525, 0.185451, 0.185651, 0.185851, 0.186051, 0.186251, 0.186451,
            0.186651, 0.186851, 0.187051, 0.187251, 0.187451, 0.187651, 0.187851, 0.188051, 0.188251, 0.188451,
            0.188651, 0.188851, 0.189051, 0.189251, 0.189451, 0.189651, 0.189851, 0.190051, 0.190251, 0.190451,
            0.190651, 0.19085, 0.191051, 0.191251, 0.191451, 0.191651, 0.19185, 0.192051, 0.192251, 0.192451, 0.192651,
            0.19285, 0.193051, 0.193251, 0.193451, 0.193651, 0.19385, 0.194051, 0.194251, 0.194451, 0.194651, 0.19485,
            0.195051, 0.195251, 0.195451, 0.195651, 0.19585, 0.196051, 0.196251, 0.196451, 0.196651, 0.19685, 0.197051,
            0.197251, 0.197451, 0.197651, 0.19785, 0.198051, 0.198251, 0.198451, 0.198651, 0.198851, 0.199051, 0.199251,
            0.199451, 0.199651, 0.199851, 0.200051, 0.200251, 0.200451, 0.200651, 0.200851, 0.201051, 0.201251,
            0.201451, 0.201651, 0.201851, 0.202051, 0.202251, 0.202451, 0.202651, 0.202851, 0.203051, 0.203251,
            0.203451, 0.203651, 0.203851, 0.204051, 0.204251, 0.204451, 0.204651, 0.204851, 0.205051, 0.205251,
            0.205451, 0.205651, 0.205851, 0.206051, 0.206251, 0.206451, 0.206651, 0.206851, 0.207051, 0.207251,
            0.207451, 0.207651, 0.207851, 0.208051, 0.208251, 0.208451, 0.208651, 0.208851, 0.209051, 0.209251,
            0.209451, 0.209651, 0.209851, 0.210051, 0.210251, 0.210451, 0.210651, 0.210851, 0.211051, 0.211251,
            0.211451, 0.211651, 0.211851, 0.212051, 0.212251, 0.212451, 0.212651, 0.212851, 0.213051, 0.213251,
            0.213451, 0.213651, 0.213851, 0.214051, 0.214251, 0.214451, 0.214651, 0.214851, 0.215051, 0.215251,
            0.215451, 0.215651, 0.215851, 0.216051, 0.216251, 0.216451, 0.216651, 0.216851, 0.217051, 0.217251,
            0.217451, 0.217651, 0.217851, 0.218051, 0.218251, 0.218451, 0.218651, 0.218851, 0.219051, 0.219251,
            0.219451, 0.219651, 0.219851, 0.220051, 0.220251, 0.220451, 0.220651, 0.220851, 0.221051, 0.221251,
            0.221451, 0.221651, 0.221851, 0.222051, 0.222251, 0.222451, 0.222651, 0.222851, 0.223051, 0.223251,
            0.223451, 0.223651, 0.223851, 0.224051, 0.224251, 0.224451, 0.224651, 0.224851, 0.225051, 0.225251,
            0.225451, 0.225651, 0.225851, 0.226051, 0.226251, 0.226451, 0.226651, 0.226851, 0.227051, 0.227251,
            0.227451, 0.227651, 0.227851, 0.228051, 0.228251, 0.228451, 0.228651, 0.228851, 0.229051, 0.229251,
            0.229451, 0.229651, 0.229851, 0.230051, 0.230251, 0.230451, 0.230651, 0.230851, 0.231051, 0.231251,
            0.231451, 0.231651, 0.231851, 0.232051, 0.232251, 0.232451, 0.232651, 0.232851, 0.233051, 0.233251,
            0.233451, 0.233651, 0.233851, 0.234051, 0.234251, 0.234451, 0.234651, 0.234851, 0.235051, 0.235251,
            0.235451, 0.235651, 0.235851, 0.236051, 0.236251, 0.236451, 0.236651, 0.236851, 0.237051, 0.237251,
            0.237451, 0.237651, 0.237851, 0.238051, 0.238251, 0.238451, 0.238651, 0.238851, 0.239051, 0.239251,
            0.239451, 0.239651, 0.239851, 0.240051, 0.240251, 0.240451, 0.240651, 0.240851, 0.241051, 0.241251,
            0.241451, 0.241651, 0.241851, 0.242051, 0.242251, 0.242451, 0.242651, 0.242851, 0.243051, 0.243251,
            0.243451, 0.243651, 0.243851, 0.244051, 0.244251, 0.244451, 0.244651, 0.244851, 0.245051, 0.245251,
            0.245451, 0.245651, 0.245851, 0.246051, 0.246251, 0.246451, 0.246651, 0.246851, 0.247051, 0.247251,
            0.247451, 0.247651, 0.247851, 0.248051, 0.248251, 0.248451, 0.248651, 0.248851, 0.249051, 0.249251,
            0.249451, 0.249651, 0.249851, 0.250051, 0.250251, 0.250451, 0.250651, 0.250851, 0.251051, 0.251251,
            0.251451, 0.251651, 0.251851, 0.252051, 0.252251, 0.252451, 0.252651, 0.252851, 0.253051, 0.253251,
            0.253451, 0.253651, 0.253851, 0.254051, 0.254251, 0.254451, 0.254651, 0.254851, 0.255051, 0.255251,
            0.255451, 0.255651, 0.255851, 0.256051, 0.256251, 0.256451, 0.256651, 0.256851, 0.257051, 0.257251,
            0.257451, 0.257651, 0.257851, 0.258051, 0.258251, 0.258451, 0.258651, 0.258851, 0.259051, 0.259251,
            0.259451, 0.259651, 0.259851, 0.260051, 0.260251, 0.260451, 0.260651, 0.260851, 0.261051, 0.261251,
            0.261451, 0.261651, 0.261851, 0.262051, 0.262251, 0.262451, 0.262651, 0.262851, 0.263051, 0.263251,
            0.263451, 0.263651, 0.263851, 0.264051, 0.264251, 0.264451, 0.264651, 0.264851, 0.265051, 0.265251,
            0.265451, 0.265651, 0.265851, 0.266051, 0.266251, 0.266451, 0.266651, 0.266851, 0.267051, 0.267251,
            0.267451, 0.267651, 0.267851, 0.268051, 0.268251, 0.268451, 0.268651, 0.268851, 0.269051, 0.269251,
            0.269451, 0.269651, 0.269851, 0.270051, 0.270251, 0.270451, 0.270651, 0.270851, 0.271051, 0.271251,
            0.271451, 0.271651, 0.271851, 0.272051, 0.272251, 0.272451, 0.272651, 0.272851, 0.273051, 0.273251,
            0.273451, 0.273651, 0.273851, 0.274051, 0.274251, 0.274451, 0.274651, 0.274851, 0.275051, 0.275251,
            0.275451, 0.275651, 0.275851, 0.276051, 0.276251, 0.276451, 0.276651, 0.276851, 0.277051, 0.277251,
            0.277451, 0.277651, 0.277851, 0.278051, 0.278251, 0.278451, 0.278651, 0.278851, 0.279051, 0.279251,
            0.279451, 0.279651, 0.279851, 0.280051, 0.280251, 0.280451, 0.280651, 0.280851, 0.281051, 0.281251,
            0.281451, 0.281651, 0.281851, 0.282051, 0.282251, 0.282451, 0.282651, 0.282851, 0.283051, 0.283251,
            0.283451, 0.283651, 0.283851, 0.284051, 0.284251, 0.284451, 0.284651, 0.284851, 0.285051, 0.285251,
            0.285451, 0.285651, 0.285851, 0.286051, 0.286251, 0.286451, 0.286651, 0.286851, 0.287051, 0.287251,
            0.287451, 0.287651, 0.287851, 0.288051, 0.288251, 0.288451, 0.288651, 0.288851, 0.289051, 0.289251,
            0.289451, 0.289651, 0.289851, 0.290051, 0.290251, 0.290451, 0.290651, 0.290851, 0.291051, 0.291251,
            0.291451, 0.291651, 0.291851, 0.292051, 0.292251, 0.292451, 0.292651, 0.292851, 0.293051, 0.293251,
            0.293451, 0.293651, 0.293851, 0.294051, 0.294251, 0.294451, 0.294651, 0.294851, 0.295051, 0.295251,
            0.295451, 0.295651, 0.295851, 0.296051, 0.296251, 0.296451, 0.296651, 0.296851, 0.297051, 0.297251,
            0.297451, 0.297651, 0.297851, 0.298051, 0.298251, 0.298451, 0.298651, 0.298851, 0.299051, 0.299251,
            0.299451, 0.299651, 0.299851, 0.300051, 0.300251, 0.300451, 0.300651, 0.300851, 0.301051, 0.301251,
            0.301451, 0.301651, 0.301851, 0.302051, 0.302251, 0.302451, 0.302651, 0.302851, 0.303051, 0.303251,
            0.303451, 0.303651, 0.303851, 0.304051, 0.304251, 0.304451, 0.304651, 0.304851, 0.305051, 0.305251,
            0.305451, 0.305651, 0.305851, 0.306051, 0.306251, 0.306451, 0.306651, 0.306851, 0.307051, 0.307251,
            0.307451, 0.307651, 0.307851, 0.308051, 0.308251, 0.308451, 0.308651, 0.308851, 0.309051, 0.309251,
            0.309451, 0.309651, 0.309851, 0.310051, 0.310251, 0.310451, 0.310651, 0.310851, 0.311051, 0.311251,
            0.311451, 0.311651, 0.311851, 0.312051, 0.312251, 0.312451, 0.312651, 0.312851, 0.313051, 0.313251,
            0.313451, 0.313651, 0.313851, 0.314051, 0.314251, 0.314451, 0.314651, 0.314851, 0.315051, 0.315251,
            0.315451, 0.315651, 0.315851, 0.316051, 0.316251, 0.316451, 0.316651, 0.316851, 0.317051, 0.317251,
            0.317451, 0.317651, 0.317851, 0.318051, 0.318251, 0.318451, 0.318651, 0.318851, 0.319051, 0.319251,
            0.319451, 0.319651, 0.319851, 0.320051, 0.320251, 0.320451, 0.320651, 0.320851, 0.321051, 0.321251,
            0.321451, 0.321651, 0.321851, 0.322051, 0.322251, 0.322451, 0.322651, 0.322851, 0.323051, 0.323251,
            0.323451, 0.323651, 0.323851, 0.324051, 0.324251, 0.324451, 0.324651, 0.324851, 0.325051, 0.325251,
            0.325451, 0.325651, 0.325851, 0.326051, 0.326251, 0.326451, 0.326651, 0.326851, 0.327051, 0.327251,
            0.327451, 0.327651, 0.327851, 0.328051, 0.328251, 0.328451, 0.328651, 0.328851, 0.329051, 0.329251,
            0.329451, 0.329651, 0.329851, 0.330051, 0.330251, 0.330451, 0.330651, 0.330851, 0.331051, 0.331251,
            0.331451, 0.331651, 0.331851, 0.332051, 0.332251, 0.332451, 0.332651, 0.332851, 0.333051, 0.333251,
            0.333451, 0.333651, 0.333851, 0.334051, 0.334251, 0.334451, 0.334651, 0.334851, 0.335051, 0.335251,
            0.335451, 0.335651, 0.335851, 0.336051, 0.336251, 0.336451, 0.336651, 0.336851, 0.337051, 0.337251,
            0.337451, 0.337651, 0.337851, 0.338051, 0.338251, 0.338451, 0.338651, 0.338851, 0.339051, 0.339251,
            0.339451, 0.339651, 0.339851, 0.340051, 0.340251, 0.340451, 0.340651, 0.340851, 0.341051, 0.341251,
            0.341451, 0.341651, 0.341851, 0.342051, 0.342251, 0.342451, 0.342651, 0.342851, 0.343051, 0.343251,
            0.343451, 0.343651, 0.343851, 0.344051, 0.344251, 0.344451, 0.344651, 0.344851, 0.345051, 0.345251,
            0.345451, 0.345651, 0.345851, 0.346051, 0.346251, 0.346451, 0.346651, 0.346851, 0.347051, 0.347251,
            0.347451, 0.347651, 0.347851, 0.348051, 0.348251, 0.348451, 0.348651, 0.348851, 0.349051, 0.349251,
            0.349451, 0.349651, 0.349851, 0.350051, 0.350251, 0.350451, 0.350651, 0.350851, 0.351051, 0.351251,
            0.351451, 0.351651, 0.351851, 0.352051, 0.352251, 0.352451, 0.352651, 0.352851, 0.353051, 0.353251,
            0.353451, 0.353651, 0.353851, 0.354051, 0.354251, 0.354451, 0.354651, 0.354851, 0.355051, 0.355251,
            0.355451, 0.355651, 0.355851, 0.356051, 0.356251, 0.356451, 0.356651, 0.356851, 0.357051, 0.357251,
            0.357451, 0.357651, 0.357851, 0.358051, 0.358251, 0.358451, 0.358651, 0.358851, 0.359051, 0.359251,
            0.359451, 0.359651, 0.359851, 0.360051, 0.360251, 0.360451, 0.360651, 0.360851, 0.361051, 0.361251,
            0.361451, 0.361651, 0.361851, 0.362051, 0.362251, 0.362451, 0.362651, 0.362851, 0.363051, 0.363251,
            0.363451, 0.363651, 0.363851, 0.364051, 0.364251, 0.364451, 0.364651, 0.364851, 0.365051, 0.365251,
            0.365451, 0.365651, 0.365851, 0.366051, 0.366251, 0.366451, 0.366651, 0.366851, 0.367051, 0.367251,
            0.367451, 0.367651, 0.367851, 0.368051, 0.368251, 0.368451, 0.368651, 0.368851, 0.369051, 0.369251,
            0.369451, 0.369651, 0.369851, 0.370051, 0.370251, 0.370451, 0.370651, 0.370851, 0.371051, 0.371251,
            0.371451, 0.371651, 0.371851, 0.372051, 0.372251, 0.372451, 0.372651, 0.372851, 0.373051, 0.373251,
            0.373451, 0.373651, 0.373851, 0.374051, 0.374251, 0.374451, 0.374651, 0.374851, 0.375051, 0.375251,
            0.375451, 0.375651, 0.375851, 0.376051, 0.376251, 0.376451, 0.376651, 0.376851, 0.377051, 0.377251,
            0.377451, 0.377651, 0.377851, 0.378051, 0.378251, 0.378451, 0.378651, 0.378851, 0.379051, 0.379251,
            0.379451, 0.379651, 0.379851, 0.380051, 0.380251, 0.380451, 0.380651, 0.380851, 0.381051, 0.381251,
            0.381451, 0.381651, 0.381851, 0.382051, 0.382251, 0.382451, 0.382651, 0.382851, 0.383051, 0.383251,
            0.383451, 0.383651, 0.383851, 0.384051, 0.384251, 0.384451, 0.384651, 0.384851, 0.385051, 0.385251,
            0.385451, 0.385651, 0.385851, 0.386051, 0.386251, 0.386451, 0.386651, 0.386851, 0.387051, 0.387251,
            0.387451, 0.387651, 0.387851, 0.388051, 0.388251, 0.388451, 0.388651, 0.388851, 0.389051, 0.389251,
            0.389451, 0.389651, 0.389851, 0.390051, 0.390251, 0.390451, 0.390651, 0.390851, 0.391051, 0.391251,
            0.391451, 0.391651, 0.391851, 0.392051, 0.392251, 0.392451, 0.392651, 0.392851, 0.393051, 0.393251,
            0.393451, 0.393651, 0.393851, 0.394051, 0.394251, 0.394451, 0.394651, 0.394851, 0.395051, 0.395251,
            0.395451, 0.395651, 0.395851, 0.396051, 0.396251, 0.396451, 0.396651, 0.396851, 0.397051, 0.397251,
            0.397451, 0.397651, 0.397851, 0.398051, 0.398251, 0.398451, 0.398651, 0.398851, 0.399051, 0.399251,
            0.399451, 0.399651, 0.399851, 0.400051, 0.400251, 0.400451, 0.400651, 0.400851, 0.401051, 0.401251,
            0.401451, 0.401651, 0.401851, 0.402051, 0.402251, 0.402451, 0.402651, 0.402851, 0.403051, 0.403251,
            0.403451, 0.403651, 0.403851, 0.404051, 0.404251, 0.404451, 0.404651, 0.404851, 0.405051, 0.405251,
            0.405451, 0.405651, 0.405851, 0.406051, 0.406251, 0.406451, 0.406651, 0.406851, 0.407051, 0.407251,
            0.407451, 0.407651, 0.407851, 0.408051, 0.408251, 0.408451, 0.408651, 0.408851, 0.409051
    };
    REQUIRE( frame_times.as<double>() == ApproxVector(exp_frame_times) );

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