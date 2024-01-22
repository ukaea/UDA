#pragma once

#ifndef UDA_CLIENT_HOSTLIST_H
#define UDA_CLIENT_HOSTLIST_H

#include "export.h"
#include <clientserver/socketStructs.h>
#include <string>

void udaClientFreeHostList();
const HostData* udaClientFindHostByAlias(const char* alias);
const HostData* udaClientFindHostByName(const char* name);
void udaClientInitHostList();

#endif // UDA_CLIENT_HOSTLIST_H
