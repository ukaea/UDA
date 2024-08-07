#include "legacy_client.h"

void idamFree(int handle)
{
    udaFree(handle);
}

void idamFreeAll()
{
    udaFreeAll();
}

