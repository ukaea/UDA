// Request Userid from O/S
//-------------------------------------------------------------------------

#include "userid.h"

#ifndef _WIN32
#  include <unistd.h>
#else
#  include <Windows.h>
#endif

#include <cstdlib>

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
    if((user = cuserid(nullptr)) != nullptr) {
        copyString(user, uid, STRING_LENGTH);
        return;
    } else
#  endif
    if ((user = getlogin()) != nullptr
            || (user = getenv("USER")) != nullptr
            || (user = getenv("LOGNAME")) != nullptr) {
        copyString(user, uid, STRING_LENGTH);
        return;
    }
#endif // _WIN32
}
