#ifndef UDA_CLIENT_CLOSEDOWN_H
#define UDA_CLIENT_CLOSEDOWN_H

#include <clientserver/socketStructs.h>
#include "export.h"

#ifdef FATCLIENT
#  define closedown closedownFat
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum class ClosedownType {
    CLOSE_SOCKETS = 0,
    CLOSE_ALL = 1,
};

LIBRARY_API int
closedown(ClosedownType type, SOCKETLIST* socket_list, XDR* client_input, XDR* client_output, bool* reopen_logs);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_CLOSEDOWN_H
