#pragma once

#ifndef UDA_SERVER_UDASERVER_H
#define UDA_SERVER_UDASERVER_H

#include "clientserver/udaStructs.h"

#ifdef __cplusplus
extern "C" {
#endif

int udaServer(CLIENT_BLOCK client_block);

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_UDASERVER_H