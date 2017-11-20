#include "imas_common.h"

#include <stdlib.h>
#include <string.h>

#include <clientserver/udaTypes.h>

static char imasIdsVersion[256] = "";            // The IDS Data Model Version
static char imasIdsDevice[256] = "";            // The IDS Data Model Device name

void putImasIdsVersion(const char* version)
{ strcpy(imasIdsVersion, version); }

const char* getImasIdsVersion()
{ return imasIdsVersion; }

void putImasIdsDevice(const char* device)
{ strcpy(imasIdsDevice, device); }

const char* getImasIdsDevice()
{ return imasIdsDevice; }

static int sliceIdx1;
static int sliceIdx2;
static double sliceTime1;
static double sliceTime2;

void setSliceIdx(int idx1, int idx2)
{
    sliceIdx1 = idx1;
    sliceIdx2 = idx2;
}

int getSliceIdx1()
{
    return sliceIdx1;
}

int getSliceIdx2()
{
    return sliceIdx2;
}

void setSliceTime(double time1, double time2)
{
    sliceTime1 = time1;
    sliceTime2 = time1;
}

double getSliceTime1()
{
    return sliceTime1;
}

double getSliceTime2()
{
    return sliceTime2;
}

/**
 *  * Convert name to IMAS type
 *   * @param typeName
 *    * @return
 *     */
int findIMASType(const char* typeName)
{
    if (typeName == NULL)                   return UNKNOWN_TYPE;
    if (StringIEquals(typeName, "int"))     return INT;
    if (StringIEquals(typeName, "float"))   return FLOAT;
    if (StringIEquals(typeName, "double"))  return DOUBLE;
    if (StringIEquals(typeName, "string"))  return STRING;
    return UNKNOWN_TYPE;
}

/**
 *  * Convert IMAS type to IDAM type
 *   * @param type
 *    * @return
 *     */
int findIMASIDAMType(int type)
{
    switch (type) {
        case INT:           return UDA_TYPE_INT;
        case FLOAT:         return UDA_TYPE_FLOAT;
        case DOUBLE:        return UDA_TYPE_DOUBLE;
        case STRING:        return UDA_TYPE_STRING;
        case STRING_VECTOR: return UDA_TYPE_STRING;
        default:            return UDA_TYPE_UNKNOWN;
    }
    return UDA_TYPE_UNKNOWN;
}
