#ifndef IDAM_CLIENTSERVER_READXDRFILE_H
#define IDAM_CLIENTSERVER_READXDRFILE_H

#include <rpc/types.h>
#include <rpc/xdr.h>

#define MAXDOLOOPLIMIT 500			// ~50MB file

int sendXDRFile(XDR *xdrs, char *xdrfile);
int receiveXDRFile(XDR *xdrs, char *xdrfile);

#endif // IDAM_CLIENTSERVER_READXDRFILE_H

