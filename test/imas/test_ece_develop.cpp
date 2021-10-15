#if 0
#!/bin/bash
g++ test_ece_develop.cpp -g -O0 -gdwarf-3 -o test -DHOME=$HOME -I$HOME/iter/uda/source -I$HOME/iter/uda/source/wrappers \
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
#define SHOT_NUM "50355" // WEST

#define SHOT_NUM_10000 "10000" // WEST
#define SHOT_NUM_33456 "33456" // WEST
#define SHOT_NUM_50819 "50819" // WEST

#define SHOT_NUM_50355 "50355" // WEST

#define SHOT_NUM_80 "80" // WEST

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

    const uda::Result& shape_of = client.get("imas::get(idx=0, group='ece', variable='channel/Shape_of', expName='WEST', type=int, rank=0, shot=" SHOT_NUM_33456 ", )", "");
    const uda::Scalar* v_shape_of = dynamic_cast<const uda::Scalar*>(shape_of.data());
    std::cout << "channel/Shape_of: " << v_shape_of->as<int>() << std::endl;

    const uda::Result& name = client.get("imas::get(idx=0, group='ece', variable='channel/1/name', expName='WEST', type=string, rank=0, shot=" SHOT_NUM_33456 ", )", "");
    const uda::Data * data1 = name.data();
    const uda::String* s1 = dynamic_cast<const uda::String*>(data1);

    char* result;
    std::cout << "channel/1/name : " << s1->str() << "\n";

    const uda::Result& name2 = client.get("imas::get(idx=0, group='ece', variable='channel/10/name', expName='WEST', type=string, rank=0, shot=" SHOT_NUM_33456 ", )", "");
    const uda::Data * data2 = name2.data();
    const uda::String* s2 = dynamic_cast<const uda::String*>(data2);

    std::cout << "channel/10/name : " << s2->str() << "\n";

    /*const uda::Result& sigfreq = client.get("imas::get(idx=0, group='ece', variable='channel/1/frequency', expName='WEST', type=double, rank=1, shot=" SHOT_NUM_33456 ", )", "");
    const uda::Data *data = sigfreq.data();
    const uda::Array* arrfreq = dynamic_cast<const uda::Array*>(data);

    std::cout << "first values for channel/1/frequency : ";
    for (int j = 0; j < 10; ++j) {
        std::cout << arrfreq->as<double>().at(j) << " ";
    }
    std::cout << "..." << std::endl;

    const uda::Result& sigfreq2 = client.get("imas::get(idx=0, group='ece', variable='channel/7/frequency', expName='WEST', type=double, rank=1, shot=" SHOT_NUM_33456 ", )", "");
    data = sigfreq2.data();
    const uda::Array* arrfreq2 = dynamic_cast<const uda::Array*>(data);

    std::cout << "first values for channel/2/frequency : ";
    for (int j = 0; j < 10; ++j) {
        std::cout << arrfreq2->as<double>().at(j) << " ";
    }
    std::cout << "..." << std::endl;*/


    const uda::Result& sigtime = client.get("imas::get(idx=0, group='ece', variable='channel/1/t_e/time', expName='WEST', type=double, rank=1, shot=" SHOT_NUM_33456 ", )", "");
    const uda::Data *data = sigtime.data();
    const uda::Array* arrtime = dynamic_cast<const uda::Array*>(data);

    std::cout << "first values for channel/1/t_e/time : ";
    for (int j = 0; j < 10; ++j) {
        std::cout << arrtime->as<double>().at(j) << " ";
    }
    std::cout << "..." << std::endl;

    const uda::Result& sig2 = client.get("imas::get(idx=0, group='ece', variable='channel/1/t_e/data', expName='WEST', type=double, rank=1, shot=" SHOT_NUM_33456 ", )", "");
    data = sig2.data();
    const uda::Array* arr2 = dynamic_cast<const uda::Array*>(data);

    std::cout << "first values for channel/1/t_e/data for shot 33456 : ";
    for (int j = 0; j < 10; ++j) {
        std::cout << arr2->as<double>().at(j) << " ";
    }
    std::cout << "..." << std::endl;

    const uda::Result& sigharm = client.get("imas::get(idx=0, group='ece', variable='channel/1/harmonic/data', expName='WEST', type=double, rank=1, shot=" SHOT_NUM_33456 ", )", "");
    data = sigharm.data();
    const uda::Array* arrharm = dynamic_cast<const uda::Array*>(data);

    std::cout << "first values for channel/1/harmonic/data : ";
    for (int j = 0; j < 10; ++j) {
        std::cout << arrharm->as<double>().at(j) << " ";
    }
    std::cout << "..." << std::endl;

    const uda::Result& sigharm_time = client.get("imas::get(idx=0, group='ece', variable='channel/1/harmonic/time', expName='WEST', type=double, rank=1, shot=" SHOT_NUM_33456 ", )", "");
    data = sigharm_time.data();
    const uda::Array* arrharm_time = dynamic_cast<const uda::Array*>(data);

    std::cout << "first values for channel/1/harmonic/time : ";
    for (int j = 0; j < 10; ++j) {
        std::cout << arrharm_time->as<double>().at(j) << " ";
    }
    std::cout << "..." << std::endl;



    return 0;
}

