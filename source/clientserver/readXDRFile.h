#pragma once

#ifndef UDA_CLIENTSERVER_READXDRFILE_H
#  define UDA_CLIENTSERVER_READXDRFILE_H

#  include "export.h"
#  include <rpc/types.h>
#  include <rpc/xdr.h>

#  define MAXDOLOOPLIMIT 500 // ~50MB file

int sendXDRFile(XDR* xdrs, const char* xdrfile);
int receiveXDRFile(XDR* xdrs, const char* xdrfile);

#endif // UDA_CLIENTSERVER_READXDRFILE_H
