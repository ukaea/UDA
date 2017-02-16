// Timing of Socket Read Attempts via Select
//
//----------------------------------------------------------------

#ifndef IDAM_CLIENT_UPDATESELECTPARMS_H
#define IDAM_CLIENT_UPDATESELECTPARMS_H

#include <time.h>
#include <sys/select.h> 

void idamUpdateSelectParms(int fd, fd_set *rfds, struct timeval *tv);

#endif // IDAM_CLIENT_UPDATESELECTPARMS_H