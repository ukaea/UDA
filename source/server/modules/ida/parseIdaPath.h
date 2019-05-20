#ifndef UDA_PARSEIDAPATH_H
#define UDA_PARSEIDAPATH_H

#include <clientserver/udaStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

void parseIDAPath(REQUEST_BLOCK* request_block);
void parseXMLPath(REQUEST_BLOCK* request_block);

#ifdef __cplusplus
}
#endif

#endif // UDA_PARSEIDAPATH_H

