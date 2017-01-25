#ifndef IDAM_CLIENTSERVER_PROTOCOLXML2_H
#define IDAM_CLIENTSERVER_PROTOCOLXML2_H

#ifdef GENERALSTRUCTS
#  include "idamgenstruct.h"
#  include "idamclientserver.h"
#  ifdef __APPLE__
#    include <sys/types.h>
#    include <sys/socket.h>
#    include <sys/uio.h>
#  else
#    include <sys/sendfile.h>
#  endif
#endif

#include <stdio.h>
#include <rpc/rpc.h>

#ifdef FATCLIENT
#  define protocolXML2 protocolXML2Fat
#endif

int protocolXML2(XDR * xdrs, int protocol_id, int direction, int * token, void * str);

#endif // IDAM_CLIENTSERVER_PROTOCOLXML2_H

