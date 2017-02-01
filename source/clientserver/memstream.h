#ifndef IDAM_CLIENTSERVER_MAC_MEMSTREAM_H
#define IDAM_CLIENTSERVER_MAC_MEMSTREAM_H

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

FILE * open_memstream(char **cp, size_t *lenp);

#ifdef __cplusplus
}
#endif

#endif //IDAM_CLIENTSERVER_MAC_MEMSTREAM_H
