#ifndef UDA_SERVER_SLEEPSERVER_H
#define UDA_SERVER_SLEEPSERVER_H

#include <structures/genStructs.h>
#include <clientserver/export.h>
#include <rpc/rpc.h>

int sleepServer(XDR* server_input, XDR* server_output, LOGMALLOCLIST* logmalloclist,
                USERDEFINEDTYPELIST* userdefinedtypelist, int protocolVersion, NTREE* full_ntree,
                LOGSTRUCTLIST* log_struct_list);

#endif // UDA_SERVER_SLEEPSERVER_H
