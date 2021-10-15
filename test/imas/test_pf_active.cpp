#if 0
#!/bin/bash
g++ test_pf_active.cpp -g -O0 -gdwarf-3 -o test -DHOME=$HOME -I$HOME/iter/uda/source -I$HOME/iter/uda/source/wrappers \
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
#define SHOT_NUM "51371" // WEST

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

    const uda::Result& pf_active_elements_shapeOf = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/element/Shape_of', expName='WEST', type=int, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Scalar* scalar_pf_active_elements_shapeOf = dynamic_cast<const uda::Scalar*>(pf_active_elements_shapeOf.data());
    std::cout << "coil/1/element/Shape_of: " << scalar_pf_active_elements_shapeOf->as<int>() << std::endl;

    const uda::Result& pf_active_coils_shapeOf = client.get("imas::get(idx=0, group='pf_active', variable='coil/Shape_of', expName='WEST', type=int, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Scalar* scalar_pf_active_coils_shapeOf = dynamic_cast<const uda::Scalar*>(pf_active_coils_shapeOf.data());
    std::cout << "coil/Shape_of: " << scalar_pf_active_coils_shapeOf->as<int>() << std::endl;

    const uda::Result& pf_active_R = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/element/1/geometry/rectangle/r', expName='WEST', type=double, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Scalar* scalar_pf_active_R = dynamic_cast<const uda::Scalar*>(pf_active_R.data());
    std::cout << "coil/1/element/1/geometry/rectangle/r: " << scalar_pf_active_R->as<double>() << std::endl;

    const uda::Result& pf_active_R21 = client.get("imas::get(idx=0, group='pf_active', variable='coil/2/element/1/geometry/rectangle/r', expName='WEST', type=double, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Scalar* scalar_pf_active_R21 = dynamic_cast<const uda::Scalar*>(pf_active_R21.data());
    std::cout << "coil/2/element/1/geometry/rectangle/r: " << scalar_pf_active_R21->as<double>() << std::endl;

    const uda::Result& pf_active_Z = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/element/1/geometry/rectangle/z', expName='WEST', type=double, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Scalar* scalar_pf_active_Z = dynamic_cast<const uda::Scalar*>(pf_active_Z.data());
    std::cout << "coil/1/element/1/geometry/rectangle/z: " << scalar_pf_active_Z->as<double>() << std::endl;

    const uda::Result& pf_active_Z21 = client.get("imas::get(idx=0, group='pf_active', variable='coil/2/element/1/geometry/rectangle/z', expName='WEST', type=double, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Scalar* scalar_pf_active_Z21 = dynamic_cast<const uda::Scalar*>(pf_active_Z21.data());
    std::cout << "coil/2/element/1/geometry/rectangle/z: " << scalar_pf_active_Z21->as<double>() << std::endl;

    const uda::Result& pf_active_height = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/element/1/geometry/rectangle/height', expName='WEST', type=double, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Scalar* scalar_pf_active_height = dynamic_cast<const uda::Scalar*>(pf_active_height.data());
    std::cout << "coil/1/element/1/geometry/rectangle/height: " << scalar_pf_active_height->as<double>() << std::endl;

    const uda::Result& pf_active_height2 = client.get("imas::get(idx=0, group='pf_active', variable='coil/2/element/1/geometry/rectangle/height', expName='WEST', type=double, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Scalar* scalar_pf_active_height2 = dynamic_cast<const uda::Scalar*>(pf_active_height2.data());
    std::cout << "coil/1/element/1/geometry/rectangle/height2: " << scalar_pf_active_height2->as<double>() << std::endl;

    const uda::Result& pf_active_coil_name = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/name', expName='WEST', type=string, rank=1, shot=" SHOT_NUM ", )", "");
    const uda::Data * pf_active_coil_name_data = pf_active_coil_name.data();
    const uda::String* pf_active_coil_name_data_s = dynamic_cast<const uda::String*>(pf_active_coil_name_data);
    std::cout << "coil/1/name: " << pf_active_coil_name_data_s->str() << "\n";

    const uda::Result& pf_active_coil_name2 = client.get("imas::get(idx=0, group='pf_active', variable='coil/2/name', expName='WEST', type=string, rank=1, shot=" SHOT_NUM ", )", "");
    const uda::Data * pf_active_coil_name_data2 = pf_active_coil_name2.data();
    const uda::String* pf_active_coil_name_data_s2 = dynamic_cast<const uda::String*>(pf_active_coil_name_data2);
    std::cout << "coil/2/name: " << pf_active_coil_name_data_s2->str() << "\n";

    const uda::Result& pf_active_coil_name17 = client.get("imas::get(idx=0, group='pf_active', variable='coil/17/name', expName='WEST', type=string, rank=1, shot=" SHOT_NUM ", )", "");
    const uda::Data * pf_active_coil_name_data17 = pf_active_coil_name17.data();
    const uda::String* pf_active_coil_name_data_s17 = dynamic_cast<const uda::String*>(pf_active_coil_name_data17);
    std::cout << "coil/17/name: " << pf_active_coil_name_data_s17->str() << "\n";

    const uda::Result& pf_active_element_coil_name = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/element/1/name', expName='WEST', type=string, rank=1, shot=" SHOT_NUM ", )", "");
    const uda::Data * pf_active_element_coil_name_data = pf_active_element_coil_name.data();
    const uda::String* pf_active_element_coil_name_data_s = dynamic_cast<const uda::String*>(pf_active_element_coil_name_data);
    std::cout << "coil/1/element/1/name: " << pf_active_element_coil_name_data_s->str() << "\n";

    const uda::Result& pf_active_element_coil_name21 = client.get("imas::get(idx=0, group='pf_active', variable='coil/2/element/1/name', expName='WEST', type=string, rank=1, shot=" SHOT_NUM ", )", "");
    const uda::Data * pf_active_element_coil_name_data21 = pf_active_element_coil_name21.data();
    const uda::String* pf_active_element_coil_name_data_s21 = dynamic_cast<const uda::String*>(pf_active_element_coil_name_data21);
    std::cout << "coil/2/element/1/name: " << pf_active_element_coil_name_data_s21->str() << "\n";

    const uda::Result& pf_active_coil_id1 = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/identifier', expName='WEST', type=string, rank=1, shot=" SHOT_NUM ", )", "");
    const uda::Data * pf_active_coil_id1_data = pf_active_coil_id1.data();
    const uda::String* pf_active_coil_id1_data_s = dynamic_cast<const uda::String*>(pf_active_coil_id1_data);
    std::cout << "coil/1/identifier: " << pf_active_coil_id1_data_s->str() << "\n";

    const uda::Result& pf_active_coil_id2 = client.get("imas::get(idx=0, group='pf_active', variable='coil/2/identifier', expName='WEST', type=string, rank=1, shot=" SHOT_NUM ", )", "");
    const uda::Data * pf_active_coil_id2_data = pf_active_coil_id2.data();
    const uda::String* pf_active_coil_id2_data_s = dynamic_cast<const uda::String*>(pf_active_coil_id2_data);
    std::cout << "coil/2/identifier: " << pf_active_coil_id2_data_s->str() << "\n";

    const uda::Result& pf_active_element_coil_id21 = client.get("imas::get(idx=0, group='pf_active', variable='coil/2/element/1/identifier', expName='WEST', type=string, rank=1, shot=" SHOT_NUM ", )", "");
    const uda::Data * pf_active_element_coil_id21_data = pf_active_element_coil_id21.data();
    const uda::String* pf_active_element_coil_id21_data_s = dynamic_cast<const uda::String*>(pf_active_element_coil_id21_data);
    std::cout << "coil/2/element/1/identifier: " << pf_active_element_coil_id21_data_s->str() << "\n";

    const uda::Result& pf_active_turns1 = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/element/1/turns_with_sign', expName='WEST', type=int, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Scalar* scalar_pf_active_turns1 = dynamic_cast<const uda::Scalar*>(pf_active_turns1.data());
    std::cout << "coil/1/element/1/turns_with_sign: " << scalar_pf_active_turns1->as<int>() << std::endl;

    return 0;
}

