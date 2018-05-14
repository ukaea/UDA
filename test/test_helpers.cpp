#include "test_helpers.h"

#include <cstdarg>

std::string uda::test::format(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    int len = vsnprintf(NULL, 0, fmt, args);

    char* str = (char*)malloc((len + 1) * sizeof(char));

    vsnprintf(str, len, fmt, args);
    str[len] = '\0';

    std::string result{ str };

    free(str);

    va_end(args);

    return result;
}
