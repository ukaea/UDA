/*
 * Minimal C consumer of the installed UDA client library.
 *
 * It must compile, link and run using only the flags published by
 * pkg-config for uda-client - nothing here may rely on the UDA build tree
 * or on dependencies the installed .pc file does not declare.
 *
 * No server connection is made, so this is safe to run anywhere.
 */

#include <uda.h>

#include <stdio.h>

int main(void)
{
    const char* version = getUdaBuildVersion();
    const char* host = getIdamServerHost();

    printf("uda client build version: %s\n", version ? version : "(unknown)");
    printf("default server host: %s\n", host ? host : "(unset)");

    idamFreeAll();

    return 0;
}
