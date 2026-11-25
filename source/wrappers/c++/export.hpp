#ifndef UDA_CXX_EXPORT_HPP
#define UDA_CXX_EXPORT_HPP

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
