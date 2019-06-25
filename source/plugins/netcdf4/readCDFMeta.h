#ifndef UDA_PLUGIN_READCDFMETA_H
#define UDA_PLUGIN_READCDFMETA_H

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

#endif // UDA_PLUGIN_READCDFMETA_H
