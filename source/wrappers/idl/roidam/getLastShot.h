#ifndef UDA_WRAPPERS_IDL_GETLASTSHOT_H
#define UDA_WRAPPERS_IDL_GETLASTSHOT_H

#include <libpq-fe.h>

int getLastShot(PGconn *DBConnect, int *exp_number, int verbose, int debug);

#endif // UDA_WRAPPERS_IDL_GETLASTSHOT_H
