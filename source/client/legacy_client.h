#ifndef LEGACY_CLIENT_H
#define LEGACY_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API inline void idamFree(int handle)
{
    udaFree(handle);
}

LIBRARY_API inline void idamFreeAll()
{
    udaFreeAll();
}

#ifdef __cplusplus
}
#endif


#endif
