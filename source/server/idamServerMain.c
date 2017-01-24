/*---------------------------------------------------------------
* IDAM Data Server: Program stub that call the shared library API
*----------------------------------------------------------------
* Change History:

08Jul2009	dgm	Created
21May2014	dgm	Added an optional call to sleep
			Driven by the environment variable IDAM_SLEEP=value
			Purpose is to delay the program to enable a debugger to attach to the process
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
