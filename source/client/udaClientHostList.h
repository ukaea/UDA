#pragma once

namespace uda::client
{

void udaClientFreeHostList();

const client_server::HostData* udaClientFindHostByAlias(const char* alias);

const client_server::HostData* udaClientFindHostByName(const char* name);

void udaClientInitHostList();

} // namespace uda::client
