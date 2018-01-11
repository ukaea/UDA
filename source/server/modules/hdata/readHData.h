#ifndef IDAM_READHDATA_H
#define IDAM_READHDATA_H

#ifdef NOTGENERICENABLED
typedef int PGconn;
#else
#include <libpq-fe.h>
#endif

#include <clientserver/udaStructs.h>

int readHData(PGconn *DBConnect, REQUEST_BLOCK request_block, DATA_SOURCE data_source,
	      DATA_BLOCK *data_block);

#endif // IDAM_READHDATA_H

