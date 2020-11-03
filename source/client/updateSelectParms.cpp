// Timing of Socket Read Attempts via Select
//
//----------------------------------------------------------------

#include "updateSelectParms.h"

void udaUpdateSelectParms(int fd, fd_set *rfds, struct timeval *tv) {
    FD_ZERO(rfds);
    FD_SET(fd, rfds);
    tv->tv_sec = 0;
    tv->tv_usec = 500;	// in microsecs => 0.5 ms wait
}
