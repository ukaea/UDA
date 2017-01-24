//! $LastChangedRevision: 353 $
//! $LastChangedDate: 2013-11-18 15:32:28 +0000 (Mon, 18 Nov 2013) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/clientserver/userid.c $

// Request Userid from O/S
//
// Change History
//
// 1.0	dgmuir 08 Nov 2005	Production Version
// 31Jan2011	dgm	Windows sockets implementation
// 07Mar2012	dgm	Removed legacy references to popen - replace using standard system function calls
//                      and environment variables.

//-------------------------------------------------------------------------

#include "userid.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "TrimString.h"
#include "idamDefines.h"

void userid(char *uid) {

#ifdef _WIN32
    int l = STRING_LENGTH-1;
    GetUserName(uid, &l);
    return;
#endif

    char *user;
    uid[0] = '\0';
    if((user = getlogin()) != NULL) {
        copyString(user, uid, STRING_LENGTH);
        return;
    } else
#ifndef __APPLE__
        if((user = cuserid(NULL)) != NULL) {
            copyString(user, uid, STRING_LENGTH);
            return;
        } else
#endif
            if((user = getenv("USER")) != NULL) {
                copyString(user, uid, STRING_LENGTH);
                return;
            } else if((user = getenv("LOGNAME")) != NULL) {
                copyString(user, uid, STRING_LENGTH);
                return;
            }

    return;
}
