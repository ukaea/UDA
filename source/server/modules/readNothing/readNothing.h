
#ifndef IDAM_READNOTHING_H
#define IDAM_READNOTHING_H

#include "idamclientserver.h"
#include "idamserver.h"

int readNothing(DATA_SOURCE data_source, SIGNAL_DESC signal_desc, DATA_BLOCK *data_block);

#ifndef NONOTHINGPLUGIN

#define HEAPERROR 		100008
//#define TIMETEST  

#endif

#endif // IDAM_READNOTHING_H
