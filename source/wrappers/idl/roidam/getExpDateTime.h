#ifndef UDA_WRAPPERS_IDL_GETEXPDATETIME_H
#define UDA_WRAPPERS_IDL_GETEXPDATETIME_H

#include <libpq-fe.h>

int getExpDateTime(PGconn *DBConnect, int exp_number, char *shotdate, char *shottime, int verbose);

#endif // UDA_WRAPPERS_IDL_GETEXPDATETIME_H
