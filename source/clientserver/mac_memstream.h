#ifndef IDAM_CLIENTSERVER_MAC_MEMSTREAM_H
#define IDAM_CLIENTSERVER_MAC_MEMSTREAM_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __APPLE__

#include <stdio.h>

FILE * open_memstream(char **cp, size_t *lenp);

#endif

#ifdef __cplusplus
}
#endif

#endif //IDAM_CLIENTSERVER_MAC_MEMSTREAM_H
