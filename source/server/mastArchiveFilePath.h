#ifndef UDA_SERVER_MASTARCHIVEFILEPATH_H
#define UDA_SERVER_MASTARCHIVEFILEPATH_H

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NOIDAPLUGIN
LIBRARY_API void mastArchiveFilePath(int pulno, int pass, char* file, char* path);
#endif

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_MASTARCHIVEFILEPATH_H

