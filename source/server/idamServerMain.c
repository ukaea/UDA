/*---------------------------------------------------------------
* IDAM Data Server: Program stub that call the shared library API
*--------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern int idamServer(int argc, char ** argv);

int main(int argc, char ** argv)
{
    // Optional sleep at startup

    char * env = getenv("IDAM_SLEEP");
    if (env != NULL) sleep(atoi(env));

    // Run server

    int rc = idamServer(argc, argv);

    return rc;
}
