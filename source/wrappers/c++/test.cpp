#include "UDA.hpp"

int main()
{

    uda::Client::setServerHostName("idam0");
    uda::Client::setServerPort(56561);

    uda::Client client;

    const uda::Result& result = client.get("ip", "13500");

    uda::Dim dim = result.dim(0);
    std::vector<float> dim_data = dim.as<float>();

    std::cout << dim.type().name() << std::endl;
    std::cout << dim_data[0] << std::endl;

    uda::Array* data = dynamic_cast<uda::Array*>(result.data());

    std::cout << data->isNull() << std::endl;

    return 0;
}
