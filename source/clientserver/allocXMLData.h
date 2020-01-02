#ifndef UDA_CLIENTSERVER_ALLOCXMLDATA_H
#define UDA_CLIENTSERVER_ALLOCXMLDATA_H

#ifdef HIERARCHICAL_DATA

#include "idamclientserver.h"
#include "idamclientserverxml.h"

#ifdef __cplusplus
extern "C" {
#endif

int alloc_efit(EFIT *efit);
int alloc_pfcircuit(PFCIRCUIT *str);
int alloc_pfcoils(PFCOILS *str);
int alloc_pfpassive(PFPASSIVE *str);
int alloc_fluxloop(FLUXLOOP *str);
int alloc_limiter(LIMITER *str);

#ifdef __cplusplus
}
#endif

#endif

#endif // UDA_CLIENTSERVER_ALLOCXMLDATA_H
