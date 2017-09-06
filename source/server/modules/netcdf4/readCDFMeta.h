//
// Created by jholloc on 09/05/16.
//

#ifndef IDAM_READCDFMETA_H
#define IDAM_READCDFMETA_H

struct METAXML {
    char* xml;
    int lheap;
    int nxml;
};
typedef struct METAXML METAXML;

void allocMetaXML(METAXML* str);

void addMetaXML(METAXML* str, char* tag);

int addTextMetaXML(int fd, int grpid, METAXML* str, char* tag);

int addIntMetaXML(int fd, int grpid, METAXML* str, char* tag);

#endif //IDAM_READCDFMETA_H
