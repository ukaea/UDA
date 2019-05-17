#ifndef UDA_CLIENTSERVER_READXDRFILE_H
#define UDA_CLIENTSERVER_READXDRFILE_H

#include <rpc/types.h>
#include <rpc/xdr.h>

#define MAXDOLOOPLIMIT 500			// ~50MB file

#ifdef __cplusplus
extern "C" {
#endif

int sendXDRFile(XDR* xdrs, char* xdrfile);
int receiveXDRFile(XDR* xdrs, char* xdrfile);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_READXDRFILE_H

