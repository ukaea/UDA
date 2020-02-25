#ifndef UDA_PLUGIN_READCDFMETA_H
#define UDA_PLUGIN_READCDFMETA_H

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MetaXML {
    char* xml;
    int lheap;
    int nxml;
} METAXML;

LIBRARY_API void allocMetaXML(METAXML* str);

LIBRARY_API void addMetaXML(METAXML* str, const char* tag);

LIBRARY_API int addTextMetaXML(int fd, int grpid, METAXML* str, const char* tag);

LIBRARY_API int addIntMetaXML(int fd, int grpid, METAXML* str, const char* tag);

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGIN_READCDFMETA_H
