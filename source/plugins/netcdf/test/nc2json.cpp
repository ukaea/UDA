#include "capnp_reader.hpp"
#include "netcdf_reader.hpp"
#include "capnp_json_reader.hpp"

#include <string>
#include <iostream>
#include <exception>
#include <boost/program_options.hpp>

using namespace uda::plugins::netcdf;
namespace prog_opt = boost::program_options;

int main(int argc, char** argv)
{
    prog_opt::options_description desc("CLI for converting netcdf file to json. \nOptions:");
    desc.add_options()
        ("help", "produce help message")
        ("input", prog_opt::value<std::string>(), "Input netCDF file to parse")
        ("output", prog_opt::value<std::string>(), "File name of output json")
        ("nc-path", prog_opt::value<std::string>(), "NetCDF path to parse from input file")
        ("stream-output", prog_opt::value<bool>(), "output text to stdout instead of writing json file")
        ;

    prog_opt::variables_map args;        
    prog_opt::store(prog_opt::parse_command_line(argc, argv, desc), args);
    prog_opt::notify(args);    

    if (args.count("help")) 
    {
        std::cout << desc << "\n";
        return 0;
    }

    if (!args.count("input"))
    {
        std::cerr << "error: input file path is required" <<std::endl;
        std::cerr << desc << std::endl;
    }

    try 
    {
        auto path = args.count("nc-path") > 0 ? args.at("nc-path").as<std::string>() : "/";

        Reader nc_reader(args.at("input").as<std::string>());
        auto capnp_buffer = nc_reader.read_data(path);

        if (args.count("output"))
        {
            CapnpJsonReader serialiser(capnp_buffer);
            serialiser.save_to_json(args.at("output").as<std::string>());
        }
        else if (args.count("stream-output"))
        {
            CapnpJsonReader serialiser(capnp_buffer);
            serialiser.print_tree_reader();
        }
    }
    catch(std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
