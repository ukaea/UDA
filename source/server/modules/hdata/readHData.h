#ifndef IDAM_READHDATA_H
#define IDAM_READHDATA_H

#include <include/idamclientserver.h>
#include <include/idamserver.h>
#include <server/sqllib.h>

int readHData(PGconn *DBConnect, REQUEST_BLOCK request_block, DATA_SOURCE data_source,
	      DATA_BLOCK *data_block);

#endif // IDAM_READHDATA_H

