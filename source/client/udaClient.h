#pragma once

#ifndef UDA_CLIENT_UDACLIENT_H
#define UDA_CLIENT_UDACLIENT_H

#include "genStructs.h"
#include "udaStructs.h"
#include "client.h"

int idamClient(REQUEST_BLOCK* request_block, int* indices);

void updateClientBlock(CLIENT_BLOCK* str, const CLIENT_FLAGS* client_flags, unsigned int private_flags);

void setUserDefinedTypeList(USERDEFINEDTYPELIST* userdefinedtypelist);

void setLogMallocList(LOGMALLOCLIST* logmalloclist_in);

#endif // UDA_CLIENT_UDACLIENT_H