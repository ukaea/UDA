#pragma once

#ifndef UDA_SERVER_GETSERVERENVIRONMENT_H
#  define UDA_SERVER_GETSERVERENVIRONMENT_H

#  include "export.h"
#  include "clientserver/udaStructs.h"

void printServerEnvironment(const ENVIRONMENT* environment);
ENVIRONMENT* getServerEnvironment();

#endif // UDA_SERVER_GETSERVERENVIRONMENT_H
