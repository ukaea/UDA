#define CATCH_CONFIG_MAIN

#include <iostream>
#include <fstream>

#include <catch2/catch_all.hpp>
#include <nlohmann/json.hpp>

#include "netcdf_reader.hpp"
#include "capnp_json_reader.hpp"
#include "write_testfile.hpp"

using json = nlohmann::json;
using namespace uda::plugins::netcdf;

void write_capnp_tree_to_json_file(TreeReader* tree, std::string path)
{
    uda::plugins::netcdf::CapnpJsonReader json_adaptor(tree);
    json_adaptor.save_to_json(path);
}

bool json_files_match(std::string file_1, std::string file_2)
{
    return json::parse(std::ifstream(file_1)) == json::parse(std::ifstream(file_2));
}

TEST_CASE("create test netcdf files")
{
    std::filesystem::remove_all("./unit_tests");
    nc_test_files::write_test_files("./unit_tests");
}

TEST_CASE("Get groups recursively")
{
    Reader file_reader("unit_tests/signal_tests.nc");
    auto groups = file_reader.get_groups();
    std::set<std::string> result(groups.begin(), groups.end());
    std::set<std::string> expected_result {"/", "/device_1", "/device_1/subgroup1", "/device_2"};
    REQUIRE (result == expected_result);
}

TEST_CASE("Get variables recursively")
{
    Reader file_reader("unit_tests/signal_tests.nc");
    auto groups = file_reader.get_variables();
    std::set<std::string> result(groups.begin(), groups.end());
    std::set<std::string> expected_result {"/device_1/subgroup1/double_variable_1d", 
    "/device_1/subgroup1/float_variable_1d", "/device_1/subgroup1/int_variable_1d", "/device_2/int_variable_2d",
    "/dim1", "/dim2", "/dim3" };
    REQUIRE (result == expected_result);
}

TEST_CASE("Get all co-ordinate variables")
{
    Reader file_reader("unit_tests/signal_tests.nc");
    auto groups = file_reader.get_coordinates();
    std::set<std::string> result(groups.begin(), groups.end());
    std::set<std::string> expected_result {"dim1", "dim2", "dim3"};
    REQUIRE (result == expected_result);
}

// TEST_CASE("Get all index-only dimensions")
// {
//     Reader file_reader("tests/input_files/passive_efit.nc");
//     auto groups = file_reader.get_index_dimensions();
//     std::set<std::string> result(groups.begin(), groups.end());
//
//     std::set<std::string> expected_result  { "singleDim" };
//     REQUIRE (result == expected_result);
// }

TEST_CASE("Request path exists")
{
    Reader file_reader("unit_tests/signal_tests.nc");

    for (const auto& request: file_reader.get_variables())
    {
        RequestType request_type = file_reader.check_request_path(request);
        INFO ("Request: " << request);
        REQUIRE(request_type != RequestType::INVALID_PATH);
    }

    for (const auto& request: file_reader.get_groups())
    {
        RequestType request_type = file_reader.check_request_path(request);
        INFO ("Request: " << request);
        REQUIRE(request_type != RequestType::INVALID_PATH);
    }

    std::vector<std::string> invalid_paths {"/fake/path/1", "/invalid/path/2", "ill&gal//?path*string"};
    for (const auto& request: invalid_paths)
    {
        RequestType request_type = file_reader.check_request_path(request);
        INFO ("Request: " << request);
        REQUIRE(request_type == RequestType::INVALID_PATH);
    }
}
