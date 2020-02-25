#ifndef UDA_CLIENTSERVER_TRIMSTRING_H
#define UDA_CLIENTSERVER_TRIMSTRING_H

#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#ifndef _WIN32
#  include <strings.h>
#endif

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_GNU_SOURCE) && !defined(strcasestr)
LIBRARY_API char* strcasestr(const char* haystack, const char* needle);
#endif

// Reverse a String
LIBRARY_API void reverseString(const char* in, char* out);

// Copy a String subject to a Maximum length constraint
LIBRARY_API void copyString(const char* in, char* out, int maxlength);

/**
 * Allocate and return a string built using the given format and arguments.
 * @param fmt The printf style format string
 * @param ... The arguments to use to generate the string
 * @return The allocated string, needs to be freed
 */
LIBRARY_API char* FormatString(const char* fmt, ...);

// Trim Trailing Space Characters from a String
LIBRARY_API char* TrimString(char* szSource);

// Trim Leading Space Characters from a String
LIBRARY_API char* LeftTrimString(char* str);

LIBRARY_API void StringCopy(char* dest, const char* src, size_t len);

#ifdef __GNUC__
// Convert all LowerCase Characters to Upper Case
LIBRARY_API char* strupr(char* a);

// Convert all UpperCase Characters to Lower Case
LIBRARY_API char* strlwr(char* a);
#endif

// Trim Internal Space Characters from a String
LIBRARY_API char* MidTrimString(char* str);

// Replace all instances of string `find` with string `replace` in the given string
LIBRARY_API char* StringReplaceAll(const char* string, const char* find, const char* replace);

// Replace the first instance of string `find` with string `replace` in the given string
LIBRARY_API char* StringReplace(const char* string, const char* find, const char* replace);

// Is the String an Integer Number? (Simple but not exhaustive Check)
LIBRARY_API int IsNumber(const char* a);

// Is the String a Simple Float Number?
LIBRARY_API int IsFloat(char* a);

// Is the String a Number List (#,#,#,#;#;#;#)?
LIBRARY_API int IsNumberList(char* a);

LIBRARY_API char* convertNonPrintable(char* str);

LIBRARY_API char* convertNonPrintable2(char* str);

LIBRARY_API int IsLegalFilePath(const char* str);

#if !defined(asprintf)
#  if defined(__cplusplus) && !defined(__APPLE__)
LIBRARY_API int asprintf(char** strp, const char* fmt, ...) noexcept;
#  else
LIBRARY_API int asprintf(char** strp, const char* fmt, ...);
#  endif
#endif

LIBRARY_API char** SplitString(const char* string, const char* delim);

LIBRARY_API void FreeSplitStringTokens(char*** tokens);

LIBRARY_API bool StringEquals(const char* a, const char* b);

LIBRARY_API bool StringIEquals(const char* a, const char* b);

LIBRARY_API bool StringEndsWith(const char* str, const char* find);

#define STR_STARTSWITH(X, Y) !strncmp(X, Y, strlen(Y))
#define STR_ISTARTSWITH(X, Y) !strncasecmp(X, Y, strlen(Y))

#define STR_EQUALS(X, Y) StringEquals(X, Y)
#define STR_IEQUALS(X, Y) StringIEquals(X, Y)

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_TRIMSTRING_H

