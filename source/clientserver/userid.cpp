#include "userid.h"

#ifndef _WIN32
#  include <unistd.h>
#else
#  include <Windows.h>
#endif

#include <cstdlib>

#include "stringUtils.h"
#include "udaDefines.h"

using namespace uda::client_server;

/**
 * Request userid from OS
 * @param uid OUT
 */
void uda::client_server::user_id(char* uid)
{
#ifdef _WIN32
    DWORD size = STRING_LENGTH - 1;
    GetUserName(uid, &size);
    return;
#else
    const char* user;
    uid[0] = '\0';
#  if defined(cuserid)
    if ((user = cuserid(nullptr)) != nullptr) {
        copy_string(user, uid, STRING_LENGTH);
        return;
    } else
#  endif
        if ((user = getlogin()) != nullptr || (user = getenv("USER")) != nullptr ||
            (user = getenv("LOGNAME")) != nullptr) {
        copy_string(user, uid, STRING_LENGTH);
        return;
    }
#endif // _WIN32
}
