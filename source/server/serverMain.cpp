/*---------------------------------------------------------------
* IDAM Data Server: Program stub that call the shared library API
*--------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "udaServer.h"

int main(int argc, char ** argv)
{
    // Optional sleep at startup

    char * env = getenv("UDA_SLEEP");
    if (env != NULL) {
        sleep((unsigned int)atoi(env));
    }

    // Run server

    CLIENT_BLOCK client_block = {0};

    int rc = udaServer(client_block);

    return rc;
}
