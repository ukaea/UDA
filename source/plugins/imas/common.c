// TODO: Use in netcdf4 PUT plugin and build with macro NETCDF4BUILD 

#include "common.h"

#include <stddef.h>
#include <string.h>
#include <malloc.h>

#include <clientserver/TrimString.h>
#include <clientserver/idamTypes.h>
#include <clientserver/idamErrorLog.h>
#include <include/idamclientserverprivate.h>
#include <logging/idamLog.h>

// Extract list
// Patterns:  comma separated substrings - sub-strings defined by matching quotes pair
// item
// item, item, item
// 'item', 'item', 'it,em', 'it "e", m', 'ite e m' 

void getIdamNameValuePairItemList(char* list, char*** itemList, unsigned short* count, char quote,
                                  char delimiter) {

    unsigned int i, length;
    *count = 0;
    *itemList = NULL;

    if (list == NULL || list[0] == '\0') return;

// Copy the original and replace delimiter characters within sub-strings to avoid parse error
// Sub-strings defined by matching quote pair

    char* work = NULL;
    unsigned short state = 0;

    length = strlen(list);
    work = (char*) malloc((length + 1) * sizeof(char));
    strcpy(work, list);

    if (quote != ' ') {                // No quotes used to group list members so skip
        for (i = 0; i < length; i++) {
            if (work[i] == quote && !state) {
                state = (unsigned short) list[i];    // Opening quote
                continue;
            }
            if (work[i] == quote && state == quote) {
                state = 0;                // Closing matched quote
                continue;
            }
            if (state != 0 && list[i] == delimiter)
                work[i] = ' ';        // Replace embedded (ignorable) delimiter in work string
        }
    }

// How many items? Scan the list and count

    char* p = NULL, * p0 = NULL;

    *count = 1;
    p0 = work;

    while ((p = strchr(p0, delimiter)) != NULL) {        // Comma separated list
        (*count)++;
        p0 = &p[1];
    }

// String Splits

    unsigned short* splits = (unsigned short*) malloc((*count + 1) * sizeof(unsigned short));

    p0 = work;
    i = 0;
    while ((p = strchr(p0, delimiter)) != NULL) {

//fprintf(stdout,"[%d] %d\n", i, p - work + 1);   

        splits[i++] = p - work;
        p[0] = '\0';
        p0 = &p[1];
    }

    strcpy(work, list);                    // Work on the original

// Allocate sub-string array

    *itemList = (char**) malloc(*count * sizeof(char*));

// Construct the list  

    p = work;
    for (i = 1; i < *count; i++) {
        work[splits[i - 1]] = '\0';                // Split the string

        (*itemList)[i - 1] = (char*) malloc((strlen(p) + 1) * sizeof(char));
        strcpy((*itemList)[i - 1], p);

//fprintf(stdout,"[%d] %s\n", i-1, p);  

        p = &work[splits[i - 1] + 1];
    }
    (*itemList)[*count - 1] = (char*) malloc((strlen(p) + 1) * sizeof(char));
    strcpy((*itemList)[*count - 1], p);

//fprintf(stdout,"[%d] %s\n", *count-1, p);

// Remove leading and trailing quotes

    if (quote != ' ') {
        for (i = 0; i < *count; i++) {
            p0 = (*itemList)[i];
            if ((p = strchr(p0, quote)) != NULL) p[0] = ' ';
            if ((p = strrchr(p0, quote)) != NULL) p[0] = ' ';
            TrimString((*itemList)[i]);
            LeftTrimString((*itemList)[i]);

//fprintf(stdout,"T[%d] %s\n", i, p0);

        }
    }

    if (splits != NULL) free((void*) splits);
    if (work != NULL) free((void*) work);

    return;
}

// Free file list

void freeIdamNameValuePairItemList(char*** list, unsigned short count) {
    unsigned int i;
    for (i = 0; i < count; i++) if ((*list)[i] != NULL) free((void*) (*list)[i]);
    free((void*) *list);
    *list = NULL;
}


int getIdamNameValuePairVarArray(char* values, char quote, char delimiter, unsigned short varSize, int varType,
                                 void** varData) {

// Unpack 'values' sent via a name-value pair string using quote and delimiter to parse the string  
// Build and return a 'data' array ('varData') from the passed string list of 'values'
// varSize is expected size (number of elements) of the array
// varType is the IDAM TYPE
// varData is the returned data array - responsibility for freeing heap is passed also with this array
// Return the list size or a negative error code

    *varData = NULL;    // initialise the returned value

    int err = 0, i;
    unsigned short dataCount = 0;
    char** dataList = NULL;
    void* data = NULL;

// Parse the string into a list

    getIdamNameValuePairItemList(values, &dataList, &dataCount, quote, delimiter);

    if (varSize > 0 && dataCount != varSize) {        // Check counts for consistency if known in advance
        err = 999;
        idamLog(LOG_ERROR,
                "getNameValuePairVarArray: The number of values passed by argument is inconsistent with the specified Size value!\n");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "getNameValuePairVarArray", err,
                     "The number of values passed by argument is inconsistent with the specified Size value!");
        freeIdamNameValuePairItemList(&dataList, dataCount);
        return -err;
    }

// Create the data array

    switch (varType) {
/*
      case TYPE_BYTE: {

	 char *d = (char *)malloc(dataCount*sizeof(char));
	 int *id = (int *)malloc(dataCount*sizeof(int));
	 for(i=0;i<dataCount;i++){
	    id[i] = (int) atoi(dataList[i]);  
	    d[i] = (char)id[i]; 
	 }
	 data = (void *)d;
	 free((void *)id);
	 break;
      }
*/
        case (TYPE_UNSIGNED_CHAR): {

            unsigned char* d = (unsigned char*) malloc(dataCount * sizeof(unsigned char));
            unsigned int* id = (unsigned int*) malloc(dataCount * sizeof(unsigned int));
            for (i = 0; i < dataCount; i++) {
                id[i] = (int) atoi(dataList[i]);
                d[i] = (unsigned char) id[i];
            }
            data = (void*) d;
            free((void*) id);
            break;
        }

        case (TYPE_CHAR): {

            char* d = (char*) malloc(dataCount * sizeof(char));
            for (i = 0; i < dataCount; i++) d[i] = (char) (dataList[i])[0];
            data = (void*) d;
            break;
        }

        case (TYPE_SHORT): {

            short* d = (short*) malloc(dataCount * sizeof(short));
            for (i = 0; i < dataCount; i++) d[i] = (short) atoi(dataList[i]);
            data = (void*) d;
            break;
        }

        case (TYPE_UNSIGNED_SHORT): {

            unsigned short* d = (unsigned short*) malloc(dataCount * sizeof(unsigned short));
            for (i = 0; i < dataCount; i++) d[i] = (unsigned short) atoi(dataList[i]);
            data = (void*) d;
            break;
        }

        case (TYPE_INT): {

            int* d = (int*) malloc(dataCount * sizeof(int));
            for (i = 0; i < dataCount; i++) d[i] = (int) atoi(dataList[i]);
            data = (void*) d;
            break;
        }

        case (TYPE_UNSIGNED_INT): {

            unsigned int* d = (unsigned int*) malloc(dataCount * sizeof(int));
            for (i = 0; i < dataCount; i++) d[i] = (unsigned int) atoi(dataList[i]);
            data = (void*) d;
            break;
        }

        case (TYPE_LONG64): {

            long long* d = (long long*) malloc(dataCount * sizeof(long long));
            for (i = 0; i < dataCount; i++) d[i] = (long long) atoi(dataList[i]);
            data = (void*) d;
            break;
        }

        case (TYPE_UNSIGNED_LONG64): {

            unsigned long long* d = (unsigned long long*) malloc(dataCount * sizeof(unsigned long long));
            for (i = 0; i < dataCount; i++) d[i] = (unsigned long long) atoi(dataList[i]);
            data = (void*) d;
            break;
        }

        case (TYPE_FLOAT): {

            float* d = (float*) malloc(dataCount * sizeof(float));
            for (i = 0; i < dataCount; i++) d[i] = (float) atof(dataList[i]);
            data = (void*) d;
            break;
        }

        case (TYPE_DOUBLE): {

            double* d = (double*) malloc(dataCount * sizeof(double));
            for (i = 0; i < dataCount; i++) d[i] = (double) atof(dataList[i]);
            data = (void*) d;
            break;
        }
        default:
            err = 999;
            idamLog(LOG_ERROR,
                    "getNameValuePairVarArray: The data type of the values passed by argument is not recognised!\n");
            addIdamError(&idamerrorstack, CODEERRORTYPE, "getNameValuePairVarArray", err,
                         "The data type of the values passed by argument is not recognised!");
            break;
    }

    freeIdamNameValuePairItemList(&dataList, dataCount);

    if (err == 0 && data == NULL) {
        err = 999;
        idamLog(LOG_ERROR,
                "getNameValuePairVarArray: Processing of the specified type of data passed by argument has not been implemented!\n");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "getNameValuePairVarArray", err,
                     "Processing of the specified type of data passed by argument has not been implemented!");
        return -err;
    }

    if (err > 0) {
        if (data != NULL) free(data);
        return -err;
    }

    *varData = (void*) data;

    return dataCount;
}

int findIdamType(char* typeName) {
    if (typeName == NULL) return (TYPE_UNDEFINED);
    if (!strcasecmp(typeName, "byte")) return TYPE_CHAR;
    if (!strcasecmp(typeName, "char")) return TYPE_CHAR;
    if (!strcasecmp(typeName, "short")) return TYPE_SHORT;
    if (!strcasecmp(typeName, "int")) return TYPE_INT;
    if (!strcasecmp(typeName, "int64")) return TYPE_LONG64;
    if (!strcasecmp(typeName, "float")) return TYPE_FLOAT;
    if (!strcasecmp(typeName, "double")) return TYPE_DOUBLE;
    if (!strcasecmp(typeName, "ubyte")) return TYPE_UNSIGNED_CHAR;
    if (!strcasecmp(typeName, "ushort")) return TYPE_UNSIGNED_SHORT;
    if (!strcasecmp(typeName, "uint")) return TYPE_UNSIGNED_INT;
    if (!strcasecmp(typeName, "uint64")) return TYPE_UNSIGNED_LONG64;
    if (!strcasecmp(typeName, "text")) return TYPE_STRING;
    if (!strcasecmp(typeName, "string")) return TYPE_STRING;
    if (!strcasecmp(typeName, "vlen")) return TYPE_VLEN;
    if (!strcasecmp(typeName, "compound")) return TYPE_COMPOUND;
    if (!strcasecmp(typeName, "opaque")) return TYPE_OPAQUE;
    if (!strcasecmp(typeName, "enum")) return TYPE_ENUM;
    return (TYPE_UNDEFINED);
}

char* convertIdam2StringType(int type) {
    switch (type) {
        case TYPE_CHAR:
            return ("char");
        case TYPE_SHORT:
            return ("short");
        case TYPE_INT:
            return ("int");
        case TYPE_LONG64:
            return ("int64");
        case TYPE_FLOAT:
            return ("float");
        case TYPE_DOUBLE:
            return ("double");
        case TYPE_UNSIGNED_CHAR:
            return ("ubyte");
        case TYPE_UNSIGNED_SHORT:
            return ("ushort");
        case TYPE_UNSIGNED_INT:
            return ("uint");
        case TYPE_UNSIGNED_LONG64:
            return ("ulong64");
            //case TYPE_VLEN:		return(NC_VLEN);
            //case TYPE_COMPOUND:	return(NC_COMPOUND);
            //case TYPE_OPAQUE:		return(NC_OPAQUE);
            //case TYPE_ENUM:		return(NC_ENUM);
        case TYPE_STRING:
            return ("string");
        default:
            return "unknown";
    }
    return "unknown";
}

int findHDF5Type(char* typeName) {
    return 0;
//    if (!strcasecmp(typeName, "byte")) return H5T_NATIVE_SCHAR;
//    if (!strcasecmp(typeName, "char")) return H5T_NATIVE_CHAR;
//    if (!strcasecmp(typeName, "short")) return H5T_NATIVE_SHORT;
//    if (!strcasecmp(typeName, "int")) return H5T_NATIVE_INT;
//    if (!strcasecmp(typeName, "int64")) return H5T_NATIVE_LLONG;
//    if (!strcasecmp(typeName, "float")) return H5T_NATIVE_FLOAT;
//    if (!strcasecmp(typeName, "double")) return H5T_NATIVE_DOUBLE;
//    if (!strcasecmp(typeName, "ubyte")) return H5T_NATIVE_UCHAR;
//    if (!strcasecmp(typeName, "ushort")) return H5T_NATIVE_USHORT;
//    if (!strcasecmp(typeName, "uint")) return H5T_NATIVE_UINT;
//    if (!strcasecmp(typeName, "uint64")) return H5T_NATIVE_ULLONG;
//    if (!strcasecmp(typeName, "text")) return H5T_C_S1;
//    if (!strcasecmp(typeName, "string")) return H5T_C_S1;
//    //if(!strcasecmp(typeName, "vlen"))     return NC_VLEN;
//    //if(!strcasecmp(typeName, "compound")) return NC_COMPOUND;
//    //if(!strcasecmp(typeName, "opaque"))   return NC_OPAQUE;
//    //if(!strcasecmp(typeName, "enum"))     return NC_ENUM;
//    return 0;
}

/*
#ifndef IdamIMASMDSPluginInclude

int convertIdam2HDF5Type(int type)
{
    switch (type) {
        case TYPE_CHAR:
            return (H5T_NATIVE_CHAR);
        case TYPE_SHORT:
            return (H5T_NATIVE_SHORT);
        case TYPE_INT:
            return (H5T_NATIVE_INT);
        case TYPE_LONG64:
            return (H5T_NATIVE_LLONG);
        case TYPE_FLOAT:
            return (H5T_NATIVE_FLOAT);
        case TYPE_DOUBLE:
            return (H5T_NATIVE_DOUBLE);
        case TYPE_UNSIGNED_CHAR:
            return (H5T_NATIVE_UCHAR);
        case TYPE_UNSIGNED_SHORT:
            return (H5T_NATIVE_USHORT);
        case TYPE_UNSIGNED_INT:
            return (H5T_NATIVE_UINT);
        case TYPE_UNSIGNED_LONG64:
            return (H5T_NATIVE_ULLONG);
            //case TYPE_VLEN:		return(NC_VLEN);
            //case TYPE_COMPOUND:	return(NC_COMPOUND);
            //case TYPE_OPAQUE:		return(NC_OPAQUE);
            //case TYPE_ENUM:		return(NC_ENUM);
        case TYPE_STRING:
            return (H5T_C_S1);
        default:
            return 0;
    }
    return 0; //NC_NAT;		What is the HDF5 NAT ?
}

#endif
*/

int sizeIdamType(int type) {
    switch (type) {
        case TYPE_CHAR:
            return (sizeof(char));
        case TYPE_SHORT:
            return (sizeof(short));
        case TYPE_INT:
            return (sizeof(int));
        case TYPE_LONG64:
            return (sizeof(long long));
        case TYPE_FLOAT:
            return (sizeof(float));
        case TYPE_DOUBLE:
            return (sizeof(double));
        case TYPE_UNSIGNED_CHAR:
            return (sizeof(unsigned char));
        case TYPE_UNSIGNED_SHORT:
            return (sizeof(unsigned short));
        case TYPE_UNSIGNED_INT:
            return (sizeof(unsigned int));
        case TYPE_UNSIGNED_LONG64:
            return (sizeof(unsigned long long));
        default:
            return 0;
    }
}
  	 
