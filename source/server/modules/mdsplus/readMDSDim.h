#ifndef IDAM_READMDSDIM_H
#define IDAM_READMDSDIM_H

#include <clientserver/idamStructs.h>

#define status_ok(status) (((status) & 1) == 1)

int readMDSDim(char *node, int ndim, DIMS *ddim);

#endif // IDAM_READMDSDIM_H

