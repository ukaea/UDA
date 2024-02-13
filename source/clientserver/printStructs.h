#pragma once

#include "udaStructs.h"

namespace uda::client_server
{

void printRequestData(REQUEST_DATA str);

void printRequestBlock(REQUEST_BLOCK str);

void printClientBlock(CLIENT_BLOCK str);

void printServerBlock(SERVER_BLOCK str);

void printDataBlockList(DATA_BLOCK_LIST str);

void printDataBlock(DATA_BLOCK str);

void printSystemConfig(SYSTEM_CONFIG str);

void printDataSystem(DATA_SYSTEM str);

void printDataSource(DATA_SOURCE str);

void printSignal(SIGNAL str);

void printSignalDesc(SIGNAL_DESC str);

} // namespace uda::client_server
