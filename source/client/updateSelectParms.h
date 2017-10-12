// Timing of Socket Read Attempts via Select
//
//----------------------------------------------------------------

#ifndef UDA_CLIENT_UPDATESELECTPARMS_H
#define UDA_CLIENT_UPDATESELECTPARMS_H

#include <time.h>
#ifdef __GNUC__
#  include <sys/select.h>
#elif defined(_WIN32)
#  include <winsock.h>
#endif

void idamUpdateSelectParms(int fd, fd_set* rfds, struct timeval* tv);

#endif // UDA_CLIENT_UPDATESELECTPARMS_H
