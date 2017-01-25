#ifndef IDAM_CLIENTSERVER_TRIMSTRING_H
#define IDAM_CLIENTSERVER_TRIMSTRING_H

#include <string.h>
#include <ctype.h>

// Reverse a String
void reverseString(const char* in, char* out);

// Copy a String subject to a Maximum length constraint
void copyString(const char* in, char* out, int maxlength);

// Trim Trailing Space Characters from a String
char* TrimString(char* szSource);

// Trim Leading Space Characters from a String
char* LeftTrimString(char* str);

// Convert all LowerCase Characters to Upper Case
char* strupr(char* a);

// Convert all UpperCase Characters to Lower Case
char* strlwr(char* a);

// Trim Internal Space Characters from a String
char* MidTrimString(char* str);

// Is the String an Integer Number? (Simple but not exhaustive Check)
int IsNumber(char* a);

// Is the String a Simple Float Number?
int IsFloat(char* a);

// Is the String a Number List (#,#,#,#;#;#;#)?
int IsNumberList(char* a);

char* convertNonPrintable(char* str);

char* convertNonPrintable2(char* str);

int IsLegalFilePath(char* str);

int asprintf(char** strp, const char* fmt, ...);

char** SplitString(const char* string, const char* delim);

void FreeSplitStringTokens(char*** tokens);

#define STR_STARTSWITH(X, Y) !strncmp(X, Y, strlen(Y))
#define STR_ISTARTSWITH(X, Y) !strncasecmp(X, Y, strlen(Y))

#define STR_EQUALS(X, Y) !strncmp(X, Y, strlen(Y))
#define STR_IEQUALS(X, Y) !strncasecmp(X, Y, strlen(Y))

#endif // IDAM_CLIENTSERVER_TRIMSTRING_H

