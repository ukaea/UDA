#ifndef UDA_PLUGIN_IMAS_IMAS_COMMON_H
#define UDA_PLUGIN_IMAS_IMAS_COMMON_H

#define IMAS_UNKNOWN_TYPE    0
#define IMAS_STRING          1        // Must be the same as the client for consistency
#define IMAS_INT             2
#define IMAS_FLOAT           3
#define IMAS_DOUBLE          4
#define IMAS_STRING_VECTOR   5

int findIMASType(const char* typeName);

int findIMASIDAMType(int type);

#endif // UDA_PLUGIN_IMAS_IMAS_COMMON_H
