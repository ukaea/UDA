#ifndef IDAM_PLUGINS_IDA3_READIDAITEM_H
#define IDAM_PLUGINS_IDA3_READIDAITEM_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NOIDAPLUGIN

#include <idamclientserverpublic.h>
#include <ida3.h>

#define SWAPXY

void addxmlmetastring(char ** metaxml, int * lheap, char * xml, int * nxml);

void addxmlmetaarray(char ** metaxml, int * lheap, char * tag, void * data, int ndata, int type, int * nxml);

void idaclasses(int class, char * label, char * axes, int * datarank, int * timeorder);

int itemType(unsigned short datpck, short typeno, int getbytes, char * type);

int errorType(unsigned short datpck, short typeno, int getbytes, char * type);

char * itemData(int data_type, int totsams);

void swapRank3(DATA_BLOCK * data_block, int pattern);

void addxmlmetastring(char ** metaxml, int * lheap, char * xml, int * nxml);

void addxmlmetaarray(char ** metaxml, int * lheap, char * tag, void * data, int ndata, int type, int * nxml);

int readIdaItem(char * itemname, ida_file_ptr * ida_file, short * context, DATA_BLOCK * data_block);

void idaclasses(int class, char * label, char * axes, int * datarank, int * timeorder);

#endif

#ifdef __cplusplus
}
#endif

#endif // IDAM_PLUGINS_IDA3_READIDAITEM_H