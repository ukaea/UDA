#include "stringUtils.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <string>

#ifdef __GNUC__
#  include <strings.h>
#elif defined(_WIN32)
#  include <string.h>
#  define strncasecmp _strnicmp
#endif

#ifdef __GNUG__
#include <memory>
#include <cxxabi.h>

std::string uda::common::demangle(const char* name) {

    int status = -4; // some arbitrary value to eliminate the compiler warning

    // enable c++11 by passing the flag -std=c++11 to g++
    std::unique_ptr<char, void(*)(void*)> res {
            abi::__cxa_demangle(name, NULL, NULL, &status),
            std::free
    };

    return (status==0) ? res.get() : name ;
}

#else

// does nothing if not g++
std::string demangle(const char* name) {
    return name;
}

#endif


#if !defined(_GNU_SOURCE) && !defined(strcasestr)

char* strcasestr(const char* haystack, const char* needle)
{
    char c, sc;
    size_t len;

    if ((c = *needle++) != 0) {
        c = (char)tolower(c);
        len = strlen(needle);
        do {
            do {
                if ((sc = *haystack++) == 0) {
                    return nullptr;
                }
            } while ((char)tolower((unsigned char)sc) != c);
        } while (strncasecmp(haystack, needle, len) != 0);
        haystack--;
    }
    return (char*)haystack;
}

#endif

// Reverse a String

void uda::common::reverse_string(const char* in, char* out)
{
    int lstr = (int)strlen(in);
    out[lstr] = '\0';
    for (int i = 0; i < lstr; i++) {
        out[i] = in[lstr - 1 - i];
    }
}

// Copy a String subject to a Maximum length constraint

void uda::common::copy_string(const char* in, char* out, size_t maxlength)
{
    size_t lstr = strlen(in);
    if (lstr < maxlength) {
        strcpy(out, in);
    } else {
        strncpy(out, in, maxlength - 1);
        out[maxlength - 1] = '\0';
    }
}

void uda::common::copy_string(const std::string& in, char* out, size_t maxlength)
{
    size_t lstr = in.size();
    if (lstr < maxlength) {
        strcpy(out, in.c_str());
    } else {
        strncpy(out, in.c_str(), maxlength - 1);
        out[maxlength - 1] = '\0';
    }
}

char* uda::common::format_string(const char* fmt, ...)
{
    va_list vargs;
    va_start(vargs, fmt);

    size_t len = vsnprintf(nullptr, 0, fmt, vargs) + 1;

    va_end(vargs);
    va_start(vargs, fmt);

    char* strp = (char*)malloc(len * sizeof(char));
    vsnprintf(strp, len, fmt, vargs);

    va_end(vargs);

    return strp;
}

// Trim Trailing Space Characters from a String

char* uda::common::trim_string(char* szSource)
{
    char* pszEOS;

    /*Set pointer to end of string to point to the character just
     *before the 0 at the end of the string.
     */
    pszEOS = szSource + strlen(szSource) - 1;

    while (pszEOS >= szSource && *pszEOS == ' ') {
        *pszEOS-- = '\0';
    }

    return szSource;
}

// Trim Leading Space Characters from a String

char* uda::common::left_trim_string(char* str)
{
    int trim = 0, lstr;

    lstr = (int)strlen(str);

    int i = 0;
    while (str[trim] == ' ' && i++ <= lstr) {
        trim++;
    }

    if (trim > 0) {
        lstr = lstr - trim;
        for (int j = 0; j < lstr; j++) {
            str[j] = str[j + trim];
        }
        str[lstr] = '\0';
    }
    return str;
}

#ifdef __GNUC__

// Convert all LowerCase Characters to Upper Case

char* uda::common::strupr(char* a)
{
    char* ret = a;

    while (*a != '\0') {
        if (islower(*a)) {
            *a = (char)toupper(*a);
        }
        ++a;
    }

    return ret;
}

// Convert all UpperCase Characters to Lower Case

char* uda::common::strlwr(char* a)
{
    char* ret = a;

    while (*a != '\0') {
        if (isupper(*a)) {
            *a = (char)tolower(*a);
        }
        ++a;
    }

    return ret;
}

#endif

// Trim Internal Space Characters from a String

char* uda::common::mid_trim_string(char* str)
{
    int j = 0, lstr;
    lstr = (int)strlen(str);
    for (int i = 0; i < lstr; i++) {
        if (str[i] != ' ') {
            str[j++] = str[i];
        }
    }
    str[j] = '\0';
    return str;
}

// Is the String an Integer Number? (Simple but not exhaustive Check)

int uda::common::is_number(const char* a)
{
    const char* wrk = a;
    while (*wrk != '\0') {
        if (!isdigit(*wrk) && *wrk != '-' && *wrk != '+') {
            return 0;
        }
        ++wrk;
    }
    return 1;
}

// Is the String a Simple Float Number?

int uda::common::is_float(char* a)
{
    char* wrk = a;
    while (*wrk != '\0') {
        if (!isdigit(*wrk) && *wrk != '-' && *wrk != '+' && *wrk != '.') {
            return 0;
        }
        ++wrk;
    }
    return 1;
}

// Is the String a Number List (#,#,#,#;#;#;#)?

int uda::common::is_number_list(char* a)
{
    char* wrk = a;
    while (*wrk != '\0') {
        if (!isdigit(*wrk) || *wrk != ',' || *wrk != ';') {
            return 0;
        }
        ++wrk;
    }
    if (a[0] == ',' || a[strlen(a)] == ',' || a[0] == ';' || a[strlen(a)] == ';') {
        return 0;
    }

    return 1;
}

char* uda::common::convert_non_printable(char* str)
{
    // Remove CR & LF Characters from a Number List

    char* ret = str;
    while (*str != '\0') {
        if (!isalpha(*str) && !isdigit(*str) && *str != ' ' && *str != '.' && *str != '+' && *str != '-') {
            *str = ' ';
        }
        ++str;
    }
    return ret;
}

char* uda::common::convert_non_printable2(char* str)
{
    // Remove NonPrintable Characters from a String

    char* ret = str;
    while (*str != '\0') {
        if (*str < ' ' || *str > '~') {
            *str = ' ';
        }
        ++str;
    }
    return ret;
}

int uda::common::is_legal_file_path(const char* str)
{
    // Basic check that the filename complies with good naming practice - some protection against malign embedded code!
    // Test against the Portable Filename Character Set A-Z, a-z, 0-9, <period>, <underscore> and <hyphen> and <plus>
    // Include <space> and back-slash for windows filenames only, forward-slash for the path seperator and $ for
    // environment variables

    // The API source argument can also be a server based source containing a ':' character
    // The delimiter characters separating the device or format name from the source should have been split off of the
    // path
    //

    const char* tst = str;
    while (*tst != '\0') {
        if ((*tst >= '0' && *tst <= '9') || (*tst >= 'A' && *tst <= 'Z') || (*tst >= 'a' && *tst <= 'z')) {
            tst++;
            continue;
        }

        if (strchr("_-+./$:", *tst) != nullptr) {
            tst++;
            continue;
        }

#ifdef _WIN32
        if (*tst == ' ' || *tst == '\\') {
            tst++;
            continue;
        }
#endif
        return 0; // Error - not compliant!
    }
    return 1;
}

#if !defined(asprintf)

/*
 * Allocating sprintf
 */
#  ifdef __APPLE__
int asprintf(char** strp, const char* fmt, ...)
#  else
int asprintf(char** strp, const char* fmt, ...) noexcept
#  endif
{
    va_list vargs;
    va_start(vargs, fmt);

    size_t len = vsnprintf(nullptr, 0, fmt, vargs) + 1;

    va_end(vargs);
    va_start(vargs, fmt);

    *strp = (char*)malloc(len * sizeof(char));
    len = vsnprintf(*strp, len, fmt, vargs);

    va_end(vargs);
    return (int)len;
}

#endif

/**
 * Split a string using the given deliminator and return a list of the resultant tokens, with nullptr indicating the end
 * of the list.
 *
 * The returned list should be freed after use with free_split_string_tokens().
 * @param string
 * @param delims
 * @return
 */
#ifdef _WIN32
#  define strtok_r strtok_s
#  define strdup _strdup
#endif

char** uda::common::split_string(const char* string, const char* delims)
{
    char** names = nullptr;
    char* tokptr = nullptr;
    size_t num_names = 0;

    char* temp = strdup(string);
    char* tok = strtok_r(temp, delims, &tokptr);
    while (tok != nullptr) {
        num_names++;
        names = (char**)realloc((void*)names, num_names * sizeof(char*));
        names[num_names - 1] = strdup(tok);
        tok = strtok_r(nullptr, delims, &tokptr);
    }

    num_names++;
    names = (char**)realloc((void*)names, num_names * sizeof(char*));
    names[num_names - 1] = nullptr;

    free(temp);
    return names;
}

char* uda::common::string_replace_all(const char* string, const char* find, const char* replace)
{
    char* prev_string = nullptr;
    char* new_string = strdup(string);

    do {
        free(prev_string);
        prev_string = new_string;
        new_string = string_replace(prev_string, find, replace);
    } while (!string_equals(prev_string, new_string));

    free(prev_string);
    return new_string;
}

char* uda::common::string_replace(const char* string, const char* find, const char* replace)
{
    if (find == nullptr || find[0] == '\0') {
        return strdup(string);
    }

    const char* idx = strstr(string, find);

    if (idx != nullptr) {
        int diff = strlen(replace) - strlen(find);
        size_t len = strlen(string) + diff + 1;
        char* result = (char*)malloc(len);
        size_t offset = idx - string;
        strncpy(result, string, offset);
        strcpy(result + offset, replace);
        strcpy(result + offset + strlen(replace), idx + strlen(find));
        result[len - 1] = '\0';
        return result;
    } else {
        return strdup(string);
    }
}

/**
 * Free the token list generated by split_string().
 * @param tokens
 */
void uda::common::free_split_string_tokens(char*** tokens)
{
    size_t i = 0;
    while ((*tokens)[i] != nullptr) {
        free((*tokens)[i]);
        ++i;
    }
    free(*tokens);
    *tokens = nullptr;
}

bool uda::common::string_equals(const char* a, const char* b)
{
    if (a == nullptr || b == nullptr) {
        return false;
    }

    while (*a != '\0') {
        if (*b == '\0' || *a != *b) {
            return false;
        }
        ++a;
        ++b;
    }

    return *a == *b;
}

bool uda::common::string_iequals(const char* a, const char* b)
{
    if (a == nullptr || b == nullptr) {
        return false;
    }

    while (*a != '\0') {
        if (*b == '\0' || toupper(*a) != toupper(*b)) {
            return false;
        }
        ++a;
        ++b;
    }

    return *a == *b;
}

bool uda::common::string_ends_with(const char* str, const char* find)
{
    if (str == nullptr || find == nullptr) {
        return false;
    }

    size_t len = strlen(str);
    size_t find_len = strlen(find);

    const char* a = str + len;
    const char* b = find + find_len;

    if (find_len > len) {
        return false;
    }

    size_t count = 0;
    while (count <= find_len) {
        if (*a != *b) {
            return false;
        }
        --a;
        --b;
        ++count;
    }

    return true;
}
