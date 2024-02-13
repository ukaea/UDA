#pragma once

#include <stdio.h>

#if defined(__APPLE__) && __DARWIN_C_LEVEL < 200809L

FILE* open_memstream(char** cp, size_t* lenp);

#endif