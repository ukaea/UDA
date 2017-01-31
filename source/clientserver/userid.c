// Request Userid from O/S
//-------------------------------------------------------------------------

#include "userid.h"

#include <unistd.h>
#include <stdlib.h>

#include "stringUtils.h"
#include "idamDefines.h"

void userid(char* uid)
{

#ifdef _WIN32
    int l = STRING_LENGTH-1;
    GetUserName(uid, &l);
    return;
#endif

    char* user;
    uid[0] = '\0';
    if ((user = getlogin()) != NULL) {
        copyString(user, uid, STRING_LENGTH);
        return;
    } else
#if defined(cuserid)
        if((user = cuserid(NULL)) != NULL) {
            copyString(user, uid, STRING_LENGTH);
            return;
        } else
#endif
    if ((user = getenv("USER")) != NULL) {
        copyString(user, uid, STRING_LENGTH);
        return;
    } else if ((user = getenv("LOGNAME")) != NULL) {
        copyString(user, uid, STRING_LENGTH);
        return;
    }

    return;
}
