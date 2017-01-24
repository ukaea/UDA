//! $LastChangedRevision: 87 $
//! $LastChangedDate: 2008-12-19 15:34:15 +0000 (Fri, 19 Dec 2008) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/client/UpdateSelectParms.c $

// Timing of Socket Read Attempts via Select
//
//----------------------------------------------------------------

#include "UpdateSelectParms.h"

void idamUpdateSelectParms(int fd, fd_set *rfds, struct timeval *tv) {
    FD_ZERO(rfds);
    FD_SET(fd, rfds);
    tv->tv_sec = 0;
    tv->tv_usec = 500;	// in microsecs => 0.5 ms wait
}
