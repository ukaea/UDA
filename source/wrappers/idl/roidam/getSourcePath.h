#ifndef UDA_WRAPPERS_IDL_GETSOURCEPATH_H
#define UDA_WRAPPERS_IDL_GETSOURCEPATH_H

#include <libpq-fe.h>

int getSourcePath(PGconn *DBConnect, int exp_number, int pass, char *alias, char *type, int iscase, int verbose, char *path);

#endif // UDA_WRAPPERS_IDL_GETSOURCEPATH_H
