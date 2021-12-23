#pragma once

#ifndef UDA_SERVER_SLEEPSERVER_H
#define UDA_SERVER_SLEEPSERVER_H

#include <rpc/rpc.h>

#include <structures/genStructs.h>
#include <clientserver/export.h>

struct IoData;

int sleepServer(XDR* server_input, XDR* server_output, LOGMALLOCLIST* logmalloclist,
                USERDEFINEDTYPELIST* userdefinedtypelist, int protocolVersion, LOGSTRUCTLIST* log_struct_list,
                int server_tot_block_time, int server_timeout, IoData* io_data, int private_flags, int malloc_source);

#endif // UDA_SERVER_SLEEPSERVER_H
