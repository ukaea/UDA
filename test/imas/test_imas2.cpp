#if 0
#!/bin/bash
g++ test_imas2.cpp -g -O0 -gdwarf-3 -o test -DHOME=/Imas_public -I/Imas_public/uda/source -I/Imas_public/uda/source/wrappers \
-L/Imas_public/uda/lib -Wl,-rpath,/Imas_public/uda/lib  -luda_cpp -lssl -lcrypto -lxml2
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

    /*const uda::Result& hom = client.get("imas::get(idx=0, group='magnetics', variable='ids_properties/homogeneous_time', expName='WEST', type=int, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Scalar* v_hom = dynamic_cast<const uda::Scalar*>(hom.data());
    std::cout << "ids_properties/homogeneous_time : " << v_hom->as<int>() << std::endl;*/

    const uda::Result& tor_angle = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/toroidal_angle', expName='WEST', type=float, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Scalar* v_tor_angle = dynamic_cast<const uda::Scalar*>(tor_angle.data());
    std::cout << "bpol_probe/1/toroidal_angle: " << v_tor_angle->as<float>() << std::endl;

    const uda::Result& tor_angle2 = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/2/toroidal_angle', expName='WEST', type=float, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Scalar* v_tor_angle2 = dynamic_cast<const uda::Scalar*>(tor_angle2.data());
    std::cout << "bpol_probe/2/toroidal_angle: " << v_tor_angle2->as<float>() << std::endl;

    /*const uda::Result& tor_angle2 = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/56/toroidal_angle', expName='WEST', type=double, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Scalar* v_tor_angle2 = dynamic_cast<const uda::Scalar*>(tor_angle2.data());
    std::cout << "bpol_probe/56/toroidal_angle: " << v_tor_angle2->as<double>() << std::endl;*/

    const uda::Result& pol_angle = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/poloidal_angle', expName='WEST', type=double, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Scalar* v_pol_angle = dynamic_cast<const uda::Scalar*>(pol_angle.data());
    std::cout << "bpol_probe/1/pol_angle: " << v_pol_angle->as<double>() << std::endl;


    const uda::Result& shape_of = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/Shape_of', expName='WEST', type=int, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Scalar* v_shape_of = dynamic_cast<const uda::Scalar*>(shape_of.data());
    std::cout << "flux_loop/Shape_of: " << v_shape_of->as<int>() << std::endl;

    const uda::Result& size2 = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/position/Shape_of', expName='WEST', type=int, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Data * data11 = size2.data();
    const uda::Scalar* scalar11 = dynamic_cast<const uda::Scalar*>(data11);

    int sz11 = scalar11->as<int>();

    std::cout << "flux_loop/1/position/Shape_of: " << sz11 << std::endl;

    const uda::Result& posR = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/position/1/r', expName='WEST', type=double, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Data * dataR = posR.data();
    const uda::Scalar* arDataR = dynamic_cast<const uda::Scalar*>(dataR);
    std::cout << "flux_loop/1/position/1/r: " << arDataR->as<double>() << std::endl;

    const uda::Result& posR2 = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/2/position/1/r', expName='WEST', type=double, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Scalar* scalar2 = dynamic_cast<const uda::Scalar*>(posR2.data());
    std::cout << "flux_loop/2/position/1/r: " << scalar2->as<double>() << std::endl;

    /*const uda::Result& phi = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/2/position/1/phi', expName='WEST', type=double, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Scalar* scalar1 = dynamic_cast<const uda::Scalar*>(phi.data());
    std::cout << "flux_loop/2/position/1/phi: " << scalar1->as<double>() << std::endl;*/

    /*const uda::Result& sig1 = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/flux', expName='TORE', type=double, rank=1, shot=" SHOT_NUM_TORE_SUPRA ", )", "");
    const uda::Data * data1 = sig1.data();
    const uda::Array* arr1 = dynamic_cast<const uda::Array*>(data1);

    const uda::Dim& time = arr1->dims().at(0);

    std::cout << "time: ";
    for (int j = 0; j < 10; ++j) {
        std::cout << time.at<double>(j) << " ";
    }
    std::cout << "..." << std::endl;

    std::cout << "data : ";
    for (int j = 0; j < 10; ++j) {
        std::cout << arr1->as<double>().at(j) << " ";
    }
    std::cout << "..." << std::endl;*/

    const uda::Result& name = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/name', expName='WEST', type=string, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Data * data4 = name.data();
    const uda::String* s = dynamic_cast<const uda::String*>(data4);

    std::cout << "flux_loop/1/name : ";
    std::cout << s->str();
    std::cout << "\n" << std::endl;

    const uda::Result& sigts = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/flux/data', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
    const uda::Data * datats = sigts.data();
    const uda::Array* arrts = dynamic_cast<const uda::Array*>(datats);

    std::cout << "first values for flux_loop/1/flux/data : ";
    for (int j = 0; j < 10; ++j) {
        std::cout << arrts->as<double>().at(j) << " ";
    }
    std::cout << "..." << std::endl;

    const uda::Result& sigtime = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/flux/time', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
    const uda::Data * datatime = sigtime.data();
    const uda::Array* arrtime = dynamic_cast<const uda::Array*>(datatime);

    std::cout << "first values for flux_loop/1/flux/time : ";
    for (int j = 0; j < 10; ++j) {
        std::cout << arrtime->as<double>().at(j) << " ";
    }
    std::cout << "..." << std::endl;

    const uda::Result& sigts2 = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/2/flux/data', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
    const uda::Data * datats2 = sigts2.data();
    const uda::Array* arrts2 = dynamic_cast<const uda::Array*>(datats2);

    std::cout << "first values for flux_loop/2/flux/data : ";
    for (int j = 0; j < 10; ++j) {
        std::cout << arrts2->as<double>().at(j) << " ";
    }
    std::cout << "..." << std::endl;

    const uda::Result& sig2 = client.get("imas::get(idx=0, group='tf', variable='b_field_tor_vacuum_r/data', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
    const uda::Data * data2 = sig2.data();
    const uda::Array* arr2 = dynamic_cast<const uda::Array*>(data2);

    std::cout << "first values for b_field_tor_vacuum_r/data : ";
    for (int j = 0; j < 10; ++j) {
        std::cout << arr2->as<double>().at(j) << " ";
    }
    std::cout << "..." << std::endl;

    /*const uda::Result& pfactive = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/current', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
    const uda::Data * data_pfactive = pfactive.data();
    const uda::Array* arr_data_pfactive = dynamic_cast<const uda::Array*>(data_pfactive);

    std::cout << "first values for pf_active/coil/1/current/data : ";
    for (int j = 0; j < 10; ++j) {
        std::cout << arr_data_pfactive->as<double>().at(j) << " ";
    }
    std::cout << "..." << std::endl;

    const uda::Result& pfactive2 = client.get("imas::get(idx=0, group='pf_active', variable='coil/2/current', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
    const uda::Data * data_pfactive2 = pfactive2.data();
    const uda::Array* arr_data_pfactive2 = dynamic_cast<const uda::Array*>(data_pfactive2);

    std::cout << "first values for pf_active/coil/2/current/data : ";
    for (int j = 0; j < 10; ++j) {
        std::cout << arr_data_pfactive2->as<double>().at(j) << " ";
    }
    std::cout << "..." << std::endl;*/

    const uda::Result& shof_pfactive = client.get("imas::get(idx=0, group='pf_active', variable='coil/Shape_of', expName='WEST', type=int, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Data * data_shof_pfactive = shof_pfactive.data();
    const uda::Scalar* scalar_data_shof_pfactive = dynamic_cast<const uda::Scalar*>(data_shof_pfactive);

    int shof = scalar_data_shof_pfactive->as<int>();

    std::cout << "coil/Shape_of : " << shof << std::endl;

    /*const uda::Result& shof_magnetics = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/Shape_of', expName='WEST', type=int, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Data * data_shof_magnetics = shof_magnetics.data();
    const uda::Scalar* scalar_data_shof_magnetics = dynamic_cast<const uda::Scalar*>(data_shof_magnetics);

    int shof = scalar_data_shof_magnetics->as<int>();

    std::cout << "bpol_probe/Shape_of : " << shof << std::endl;*/


    /*const uda::Result& sig3 = client.get("imas::get(idx=0, group='magnetics', variable='method/1/ip/time', expName='WEST', type=double, rank=1, shot=" SHOT_NUM_TORE_SUPRA ", )", "");
    const uda::Data * data3 = sig3.data();
    const uda::Array* arr3 = dynamic_cast<const uda::Array*>(data3);

    std::cout << "time : ";
    for (int j = 0; j < 10; ++j) {
        std::cout << arr3->as<double>().at(j) << " ";
    }
    std::cout << "..." << std::endl;*/


    const uda::Result& shof_method_magnetics = client.get("imas::get(idx=0, group='magnetics', variable='method/Shape_of', expName='WEST', type=int, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Data * data_shof_method_magnetics = shof_method_magnetics.data();
    const uda::Scalar* scalar_data_shof_method_magnetics = dynamic_cast<const uda::Scalar*>(data_shof_method_magnetics);
    int shof_method = scalar_data_shof_method_magnetics->as<int>();
    std::cout << "method/Shape_of : " << shof_method << std::endl;

    const uda::Result& ip0 = client.get("imas::get(idx=0, group='magnetics', variable='method/1/ip/data', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
    const uda::Data * data_ip0 = ip0.data();
    const uda::Array* arr_data_ip0 = dynamic_cast<const uda::Array*>(data_ip0);

    std::cout << "first values for method/1/ip : ";
    for (int j = 0; j < 10; ++j) {
        std::cout << arr_data_ip0->as<double>().at(j) << " ";
    }
    std::cout << "..." << std::endl;


    const uda::Result& dmf = client.get("imas::get(idx=0, group='magnetics', variable='method/1/diamagnetic_flux/data', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
    const uda::Data * data_dmf = dmf.data();
    const uda::Array* arr_data_dmf = dynamic_cast<const uda::Array*>(data_dmf);

    std::cout << "first values for method/1/diamagnetic_flux : ";
    for (int j = 0; j < 10; ++j) {
        std::cout << arr_data_dmf->as<double>().at(j) << " ";
    }
    std::cout << "..." << std::endl;

    const uda::Result& bpol_name = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/60/name', expName='WEST', type=string, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Data * bpol_data_name = bpol_name.data();
    const uda::String* s_bpol_data_name = dynamic_cast<const uda::String*>(bpol_data_name);

    std::cout << "bpol_probe/60/name : ";
    std::cout << s_bpol_data_name->str();
    std::cout << "\n" << std::endl;


    const uda::Result& bpol_probe_posR = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/20/position/r', expName='WEST', type=double, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Scalar* scalar_bpol_probe_posR = dynamic_cast<const uda::Scalar*>(bpol_probe_posR.data());
    std::cout << "bpol_probe/20/position/r: " << scalar_bpol_probe_posR->as<double>() << std::endl;


    const uda::Result& validity = client.get("imas::get(idx=0, group='interfero_polarimeter', variable='channel/1/validity', expName='WEST', type=int, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Scalar* v_validity = dynamic_cast<const uda::Scalar*>(validity.data());
    std::cout << "channel/1/validity: " << v_validity->as<int>() << std::endl;

    const uda::Result& validity2 = client.get("imas::get(idx=0, group='interfero_polarimeter', variable='channel/2/validity', expName='WEST', type=int, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Scalar* v_validity2 = dynamic_cast<const uda::Scalar*>(validity2.data());
    std::cout << "channel/2/validity: " << v_validity2->as<int>() << std::endl;

    const uda::Result& validity3 = client.get("imas::get(idx=0, group='interfero_polarimeter', variable='channel/3/validity', expName='WEST', type=int, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Scalar* v_validity3 = dynamic_cast<const uda::Scalar*>(validity3.data());
    std::cout << "channel/3/validity: " << v_validity3->as<int>() << std::endl;

    /*
    const uda::Result& mag = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/field/data', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
    const uda::Data * data_mag = mag.data();
    const uda::Array* arr_data_mag = dynamic_cast<const uda::Array*>(data_mag);

    std::cout << "first values for bpol_probe/1/field : ";
    for (int j = 0; j < 10; ++j) {
        std::cout << arr_data_mag->as<double>().at(j) << " ";
    }
    std::cout << "..." << std::endl;

    const uda::Result& mag2 = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/2/field/data', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
    const uda::Data * data_mag2 = mag2.data();
    const uda::Array* arr_data_mag2 = dynamic_cast<const uda::Array*>(data_mag2);

    std::cout << "first values for bpol_probe/2/field : ";
    for (int j = 0; j < 10; ++j) {
        std::cout << arr_data_mag2->as<double>().at(j) << " ";
    }
    std::cout << "..." << std::endl;

    const uda::Result& mag55 = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/55/field/data', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
    const uda::Data * data_mag55 = mag55.data();
    const uda::Array* arr_data_mag55 = dynamic_cast<const uda::Array*>(data_mag55);

    std::cout << "first values for bpol_probe/55/field : ";
    for (int j = 0; j < 10; ++j) {
        std::cout << arr_data_mag55->as<double>().at(j) << " ";
    }
    std::cout << "..." << std::endl;

    const uda::Result& mag56 = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/56/field/data', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
    const uda::Data * data_mag56 = mag56.data();
    const uda::Array* arr_data_mag56 = dynamic_cast<const uda::Array*>(data_mag56);

    std::cout << "first values for bpol_probe/56/field : ";
    for (int j = 0; j < 10; ++j) {
        std::cout << arr_data_mag56->as<double>().at(j) << " ";
    }
    std::cout << "..." << std::endl;

    const uda::Result& mag110 = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/110/field/data', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
    const uda::Data * data_mag110 = mag110.data();
    const uda::Array* arr_data_mag110 = dynamic_cast<const uda::Array*>(data_mag110);

    std::cout << "first values for bpol_probe/110/field : ";
    for (int j = 0; j < 10; ++j) {
        std::cout << arr_data_mag110->as<double>().at(j) << " ";
    }
    std::cout << "..." << std::endl;

    const uda::Result& mag111 = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/111/field/data', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
    const uda::Data * data_mag111 = mag111.data();
    const uda::Array* arr_data_mag111 = dynamic_cast<const uda::Array*>(data_mag111);

    std::cout << "first values for bpol_probe/111/field : ";
    for (int j = 0; j < 10; ++j) {
        std::cout << arr_data_mag111->as<double>().at(j) << " ";
    }
    std::cout << "..." << std::endl;

    */


    /*const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/field/data', expName='TORE', type=double, rank=1, shot=" SHOT_NUM_TORE_SUPRA ", )", "");
    uda::Data * data = result.data();

    uda::Array * arr = dynamic_cast<uda::Array*>(data);

    uda::Dim time = arr->dims().at(0);
    std::vector<float> values = arr->as<float>();

    std::ofstream out("temp.csv");
    for (int i = 0; i < arr->as<float>().size(); ++i) {
        out << time.at<float>(i) << "," << values.at(i) << "\n";
    }*/




    return 0;
}

