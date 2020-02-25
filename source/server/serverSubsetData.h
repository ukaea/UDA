#ifndef UDA_SERVER_SERVERSUBSETDATA_H
#define UDA_SERVER_SERVERSUBSETDATA_H

#include <clientserver/parseXML.h>
#include <clientserver/udaStructs.h>
#include <structures/genStructs.h>

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int idamserverSubsetData(DATA_BLOCK *data_block, ACTION action, LOGMALLOCLIST* logmalloclist);
LIBRARY_API int idamserversubsetindices(char *operation, DIMS *dim, double value, unsigned int *subsetindices);
LIBRARY_API int idamserverParseServerSide(REQUEST_BLOCK *request_block, ACTIONS *actions_serverside);
LIBRARY_API int idamserverNewDataArray2(DIMS *dims, int rank, int dimid,
                            char *data, int ndata, int data_type, int notoperation, int reverse,
                            int start, int end, int start1, int end1, int *n, void **newdata);

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_SERVERSUBSETDATA_H
