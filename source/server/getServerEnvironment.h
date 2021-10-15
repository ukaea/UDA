#pragma once

#ifndef UDA_SERVER_GETSERVERENVIRONMENT_H
#define UDA_SERVER_GETSERVERENVIRONMENT_H

#include <clientserver/udaStructs.h>
#include <clientserver/export.h>

void printServerEnvironment(const ENVIRONMENT* environment);
ENVIRONMENT* getServerEnvironment();

#endif // UDA_SERVER_GETSERVERENVIRONMENT_H
