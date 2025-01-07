---
layout: default
title: API changes
nav_order: 7
---

# API changes

This page aims to detail specific changes to the UDA client API, as well as describing how deprecations will be managed.


## Notable release history

| UDA version | Description of API changes |
|:------------|:---------------------------|
| 2.7.0 | Some accidental breaking api changes are introduced, including some name changes from idam to uda, and the introduction of a new `client_flags` argument to some functions |
| 2.7.6 | A legacy mapping header is introduced which reintroduces the syntax that was lost in 2.7.0, while keeping the option to use new updated names (such as udaFree instead of idamFree) |
| (planned) 3.0.0 | Removal of all references to legacy "idam" name in all api functions. Imposing consistent naming convention in all API functions. Legacy API mapping header to be updated to permit continued use of old names for a more forgiving deprecation of old syntax |


## The legacy api headers

Where old functions names are deprecated the old syntax may still be available in a header file mapping the old syntax to the new. This will allow codes to continue to build against newer UDA library versions until they are ready to update. 

The naming convention for these header files is that they mirror the existing headers they correspond to but with the word legacy added. For example to create a name mapping for a function which is declared in the `client.h` file, such as `udaFree`, the new header will be called `legacy_client.h`. To make use of a previous API function signature that is being replaced, client code simply needs to include the corresponding legacy header. 

The full set of existing legacy api header files will be described below. 

### legacy_client.h
This header contains the mappings for all functions from `client/client.h` The format of the file is as shown below and simply provides a mapping from the new names (`udaFree`) back to the deprecated syntax (`idamFree`). By including this header in client code, the old names are made available again.

```c++
LIBRARY_API inline void idamFree(int handle)
{
    udaFree(handle);
}

LIBRARY_API inline void idamFreeAll()
{
    udaFreeAll();
}

```

### legacy_accAPI.h
This header contains the mappings for all functions from `clent/accAPI.h` whose definitions were changed in release 2.7.0. Note that to implement this header some API functions had to be changed to pre-empt the new v3.0 syntax (such as `udaUnlockThread` instead of `unlockUdaThread` ). It's also worth being aware of the two options available: either to enable the interface using the extra `client_flags` argument or the one without. The version without `client_flags` is the default and the other behaviour can be enabled by setting a compile flag called `UDA_CLIENT_FLAGS_API`.

```c++
#ifdef UDA_CLIENT_FLAGS_API

     LIBRARY_API inline DATA_BLOCK* acc_getCurrentDataBlock(CLIENT_FLAGS* client_flags)
    {
        return udaGetCurrentDataBlock();
    }

    LIBRARY_API inline void unlockUdaThread(CLIENT_FLAGS* client_flags)
    {
        udaUnlockThread();
    }

    LIBRARY_API inline void freeIdamThread(CLIENT_FLAGS* client_flags)
    {
        udaFreeThread();
    }

#else

    LIBRARY_API inline DATA_BLOCK* acc_getCurrentDataBlock()
    {
        return udaGetCurrentDataBlock();
    }

    LIBRARY_API inline void unlockUdaThread()
    {
        udaUnlockThread();
    }

    LIBRARY_API inline void freeIdamThread()
    {
        udaFreeThread();
    }
```
