#ifndef UDA_CLIENT_UPDATESELECTPARMS_H
#define UDA_CLIENT_UPDATESELECTPARMS_H

#include <time.h>
#include <clientserver/export.h>

#ifndef _WIN32
#  include <sys/select.h>
#else
#  include <winsock.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API void udaUpdateSelectParms(int fd, fd_set* rfds, struct timeval* tv);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_UPDATESELECTPARMS_H
