#if 0
#!/bin/bash
g++ test0.cpp -g -O0 -gdwarf-3 -o test $(pkg-config --cflags uda-cpp) $(pkg-config --libs uda-cpp)
exit 0
#endif

#include <typeinfo>
#include <iostream>
#include <fstream>

#include <UDA.hpp>

int main() {
    uda::Client client;

    const uda::Result& result = client.get("ayc_te", "30420");
    
    uda::Data* data = result.data();

    std::cout << "size = " << data->size() << std::endl;
    std::cout << "type = " << data->type().name() << std::endl;

    uda::Array* arr = dynamic_cast< uda::Array*>(data);
    
    std::vector<float> values = arr->as<float>();
    std::cout << "value(0) = " << values.at(0) << "\n"; 

    bool hasErrors = result.hasErrors();

    std::cout << "hasErrors = " << hasErrors << std::endl;

    if (hasErrors) {

        uda::Data* errors = result.errors();

        std::cout << "size = " << errors->size() << std::endl;
        std::cout << "type = " << errors->type().name() << std::endl;

        uda::Array* errArr = dynamic_cast< uda::Array*>(errors);
    
        std::vector<float> errVals = errArr->as<float>();
        std::cout << "value(0) = " << errVals.at(0) << "\n"; 
    }

    return 0;
}

