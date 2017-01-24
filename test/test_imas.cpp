#if 0
#!/bin/bash
g++ test_imas.cpp -g -O0 -gdwarf-3 -o test_imas \
    -I$HOME/itmwork/idam/latest/source/include -I$HOME/itmwork/idam/latest/source/wrappers/c++ \
    -I$BOOST_HOME/include \
    -Wl,-rpath,$HOME/itmwork/IdamInstall/lib -L$HOME/itmwork/IdamInstall/lib \
    -Wl,-rpath,$HOME/lib -L$HOME/lib \
    -lidamcpp -lidam64 \
    -DHOME=$HOME
exit 0
#endif

#include <Idam.hpp>
#include <typeinfo>
#include <iostream>
#include <sstream>

#define QUOTE_(X) #X
#define QUOTE(X) QUOTE_(X)
#define SHOT_NUM "43970" // TORE SUPRA
//#define SHOT_NUM "18299" // MAST

int main() {
    setenv("IDAM_LOG_MODE", "a", 1);
    setenv("IDAM_LOG_LEVEL", "DEBUG", 1);
    setenv("IDAM_DEBUG_APPEND", "a", 1);

    Idam::Client::setProperty(Idam::PROP_DEBUG, true);
    Idam::Client::setProperty(Idam::PROP_VERBOSE, true);

    Idam::Client::setServerHostName("localhost");
    Idam::Client::setServerPort(56565);

    Idam::Client client;

    client.get("imas::open(file='ids', shot=" SHOT_NUM ", run=1)", "");

    const Idam::Result& size = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/Shape_of', type=int, rank=0, shot=" SHOT_NUM ", )", "");
    const Idam::Data * data = size.data();
    const Idam::Scalar* scalar = dynamic_cast<const Idam::Scalar*>(data);

    int sz = scalar->as<int>();

    const Idam::Result& pos = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/position/1/r', type=double, rank=0, shot=" SHOT_NUM ", )", "");
    scalar = dynamic_cast<const Idam::Scalar*>(pos.data());
    std::cout << "flux_loop/1/position/1/r: " << scalar->as<double>() << std::endl;

    for (int i = 1; i < sz+1; ++i) {
        std::stringstream ss;
        ss << "imas::get(idx=0, group='magnetics', variable='bpol_probe/" << i << "/name', type=string, rank=0, shot=" SHOT_NUM ")";
        const Idam::Result& name = client.get(ss.str(), "");

        data = name.data();
        const Idam::String* str = dynamic_cast<const Idam::String*>(data);
        std::cout << str->str() << std::endl;

        ss.clear();
        ss.str("");
        ss << "imas::get(idx=0, group='magnetics', variable='bpol_probe/" << i << "/field/time', type=double, rank=1, shot=" SHOT_NUM ")";
        const Idam::Result& field = client.get(ss.str(), "");

        data = field.data();
        const Idam::Array* arr = dynamic_cast<const Idam::Array*>(data);

        const Idam::Dim& time = arr->dims().at(0);

        std::cout << "time: ";
        for (int j = 0; j < 10; ++j) {
            std::cout << time.at<unsigned int>(j) << " ";
        }
        std::cout << "..." << std::endl;

        std::cout << "data: ";
        for (int j = 0; j < 10; ++j) {
            std::cout << arr->as<double>().at(j) << " ";
        }
        std::cout << "..." << std::endl;
    }

    return 0;
}

