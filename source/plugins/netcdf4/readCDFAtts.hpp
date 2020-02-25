#ifndef UDA_PLUGIN_READCDFATTS_H
#define UDA_PLUGIN_READCDFATTS_H

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int readCDF4Atts(int grpid, int varid, char* units, char* title, char* cls, char* comment);

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGIN_READCDFATTS_H
