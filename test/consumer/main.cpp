#include <client/udaClient.h>
#include <client/udaGetAPI.h>
#include <cstdio>

int main(int argc, char** argv)
{
    // Keep a reference to the request API to exercise the archive's dependency
    // closure. The normal smoke test makes no network request.
    if (argc == 3)
        return idamGetAPI(argv[1], argv[2]);

    const char* version = getUdaBuildVersion();
    if (!version || !*version)
        return 1;
    std::puts(version);
    return 0;
}
