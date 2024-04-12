#include "capnp_reader.hpp"
#include "netcdf_reader.hpp"
#include "capnp_json_reader.hpp"
#include "write_testfile.hpp"

#include <string>
#include <vector>

using namespace uda::plugins::netcdf;

int main()
{
    std::string folder = "./test_file_inputs";
    nc_test_files::write_test_files(folder);
    Reader nc_reader(folder + "/signal_tests.nc");
    auto capnp_buffer = nc_reader.read_data("/");

    CapnpJsonReader json_writer(capnp_buffer);
    std::string output_folder = "./test_outputs";
    nc_test_files::create_testfile_folder(output_folder);
    json_writer.save_to_json(output_folder + "/signal_tests.json");
    return 0;
}
