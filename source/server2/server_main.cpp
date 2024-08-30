#include <cstdlib>
#if defined(__GNUC__)
#  include <unistd.h>
#else
#  define NOMINMAX
#  include <windows.h>
#  define sleep Sleep
#endif

#include "server.hpp"
#include "server_exceptions.h"

int main()
{
    // Optional sleep at startup

    char * env = getenv("UDA_SLEEP");
    if (env != nullptr) {
        sleep((unsigned int)atoi(env));
    }

    // Run server

    try {
        uda::Server server;
//        server.run();
    } catch (uda::server::Exception& ex) {
        return ex.code();
    }

    return 0;
}
