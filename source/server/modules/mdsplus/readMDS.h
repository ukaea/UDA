#ifndef IDAM_READMDS_H
#define IDAM_READMDS_H

#include <clientserver/idamStructs.h>

int readMDS(DATA_SOURCE data_source, SIGNAL_DESC signal_desc, DATA_BLOCK *data_block);

#ifndef NOMDSPLUSPLUGIN

#include <mdsdescrip.h> 
#include <mdslib.h> 
#include <mdsshr.h> 
#include <mdstypes.h> 
 
#define status_ok(status) (((status) & 1) == 1) 

#ifdef MDSSANDBOX
void __mdscall_init();		// MDS+ Security sand-box
#endif

int readMDSType(int type);

#endif

#endif // IDAM_READMDS_H
