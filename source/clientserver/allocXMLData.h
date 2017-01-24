
#ifndef IDAM_ALLOCXMLDATA_H
#define IDAM_ALLOCXMLDATA_H

#ifdef HIERARCHICAL_DATA

#include "idamclientserver.h"
#include "idamclientserverxml.h"

int alloc_efit(EFIT *efit);
int alloc_pfcircuit(PFCIRCUIT *str);
int alloc_pfcoils(PFCOILS *str);
int alloc_pfpassive(PFPASSIVE *str);
int alloc_fluxloop(FLUXLOOP *str);
int alloc_limiter(LIMITER *str);

#endif

#endif // IDAM_ALLOCXMLDATA_H
