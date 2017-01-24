
#ifndef IDAM_READHDATA_H
#define IDAM_READHDATA_H

#include "idamclientserver.h"
#include "idamserver.h"
#include "sqllib.h"

int readHData(PGconn *DBConnect, REQUEST_BLOCK request_block, DATA_SOURCE data_source,
	      DATA_BLOCK *data_block);

#endif // IDAM_READHDATA_H

