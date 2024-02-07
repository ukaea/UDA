#pragma once

#ifndef UDA_CLIENTSERVER_STRINGUTILS_H
#  define UDA_CLIENTSERVER_STRINGUTILS_H

#  include <algorithm>
#  include <ctype.h>
#  include <stdbool.h>
#  include <string.h>
#  include <string>

#  ifndef _WIN32
#    include <strings.h>
#  endif

#  if !defined(_GNU_SOURCE) && !defined(strcasestr)
char* strcasestr(const char* haystack, const char* needle);
#  endif

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

void StringCopy(char* dest, const char* src, size_t len);

#  ifdef __GNUC__
// Convert all LowerCase Characters to Upper Case
char* strupr(char* a);

// Convert all UpperCase Characters to Lower Case
char* strlwr(char* a);
#  endif

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

#  if !defined(asprintf)
#    if defined(__cplusplus) && !defined(__APPLE__)
int asprintf(char** strp, const char* fmt, ...) noexcept;
#    else
int asprintf(char** strp, const char* fmt, ...);
#    endif
#  endif

char** SplitString(const char* string, const char* delim);

void FreeSplitStringTokens(char*** tokens);

bool StringEquals(const char* a, const char* b);

bool StringIEquals(const char* a, const char* b);

bool StringEndsWith(const char* str, const char* find);

#  define STR_STARTSWITH(X, Y) !strncmp(X, Y, strlen(Y))
#  define STR_ISTARTSWITH(X, Y) !strncasecmp(X, Y, strlen(Y))

#  define STR_EQUALS(X, Y) StringEquals(X, Y)
#  define STR_IEQUALS(X, Y) StringIEquals(X, Y)

namespace uda
{
// remove non printable characters
static inline void convert_non_printable(std::string& str)
{
    std::replace_if(
        str.begin(), str.end(), [](char c) { return c < ' ' || c > '~'; }, ' ');
}

// trim from start (in place)
static inline void ltrim(std::string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
}

// trim from end (in place)
static inline void rtrim(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
}
} // namespace uda

#endif // UDA_CLIENTSERVER_STRINGUTILS_H
