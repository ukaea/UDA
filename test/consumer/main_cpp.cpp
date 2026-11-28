#include <UDA.hpp>
#include <signal.hpp>
#include <client/udaClient.h>
#include <cstdio>

int main(int argc, char** argv)
{
    uda::Client client;
    if (argc == 3)
        client.get(argv[1], argv[2]);
    const char* version = getUdaBuildVersion();
    if (!version || !*version)
        return 1;
    std::puts(version);
    return 0;
}
