/*---------------------------------------------------------------
* IDAM Data Server: Program stub that call the shared library API
*--------------------------------------------------------------*/

#include <cstdlib>
#if defined(__GNUC__)
#  include <unistd.h>
#else
#  include <windows.h>
#  define sleep Sleep
#endif

#include "udaServer.h"

int main(int argc, char ** argv)
{
    // Optional sleep at startup

    char * env = getenv("UDA_SLEEP");
    if (env != nullptr) {
        sleep((unsigned int)atoi(env));
    }

    // Run server

    CLIENT_BLOCK client_block = {0};

    int rc = udaServer(client_block);

    return rc;
}
