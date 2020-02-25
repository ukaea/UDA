#ifndef UDA_CLIENTSERVER_READXDRFILE_H
#define UDA_CLIENTSERVER_READXDRFILE_H

#include <rpc/types.h>
#include <rpc/xdr.h>

#define MAXDOLOOPLIMIT 500			// ~50MB file

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int sendXDRFile(XDR* xdrs, char* xdrfile);
LIBRARY_API int receiveXDRFile(XDR* xdrs, char* xdrfile);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_READXDRFILE_H

