#ifndef IDAM_IDAMCLIENT_H
#define IDAM_IDAMCLIENT_H

#include <include/idamgenstructpublic.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int initEnvironment;
extern int altRank;

extern LOGMALLOCLIST * logmalloclist;
extern int malloc_source;

extern unsigned int lastMallocIndex;
extern unsigned int * lastMallocIndexValue;

#ifdef __cplusplus
}
#endif

#endif // IDAM_IDAMCLIENT_H
