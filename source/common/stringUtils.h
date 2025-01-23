#pragma once

#include <algorithm>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <string>

#ifndef _WIN32
#  include <strings.h>
#endif

namespace uda::common
{

std::string demangle(const char* name);

// Reverse a String
void reverse_string(const char* in, char* out);

// Copy a String subject to a Maximum length constraint
void copy_string(const char* in, char* out, size_t maxlength);

void copy_string(const std::string& in, char* out, size_t maxlength);

/**
 * Allocate and return a string built using the given format and arguments.
 * @param fmt The printf style format string
 * @param ... The arguments to use to generate the string
 * @return The allocated string, needs to be freed
 */
char* format_string(const char* fmt, ...);

// Trim Trailing Space Characters from a String
char* trim_string(char* szSource);

// Trim Leading Space Characters from a String
char* left_trim_string(char* str);

#ifdef __GNUC__

// Convert all LowerCase Characters to Upper Case
char* strupr(char* a);

// Convert all UpperCase Characters to Lower Case
char* strlwr(char* a);

#endif

// Trim Internal Space Characters from a String
char* mid_trim_string(char* str);

// Replace all instances of string `find` with string `replace` in the given string
char* string_replace_all(const char* string, const char* find, const char* replace);

// Replace the first instance of string `find` with string `replace` in the given string
char* string_replace(const char* string, const char* find, const char* replace);

// Is the String an Integer Number? (Simple but not exhaustive Check)
int is_number(const char* a);

// Is the String a Simple Float Number?
int is_float(char* a);

// Is the String a Number List (#,#,#,#;#;#;#)?
int is_number_list(char* a);

char* convert_non_printable(char* str);

char* convert_non_printable2(char* str);

int is_legal_file_path(const char* str);

#if !defined(asprintf)
#  if defined(__cplusplus) && !defined(__APPLE__)
int asprintf(char** strp, const char* fmt, ...) noexcept;
#  else

int asprintf(char** strp, const char* fmt, ...);

#  endif
#endif

char** split_string(const char* string, const char* delim);

void free_split_string_tokens(char*** tokens);

bool string_equals(const char* a, const char* b);

bool string_iequals(const char* a, const char* b);

bool string_ends_with(const char* str, const char* find);

#define STR_STARTSWITH(X, Y) !strncmp(X, Y, strlen(Y))
#define STR_ISTARTSWITH(X, Y) !strncasecmp(X, Y, strlen(Y))

#define STR_EQUALS(X, Y) uda::common::string_equals(X, Y)
#define STR_IEQUALS(X, Y) uda::common::string_iequals(X, Y)

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

} // namespace uda::client_server
