#ifndef IDAM_READNOTHING_H
#define IDAM_READNOTHING_H

#include <clientserver/udaStructs.h>

int readNothing(DATA_SOURCE data_source, SIGNAL_DESC signal_desc, DATA_BLOCK *data_block);

#ifndef NONOTHINGPLUGIN
#  define HEAPERROR 100008
#endif

#endif // IDAM_READNOTHING_H
