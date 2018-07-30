#ifndef UDA_PLUGINS_NETCDF_READCDFMETA_H
#define UDA_PLUGINS_NETCDF_READCDFMETA_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MetaXML {
    char* xml;
    int lheap;
    int nxml;
} METAXML;

void allocMetaXML(METAXML* str);

void addMetaXML(METAXML* str, char* tag);

int addTextMetaXML(int fd, int grpid, METAXML* str, char* tag);

int addIntMetaXML(int fd, int grpid, METAXML* str, char* tag);

#ifdef __cplusplus
extern "C" {
#endif

#endif // UDA_PLUGINS_NETCDF_READCDFMETA_H
