//! $LastChangedRevision: 87 $
//! $LastChangedDate: 2008-12-19 15:34:15 +0000 (Fri, 19 Dec 2008) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/client/UpdateSelectParms.c $

// Timing of Socket Read Attempts via Select
//
//----------------------------------------------------------------

#ifndef IDAM_UPDATESELECTPARMS_H
#define IDAM_UPDATESELECTPARMS_H

#include "idamclientserver.h"
#include "idamclient.h"

void idamUpdateSelectParms(int fd, fd_set *rfds, struct timeval *tv);

#endif // IDAM_UPDATESELECTPARMS_H
