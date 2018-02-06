// Timing of Socket Read Attempts via Select
//
//----------------------------------------------------------------

#ifndef UDA_CLIENT_UPDATESELECTPARMS_H
#define UDA_CLIENT_UPDATESELECTPARMS_H

#include <time.h>
#ifndef _WIN32
#  include <sys/select.h>
#else
#  include <winsock.h>
#endif

void idamUpdateSelectParms(int fd, fd_set* rfds, struct timeval* tv);

#endif // UDA_CLIENT_UPDATESELECTPARMS_H
