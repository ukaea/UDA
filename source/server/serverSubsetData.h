#pragma once

#include "structures/genStructs.h"
#include "clientserver/udaStructs.h"
#include "clientserver/parseXML.h"
#include "serverPlugin.h"

int serverSubsetData(DATA_BLOCK* data_block, const ACTION& action, LOGMALLOCLIST* logmalloclist);
int serverParseServerSide(REQUEST_DATA* request_block, ACTIONS* actions_serverside,
                                      const PLUGINLIST* plugin_list);
