#if 0
#!/bin/bash
g++ test0.cpp -g -O0 -gdwarf-3 -o test \
    -I$HOME/itmwork/IdamInstall/include/idam -I$HOME/itmwork/IdamInstall/include/idam/c++ \
    -I$BOOST_HOME/include \
    -Wl,-rpath,$HOME/itmwork/IdamInstall/lib -L$HOME/itmwork/IdamInstall/lib \
    -Wl,-rpath,$HOME/lib -L$HOME/lib \
    -lidamcpp -lidam64
exit 0
#endif

#include "Idam.hpp"
#include <typeinfo>
#include <iostream>
#include <fstream>

int main() {
    Idam::Client::setServerHostName("localhost");
    Idam::Client::setServerPort(56565);

    Idam::Client client;

    const Idam::Result& result = client.get("TORE::read(element=/bpol_probe/Size_of, shot=43970)", "");
    
    Idam::Data * data = result.data();

    std::cout << "size = " << data->size() << std::endl;
    std::cout << "type = " << data->type().name() << std::endl;

    Idam::Array * arr = dynamic_cast< Idam::Array*>(data);
    
    std::vector<int> values = arr->as<int>();
    std::cout << "value(0) = " << values.at(0) << "\n"; 

    return 0;
}

