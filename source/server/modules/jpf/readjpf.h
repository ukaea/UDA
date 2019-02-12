#ifndef IDAM_READJPF_H
#define IDAM_READJPF_H

#include <stdio.h>

#include <clientserver/udaStructs.h>

int readJPF(REQUEST_BLOCK *request_block, DATA_BLOCK *data_block);

#ifndef NOJPFPLUGIN

#include "getfix/getfix.h"

int mydprintf(FILE* fp, const char *  fmt, ...);

#endif

#endif // IDAM_READJPF_H

