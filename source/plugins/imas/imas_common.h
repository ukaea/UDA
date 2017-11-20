#ifndef UDA_PLUGIN_IMAS_IMAS_COMMON_H
#define UDA_PLUGIN_IMAS_IMAS_COMMON_H

#define UNKNOWN_TYPE    0
#define STRING          1        // Must be the same as the client for consistency
#define INT             2
#define FLOAT           3
#define DOUBLE          4
#define STRING_VECTOR   5

int findIMASType(const char* typeName);

int findIMASIDAMType(int type);

#endif // UDA_PLUGIN_IMAS_IMAS_COMMON_H
