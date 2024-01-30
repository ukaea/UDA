#ifndef UDA_EXPORT_H
#define UDA_EXPORT_H

#ifdef _WIN32
#  ifdef UDA_EXPORT
#    define __declspec(dllexport)
#  else
#    define __declspec(dllimport)
#  endif
#else
#  define LIBRARY_API
#endif

#endif // UDA_EXPORT_H
