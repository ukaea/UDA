#include "UDA.hpp"

int main() {

    Idam::Client::setServerHostName("idam0");
    Idam::Client::setServerPort(56561);

    Idam::Client client;

    const Idam::Result& result = client.get("ip", "13500");

    Idam::Dim dim = result.dim(0);
    std::vector<float> dim_data = dim.as<float>();

    std::cout << dim.type().name() << std::endl;
    std::cout << dim_data[0] << std::endl;

    Idam::Array * data = dynamic_cast<Idam::Array *>(result.data());

    std::cout << data->isNull() << std::endl;

    return 0;
}

