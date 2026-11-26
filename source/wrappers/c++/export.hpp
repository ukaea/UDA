#ifndef UDA_CXX_EXPORT_HPP
#define UDA_CXX_EXPORT_HPP

// UDA_CXX_API marks the C++ wrapper's public classes. It depends on how the wrapper is built:
//   UDA_CXX_STATIC  static wrapper archive: no decoration.
//   UDA_CXX_SHARED  standalone shared wrapper: export from its DLL when UDA_CXX_EXPORT is set,
//                   import otherwise (Windows). Unix needs no decoration.
//   otherwise       in-tree build: follows the core LIBRARY_API.

#include <clientserver/export.h>

// Standalone builds link to a separate core library, so its DLL annotations
// must be independent of those on the wrapper.
#if defined(UDA_CXX_STATIC)
#  define UDA_CXX_API
#elif defined(_WIN32) && defined(UDA_CXX_SHARED)
#  if defined(UDA_CXX_EXPORT)
#    define UDA_CXX_API __declspec(dllexport)
#  else
#    define UDA_CXX_API __declspec(dllimport)
#  endif
#else
#  define UDA_CXX_API LIBRARY_API
#endif

#endif
