#pragma once

#include "clientserver/socketStructs.h"
#include <string>

namespace uda::client {

void udaClientFreeHostList();

const uda::client_server::HostData *udaClientFindHostByAlias(const char *alias);

const uda::client_server::HostData *udaClientFindHostByName(const char *name);

void udaClientInitHostList();

}
