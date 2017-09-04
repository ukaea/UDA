#ifndef IDAM_READSQL_H
#define IDAM_READSQL_H

#include <libpq-fe.h>

#include <clientserver/udaStructs.h>
#include <structures/genStructs.h>

#ifdef USEREADSOAP 
#  include "soapStub.h"
int readSOAP(char *filename, struct _ns1__device *device);
void freeSOAP();
#endif

int readSQL(PGconn *DBConnect, REQUEST_BLOCK request_block, DATA_SOURCE data_source, DATA_BLOCK *data_block, USERDEFINEDTYPELIST* userdefinedtypelist);

#ifndef NOTGENERICENABLED

#  ifdef TESTCODEX
extern PGconn *gDBConnect;
#  endif

#  ifdef USEREADSOAP
extern int enable_malloc_log;
#  endif    

#endif

#endif // IDAM_READSQL_H

