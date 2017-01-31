#ifndef UDA_WRAPPERS_IDL_GETLATESTSOURCEPASS_H
#define UDA_WRAPPERS_IDL_GETLATESTSOURCEPASS_H

#include <libpq-fe.h>

int getLatestSourcePass(PGconn *DBConnect, int exp_number, char *source, int verbose, int *pass);

#endif // UDA_WRAPPERS_IDL_GETLATESTSOURCEPASS_H
