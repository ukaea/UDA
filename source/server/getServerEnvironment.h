#pragma once

#ifndef UDA_SERVER_GETSERVERENVIRONMENT_H
#define UDA_SERVER_GETSERVERENVIRONMENT_H

#include "udaStructs.h"
#include "export.h"

void printServerEnvironment(const ENVIRONMENT* environment);
ENVIRONMENT* getServerEnvironment();

#endif // UDA_SERVER_GETSERVERENVIRONMENT_H
