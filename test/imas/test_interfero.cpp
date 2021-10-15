#if 0
#!/bin/bash
g++ test_interfero.cpp -g -O0 -gdwarf-3 -o test -DHOME=$HOME -I$HOME/iter/uda/source -I$HOME/iter/uda/source/wrappers \
-L$HOME/iter/uda/lib -Wl,-rpath,$HOME/iter/uda/lib  -luda_cpp -lssl -lcrypto -lxml2
exit 0
#endif

#include <c++/UDA.hpp>
#include <typeinfo>
#include <iostream>
#include <sstream>
#include <fstream>

#define QUOTE_(X) #X
#define QUOTE(X) QUOTE_(X)
#define SHOT_NUM_TORE_SUPRA "43979" // WEST
#define SHOT_NUM "50355" // WEST

int main() {
    setenv("UDA_PLUGIN_DIR", QUOTE(HOME) "/iter/uda/etc/plugins", 1);
    setenv("UDA_PLUGIN_CONFIG", QUOTE(HOME) "/iter/uda/test/idamTest.conf", 1);
    setenv("UDA_SARRAY_CONFIG", QUOTE(HOME) "/iter/uda/bin/plugins/idamgenstruct.conf", 1);
    setenv("UDA_WEST_MAPPING_FILE", QUOTE(HOME) "/iter/uda/source/plugins/west/WEST_mappings/IDAM_mapping.xml", 1);
    setenv("UDA_WEST_MAPPING_FILE_DIRECTORY", QUOTE(HOME) "/iter/uda/source/plugins/west/WEST_mappings", 1);
    setenv("UDA_LOG", QUOTE(HOME) "/iter/uda/", 1);
    setenv("UDA_LOG_MODE", "a", 1);
    setenv("UDA_LOG_LEVEL", "DEBUG", 1);
    setenv("UDA_DEBUG_APPEND", "a", 1);

    uda::Client::setProperty(uda::PROP_DEBUG, true);
    uda::Client::setProperty(uda::PROP_VERBOSE, true);

    uda::Client::setServerHostName("localhost");
    uda::Client::setServerPort(56565);

    uda::Client client;

    const uda::Result& sig = client.get("imas::get(idx=0, group='interfero_polarimeter', variable='channel/1/n_e_line/data', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
    const uda::Data * data = sig.data();
    const uda::Array* arr = dynamic_cast<const uda::Array*>(data);

    std::cout << "first values for channel/1/n_e_line/data : ";
    for (int j = 0; j < 10; ++j) {
        std::cout << arr->as<double>().at(j) << " ";
    }
    std::cout << "..." << std::endl;

    const uda::Result& sig3 = client.get("imas::get(idx=0, group='interfero_polarimeter', variable='channel/3/n_e_line/data', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
    const uda::Data * data3 = sig3.data();
    const uda::Array* arr3 = dynamic_cast<const uda::Array*>(data3);

    std::cout << "first values for channel/3/n_e_line/data : ";
    for (int j = 0; j < 10; ++j) {
        std::cout << arr3->as<double>().at(j) << " ";
    }
    std::cout << "..." << std::endl;

    const uda::Result& sigtime = client.get("imas::get(idx=0, group='interfero_polarimeter', variable='channel/3/n_e_line/time', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
    const uda::Data * data_time = sigtime.data();
    const uda::Array* arrtime = dynamic_cast<const uda::Array*>(data_time);

    std::cout << "first values for channel/3/n_e_line/time : ";
    for (int j = 0; j < 10; ++j) {
        std::cout << arrtime->as<double>().at(j) << " ";
    }
    std::cout << "..." << std::endl;


    const uda::Result& validity = client.get("imas::get(idx=0, group='interfero_polarimeter', variable='channel/1/validity', expName='WEST', type=int, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Scalar* v_validity = dynamic_cast<const uda::Scalar*>(validity.data());
    std::cout << "channel/1/validity: " << v_validity->as<int>() << std::endl;

    const uda::Result& validity2 = client.get("imas::get(idx=0, group='interfero_polarimeter', variable='channel/2/validity', expName='WEST', type=int, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Scalar* v_validity2 = dynamic_cast<const uda::Scalar*>(validity2.data());
    std::cout << "channel/2/validity: " << v_validity2->as<int>() << std::endl;

    const uda::Result& validity3 = client.get("imas::get(idx=0, group='interfero_polarimeter', variable='channel/3/validity', expName='WEST', type=int, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Scalar* v_validity3 = dynamic_cast<const uda::Scalar*>(validity3.data());
    std::cout << "channel/3/validity: " << v_validity3->as<int>() << std::endl;




    return 0;
}

