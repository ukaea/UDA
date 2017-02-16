#ifndef IDAM_READUFILE_H
#define IDAM_READUFILE_H

#include <clientserver/udaStructs.h>

int readUFile(DATA_SOURCE data_source, SIGNAL_DESC signal_desc, DATA_BLOCK *data_block);

#ifndef NOUFILEPLUGIN

#define UFILE_ERROR_OPENING_FILE 200
#define UFILE_ERROR_ALLOCATING_DIM_HEAP 201
#define UFILE_ERROR_ALLOCATING_DATA_HEAP 202

#define TEST 0
#define UFILE_BUFFER 256

#endif

#endif // IDAM_READUFILE_H

