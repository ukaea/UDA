#pragma once

#ifndef UDA_SERVER_GETSERVERENVIRONMENT_H
#  define UDA_SERVER_GETSERVERENVIRONMENT_H

#  include "clientserver/udaStructs.h"

void printServerEnvironment(const uda::client_server::Environment* environment);
uda::client_server::Environment* getServerEnvironment();

#endif // UDA_SERVER_GETSERVERENVIRONMENT_H
