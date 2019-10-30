#ifndef UDA_PLUGIN_READCDFMETA_H
#define UDA_PLUGIN_READCDFMETA_H

typedef struct MetaXML {
    char* xml;
    int lheap;
    int nxml;
} METAXML;

void allocMetaXML(METAXML* str);

void addMetaXML(METAXML* str, const char* tag);

int addTextMetaXML(int fd, int grpid, METAXML* str, const char* tag);

int addIntMetaXML(int fd, int grpid, METAXML* str, const char* tag);

#endif // UDA_PLUGIN_READCDFMETA_H
