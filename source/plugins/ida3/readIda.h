#ifndef IDAM_PLUGINS_IDA3_READIDA2_H
#define IDAM_PLUGINS_IDA3_READIDA2_H

#include <clientserver/udaStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

int readIda3(DATA_BLOCK* data_block, int exp_number, int pass, const char* source_alias, const char* signal_name,
                   const char* filename, const char* path, char type, const ENVIRONMENT* environment);

#ifdef __cplusplus
}
#endif

#endif // IDAM_PLUGINS_IDA3_READIDA2_H
