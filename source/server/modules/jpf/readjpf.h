#ifndef IDAM_READJPF_H
#define IDAM_READJPF_H

#include <clientserver/idamStructs.h>

int readJPF(DATA_SOURCE data_source, SIGNAL_DESC signal_desc, DATA_BLOCK *data_block);

#ifndef NOJPFPLUGIN

#include "getfix/getfix.h"

#if	(0)
#include "netcsl7.h"
#endif

int mydprintf(FILE* fp, const char *  fmt, ...);

#endif

#endif // IDAM_READJPF_H

