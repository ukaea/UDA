#ifndef UDA_SERVER_MODULES_READSQL_READSQL_H
#define UDA_SERVER_MODULES_READSQL_READSQL_H

#include <clientserver/udaStructs.h>
#include <server/sqllib.h>
#include <structures/genStructs.h>

#ifdef USEREADSOAP 
#  include "soapStub.h"
int readSOAP(char *filename, struct _ns1__device *device);
void freeSOAP();
#endif

int readSQL(PGconn *DBConnect, REQUEST_BLOCK request_block, DATA_SOURCE data_source, DATA_BLOCK *data_block, USERDEFINEDTYPELIST* userdefinedtypelist);
int readCMDSQL(PGconn *DBConnect, REQUEST_BLOCK request_block, DATA_SOURCE data_source, DATA_BLOCK *data_block, USERDEFINEDTYPELIST* userdefinedtypelist);

#ifndef NOTGENERICENABLED

#  ifdef TESTCODEX
extern PGconn *gDBConnect;
#  endif

#  ifdef USEREADSOAP
extern int enable_malloc_log;
#  endif    

#endif

#endif // UDA_SERVER_MODULES_READSQL_READSQL_H

