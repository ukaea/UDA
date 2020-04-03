#ifndef UDA_SERVER_CREATEXDRSTREAM_H
#define UDA_SERVER_CREATEXDRSTREAM_H

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API void CreateXDRStream();

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_CREATEXDRSTREAM_H
