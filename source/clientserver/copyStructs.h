#ifndef UDA_CLIENTSERVER_COPYSTRUCTS_H
#define UDA_CLIENTSERVER_COPYSTRUCTS_H

#include "udaStructs.h"

void copyRequestData(REQUEST_DATA* out, REQUEST_DATA in);
void copyRequestBlock(REQUEST_BLOCK* out, REQUEST_BLOCK in);

#endif // UDA_CLIENTSERVER_COPYSTRUCTS_H
