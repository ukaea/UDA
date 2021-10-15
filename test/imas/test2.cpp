#if 0
#!/bin/bash
g++ test2.cpp -g -O0 -gdwarf-3 -o test \
    -I$HOME/itmwork/idam/latest/source/include -I$HOME/itmwork/idam/latest/source/wrappers/c++ \
    -I$BOOST_HOME/include \
    -Wl,-rpath,$HOME/itmwork/IdamInstall/lib -L$HOME/itmwork/IdamInstall/lib \
    -Wl,-rpath,$HOME/lib -L$HOME/lib \
    -lidamcpp -lidamfat64 \
    -Wl,-rpath,/afs/ipp/itm/switm/lib/netcdf/4.3.0/lib -L/afs/ipp/itm/switm/lib/netcdf/4.3.0/lib -lnetcdf
exit 0
#endif

#include "Idam.hpp"
#include <typeinfo>
#include <iostream>
#include <fstream>

int main() {

    //setenv("IDAM_SQLHOST", "idam1", 1);
    //setenv("IDAM_SQLPORT", "56566", 1);
    setenv("IDAM_PLUGIN_DIR", "/u/lfleury/itmwork/IdamInstall/bin/plugins", 1);
    setenv("IDAM_PLUGIN_CONFIG", "/u/lfleury/itmwork/IdamInstall/bin/plugins/idamTest.conf", 1);
    setenv("IDAM_SARRAY_CONFIG", "/u/lfleury/itmwork/IdamInstall/bin/plugins/idamgenstruct.conf", 1);

    Idam::Client::setServerHostName("rca.fusion.org.uk");
    //Idam::Client::setServerHostName("localhost");
    Idam::Client::setServerPort(56565);

    Idam::Client client;

    //const Idam::Result& result = client.get("ts::read(element=/bpol_probe/8/position/r)", "");
    const Idam::Result& result = client.get("ts::read(element=/bpol_probe/8/poloidal_angle)", "");
    
    Idam::Data * data = result.data();

    std::cout << "size = " << data->size() << std::endl;
    std::cout << "type = " << data->type().name() << std::endl;

    Idam::Array * arr = dynamic_cast< Idam::Array*>(data);
    
    std::ofstream out("temp.csv");
    //Idam::Dim time = arr->dims().at(0);
    std::vector<float> values = arr->as<float>();
    
    for (int i = 0; i < arr->as<float>().size(); ++i) {
        out << values.at(i) << "\n";
    }

    return 0;
}

