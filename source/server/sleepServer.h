#ifndef UDA_SERVER_SLEEPSERVER_H
#define UDA_SERVER_SLEEPSERVER_H

#include <structures/genStructs.h>
#include <clientserver/export.h>
#include <rpc/rpc.h>
#include "writer.h"

int sleepServer(XDR* server_input, XDR* server_output, LOGMALLOCLIST* logmalloclist,
                USERDEFINEDTYPELIST* userdefinedtypelist, int protocolVersion, NTREE* full_ntree,
                LOGSTRUCTLIST* log_struct_list, int server_tot_block_time, int server_timeout, IoData* io_data);

#endif // UDA_SERVER_SLEEPSERVER_H
