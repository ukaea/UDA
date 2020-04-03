#ifndef UDA_CLIENTSERVER_PARSEOPERATION_H
#define UDA_CLIENTSERVER_PARSEOPERATION_H

#include "parseXML.h"

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int idamParseOperation(SUBSET* sub);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_PARSEOPERATION_H
