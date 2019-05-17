#ifndef UDA_CLIENTSERVER_TRIMSTRING_H
#define UDA_CLIENTSERVER_TRIMSTRING_H

#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#ifndef _WIN32
#  include <strings.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_GNU_SOURCE) && !defined(strcasestr)
char* strcasestr(const char* haystack, const char* needle);
#endif

// Reverse a String
void reverseString(const char* in, char* out);

// Copy a String subject to a Maximum length constraint
void copyString(const char* in, char* out, int maxlength);

/**
 * Allocate and return a string built using the given format and arguments.
 * @param fmt The printf style format string
 * @param ... The arguments to use to generate the string
 * @return The allocated string, needs to be freed
 */
char* FormatString(const char* fmt, ...);

// Trim Trailing Space Characters from a String
char* TrimString(char* szSource);

// Trim Leading Space Characters from a String
char* LeftTrimString(char* str);

#ifdef __GNUC__
void StringCopy(char* dest, const char* src, size_t len);

// Convert all LowerCase Characters to Upper Case
char* strupr(char* a);

// Convert all UpperCase Characters to Lower Case
char* strlwr(char* a);
#endif

// Trim Internal Space Characters from a String
char* MidTrimString(char* str);

// Replace all instances of string `find` with string `replace` in the given string
char* StringReplaceAll(const char* string, const char* find, const char* replace);

// Replace the first instance of string `find` with string `replace` in the given string
char* StringReplace(const char* string, const char* find, const char* replace);

// Is the String an Integer Number? (Simple but not exhaustive Check)
int IsNumber(const char* a);

// Is the String a Simple Float Number?
int IsFloat(char* a);

// Is the String a Number List (#,#,#,#;#;#;#)?
int IsNumberList(char* a);

char* convertNonPrintable(char* str);

char* convertNonPrintable2(char* str);

int IsLegalFilePath(const char* str);

#if !defined(asprintf)
int asprintf(char** strp, const char* fmt, ...);
#endif

char** SplitString(const char* string, const char* delim);

void FreeSplitStringTokens(char*** tokens);

bool StringEquals(const char* a, const char* b);

bool StringIEquals(const char* a, const char* b);

bool StringEndsWith(const char* str, const char* find);

#define STR_STARTSWITH(X, Y) !strncmp(X, Y, strlen(Y))
#define STR_ISTARTSWITH(X, Y) !strncasecmp(X, Y, strlen(Y))

#define STR_EQUALS(X, Y) StringEquals(X, Y)
#define STR_IEQUALS(X, Y) StringIEquals(X, Y)

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_TRIMSTRING_H

