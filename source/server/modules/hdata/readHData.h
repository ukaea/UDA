#ifndef UDA_SERVER_MODULES_HDATA_READHDATA_H
#define UDA_SERVER_MODULES_HDATA_READHDATA_H

#include <clientserver/udaStructs.h>
#include <clientserver/sqllib.h>

int readHData(PGconn *DBConnect, REQUEST_BLOCK request_block, DATA_SOURCE data_source,
	      DATA_BLOCK *data_block);

#endif // UDA_SERVER_MODULES_HDATA_READHDATA_H

