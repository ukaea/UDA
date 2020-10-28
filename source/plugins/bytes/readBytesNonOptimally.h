#ifndef UDA_PLUGIN_READBYTESNONOPTIMALLY_H
#define UDA_PLUGIN_READBYTESNONOPTIMALLY_H

#include <clientserver/udaStructs.h>
#include <clientserver/export.h>

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int readBytes(DATA_SOURCE data_source, SIGNAL_DESC signal_desc, DATA_BLOCK *data_block, const ENVIRONMENT* environment);

#ifndef NOBINARYPLUGIN

#define BYTEFILEDOESNOTEXIST 	100001
#define BYTEFILEATTRIBUTEERROR 	100002
#define BYTEFILEISNOTREGULAR 	100003 
#define BYTEFILEOPENERROR 	    100004
#define BYTEFILEHEAPERROR 	    100005
#define BYTEFILEMD5ERROR	    100006
#define BYTEFILEMD5DIFF		    100007

#endif

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGIN_READBYTESNONOPTIMALLY_H

