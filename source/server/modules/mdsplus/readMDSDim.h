#ifndef UDA_READMDSDIM_H
#define UDA_READMDSDIM_H

#include <clientserver/udaStructs.h>

#define status_ok(status) (((status) & 1) == 1)

int readMDSDim(char *node, int ndim, DIMS *ddim);

#endif // UDA_READMDSDIM_H

