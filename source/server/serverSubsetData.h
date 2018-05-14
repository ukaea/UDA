#ifndef IDAM_SERVER_IDAMSERVERSUBSETDATA_H
#define IDAM_SERVER_IDAMSERVERSUBSETDATA_H

#include <clientserver/parseXML.h>
#include <clientserver/udaStructs.h>
#include <structures/genStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

int idamserverSubsetData(DATA_BLOCK *data_block, ACTION action, LOGMALLOCLIST* logmalloclist);
int idamserversubsetindices(char *operation, DIMS *dim, double value, unsigned int *subsetindices);
int idamserverParseServerSide(REQUEST_BLOCK *request_block, ACTIONS *actions_serverside);
int idamserverNewDataArray2(DIMS *dims, int rank, int dimid,
                            char *data, int ndata, int data_type, int notoperation, int reverse,
                            int start, int end, int start1, int end1, int *n, void **newdata);

#ifdef __cplusplus
}
#endif

#endif // IDAM_SERVER_IDAMSERVERSUBSETDATA_H
