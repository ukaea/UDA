#ifndef UDA_CLIENTSERVER_EXPORT_H
#define UDA_CLIENTSERVER_EXPORT_H

#ifdef _WIN32
#  ifdef UDA_EXPORT
#    define LIBRARY_API __declspec(dllexport)
#  else
#    define LIBRARY_API __declspec(dllimport)
#  endif
#else
#  define LIBRARY_API
#endif

#endif // UDA_CLIENTSERVER_EXPORT_H
