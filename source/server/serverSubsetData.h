#pragma once

#include "clientserver/parseXML.h"
#include "clientserver/udaStructs.h"
#include "serverPlugin.h"
#include "structures/genStructs.h"

int serverSubsetData(uda::client_server::DATA_BLOCK* data_block, const uda::client_server::ACTION& action,
                     LOGMALLOCLIST* logmalloclist);
int serverParseServerSide(uda::client_server::REQUEST_DATA* request_block,
                          uda::client_server::ACTIONS* actions_serverside, const PLUGINLIST* plugin_list);
