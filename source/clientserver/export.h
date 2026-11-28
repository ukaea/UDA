#ifndef UDA_CLIENTSERVER_EXPORT_H
#define UDA_CLIENTSERVER_EXPORT_H

// LIBRARY_API marks the symbols of the core UDA libraries that consumers call.
//   UDA_STATIC      static client: the symbols are linked directly, so no decoration.
//   UDA_EXPORT      defined when building the core libraries: export from the DLL.
//   otherwise       import from the DLL (Windows); no decoration on Unix.

#if defined(UDA_STATIC)
#  define LIBRARY_API
#elif defined(_WIN32)
#  ifdef UDA_EXPORT
#    define LIBRARY_API __declspec(dllexport)
#  else
#    define LIBRARY_API __declspec(dllimport)
#  endif
#else
#  define LIBRARY_API
#endif

#endif // UDA_CLIENTSERVER_EXPORT_H
