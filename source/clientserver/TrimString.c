#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "TrimString.h"

// Reverse a String

void reverseString(const char* in, char* out)
{
    int i, lstr = (int) strlen(in);
    out[lstr] = '\0';
    for (i = 0; i < lstr; i++) out[i] = in[lstr - 1 - i];
}

// Copy a String subject to a Maximum length constraint

void copyString(const char* in, char* out, int maxlength)
{
    int lstr = (int) strlen(in);
    if (lstr < maxlength)
        strcpy(out, in);
    else {
        strncpy(out, in, maxlength - 1);
        out[maxlength - 1] = '\0';
    }
}

// Trim Trailing Space Characters from a String

char* TrimString(char* szSource)
{
    char* pszEOS;

    /*Set pointer to end of string to point to the character just
     *before the 0 at the end of the string.
     */
    pszEOS = szSource + strlen(szSource) - 1;

    while (pszEOS >= szSource && *pszEOS == ' ')
        *pszEOS-- = '\0';

    return szSource;
}

// Trim Leading Space Characters from a String

char* LeftTrimString(char* str)
{
    int i = 0, trim = 0, lstr;

    lstr = (int) strlen(str);

    while (str[trim] == ' ' && i++ <= lstr) trim++;

    if (trim > 0) {
        lstr = lstr - trim;
        for (i = 0; i < lstr; i++) str[i] = str[i + trim];
        str[lstr] = '\0';
    }
    return str;
}


// Convert all LowerCase Characters to Upper Case

char* strupr(char* a)
{
    char* ret = a;

    while (*a != '\0') {
        if (islower (*a))
            *a = toupper(*a);
        ++a;
    }

    return ret;
}


// Convert all UpperCase Characters to Lower Case

char* strlwr(char* a)
{
    char* ret = a;

    while (*a != '\0') {
        if (isupper (*a))
            *a = tolower(*a);
        ++a;
    }

    return ret;
}

// Trim Internal Space Characters from a String

char* MidTrimString(char* str)
{
    int i = 0, j = 0, lstr;
    lstr = (int) strlen(str);
    for (i = 0; i < lstr; i++) {
        if (str[i] != ' ') str[j++] = str[i];
    }
    str[j] = '\0';
    return (str);
}

// Is the String an Integer Number? (Simple but not exhaustive Check)

int IsNumber(char* a)
{
    char* wrk = a;
    while (*wrk != '\0') {
        if (!isdigit (*wrk) && *wrk != '-' && *wrk != '+') return 0;
        ++wrk;
    }
    return 1;
}

// Is the String a Simple Float Number?

int IsFloat(char* a)
{
    char* wrk = a;
    while (*wrk != '\0') {
        if (!isdigit (*wrk) && *wrk != '-' && *wrk != '+' && *wrk != '.') return 0;
        ++wrk;
    }
    return 1;
}


// Is the String a Number List (#,#,#,#;#;#;#)?

int IsNumberList(char* a)
{
    char* wrk = a;
    while (*wrk != '\0') {
        if (!isdigit (*wrk) || *wrk != ',' || *wrk != ';') return 0;
        ++wrk;
    }
    if (a[0] == ',' || a[strlen(a)] == ',' ||
        a[0] == ';' || a[strlen(a)] == ';')
        return 0;

    return 1;
}

char* convertNonPrintable(char* str)
{

// Remove CR & LF Characters from a Number List

    char* ret = str;
    while (*str != '\0') {
        if (!isalpha (*str) && !isdigit(*str)
            && *str != ' ' && *str != '.' && *str != '+' && *str != '-')
            *str = ' ';
        ++str;
    }
    return ret;
}

char* convertNonPrintable2(char* str)
{

// Remove NonPrintable Characters from a String

    char* ret = str;
    while (*str != '\0') {
        if (*str < ' ' || *str > '~') *str = ' ';
        ++str;
    }
    return ret;
}

int IsLegalFilePath(char* str)
{

// Basic check that the filename complies with good naming practice - some protection against malign embedded code!
// Test against the Portable Filename Character Set A-Z, a-z, 0-9, <period>, <underscore> and <hyphen> and <plus>
// Include <space> and back-slash for windows filenames only, forward-slash for the path seperator and $ for environment variables

// The API source argument can also be a server based source containing a ':' character
// The delimiter characters separating the device or format name from the source should have been split off of the path
//

    char* tst = str;
    while (*tst != '\0') {
        if ((*tst >= '0' && *tst <= '9') || (*tst >= 'A' && *tst <= 'Z') || (*tst >= 'a' && *tst <= 'z')) {
            tst++;
            continue;
        }

        if (strchr("_-+./$:", *tst) != NULL) {
            tst++;
            continue;
        }

#ifdef _WIN32
        if( *tst == ' ' || (pathName && *tst == '\\')) {
            tst++;
            continue;
        }
#endif
        return 0;    // Error - not compliant!
    }
    return 1;
}

#if !defined(asprintf)
/*
 * Allocating sprintf
 */
int asprintf(char** strp, const char* fmt, ...)
{
    va_list vargs;
    va_start(vargs, fmt);

    int len = vsnprintf(NULL, 0, fmt, vargs);
    *strp = malloc(len * sizeof(char));
    len = vsnprintf(*strp, (size_t)len, fmt, vargs);

    va_end(vargs);
    return len;
}
#endif

char** SplitString(const char* string, const char* delims)
{
    char** names = NULL;
    char* tokptr = NULL;
    size_t num_names = 0;

    char* temp = strdup(string);
    char* tok = strtok_r(temp, delims, &tokptr);
    while (tok != NULL) {
        num_names++;
        names = (char**)realloc((void*)names, num_names * sizeof(char*));
        names[num_names - 1] = strdup(tok);
        tok = strtok_r(NULL, delims, &tokptr);
    }

    num_names++;
    names = (char**)realloc((void*)names, num_names * sizeof(char*));
    names[num_names - 1] = NULL;

    free(temp);
    return names;
}

void FreeSplitStringTokens(char*** tokens)
{
    size_t i = 0;
    while ((*tokens)[i] != NULL) {
        free((*tokens)[i]);
        ++i;
    }
    free(*tokens);
    *tokens = NULL;
}
