#include "userid.h"

#ifndef _WIN32
#  include <unistd.h>
#else
#  include <Windows.h>
#endif

#include <cstdlib>

#include "common/stringUtils.h"
#include "udaDefines.h"

using namespace uda::client_server;
using namespace uda::common;

/**
 * Request userid from OS
 * @param uid OUT
 */
void uda::client_server::user_id(char* uid)
{
#ifdef _WIN32
    DWORD size = StringLength - 1;
    GetUserName(uid, &size);
    return;
#else
    const char* user;
    uid[0] = '\0';
#  if defined(cuserid)
    if ((user = cuserid(nullptr)) != nullptr) {
        copy_string(user, uid, StringLength);
        return;
    } else
#  endif
        if ((user = getlogin()) != nullptr || (user = getenv("USER")) != nullptr ||
            (user = getenv("LOGNAME")) != nullptr) {
        copy_string(user, uid, StringLength);
        return;
    }
#endif // _WIN32
}
