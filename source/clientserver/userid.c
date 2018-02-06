// Request Userid from O/S
//-------------------------------------------------------------------------

#include "userid.h"

#ifndef _WIN32
#  include <unistd.h>
#else
#  include <Windows.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include "stringUtils.h"
#include "udaDefines.h"

void userid(char* uid)
{
#ifdef _WIN32
    DWORD size = STRING_LENGTH - 1;
    GetUserName(uid, &size);
    return;
#else
    const char* user;
    uid[0] = '\0';
#  if defined(cuserid)
    if((user = cuserid(NULL)) != NULL) {
        copyString(user, uid, STRING_LENGTH);
        return;
    } else
#  endif
    if ((user = getlogin()) != NULL) {
        copyString(user, uid, STRING_LENGTH);
        return;
    } else if ((user = getenv("USER")) != NULL) {
        copyString(user, uid, STRING_LENGTH);
        return;
    } else if ((user = getenv("LOGNAME")) != NULL) {
        copyString(user, uid, STRING_LENGTH);
        return;
    }

    return;
#endif // _WIN32
}
