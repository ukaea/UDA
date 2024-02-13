#pragma once

#ifndef UDA_CLIENT_HOSTLIST_H
#  define UDA_CLIENT_HOSTLIST_H

#  include "clientserver/socketStructs.h"
#  include <string>

void udaClientFreeHostList();
const uda::client_server::HostData* udaClientFindHostByAlias(const char* alias);
const uda::client_server::HostData* udaClientFindHostByName(const char* name);
void udaClientInitHostList();

#endif // UDA_CLIENT_HOSTLIST_H
