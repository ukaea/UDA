#ifndef IDAM_READJPF_H
#define IDAM_READJPF_H

#include <stdio.h>

#include <clientserver/udaStructs.h>

int readJPF(DATA_SOURCE data_source, SIGNAL_DESC signal_desc, DATA_BLOCK *data_block);

#ifndef NOJPFPLUGIN

#include "getfix/getfix.h"

int mydprintf(FILE* fp, const char *  fmt, ...);

#endif

#endif // IDAM_READJPF_H

