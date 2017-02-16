#ifndef IDAM_READIDAITEM_H
#define IDAM_READIDAITEM_H

#ifndef NOIDAPLUGIN

#include <ida3.h>
#include <clientserver/udaStructs.h>

#define SWAPXY

int itemType(unsigned short datpck, short typeno, int getbytes, char* type);
int errorType(unsigned short datpck, short typeno, int getbytes, char* type);
char* itemData(int data_type, int totsams);
void swapRank3(DATA_BLOCK* data_block, int pattern);
void addxmlmetastring(char** metaxml, int* lheap, char* xml, int* nxml);
void addxmlmetaarray(char** metaxml, int* lheap, char* tag, void* data, int ndata, int type, int* nxml);
int readIdaItem(char* itemname, ida_file_ptr* ida_file, short* context, DATA_BLOCK* data_block);
void idaclasses(int class, char* label, char* axes, int* datarank, int* timeorder);

#endif

#endif // IDAM_READIDAITEM_H

