#ifndef NOIDAPLUGIN

#include "readIdaItem.h"

#include <stdlib.h>
#include <ida3.h>

#include <clientserver/initStructs.h>
#include <clientserver/udaTypes.h>
#include <clientserver/stringUtils.h>
#include <logging/logging.h>
#include <clientserver/errorLog.h>

#define SWAPXY

void addxmlmetastring(char** metaxml, int* lheap, char* xml, int* nxml);

void addxmlmetaarray(char** metaxml, int* lheap, char* tag, void* data, int ndata, int type, int* nxml);

void idaclasses(int class, char* label, char* axes, int* datarank, int* timeorder);

int itemType(unsigned short datpck, short typeno, int getbytes, char* type)
{
    int data_type = TYPE_UNKNOWN;

    if ((datpck & IDA_REAL) == IDA_REAL) {
        if ((datpck & IDA_D1) == IDA_D1) data_type = TYPE_FLOAT;        // IDA_VALU Ignored for Float types!
        if ((datpck & IDA_D2) == IDA_D2) data_type = TYPE_FLOAT;        // => Calibration factor is always applied
        if ((datpck & IDA_D4) == IDA_D4) data_type = TYPE_FLOAT;
        if ((datpck & IDA_D8) == IDA_D8) data_type = TYPE_DOUBLE;
    } else {
        if ((datpck & IDA_INTG & IDA_SGND) == (IDA_INTG & IDA_SGND)) {    // Signed Integer types
            if ((datpck & IDA_VALU) == IDA_VALU) {
                if ((datpck & IDA_D1) == IDA_D1) data_type = TYPE_CHAR;    // Calibration Not applied
                if ((datpck & IDA_D2) == IDA_D2) data_type = TYPE_SHORT;
                if ((datpck & IDA_D4) == IDA_D4) data_type = TYPE_INT;
                if ((datpck & IDA_D8) == IDA_D8) data_type = TYPE_LONG64;
            } else {                            // Calibration Applied
                if (typeno == 6 && STR_EQUALS(type, "scalar")) {
                    if ((datpck & IDA_D1) == IDA_D1) data_type = TYPE_CHAR;
                    if ((datpck & IDA_D2) == IDA_D2) data_type = TYPE_SHORT;
                    if ((datpck & IDA_D4) == IDA_D4) data_type = TYPE_INT;
                    if ((datpck & IDA_D8) == IDA_D8) data_type = TYPE_LONG64;
                } else {
                    if (!getbytes) {
                        if ((datpck & IDA_D1) == IDA_D1) data_type = TYPE_FLOAT;    // Cast to Float Type
                        if ((datpck & IDA_D2) == IDA_D2) data_type = TYPE_FLOAT;
                        if ((datpck & IDA_D4) == IDA_D4) data_type = TYPE_FLOAT;
                        if ((datpck & IDA_D8) == IDA_D8) data_type = TYPE_DOUBLE;
                    } else {
                        if ((datpck & IDA_D1) == IDA_D1) data_type = TYPE_CHAR;
                        if ((datpck & IDA_D2) == IDA_D2) data_type = TYPE_SHORT;
                        if ((datpck & IDA_D4) == IDA_D4) data_type = TYPE_INT;
                        if ((datpck & IDA_D8) == IDA_D8) data_type = TYPE_LONG64;
                    }
                }
            }
        } else {
            if ((datpck & IDA_INTG) == IDA_INTG) {                // Unsigned Integer types
                if ((datpck & IDA_VALU) == IDA_VALU) {
                    if ((datpck & IDA_D1) == IDA_D1) data_type = TYPE_UNSIGNED_CHAR;
                    if ((datpck & IDA_D2) == IDA_D2) data_type = TYPE_UNSIGNED_SHORT;
                    if ((datpck & IDA_D4) == IDA_D4) data_type = TYPE_UNSIGNED;
                    if ((datpck & IDA_D8) == IDA_D8) data_type = TYPE_UNSIGNED_LONG64;
                } else {
                    if (typeno == 6 && STR_EQUALS(type, "scalar")) {
                        if ((datpck & IDA_D1) == IDA_D1) data_type = TYPE_UNSIGNED_CHAR;
                        if ((datpck & IDA_D2) == IDA_D2) data_type = TYPE_UNSIGNED_SHORT;
                        if ((datpck & IDA_D4) == IDA_D4) data_type = TYPE_UNSIGNED;
                        if ((datpck & IDA_D8) == IDA_D8) data_type = TYPE_UNSIGNED_LONG64;
                    } else {
                        if (!getbytes) {
                            if ((datpck & IDA_D1) == IDA_D1) data_type = TYPE_FLOAT;
                            if ((datpck & IDA_D2) == IDA_D2) data_type = TYPE_FLOAT;
                            if ((datpck & IDA_D4) == IDA_D4) data_type = TYPE_FLOAT;
                            if ((datpck & IDA_D8) == IDA_D8) data_type = TYPE_DOUBLE;
                        } else {
                            if ((datpck & IDA_D1) == IDA_D1) data_type = TYPE_UNSIGNED_CHAR;
                            if ((datpck & IDA_D2) == IDA_D2) data_type = TYPE_UNSIGNED_SHORT;
                            if ((datpck & IDA_D4) == IDA_D4) data_type = TYPE_UNSIGNED;
                            if ((datpck & IDA_D8) == IDA_D8) data_type = TYPE_UNSIGNED_LONG64;
                        }
                    }
                }
            } else {
                if ((datpck & (IDA_CHAR | IDA_D1 | IDA_VALU)) == (IDA_CHAR | IDA_D1 | IDA_VALU)) data_type = TYPE_CHAR;
            }
        }
    }

    IDAM_LOG(LOG_DEBUG, "DATPCK Tests:\n");
    IDAM_LOGF(LOG_DEBUG, "Data Aquistition Device Type = %d %s\n", typeno, type);
    IDAM_LOGF(LOG_DEBUG, "getbytes property = %d\n", getbytes);
    IDAM_LOGF(LOG_DEBUG, "IDA_D1?   %d\n", ((datpck & IDA_D1) == IDA_D1));
    IDAM_LOGF(LOG_DEBUG, "IDA_D2?   %d\n", ((datpck & IDA_D2) == IDA_D2));
    IDAM_LOGF(LOG_DEBUG, "IDA_D4?   %d\n", ((datpck & IDA_D4) == IDA_D4));
    IDAM_LOGF(LOG_DEBUG, "IDA_D8?   %d\n", ((datpck & IDA_D8) == IDA_D8));
    IDAM_LOGF(LOG_DEBUG, "IDA_INTG? %d\n", ((datpck & IDA_INTG) == IDA_INTG));
    IDAM_LOGF(LOG_DEBUG, "IDA_REAL? %d\n", ((datpck & IDA_REAL) == IDA_REAL));
    IDAM_LOGF(LOG_DEBUG, "IDA_CHAR? %d\n", ((datpck & IDA_CHAR) == IDA_CHAR));
    IDAM_LOGF(LOG_DEBUG, "IDA_VALU? %d\n", ((datpck & IDA_VALU) == IDA_VALU));
    IDAM_LOGF(LOG_DEBUG, "IDA_SGND? %d\n", ((datpck & IDA_SGND) == IDA_SGND));
    IDAM_LOGF(LOG_DEBUG, "IDA_ERRB? %d\n", ((datpck & IDA_ERRB) == IDA_ERRB));
    IDAM_LOGF(LOG_DEBUG, "IDA_ENCD? %d\n", ((datpck & IDA_ENCD) == IDA_ENCD));
    IDAM_LOGF(LOG_DEBUG, "IDA_COMP? %d\n", ((datpck & IDA_COMP) == IDA_COMP));
    IDAM_LOGF(LOG_DEBUG, "ItemType? %d\n", data_type);

    return (data_type);
}

int errorType(unsigned short datpck, short typeno, int getbytes, char* type)
{
    int data_type = TYPE_UNKNOWN;

    if ((datpck & IDA_REAL) == IDA_REAL) {
        if ((datpck & IDA_E1) == IDA_E1) data_type = TYPE_FLOAT;        // IDA_VALU Ignored!
        if ((datpck & IDA_E2) == IDA_E2) data_type = TYPE_FLOAT;
        if ((datpck & IDA_E4) == IDA_E4) data_type = TYPE_FLOAT;
        if ((datpck & IDA_E8) == IDA_E8) data_type = TYPE_DOUBLE;
    } else {
        if ((datpck & IDA_INTG & IDA_SGND) == (IDA_INTG & IDA_SGND)) {
            if ((datpck & IDA_VALU) == IDA_VALU) {
                if ((datpck & IDA_E1) == IDA_E1)
                    data_type = TYPE_CHAR;    // Calibration Not applied => Genuine Signed Integer types
                if ((datpck & IDA_E2) == IDA_E2) data_type = TYPE_SHORT;
                if ((datpck & IDA_E4) == IDA_E4) data_type = TYPE_INT;
                if ((datpck & IDA_E8) == IDA_E8) data_type = TYPE_LONG64;
            } else {
                if (typeno == 6 && STR_EQUALS(type, "scalar")) {
// 31Oct08     if((datpck & IDA_E1) == IDA_E1) data_type = TYPE_SHORT;
                    if ((datpck & IDA_E1) == IDA_E1) data_type = TYPE_CHAR;
                    if ((datpck & IDA_E2) == IDA_E2) data_type = TYPE_SHORT;
                    if ((datpck & IDA_E4) == IDA_E4) data_type = TYPE_INT;
                    if ((datpck & IDA_E8) == IDA_E8) data_type = TYPE_LONG64;
                } else {
                    if (!getbytes) {
                        if ((datpck & IDA_E1) == IDA_E1) data_type = TYPE_FLOAT;
                        if ((datpck & IDA_E2) == IDA_E2) data_type = TYPE_FLOAT;
                        if ((datpck & IDA_E4) == IDA_E4) data_type = TYPE_FLOAT;
                        if ((datpck & IDA_E8) == IDA_E8) data_type = TYPE_DOUBLE;
                    } else {
                        if ((datpck & IDA_E1) == IDA_E1) data_type = TYPE_CHAR;
                        if ((datpck & IDA_E2) == IDA_E2) data_type = TYPE_SHORT;
                        if ((datpck & IDA_E4) == IDA_E4) data_type = TYPE_INT;
                        if ((datpck & IDA_E8) == IDA_E8) data_type = TYPE_LONG64;
                    }
                }
            }
        } else {
            if ((datpck & IDA_INTG) == IDA_INTG) {
                if ((datpck & IDA_VALU) == IDA_VALU) {
                    if ((datpck & IDA_E1) == IDA_E1)
                        data_type = TYPE_UNSIGNED_CHAR;    // Calibration Not applied => Genuine Unsigned Integer types
                    if ((datpck & IDA_E2) == IDA_E2) data_type = TYPE_UNSIGNED_SHORT;
                    if ((datpck & IDA_E4) == IDA_E4) data_type = TYPE_UNSIGNED;
                    if ((datpck & IDA_E8) == IDA_E8) data_type = TYPE_UNSIGNED_LONG64;
                } else {
                    if (typeno == 6 && STR_EQUALS(type, "scalar")) {
                        if ((datpck & IDA_E1) == IDA_E1) data_type = TYPE_UNSIGNED_CHAR;
                        if ((datpck & IDA_E2) == IDA_E2) data_type = TYPE_UNSIGNED_SHORT;
                        if ((datpck & IDA_E4) == IDA_E4) data_type = TYPE_UNSIGNED;
                        if ((datpck & IDA_E8) == IDA_E8) data_type = TYPE_UNSIGNED_LONG64;
                    } else {
                        if (!getbytes) {
                            if ((datpck & IDA_E1) == IDA_E1) data_type = TYPE_FLOAT;
                            if ((datpck & IDA_E2) == IDA_E2) data_type = TYPE_FLOAT;
                            if ((datpck & IDA_E4) == IDA_E4) data_type = TYPE_FLOAT;
                            if ((datpck & IDA_E8) == IDA_E8) data_type = TYPE_DOUBLE;
                        } else {
                            if ((datpck & IDA_E1) == IDA_E1) data_type = TYPE_UNSIGNED_CHAR;
                            if ((datpck & IDA_E2) == IDA_E2) data_type = TYPE_UNSIGNED_SHORT;
                            if ((datpck & IDA_E4) == IDA_E4) data_type = TYPE_UNSIGNED;
                            if ((datpck & IDA_E8) == IDA_E8) data_type = TYPE_UNSIGNED_LONG64;
                        }
                    }
                }
            } else {
                if ((datpck & (IDA_CHAR | IDA_E1 | IDA_VALU)) == (IDA_CHAR | IDA_E1 | IDA_VALU)) data_type = TYPE_CHAR;
            }
        }
    }

    IDAM_LOG(LOG_DEBUG, "DATPCK Tests:\n");
    IDAM_LOGF(LOG_DEBUG, "Data Aquistition Device Type = %d %s\n", typeno, type);
    IDAM_LOGF(LOG_DEBUG, "getbytes property = %d\n", getbytes);
    IDAM_LOGF(LOG_DEBUG, "IDA_E1?   %d\n", ((datpck & IDA_E1) == IDA_E1));
    IDAM_LOGF(LOG_DEBUG, "IDA_E2?   %d\n", ((datpck & IDA_E2) == IDA_E2));
    IDAM_LOGF(LOG_DEBUG, "IDA_E4?   %d\n", ((datpck & IDA_E4) == IDA_E4));
    IDAM_LOGF(LOG_DEBUG, "IDA_E8?   %d\n", ((datpck & IDA_E8) == IDA_E8));
    IDAM_LOGF(LOG_DEBUG, "IDA_INTG? %d\n", ((datpck & IDA_INTG) == IDA_INTG));
    IDAM_LOGF(LOG_DEBUG, "IDA_REAL? %d\n", ((datpck & IDA_REAL) == IDA_REAL));
    IDAM_LOGF(LOG_DEBUG, "IDA_CHAR? %d\n", ((datpck & IDA_CHAR) == IDA_CHAR));
    IDAM_LOGF(LOG_DEBUG, "IDA_VALU? %d\n", ((datpck & IDA_VALU) == IDA_VALU));
    IDAM_LOGF(LOG_DEBUG, "IDA_SGND? %d\n", ((datpck & IDA_SGND) == IDA_SGND));
    IDAM_LOGF(LOG_DEBUG, "IDA_ERRB? %d\n", ((datpck & IDA_ERRB) == IDA_ERRB));
    IDAM_LOGF(LOG_DEBUG, "IDA_ENCD? %d\n", ((datpck & IDA_ENCD) == IDA_ENCD));
    IDAM_LOGF(LOG_DEBUG, "IDA_COMP? %d\n", ((datpck & IDA_COMP) == IDA_COMP));
    IDAM_LOGF(LOG_DEBUG, "ItemType? %d\n", data_type);

    return data_type;
}


char* itemData(int data_type, int totsams)
{

// Allocate Heap for the DATA or DATA ERRORs

    char* data = NULL;
    switch (data_type) {
        case TYPE_FLOAT:
            data = (char*) calloc(totsams, sizeof(float));
            break;
        case TYPE_DOUBLE:
            data = (char*) calloc(totsams, sizeof(double));
            break;
        case TYPE_CHAR:
            data = (char*) calloc(totsams, sizeof(char));
            break;
        case TYPE_SHORT:
            data = (char*) calloc(totsams, sizeof(short));
            break;
        case TYPE_INT:
            data = (char*) calloc(totsams, sizeof(int));
            break;
        case TYPE_LONG:
            data = (char*) calloc(totsams, sizeof(long));
            break;
        case TYPE_LONG64:
            data = (char*) calloc(totsams, sizeof(long long));
            break;
        case TYPE_UNSIGNED_CHAR:
            data = (char*) calloc(totsams, sizeof(unsigned char));
            break;
        case TYPE_UNSIGNED_SHORT:
            data = (char*) calloc(totsams, sizeof(unsigned short));
            break;
        case TYPE_UNSIGNED:
            data = (char*) calloc(totsams, sizeof(unsigned int));
            break;
        case TYPE_UNSIGNED_LONG:
            data = (char*) calloc(totsams, sizeof(unsigned long));
            break;
        case TYPE_UNSIGNED_LONG64:
            data = (char*) calloc(totsams, sizeof(unsigned long long));
            break;
    }
    return data;
}

void swapRank3(DATA_BLOCK* data_block, int pattern)
{

    int i, j, k, count, offset = 0;
    int nx = data_block->dims[0].dim_n;
    int ny = data_block->dims[1].dim_n;
    int nt = data_block->dims[2].dim_n;

    count = 0;
    switch (data_block->data_type) {
        case TYPE_FLOAT: {
            float* dblock = (float*) data_block->data;
            float* data = (float*) itemData(data_block->data_type, data_block->data_n);
            if (pattern == 1) {            // Data Classes like DCXY
                for (i = 0; i < nt; i++) {
                    for (k = 0; k < nx; k++) {
                        for (j = 0; j < ny; j++) {        // want data[k][j][i]
                            offset = k + nx * (j + i * ny);
                            *(data + offset) = *(dblock + count);
                            count++;
                        }
                    }
                }
            } else {                // Data Classes like DCYX
                for (i = 0; i < nt; i++) {
                    for (j = 0; j < ny; j++) {
                        for (k = 0; k < nx; k++) {
                            offset = k + nx * (j + i * ny);
                            *(data + offset) = *(dblock + count);
                            count++;
                        }
                    }
                }
            }
            free((void*) data_block->data);
            data_block->data = (char*) data;
        }
            break;

        case TYPE_DOUBLE: {
            double* dblock = (double*) data_block->data;
            double* data = (double*) itemData(data_block->data_type, data_block->data_n);
            if (pattern == 1) {
                for (i = 0; i < nt; i++) {
                    for (k = 0; k < nx; k++) {
                        for (j = 0; j < ny; j++) {
                            offset = k + nx * (j + i * ny);
                            *(data + offset) = *(dblock + count);
                            count++;
                        }
                    }
                }
            } else {                // Data Classes like DCYX
                for (i = 0; i < nt; i++) {
                    for (j = 0; j < ny; j++) {
                        for (k = 0; k < nx; k++) {
                            offset = k + nx * (j + i * ny);
                            *(data + offset) = *(dblock + count);
                            count++;
                        }
                    }
                }
            }
            free((void*) data_block->data);
            data_block->data = (char*) data;
        }
            break;

        case TYPE_CHAR: {
            char* dblock = data_block->data;
            char* data = itemData(data_block->data_type, data_block->data_n);
            if (pattern == 1) {            // Data Classes like DCXY
                for (i = 0; i < nt; i++) {
                    for (k = 0; k < nx; k++) {
                        for (j = 0; j < ny; j++) {        // want data[k][j][i]
                            offset = k + nx * (j + i * ny);
                            *(data + offset) = *(dblock + count);
                            count++;
                        }
                    }
                }
            } else {                // Data Classes like DCYX
                for (i = 0; i < nt; i++) {
                    for (j = 0; j < ny; j++) {
                        for (k = 0; k < nx; k++) {
                            offset = k + nx * (j + i * ny);
                            *(data + offset) = *(dblock + count);
                            count++;
                        }
                    }
                }
            }
            free((void*) data_block->data);
            data_block->data = (char*) data;
        }
            break;

        case TYPE_SHORT: {
            short* dblock = (short*) data_block->data;
            short* data = (short*) itemData(data_block->data_type, data_block->data_n);
            if (pattern == 1) {            // Data Classes like DCXY
                for (i = 0; i < nt; i++) {
                    for (k = 0; k < nx; k++) {
                        for (j = 0; j < ny; j++) {        // want data[k][j][i]
                            offset = k + nx * (j + i * ny);
                            *(data + offset) = *(dblock + count);
                            count++;
                        }
                    }
                }
            } else {                // Data Classes like DCYX
                for (i = 0; i < nt; i++) {
                    for (j = 0; j < ny; j++) {
                        for (k = 0; k < nx; k++) {
                            offset = k + nx * (j + i * ny);
                            *(data + offset) = *(dblock + count);
                            count++;
                        }
                    }
                }
            }
            free((void*) data_block->data);
            data_block->data = (char*) data;
        }
            break;

        case TYPE_INT: {
            int* dblock = (int*) data_block->data;
            int* data = (int*) itemData(data_block->data_type, data_block->data_n);
            if (pattern == 1) {            // Data Classes like DCXY
                for (i = 0; i < nt; i++) {
                    for (k = 0; k < nx; k++) {
                        for (j = 0; j < ny; j++) {        // want data[k][j][i]
                            offset = k + nx * (j + i * ny);
                            *(data + offset) = *(dblock + count);
                            count++;
                        }
                    }
                }
            } else {                // Data Classes like DCYX
                for (i = 0; i < nt; i++) {
                    for (j = 0; j < ny; j++) {
                        for (k = 0; k < nx; k++) {
                            offset = k + nx * (j + i * ny);
                            *(data + offset) = *(dblock + count);
                            count++;
                        }
                    }
                }
            }
            free((void*) data_block->data);
            data_block->data = (char*) data;
        }
            break;

        case TYPE_LONG: {
            long* dblock = (long*) data_block->data;
            long* data = (long*) itemData(data_block->data_type, data_block->data_n);
            if (pattern == 1) {            // Data Classes like DCXY
                for (i = 0; i < nt; i++) {
                    for (k = 0; k < nx; k++) {
                        for (j = 0; j < ny; j++) {        // want data[k][j][i]
                            offset = k + nx * (j + i * ny);
                            *(data + offset) = *(dblock + count);
                            count++;
                        }
                    }
                }
            } else {                // Data Classes like DCYX
                for (i = 0; i < nt; i++) {
                    for (j = 0; j < ny; j++) {
                        for (k = 0; k < nx; k++) {
                            offset = k + nx * (j + i * ny);
                            *(data + offset) = *(dblock + count);
                            count++;
                        }
                    }
                }
            }
            free((void*) data_block->data);
            data_block->data = (char*) data;
        }
            break;

        case TYPE_LONG64: {
            long long* dblock = (long long*) data_block->data;
            long long* data = (long long*) itemData(data_block->data_type, data_block->data_n);
            if (pattern == 1) {            // Data Classes like DCXY
                for (i = 0; i < nt; i++) {
                    for (k = 0; k < nx; k++) {
                        for (j = 0; j < ny; j++) {        // want data[k][j][i]
                            offset = k + nx * (j + i * ny);
                            *(data + offset) = *(dblock + count);
                            count++;
                        }
                    }
                }
            } else {                // Data Classes like DCYX
                for (i = 0; i < nt; i++) {
                    for (j = 0; j < ny; j++) {
                        for (k = 0; k < nx; k++) {
                            offset = k + nx * (j + i * ny);
                            *(data + offset) = *(dblock + count);
                            count++;
                        }
                    }
                }
            }
            free((void*) data_block->data);
            data_block->data = (char*) data;
        }
            break;

        case TYPE_UNSIGNED_CHAR: {
            unsigned char* dblock = (unsigned char*) data_block->data;
            unsigned char* data = (unsigned char*) itemData(data_block->data_type, data_block->data_n);
            if (pattern == 1) {            // Data Classes like DCXY
                for (i = 0; i < nt; i++) {
                    for (k = 0; k < nx; k++) {
                        for (j = 0; j < ny; j++) {        // want data[k][j][i]
                            offset = k + nx * (j + i * ny);
                            *(data + offset) = *(dblock + count);
                            count++;
                        }
                    }
                }
            } else {                // Data Classes like DCYX
                for (i = 0; i < nt; i++) {
                    for (j = 0; j < ny; j++) {
                        for (k = 0; k < nx; k++) {
                            offset = k + nx * (j + i * ny);
                            *(data + offset) = *(dblock + count);
                            count++;
                        }
                    }
                }
            }
            free((void*) data_block->data);
            data_block->data = (char*) data;
        }
            break;

        case TYPE_UNSIGNED_SHORT: {
            unsigned short* dblock = (unsigned short*) data_block->data;
            unsigned short* data = (unsigned short*) itemData(data_block->data_type, data_block->data_n);
            if (pattern == 1) {            // Data Classes like DCXY
                for (i = 0; i < nt; i++) {
                    for (k = 0; k < nx; k++) {
                        for (j = 0; j < ny; j++) {        // want data[k][j][i]
                            offset = k + nx * (j + i * ny);
                            *(data + offset) = *(dblock + count);
                            count++;
                        }
                    }
                }
            } else {                // Data Classes like DCYX
                for (i = 0; i < nt; i++) {
                    for (j = 0; j < ny; j++) {
                        for (k = 0; k < nx; k++) {
                            offset = k + nx * (j + i * ny);
                            *(data + offset) = *(dblock + count);
                            count++;
                        }
                    }
                }
            }
            free((void*) data_block->data);
            data_block->data = (char*) data;
        }
            break;

        case TYPE_UNSIGNED: {
            unsigned int* dblock = (unsigned int*) data_block->data;
            unsigned int* data = (unsigned int*) itemData(data_block->data_type, data_block->data_n);
            if (pattern == 1) {            // Data Classes like DCXY
                for (i = 0; i < nt; i++) {
                    for (k = 0; k < nx; k++) {
                        for (j = 0; j < ny; j++) {        // want data[k][j][i]
                            offset = k + nx * (j + i * ny);
                            *(data + offset) = *(dblock + count);
                            count++;
                        }
                    }
                }
            } else {                // Data Classes like DCYX
                for (i = 0; i < nt; i++) {
                    for (j = 0; j < ny; j++) {
                        for (k = 0; k < nx; k++) {
                            offset = k + nx * (j + i * ny);
                            *(data + offset) = *(dblock + count);
                            count++;
                        }
                    }
                }
            }
            free((void*) data_block->data);
            data_block->data = (char*) data;
        }
            break;

        case TYPE_UNSIGNED_LONG: {
            unsigned long* dblock = (unsigned long*) data_block->data;
            unsigned long* data = (unsigned long*) itemData(data_block->data_type, data_block->data_n);
            if (pattern == 1) {            // Data Classes like DCXY
                for (i = 0; i < nt; i++) {
                    for (k = 0; k < nx; k++) {
                        for (j = 0; j < ny; j++) {        // want data[k][j][i]
                            offset = k + nx * (j + i * ny);
                            *(data + offset) = *(dblock + count);
                            count++;
                        }
                    }
                }
            } else {                // Data Classes like DCYX
                for (i = 0; i < nt; i++) {
                    for (j = 0; j < ny; j++) {
                        for (k = 0; k < nx; k++) {
                            offset = k + nx * (j + i * ny);
                            *(data + offset) = *(dblock + count);
                            count++;
                        }
                    }
                }
            }
            free((void*) data_block->data);
            data_block->data = (char*) data;
        }
            break;

        case TYPE_UNSIGNED_LONG64: {
            unsigned long long* dblock = (unsigned long long*) data_block->data;
            unsigned long long* data = (unsigned long long*) itemData(data_block->data_type, data_block->data_n);
            if (pattern == 1) {            // Data Classes like DCXY
                for (i = 0; i < nt; i++) {
                    for (k = 0; k < nx; k++) {
                        for (j = 0; j < ny; j++) {        // want data[k][j][i]
                            offset = k + nx * (j + i * ny);
                            *(data + offset) = *(dblock + count);
                            count++;
                        }
                    }
                }
            } else {                // Data Classes like DCYX
                for (i = 0; i < nt; i++) {
                    for (j = 0; j < ny; j++) {
                        for (k = 0; k < nx; k++) {
                            offset = k + nx * (j + i * ny);
                            *(data + offset) = *(dblock + count);
                            count++;
                        }
                    }
                }
            }
            free((void*) data_block->data);
            data_block->data = (char*) data;
        }
            break;

    }


    if (data_block->error_type != TYPE_UNKNOWN) {
        count = 0;
        switch (data_block->error_type) {
            case TYPE_FLOAT: {
                float* eblock = (float*) data_block->errhi;
                float* error = (float*) itemData(data_block->error_type, data_block->data_n);
                if (pattern == 1) {            // Error Data Classes like DCXY
                    for (i = 0; i < nt; i++) {
                        for (k = 0; k < nx; k++) {
                            for (j = 0; j < ny; j++) {        // want data[k][j][i]
                                offset = k + nx * (j + i * ny);
                                *(error + offset) = *(eblock + count);
                                count++;
                            }
                        }
                    }
                } else {                // Error Data Classes like DCYX
                    for (i = 0; i < nt; i++) {
                        for (j = 0; j < ny; j++) {
                            for (k = 0; k < nx; k++) {
                                offset = k + nx * (j + i * ny);
                                *(error + offset) = *(eblock + count);
                                count++;
                            }
                        }
                    }
                }
                free((void*) data_block->errhi);
                data_block->errhi = (char*) error;
            }
                break;

            case TYPE_DOUBLE: {
                double* eblock = (double*) data_block->errhi;
                double* error = (double*) itemData(data_block->error_type, data_block->data_n);
                if (pattern == 1) {            // Error Data Classes like DCXY
                    for (i = 0; i < nt; i++) {
                        for (k = 0; k < nx; k++) {
                            for (j = 0; j < ny; j++) {        // want data[k][j][i]
                                offset = k + nx * (j + i * ny);
                                *(error + offset) = *(eblock + count);
                                count++;
                            }
                        }
                    }
                } else {                // Error Data Classes like DCYX
                    for (i = 0; i < nt; i++) {
                        for (j = 0; j < ny; j++) {
                            for (k = 0; k < nx; k++) {
                                offset = k + nx * (j + i * ny);
                                *(error + offset) = *(eblock + count);
                                count++;
                            }
                        }
                    }
                }
                free((void*) data_block->errhi);
                data_block->errhi = (char*) error;
            }
                break;

            case TYPE_CHAR: {
                char* eblock = (char*) data_block->errhi;
                char* error = (char*) itemData(data_block->error_type, data_block->data_n);
                if (pattern == 1) {            // Error Data Classes like DCXY
                    for (i = 0; i < nt; i++) {
                        for (k = 0; k < nx; k++) {
                            for (j = 0; j < ny; j++) {        // want data[k][j][i]
                                offset = k + nx * (j + i * ny);
                                *(error + offset) = *(eblock + count);
                                count++;
                            }
                        }
                    }
                } else {                // Error Data Classes like DCYX
                    for (i = 0; i < nt; i++) {
                        for (j = 0; j < ny; j++) {
                            for (k = 0; k < nx; k++) {
                                offset = k + nx * (j + i * ny);
                                *(error + offset) = *(eblock + count);
                                count++;
                            }
                        }
                    }
                }
                free((void*) data_block->errhi);
                data_block->errhi = (char*) error;
            }
                break;

            case TYPE_SHORT: {
                short* eblock = (short*) data_block->errhi;
                short* error = (short*) itemData(data_block->error_type, data_block->data_n);
                if (pattern == 1) {            // Error Data Classes like DCXY
                    for (i = 0; i < nt; i++) {
                        for (k = 0; k < nx; k++) {
                            for (j = 0; j < ny; j++) {        // want data[k][j][i]
                                offset = k + nx * (j + i * ny);
                                *(error + offset) = *(eblock + count);
                                count++;
                            }
                        }
                    }
                } else {                // Error Data Classes like DCYX
                    for (i = 0; i < nt; i++) {
                        for (j = 0; j < ny; j++) {
                            for (k = 0; k < nx; k++) {
                                offset = k + nx * (j + i * ny);
                                *(error + offset) = *(eblock + count);
                                count++;
                            }
                        }
                    }
                }
                free((void*) data_block->errhi);
                data_block->errhi = (char*) error;
            }
                break;

            case TYPE_INT: {
                int* eblock = (int*) data_block->errhi;
                int* error = (int*) itemData(data_block->error_type, data_block->data_n);
                if (pattern == 1) {            // Error Data Classes like DCXY
                    for (i = 0; i < nt; i++) {
                        for (k = 0; k < nx; k++) {
                            for (j = 0; j < ny; j++) {        // want data[k][j][i]
                                offset = k + nx * (j + i * ny);
                                *(error + offset) = *(eblock + count);
                                count++;
                            }
                        }
                    }
                } else {                // Error Data Classes like DCYX
                    for (i = 0; i < nt; i++) {
                        for (j = 0; j < ny; j++) {
                            for (k = 0; k < nx; k++) {
                                offset = k + nx * (j + i * ny);
                                *(error + offset) = *(eblock + count);
                                count++;
                            }
                        }
                    }
                }
                free((void*) data_block->errhi);
                data_block->errhi = (char*) error;
            }
                break;

            case TYPE_LONG: {
                long* eblock = (long*) data_block->errhi;
                long* error = (long*) itemData(data_block->error_type, data_block->data_n);
                if (pattern == 1) {            // Error Data Classes like DCXY
                    for (i = 0; i < nt; i++) {
                        for (k = 0; k < nx; k++) {
                            for (j = 0; j < ny; j++) {        // want data[k][j][i] error_type
                                offset = k + nx * (j + i * ny);
                                *(error + offset) = *(eblock + count);
                                count++;
                            }
                        }
                    }
                } else {                // Error Data Classes like DCYX
                    for (i = 0; i < nt; i++) {
                        for (j = 0; j < ny; j++) {
                            for (k = 0; k < nx; k++) {
                                offset = k + nx * (j + i * ny);
                                *(error + offset) = *(eblock + count);
                                count++;
                            }
                        }
                    }
                }
                free((void*) data_block->errhi);
                data_block->errhi = (char*) error;
            }
                break;

            case TYPE_LONG64: {
                long long* eblock = (long long*) data_block->errhi;
                long long* error = (long long*) itemData(data_block->error_type, data_block->data_n);
                if (pattern == 1) {            // Error Data Classes like DCXY
                    for (i = 0; i < nt; i++) {
                        for (k = 0; k < nx; k++) {
                            for (j = 0; j < ny; j++) {        // want data[k][j][i]
                                offset = k + nx * (j + i * ny);
                                *(error + offset) = *(eblock + count);
                                count++;
                            }
                        }
                    }
                } else {                // Error Data Classes like DCYX
                    for (i = 0; i < nt; i++) {
                        for (j = 0; j < ny; j++) {
                            for (k = 0; k < nx; k++) {
                                offset = k + nx * (j + i * ny);
                                *(error + offset) = *(eblock + count);
                                count++;
                            }
                        }
                    }
                }
                free((void*) data_block->errhi);
                data_block->errhi = (char*) error;
            }
                break;

            case TYPE_UNSIGNED_CHAR: {
                unsigned char* eblock = (unsigned char*) data_block->errhi;
                unsigned char* error = (unsigned char*) itemData(data_block->error_type, data_block->data_n);
                if (pattern == 1) {            // Error Data Classes like DCXY
                    for (i = 0; i < nt; i++) {
                        for (k = 0; k < nx; k++) {
                            for (j = 0; j < ny; j++) {        // want data[k][j][i]
                                offset = k + nx * (j + i * ny);
                                *(error + offset) = *(eblock + count);
                                count++;
                            }
                        }
                    }
                } else {                // Error Data Classes like DCYX
                    for (i = 0; i < nt; i++) {
                        for (j = 0; j < ny; j++) {
                            for (k = 0; k < nx; k++) {
                                offset = k + nx * (j + i * ny);
                                *(error + offset) = *(eblock + count);
                                count++;
                            }
                        }
                    }
                }
                free((void*) data_block->errhi);
                data_block->errhi = (char*) error;
            }
                break;

            case TYPE_UNSIGNED_SHORT: {
                unsigned short* eblock = (unsigned short*) data_block->errhi;
                unsigned short* error = (unsigned short*) itemData(data_block->error_type, data_block->data_n);
                if (pattern == 1) {            // Error Data Classes like DCXY
                    for (i = 0; i < nt; i++) {
                        for (k = 0; k < nx; k++) {
                            for (j = 0; j < ny; j++) {        // want data[k][j][i]
                                offset = k + nx * (j + i * ny);
                                *(error + offset) = *(eblock + count);
                                count++;
                            }
                        }
                    }
                } else {                // Error Data Classes like DCYX
                    for (i = 0; i < nt; i++) {
                        for (j = 0; j < ny; j++) {
                            for (k = 0; k < nx; k++) {
                                offset = k + nx * (j + i * ny);
                                *(error + offset) = *(eblock + count);
                                count++;
                            }
                        }
                    }
                }
                free((void*) data_block->errhi);
                data_block->errhi = (char*) error;
            }
                break;

            case TYPE_UNSIGNED: {
                unsigned int* eblock = (unsigned int*) data_block->errhi;
                unsigned int* error = (unsigned int*) itemData(data_block->error_type, data_block->data_n);
                if (pattern == 1) {            // Error Data Classes like DCXY
                    for (i = 0; i < nt; i++) {
                        for (k = 0; k < nx; k++) {
                            for (j = 0; j < ny; j++) {        // want data[k][j][i]
                                offset = k + nx * (j + i * ny);
                                *(error + offset) = *(eblock + count);
                                count++;
                            }
                        }
                    }
                } else {                // Error Data Classes like DCYX
                    for (i = 0; i < nt; i++) {
                        for (j = 0; j < ny; j++) {
                            for (k = 0; k < nx; k++) {
                                offset = k + nx * (j + i * ny);
                                *(error + offset) = *(eblock + count);
                                count++;
                            }
                        }
                    }
                }
                free((void*) data_block->errhi);
                data_block->errhi = (char*) error;
            }
                break;

            case TYPE_UNSIGNED_LONG: {
                unsigned long* eblock = (unsigned long*) data_block->errhi;
                unsigned long* error = (unsigned long*) itemData(data_block->error_type, data_block->data_n);
                if (pattern == 1) {            // Error Data Classes like DCXY
                    for (i = 0; i < nt; i++) {
                        for (k = 0; k < nx; k++) {
                            for (j = 0; j < ny; j++) {        // want data[k][j][i]
                                offset = k + nx * (j + i * ny);
                                *(error + offset) = *(eblock + count);
                                count++;
                            }
                        }
                    }
                } else {                // Error Data Classes like DCYX
                    for (i = 0; i < nt; i++) {
                        for (j = 0; j < ny; j++) {
                            for (k = 0; k < nx; k++) {
                                offset = k + nx * (j + i * ny);
                                *(error + offset) = *(eblock + count);
                                count++;
                            }
                        }
                    }
                }
                free((void*) data_block->errhi);
                data_block->errhi = (char*) error;
            }
                break;

            case TYPE_UNSIGNED_LONG64: {
                unsigned long long* eblock = (unsigned long long*) data_block->errhi;
                unsigned long long* error = (unsigned long long*) itemData(data_block->error_type, data_block->data_n);
                if (pattern == 1) {            // Error Data Classes like DCXY
                    for (i = 0; i < nt; i++) {
                        for (k = 0; k < nx; k++) {
                            for (j = 0; j < ny; j++) {        // want data[k][j][i]
                                offset = k + nx * (j + i * ny);
                                *(error + offset) = *(eblock + count);
                                count++;
                            }
                        }
                    }
                } else {                // Error Data Classes like DCYX
                    for (i = 0; i < nt; i++) {
                        for (j = 0; j < ny; j++) {
                            for (k = 0; k < nx; k++) {
                                offset = k + nx * (j + i * ny);
                                *(error + offset) = *(eblock + count);
                                count++;
                            }
                        }
                    }
                }
                free((void*) data_block->errhi);
                data_block->errhi = (char*) error;
            }
                break;

        }
    }

    return;
}


void addxmlmetastring(char** metaxml, int* lheap, char* xml, int* nxml)
{
    int lstr = strlen(xml);
    if ((lstr + *nxml) >= *lheap) {
        *lheap = *lheap + 10 * STRING_LENGTH + 1;
        *metaxml = (char*) realloc((void*) *metaxml, *lheap * sizeof(char));
    }
    strcat(*metaxml, xml);
    *nxml = lstr + *nxml;
}

void addxmlmetaarray(char** metaxml, int* lheap, char* tag, void* data, int ndata, int type, int* nxml)
{
    int i, n = ndata - 1, nn = 0;
    long* ldata;
    float* fdata;
    char xml[STRING_LENGTH];
    sprintf(xml, "<%s>\n", tag);
    addxmlmetastring(metaxml, lheap, xml, nxml);
    if (type == TYPE_LONG) {
        ldata = (long*) data;
        for (i = 0; i < n; i++) {
            if (nn++ < 5) {
                sprintf(xml, "%d,", (int) ldata[i]);
            } else {
                sprintf(xml, "%d,\n", (int) ldata[i]);
                nn = 0;
            }
            addxmlmetastring(metaxml, lheap, xml, nxml);
        }
        sprintf(xml, "%d\n", (int) ldata[ndata - 1]);
        addxmlmetastring(metaxml, lheap, xml, nxml);
    } else {
        fdata = (float*) data;
        for (i = 0; i < n; i++) {
            if (nn++ < 5) {
                sprintf(xml, "%e,", fdata[i]);
            } else {
                sprintf(xml, "%e,\n", fdata[i]);
                nn = 0;
            }
            addxmlmetastring(metaxml, lheap, xml, nxml);
        }
        sprintf(xml, "%e\n", fdata[ndata - 1]);
        addxmlmetastring(metaxml, lheap, xml, nxml);
    }
    sprintf(xml, "</%s>\n", tag);
    addxmlmetastring(metaxml, lheap, xml, nxml);
}


int readIdaItem(char* itemname, ida_file_ptr* ida_file, short* context, DATA_BLOCK* data_block)
{

    char retitemname[IDA_NSIZE];
    char msg0[256] = "No IDA Signal in IDA Source File. ";
    char msg[256];

    char tunits[IDA_USIZE + 1], tlabel[IDA_LSIZE + 1];
    char xunits[IDA_USIZE + 1], xlabel[IDA_LSIZE + 1];
    char yunits[IDA_USIZE + 1], ylabel[IDA_LSIZE + 1];
    char dunits[IDA_USIZE + 1], dlabel[IDA_LSIZE + 1];
    char zunits[IDA_USIZE + 1], zlabel[IDA_LSIZE + 1];

    int i, j, rerr;

    long retshotnr, spaceused;
    long nt, nx, ny, * xsams, * tsams, maxnx;
    long* dxtsam1, * xtsams, totsams;

    unsigned short ysams, nz, z0;
    unsigned short dclass, nodoms, datpck, calib, flags, maxs, udoms;

    float* toff = NULL, * tint = NULL, * tmax = NULL;
    float* xoff = NULL, * xint = NULL, * xmax = NULL;
    float* yoff = NULL, * yint = NULL, * ymax = NULL;
    float* zval = NULL;

    float devoff, devrng, calfac, caloff;
    short devres;

    char* data = NULL, * error = NULL;

    short locn;             // Data Aquisition Device Location
    short chan;             // Data Aquisition Device Channel Number
    short typeno;           // Data Aquisition Device Type
    char type[IDA_DSIZE];   // Data Aquisition Device Type

// IDA3 specific variables

    ida_err err;
    ida_data_ptr* item;

// Client Properties and Meta data XML

    char* metaxml = NULL;                    // IDA File Data Attributes
    char xml[STRING_LENGTH];                // Small Text Buffer
    int lheap, nxml;

    int getbytes = data_block->client_block.get_bytes;    // Return RAW Data Only with XML attributes

    char classlabel[MAXNAME];
    char axesorder[4];
    int datarank, timeorder;

//-------------------------------------------------------------------------
// Check there is an IDA File pointer

    if (ida_file == NULL) return -1;

//-------------------------------------------------------------------------
// Start of Error Trap Loop

    do {

        rerr = 0;

//-------------------------------------------------------------------------
// Get the IDA signal data pointer

        item = ida_find(ida_file, itemname, 0, context);

        err = ida_error(ida_file);
        if (CDAS_ERROR(err)) {
            ida_error_mess(err, msg);
            strcat(msg0, msg);
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readIdaItem", 5, msg0);
            rerr = -5;
            break;
        }

        err = ida_get_item(item, retitemname, &retshotnr);

        if (CDAS_ERROR(err)) {
            ida_error_mess(err, msg);
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readIdaItem", 5, msg);
            rerr = -5;
            break;
        }

            IDAM_LOG(LOG_DEBUG, "\n\tItem Info:\n");
            IDAM_LOGF(LOG_DEBUG, "\t\tName:\t%s\n", retitemname);
            IDAM_LOGF(LOG_DEBUG, "\t\tShot:\t%d\n", (int) retshotnr);

//--------------------------------------------------------------------------------------------
// Create XML Meta Data Tag

        if (getbytes) {
            char filename[IDA_FSIZE + 1];
            char time[9], date[9];
            short machine = 0, type = 0;

            err = ida_get_finfo(ida_file, filename, time, date, &machine, &type);

            lheap = 10 * STRING_LENGTH + 1;
            metaxml = (char*) malloc(lheap * sizeof(char));
            sprintf(metaxml, "<ida_meta name=\"%s\" shot=\"%d\" ", retitemname, (int) retshotnr);
            nxml = strlen(metaxml);

            if (err == 0) {            // can Drop Endian type - uninitialised!
                time[8] = '\0';
                date[8] = '\0';
                sprintf(xml, "date=\"%s\" time=\"%s\" host_type=\"%d\" endian_type=\"%d\"\n", date, time, (int) machine,
                        (int) type);

                addxmlmetastring(&metaxml, &lheap, xml, &nxml);
            }
        }

//-------------------------------------------------------------------------
// get the item structure

        err = ida_get_structure(item, &dclass, &nodoms, &spaceused, &datpck);

        if (CDAS_ERROR(err)) {
            ida_error_mess(err, msg);
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readIdaItem", 6, msg);
            rerr = -6;
            break;
        }

        if (nodoms < 1) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readIdaItem", 6, "IDA3 file Corrupted (NODOMS < 1)");
            rerr = -6;
            break;
        }

        IDAM_LOG(LOG_DEBUG, "readIdaItem #1\n");

//--------------------------------------------------------------------------------------------
// Update the XML Meta Data

        idaclasses(dclass, classlabel, axesorder, &datarank, &timeorder);

        if (getbytes) {
            sprintf(xml, "dataclass=\"%d\" classlabel=\"%s\" axesorder=\"%s\" rank=\"%d\" order=\"%d\" "
                            "domaincount=\"%d\" spaceused=\"%d\" datapacking=\"%d\">\n",
                    dclass, classlabel, axesorder, datarank, timeorder, nodoms, (int) spaceused, datpck);
            addxmlmetastring(&metaxml, &lheap, xml, &nxml);
        }

//-------------------------------------------------------------------------
// The user might not have populated the full allocated space.
// We need to read the user data structure first

        dxtsam1 = (long*) calloc(nodoms, sizeof(long));
        xtsams = (long*) calloc(nodoms, sizeof(long));

        err = ida_user_struct(item, &dclass, &udoms, dxtsam1, xtsams, &ysams,
                              &totsams, &datpck);

        if (CDAS_ERROR(err)) {
            ida_error_mess(err, msg);
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readIdaItem", 7, msg);
            rerr = -7;
            break;
        }

        free(dxtsam1);
        free(xtsams);

        IDAM_LOG(LOG_DEBUG, "readIdaItem #2\n");

//-------------------------------------------------------------------------
// Get the Data Aquisition Device Type

        err = ida_get_device(item, &locn, &chan, &typeno, type);

        if (CDAS_ERROR(err)) {
            ida_error_mess(err, msg);
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readIdaItem", 8, msg);
            rerr = -8;
            break;
        }

        IDAM_LOG(LOG_DEBUG, "Device Type details:\n");
        IDAM_LOGF(LOG_DEBUG, "Location %d\n", locn);
        IDAM_LOGF(LOG_DEBUG, "Channel  %d\n", chan);
        IDAM_LOGF(LOG_DEBUG, "Type     %d  %s\n", typeno, type);

//--------------------------------------------------------------------------------------------
// Update the XML Meta Data

        if (getbytes) {
            sprintf(xml, "<device location=\"%d\" channel=\"%d\" type=\"%d\" name=\"%s\"/>\n", locn, chan, typeno,
                    type);
            addxmlmetastring(&metaxml, &lheap, xml, &nxml);
        }

//-------------------------------------------------------------------------
// Note: Axes data are always of type FLOAT !!!
//-------------------------------------------------------------------------
// Before we can read the data we need to know the number of
// array elements for each axis we don't check for ERRORS

// Time axis

        tsams = (long*) calloc(udoms, sizeof(long));

        toff = (float*) calloc(udoms, sizeof(float));
        tint = (float*) calloc(udoms, sizeof(float));
        tmax = (float*) calloc(udoms, sizeof(float));

        err = ida_get_tinfo(item, udoms, toff, tint, tmax, tsams, tunits, tlabel);

        if (err != 0) {
            ida_error_mess(err, msg);
            IDAM_LOGF(LOG_DEBUG, "\nida_get_tinfo message: %s\n", msg);
        }

        tunits[IDA_USIZE] = '\0';
        tlabel[IDA_LSIZE] = '\0';

        if (tsams[0] > 0) {
            IDAM_LOG(LOG_DEBUG, "\n\tTime axis:\n");
            IDAM_LOG(LOG_DEBUG, "\t\tNo of samples:");
            for (i = 0; i < udoms; i++) IDAM_LOGF(LOG_DEBUG, "\t%d", (int) tsams[i]);
            IDAM_LOG(LOG_DEBUG, "\n");
            IDAM_LOGF(LOG_DEBUG, "\t\tUnits:\t%s\n", tunits);
            IDAM_LOGF(LOG_DEBUG, "\t\tLabel:\t%s\n", tlabel);
        }
        IDAM_LOG(LOG_DEBUG, "readIdaItem #3\n");

//--------------------------------------------------------------------------------------------
// Update the XML Meta Data

        if (getbytes && (strstr(axesorder, "T") != NULL)) {
            sprintf(xml, "<axis id=\"t\" domaincount=\"%d\" units=\"%s\" label=\"%s\">\n", udoms, tunits, tunits);
            addxmlmetastring(&metaxml, &lheap, xml, &nxml);
            addxmlmetaarray(&metaxml, &lheap, "sams", (void*) tsams, udoms, TYPE_LONG, &nxml);
            addxmlmetaarray(&metaxml, &lheap, "off", (void*) toff, udoms, TYPE_FLOAT, &nxml);
            addxmlmetaarray(&metaxml, &lheap, "int", (void*) tint, udoms, TYPE_FLOAT, &nxml);
            addxmlmetaarray(&metaxml, &lheap, "max", (void*) tmax, udoms, TYPE_FLOAT, &nxml);
            addxmlmetastring(&metaxml, &lheap, "</axis>\n", &nxml);
        }

//-------------------------------------------------------------------------
// X axis

        xsams = (long*) calloc(udoms, sizeof(long));
        xoff = (float*) calloc(udoms, sizeof(float));
        xint = (float*) calloc(udoms, sizeof(float));
        xmax = (float*) calloc(udoms, sizeof(float));

        err = ida_get_xinfo(item, udoms, xoff, xint, xmax, xsams, xunits, xlabel);

        xunits[IDA_USIZE] = '\0';
        xlabel[IDA_LSIZE] = '\0';

        if (xsams[0] > 0) {
            IDAM_LOG(LOG_DEBUG, "\n\tX axis:\n");
            IDAM_LOG(LOG_DEBUG, "\t\tNo of samples:");
            for (i = 0; i < udoms; i++) IDAM_LOGF(LOG_DEBUG, "\t%d", (int) xsams[i]);
            IDAM_LOG(LOG_DEBUG, "\n");
            IDAM_LOGF(LOG_DEBUG, "\t\tUnits:\t%s\n", xunits);
            IDAM_LOGF(LOG_DEBUG, "\t\tLabel:\t%s\n", xlabel);
        }

        IDAM_LOG(LOG_DEBUG, "readIdaItem #4\n");

//--------------------------------------------------------------------------------------------
// Update the XML Meta Data

        if (getbytes && (strstr(axesorder, "X") != NULL)) {
            sprintf(xml, "<axis id=\"x\" domaincount=\"%d\" units=\"%s\" label=\"%s\">\n", udoms, xunits, xunits);
            addxmlmetastring(&metaxml, &lheap, xml, &nxml);
            addxmlmetaarray(&metaxml, &lheap, "sams", (void*) xsams, udoms, TYPE_LONG, &nxml);
            addxmlmetaarray(&metaxml, &lheap, "off", (void*) xoff, udoms, TYPE_FLOAT, &nxml);
            addxmlmetaarray(&metaxml, &lheap, "int", (void*) xint, udoms, TYPE_FLOAT, &nxml);
            addxmlmetaarray(&metaxml, &lheap, "max", (void*) xmax, udoms, TYPE_FLOAT, &nxml);
            addxmlmetastring(&metaxml, &lheap, "</axis>\n", &nxml);
        }

//-------------------------------------------------------------------------
// Y axis

        ysams = 0;
        yoff = (float*) calloc(1, sizeof(float));
        yint = (float*) calloc(1, sizeof(float));
        ymax = (float*) calloc(1, sizeof(float));

        err = ida_get_yinfo(item, yoff, yint, ymax, &ysams, yunits, ylabel);

        yunits[IDA_USIZE] = '\0';
        ylabel[IDA_LSIZE] = '\0';

        if (ysams > 0) {
            IDAM_LOG(LOG_DEBUG, "\n\tY axis:\n");
            IDAM_LOGF(LOG_DEBUG, "\t\tNo of samples:\t%d\n", ysams);
            IDAM_LOGF(LOG_DEBUG, "\t\tUnits:\t%s\n", yunits);
            IDAM_LOGF(LOG_DEBUG, "\t\tLabel:\t%s\n", ylabel);
        }
        IDAM_LOG(LOG_DEBUG, "readIdaItem #5\n");

//--------------------------------------------------------------------------------------------
// Update the XML Meta Data

        if (getbytes && (strstr(axesorder, "Y") != NULL)) {
            long lysams = (long) ysams;
            sprintf(xml, "<axis id=\"y\" domaincount=\"%d\" units=\"%s\" label=\"%s\">\n", 1, yunits, yunits);
            addxmlmetastring(&metaxml, &lheap, xml, &nxml);
            addxmlmetaarray(&metaxml, &lheap, "sams", (void*) &lysams, 1, TYPE_LONG, &nxml);
            addxmlmetaarray(&metaxml, &lheap, "off", (void*) yoff, 1, TYPE_FLOAT, &nxml);
            addxmlmetaarray(&metaxml, &lheap, "int", (void*) yint, 1, TYPE_FLOAT, &nxml);
            addxmlmetaarray(&metaxml, &lheap, "max", (void*) ymax, 1, TYPE_FLOAT, &nxml);
            addxmlmetastring(&metaxml, &lheap, "</axis>\n", &nxml);
        }

//-------------------------------------------------------------------------
// Z-axis needs to be treated differently

        nz = 0;
        z0 = 0;

        if ((dclass == IDA_DCZ) || (dclass == IDA_DCTZ) || (dclass == IDA_DCZT) ||
            (dclass == IDA_DCXZ) || (dclass == IDA_DCZX)) {

            IDAM_LOG(LOG_DEBUG, "A Z Type Data Class Found\n");

            err = ida_get_zsams(item, &z0, &nz, &flags, &calib);

            if (CDAS_ERROR(err)) {
                ida_error_mess(err, msg);
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readIdaItem", 12, msg);
                rerr = -12;
                break;
            }

            if (nz == 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readIdaItem", 12, "The Number of Z Samples is Zero!");
                rerr = -12;
                break;
            }

            zval = (float*) calloc(nz, sizeof(float));

            err = ida_get_zinfo(item, nz, zval, 0, &maxs, &ysams, zunits, zlabel);

            if (CDAS_ERROR(err)) {
                ida_error_mess(err, msg);
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readIdaItem", 12, msg);
                rerr = -12;
                break;
            }


            zunits[IDA_USIZE] = '\0';
            zlabel[IDA_LSIZE] = '\0';

            IDAM_LOG(LOG_DEBUG, "\n\tZ axis:\n");
            IDAM_LOGF(LOG_DEBUG, "\t\tNo of samples:\t%d\n", nz);
            IDAM_LOGF(LOG_DEBUG, "\t\tUnits:\t%s\n", zunits);
            IDAM_LOGF(LOG_DEBUG, "\t\tLabel:\t%s\n", zlabel);
        }

        IDAM_LOG(LOG_DEBUG, "Marker #0\n");
        IDAM_LOG(LOG_DEBUG, "readIdaItem #6\n");

//--------------------------------------------------------------------------------------------
// Update the XML Meta Data

        if (getbytes && (strstr(axesorder, "Z") != NULL) &&
            ((dclass == IDA_DCZ) || (dclass == IDA_DCTZ) || (dclass == IDA_DCZT) || (dclass == IDA_DCXZ) ||
             (dclass == IDA_DCZX))) {
            long lmaxs = (long) maxs;
            long lysams = (long) ysams;
            sprintf(xml, "<axis id=\"z\" domaincount=\"%d\" units=\"%s\" label=\"%s\">\n", nz, zunits, zunits);
            addxmlmetastring(&metaxml, &lheap, xml, &nxml);
            addxmlmetaarray(&metaxml, &lheap, "sams", (void*) &lysams, 1, TYPE_LONG, &nxml);
            addxmlmetaarray(&metaxml, &lheap, "off", (void*) zval, (int) nz, TYPE_FLOAT, &nxml);
            addxmlmetaarray(&metaxml, &lheap, "max", (void*) &lmaxs, 1, TYPE_FLOAT, &nxml);
            addxmlmetastring(&metaxml, &lheap, "</axis>\n", &nxml);
        }

//----------------------------------------------------------------------------
// Identify IDAM Data Types and Allocate Heap for the Data

        if (data_block->client_block.get_datadble) {
            data_block->data_type = TYPE_DOUBLE;            // Request Data as DOUBLEs
            IDAM_LOG(LOG_DEBUG, "Changing Data type to DOUBLE\n");
        } else {
            data_block->data_type = itemType(datpck, typeno, getbytes, type);
        }


        if (data_block->data_type == TYPE_UNKNOWN) {
            if (tsams != NULL) free(tsams);
            if (tint != NULL) free(tint);
            if (toff != NULL) free(toff);
            if (xsams != NULL) free(xsams);
            if (xint != NULL) free(xint);
            if (xoff != NULL) free(xoff);
            if (yint != NULL) free(yint);
            if (yoff != NULL) free(yoff);
            if (zval != NULL) free(zval);
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readIdaItem", 999, "The Data Type is Unknown");
            rerr = -999;
            break;
        }

        IDAM_LOGF(LOG_DEBUG, "readIdaItem #7: %d\n", (int) totsams);

        data = itemData(data_block->data_type, totsams);

        if (data == NULL) {
            IDAM_LOGF(LOG_DEBUG, "Error Allocating Heap for Data, # = %d\n", (int) totsams);
            IDAM_LOGF(LOG_DEBUG, "Type = %d\n", data_block->data_type);
            if (tsams != NULL) free(tsams);
            if (tint != NULL) free(tint);
            if (toff != NULL) free(toff);
            if (xsams != NULL) free(xsams);
            if (xint != NULL) free(xint);
            if (xoff != NULL) free(xoff);
            if (yint != NULL) free(yint);
            if (yoff != NULL) free(yoff);
            if (zval != NULL) free(zval);
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readIdaItem", 999, "Unable to Allocate Heap for the Data");
            rerr = -999;
            break;
        }

        IDAM_LOG(LOG_DEBUG, "Marker #1\n");
        IDAM_LOG(LOG_DEBUG, "readIdaItem #8\n");

//---------------------------------------------------------------------------
// The 3D allocation is a bit more complicated...

// The number of time samples is a problem, since some of the data
// is stored in X correlated with T or Y correlated with T.

        if ((dclass == IDA_DCTX) || (dclass == IDA_DCTXY) || (dclass == IDA_DCYTX)) {
            for (i = 0; i < udoms; i++) tsams[i] = 1;
        }

// Each domain can have a different number of time samples

        nt = 0;
        for (i = 0; i < udoms; i++) nt += tsams[i];

// Each domain can have a different number of X samples

// we allocate the maximum number for each time slice
// Here, a problem in populating the dbgout array occurs
// since the missing data will be padded with zeros (this issue
// is not yet resolved)

        nx = xsams[0];
        maxnx = xsams[0];

        for (i = 1; i < udoms; i++) {
            nx += xsams[i];
            if (xsams[i - 1] < xsams[i]) maxnx = xsams[i];
        }

        ny = ysams;

//------------------------------------------------------------------------
// OK now we start reading the data using low level routines

        //if(!getbytes){
        switch (data_block->data_type) {
            case TYPE_FLOAT:
                if (!getbytes)
                    err = ida_get_data(item, (void*) data, totsams * sizeof(float), IDA_REAL | IDA_D4);
                else
                    err = ida_get_data(item, (void*) data, totsams * sizeof(float), IDA_VALU | IDA_REAL | IDA_D4);
                break;
            case TYPE_DOUBLE:
                if (!getbytes)
                    err = ida_get_data(item, (void*) data, totsams * sizeof(double), IDA_REAL | IDA_D8);
                else
                    err = ida_get_data(item, (void*) data, totsams * sizeof(double), IDA_VALU | IDA_REAL | IDA_D8);
                break;
            case TYPE_CHAR:
                err = ida_get_data(item, (void*) data, totsams * sizeof(char), IDA_VALU | IDA_CHAR | IDA_D1 | IDA_SGND);
                break;
            case TYPE_SHORT:
                err = ida_get_data(item, (void*) data, totsams * sizeof(short),
                                   IDA_VALU | IDA_INTG | IDA_D2 | IDA_SGND);
                break;
            case TYPE_INT:
                err = ida_get_data(item, (void*) data, totsams * sizeof(int), IDA_VALU | IDA_INTG | IDA_D4 | IDA_SGND);
                break;
            case TYPE_LONG:
                err = ida_get_data(item, (void*) data, totsams * sizeof(long), IDA_VALU | IDA_INTG | IDA_D4 | IDA_SGND);
                break;
            case TYPE_LONG64:
                err = ida_get_data(item, (void*) data, totsams * sizeof(long long),
                                   IDA_VALU | IDA_INTG | IDA_D8 | IDA_SGND);
                break;
            case TYPE_UNSIGNED_CHAR:
                err = ida_get_data(item, (void*) data, totsams * sizeof(unsigned char), IDA_VALU | IDA_CHAR | IDA_D1);
                break;
            case TYPE_UNSIGNED_SHORT:
                err = ida_get_data(item, (void*) data, totsams * sizeof(unsigned short), IDA_VALU | IDA_INTG | IDA_D2);
                break;
            case TYPE_UNSIGNED:
                err = ida_get_data(item, (void*) data, totsams * sizeof(unsigned int), IDA_VALU | IDA_INTG | IDA_D4);
                break;
            case TYPE_UNSIGNED_LONG:
                err = ida_get_data(item, (void*) data, totsams * sizeof(unsigned long), IDA_VALU | IDA_INTG | IDA_D4);
                break;
            case TYPE_UNSIGNED_LONG64:
                err = ida_get_data(item, (void*) data, totsams * sizeof(unsigned long long),
                                   IDA_VALU | IDA_INTG | IDA_D8);
                break;
        }

        IDAM_LOGF(LOG_DEBUG, "ida_get_data #9  %d\n", (int) err);

        if (CDAS_ERROR(err)) {
            ida_error_mess(err, msg);
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readIdaItem", 13, msg);
            if (tsams != NULL) free(tsams);
            if (tint != NULL) free(tint);
            if (toff != NULL) free(toff);
            if (xsams != NULL) free(xsams);
            if (xint != NULL) free(xint);
            if (xoff != NULL) free(xoff);
            if (yint != NULL) free(yint);
            if (yoff != NULL) free(yoff);
            if (zval != NULL) free(zval);
            rerr = -13;
            break;
        }

// Get the data information

        err = ida_get_dinfo(item, &devoff, &devrng, &devres, &calfac, &caloff, dunits, dlabel);

        IDAM_LOG(LOG_DEBUG, "ida_get_dinfo #10\n");

        if (CDAS_ERROR(err)) {
            ida_error_mess(err, msg);
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readIdaItem", 14, msg);
            rerr = -14;
            break;
        }

        dunits[IDA_USIZE] = '\0';
        dlabel[IDA_LSIZE] = '\0';

        strcpy(data_block->data_units, dunits);
        strcpy(data_block->data_label, dlabel);

//--------------------------------------------------------------------------------------------
// Update the XML Meta Data and Close the Document

        if (getbytes) {
            sprintf(xml, "<data count=\"%d\" offset=\"%e\" range=\"%e\" resolution=\"%d\" "
                            "calfac=\"%e\" caloff=\"%e\" units=\"%s\" label=\"%s\" />\n",
                    (int) totsams, devoff, devrng, devres, calfac, caloff, dunits, dlabel);
            addxmlmetastring(&metaxml, &lheap, xml, &nxml);
            strcpy(xml, "</ida_meta>\n");
            addxmlmetastring(&metaxml, &lheap, xml, &nxml);
        }

//------------------------------------------------------------------------
// Errors Available with IDA Data

        data_block->error_type = TYPE_UNKNOWN;        // Default

        if ((datpck & IDA_ERRB) == IDA_ERRB) {        // Do Error Data Exist?

// Identify Data Types

            if (data_block->client_block.get_datadble) {
                data_block->error_type = TYPE_DOUBLE;
            } else {
                data_block->error_type = errorType(datpck, typeno, getbytes, type);
            }

            //if(!getbytes){
            error = itemData(data_block->error_type, totsams);        // Allocate Heap (NULL if TYPE_UNKNOWN)
            //} else {
            //   error = (char *)malloc(spaceused);
            //}

            if (error == NULL && data_block->error_type != TYPE_UNKNOWN) {
                IDAM_LOGF(LOG_DEBUG, "Error Allocating Heap for Error Data, # = %d\n", (int) totsams);
                IDAM_LOGF(LOG_DEBUG, "Type = %d\n", data_block->error_type);
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readIdaItem", 998,
                             "Unable to Allocate Heap for Error Data");
                if (tsams != NULL) free(tsams);
                if (tint != NULL) free(tint);
                if (toff != NULL) free(toff);
                if (xsams != NULL) free(xsams);
                if (xint != NULL) free(xint);
                if (xoff != NULL) free(xoff);
                if (yint != NULL) free(yint);
                if (yoff != NULL) free(yoff);
                if (zval != NULL) free(zval);
                rerr = -998;
                break;
            }

// Get the Errors

            switch (data_block->error_type) {
                case TYPE_FLOAT:
                    if (!getbytes)
                        err = ida_get_errors(item, (void*) error, totsams * sizeof(float),
                                             IDA_REAL | IDA_D4 | IDA_ERRB | IDA_E4);
                    else
                        err = ida_get_errors(item, (void*) error, totsams * sizeof(float),
                                             IDA_VALU | IDA_REAL | IDA_D4 | IDA_ERRB | IDA_E4);
                    break;
                case TYPE_DOUBLE:
                    if (!getbytes)
                        err = ida_get_errors(item, (void*) error, totsams * sizeof(double),
                                             IDA_REAL | IDA_D8 | IDA_ERRB | IDA_E8);
                    else
                        err = ida_get_errors(item, (void*) error, totsams * sizeof(double),
                                             IDA_VALU | IDA_REAL | IDA_D8 | IDA_ERRB | IDA_E8);
                    break;
                case TYPE_CHAR:
                    err = ida_get_errors(item, (void*) error, totsams * sizeof(char),
                                         IDA_VALU | IDA_CHAR | IDA_D1 | IDA_ERRB | IDA_E1);
                    break;
                case TYPE_SHORT:
                    err = ida_get_errors(item, (void*) error, totsams * sizeof(short),
                                         IDA_VALU | IDA_INTG | IDA_D2 | IDA_ERRB | IDA_E2);
                    break;
                case TYPE_INT:
                    err = ida_get_errors(item, (void*) error, totsams * sizeof(int),
                                         IDA_VALU | IDA_INTG | IDA_D4 | IDA_ERRB | IDA_E4);
                    break;
                case TYPE_LONG:
                    err = ida_get_errors(item, (void*) error, totsams * sizeof(long),
                                         IDA_VALU | IDA_INTG | IDA_D4 | IDA_ERRB | IDA_E4);
                    break;
                case TYPE_LONG64:
                    err = ida_get_errors(item, (void*) error, totsams * sizeof(long long),
                                         IDA_VALU | IDA_INTG | IDA_D8 | IDA_ERRB | IDA_E8);
                    break;
                case TYPE_UNSIGNED_CHAR:
                    err = ida_get_errors(item, (void*) error, totsams * sizeof(unsigned char),
                                         IDA_VALU | IDA_CHAR | IDA_D1 | IDA_ERRB | IDA_E1);
                    break;
                case TYPE_UNSIGNED_SHORT:
                    err = ida_get_errors(item, (void*) error, totsams * sizeof(unsigned short),
                                         IDA_VALU | IDA_INTG | IDA_D2 | IDA_ERRB | IDA_E2);
                    break;
                case TYPE_UNSIGNED:
                    err = ida_get_errors(item, (void*) error, totsams * sizeof(unsigned int),
                                         IDA_VALU | IDA_INTG | IDA_D4 | IDA_ERRB | IDA_E4);
                    break;
                case TYPE_UNSIGNED_LONG:
                    err = ida_get_errors(item, (void*) error, totsams * sizeof(unsigned long),
                                         IDA_VALU | IDA_INTG | IDA_D4 | IDA_ERRB | IDA_E4);
                    break;
                case TYPE_UNSIGNED_LONG64:
                    err = ida_get_errors(item, (void*) error, totsams * sizeof(unsigned long long),
                                         IDA_VALU | IDA_INTG | IDA_D8 | IDA_ERRB | IDA_E8);
                    break;
            }

            IDAM_LOG(LOG_DEBUG, "ida_get_errors #11\n");

            if (CDAS_ERROR(err)) {
                ida_error_mess(err, msg);
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readIdaItem", 15, msg);
                if (tsams != NULL) free(tsams);
                if (tint != NULL) free(tint);
                if (toff != NULL) free(toff);
                if (xsams != NULL) free(xsams);
                if (xint != NULL) free(xint);
                if (xoff != NULL) free(xoff);
                if (yint != NULL) free(yint);
                if (yoff != NULL) free(yoff);
                if (zval != NULL) free(zval);
                rerr = -15;
                break;
            }
        }

//--------------------------------------------------------------------------------------------
// We have now the data and error values lets sort the
// data according to its DCLASS value.

// NOTE: I define the first axis to be the X-axis,
// the second to be the Y-axis and the third to be the
// time axis. Of course if you have XT or YT it will
// And I don't distinguish between X or Y axis as long
// there is only one of them -> It will be always stored
// in the first index.

        data_block->data = data;
        data_block->errhi = error;
        data_block->errasymmetry = 0;

        IDAM_LOGF(LOG_DEBUG, "Data Class? %d\n", dclass);

        switch (dclass) {
            case IDA_DCT: // Multiple time points per domain
            {
                data_block->rank = 1;
                data_block->order = 0;
                data_block->data_n = nt;

                data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
                initDimBlock(&data_block->dims[0]);

// T axis

                data_block->dims[0].dim_n = nt;
                data_block->dims[0].compressed = 1;
                data_block->dims[0].method = 1;
                data_block->dims[0].udoms = (int) udoms;
                data_block->dims[0].sams = tsams;
                data_block->dims[0].offs = (char*) toff;
                data_block->dims[0].ints = (char*) tint;

                strcpy(data_block->dims[0].dim_units, tunits);
                strcpy(data_block->dims[0].dim_label, tlabel);

                if (xsams != NULL) free(xsams);
                if (xint != NULL) free(xint);
                if (xoff != NULL) free(xoff);
                if (yint != NULL) free(yint);
                if (yoff != NULL) free(yoff);
                if (zval != NULL) free(zval);

                IDAM_LOG(LOG_DEBUG, "\n\tRead f(t) data from IDA_DCT:\n");

                break;
            }

            case IDA_DCVAL: // single value per domain
            {

                data_block->order = 0;
                data_block->data_n = udoms;

                data_block->rank = 1;
                data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
                initDimBlock(&data_block->dims[0]);

// T axis

                data_block->dims[0].dim_n = udoms;
                data_block->dims[0].compressed = 1;
                data_block->dims[0].method = 2;
                data_block->dims[0].udoms = (int) udoms;
                data_block->dims[0].offs = (char*) toff;

                strcpy(data_block->dims[0].dim_units, tunits);
                strcpy(data_block->dims[0].dim_label, tlabel);

                if (tsams != NULL) free(tsams);
                if (tint != NULL) free(tint);
                if (xsams != NULL) free(xsams);
                if (xint != NULL) free(xint);
                if (xoff != NULL) free(xoff);
                if (yint != NULL) free(yint);
                if (yoff != NULL) free(yoff);
                if (zval != NULL) free(zval);

                IDAM_LOG(LOG_DEBUG, "\n\tRead f(t)data from IDA_DCVAL:\n");
                break;
            }

            case IDA_DCX: // Array in X per T domain: that will be f(x;t) data
            {

                data_block->rank = 2;
                data_block->order = 1;
                data_block->data_n = maxnx * udoms;

                data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
                initDimBlock(&data_block->dims[0]);
                initDimBlock(&data_block->dims[1]);

// X-axis

// Here IDA would be more flexible since every domain could
// have its own X-axis, but I don't think anyone
// produces such data

                data_block->dims[0].dim_n = maxnx;
                data_block->dims[0].compressed = 1;
                data_block->dims[0].method = 3;
                data_block->dims[0].udoms = (int) maxnx;
                data_block->dims[0].sams = NULL;
                data_block->dims[0].offs = (char*) xoff;
                data_block->dims[0].ints = (char*) xint;

                strcpy(data_block->dims[0].dim_units, xunits);
                strcpy(data_block->dims[0].dim_label, xlabel);

// T axis

                data_block->dims[1].dim_n = udoms;
                data_block->dims[1].compressed = 1;
                data_block->dims[1].method = 2;
                data_block->dims[1].udoms = (int) udoms;
                data_block->dims[1].sams = NULL;
                data_block->dims[1].offs = (char*) toff;
                data_block->dims[1].ints = (char*) NULL;

                strcpy(data_block->dims[1].dim_units, tunits);
                strcpy(data_block->dims[1].dim_label, tlabel);

                if (tsams != NULL) free(tsams);
                if (tint != NULL) free(tint);
                if (xsams != NULL) free(xsams);
                if (yint != NULL) free(yint);
                if (yoff != NULL) free(yoff);
                if (zval != NULL) free(zval);

                IDAM_LOG(LOG_DEBUG, "\n\tRead f(x,t) data from IDA_DCX:\n");
                IDAM_LOG(LOG_DEBUG, "\n\tWARNING: assuming same X-axis for each domain!\n");
                break;
            }

            case IDA_DCY: // Array in Y per domain: that will be f(y,t) data
            {

                data_block->rank = 2;
                data_block->order = 1;
                data_block->data_n = udoms * ny;

                data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
                initDimBlock(&data_block->dims[0]);
                initDimBlock(&data_block->dims[1]);

// Y-axis

                data_block->dims[0].dim_n = ny;
                data_block->dims[0].compressed = 1;
                data_block->dims[0].method = 3;
                data_block->dims[0].udoms = (int) ny;
                data_block->dims[0].sams = NULL;
                data_block->dims[0].offs = (char*) yoff;
                data_block->dims[0].ints = (char*) yint;

                strcpy(data_block->dims[0].dim_units, yunits);
                strcpy(data_block->dims[0].dim_label, ylabel);

// T axis

                data_block->dims[1].dim_n = udoms;
                data_block->dims[1].compressed = 1;
                data_block->dims[1].method = 2;
                data_block->dims[1].udoms = (int) udoms;
                data_block->dims[1].sams = NULL;
                data_block->dims[1].offs = (char*) toff;
                data_block->dims[1].ints = NULL;

                strcpy(data_block->dims[1].dim_units, tunits);
                strcpy(data_block->dims[1].dim_label, tlabel);

                if (tsams != NULL) free(tsams);
                if (tint != NULL) free(tint);
                if (xsams != NULL) free(xsams);
                if (xint != NULL) free(xint);
                if (xoff != NULL) free(xoff);
                if (zval != NULL) free(zval);

                IDAM_LOG(LOG_DEBUG, "\n\tRead f(x,t) data from IDA_DCY:\n");
                break;
            }

            case IDA_DCZ: // that will be f(z,t) data
            {

                data_block->rank = 2;
                data_block->order = 1;
                data_block->data_n = udoms * nz;

                data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
                initDimBlock(&data_block->dims[0]);
                initDimBlock(&data_block->dims[1]);

// Z-axis

                data_block->dims[0].dim_n = nz;
                data_block->dims[0].compressed = 1;
                data_block->dims[0].method = 2;
                data_block->dims[0].udoms = (int) nz;
                data_block->dims[0].sams = NULL;
                data_block->dims[0].offs = (char*) zval;
                data_block->dims[0].ints = NULL;

                strcpy(data_block->dims[0].dim_units, zunits);
                strcpy(data_block->dims[0].dim_label, zlabel);

// T axis

                data_block->dims[1].dim_n = udoms;
                data_block->dims[1].compressed = 1;
                data_block->dims[1].method = 2;
                data_block->dims[1].udoms = (int) udoms;
                data_block->dims[1].sams = NULL;
                data_block->dims[1].offs = (char*) toff;
                data_block->dims[1].ints = NULL;

                strcpy(data_block->dims[1].dim_units, tunits);
                strcpy(data_block->dims[1].dim_label, tlabel);

                if (tsams != NULL) free(tsams);
                if (tint != NULL) free(tint);
                if (xsams != NULL) free(xsams);
                if (xint != NULL) free(xint);
                if (xoff != NULL) free(xoff);
                if (yint != NULL) free(yint);
                if (yoff != NULL) free(yoff);

                IDAM_LOG(LOG_DEBUG, "\n\tRead f(x,t) data from IDA_DCZ:\n");
                break;
            }

            case IDA_DCTY: // 2-D TY data per domain
            {
// that will be f(y,t) data

                data_block->rank = 2;
                data_block->order = 1;
                data_block->data_n = nt * ny;

                data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
                initDimBlock(&data_block->dims[0]);
                initDimBlock(&data_block->dims[1]);

// Y-axis

                data_block->dims[0].dim_n = ny;
                data_block->dims[0].compressed = 1;
                data_block->dims[0].method = 3;
                data_block->dims[0].udoms = (int) ny;
                data_block->dims[0].sams = NULL;
                data_block->dims[0].offs = (char*) yoff;
                data_block->dims[0].ints = (char*) yint;

                strcpy(data_block->dims[0].dim_units, yunits);
                strcpy(data_block->dims[0].dim_label, ylabel);

// T axis

                data_block->dims[1].dim_n = nt;
                data_block->dims[1].compressed = 1;
                data_block->dims[1].method = 1;
                data_block->dims[1].udoms = (int) udoms;
                data_block->dims[1].sams = tsams;
                data_block->dims[1].offs = (char*) toff;
                data_block->dims[1].ints = (char*) tint;

                strcpy(data_block->dims[1].dim_units, tunits);
                strcpy(data_block->dims[1].dim_label, tlabel);

                if (xsams != NULL) free(xsams);
                if (xint != NULL) free(xint);
                if (xoff != NULL) free(xoff);
                if (zval != NULL) free(zval);

                IDAM_LOG(LOG_DEBUG, "\n\tRead f(y,t) data from IDA_DCTY:\n");
                break;
            }

            case IDA_DCYT: // 2-D YT data per domain
            {

// Unusual Data Ordering ****** Needs Checking!

                data_block->rank = 2;
                data_block->order = 0;
                data_block->data_n = nt * ny;

                data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
                initDimBlock(&data_block->dims[0]);
                initDimBlock(&data_block->dims[1]);

// Y-axis

                data_block->dims[1].dim_n = ny;
                data_block->dims[1].compressed = 1;
                data_block->dims[1].method = 3;
                data_block->dims[1].udoms = (int) ny;
                data_block->dims[1].sams = NULL;
                data_block->dims[1].offs = (char*) yoff;
                data_block->dims[1].ints = (char*) yint;

                strcpy(data_block->dims[1].dim_units, yunits);
                strcpy(data_block->dims[1].dim_label, ylabel);

// T axis

                data_block->dims[0].dim_n = nt;
                data_block->dims[0].compressed = 1;
                data_block->dims[0].method = 1;
                data_block->dims[0].udoms = (int) udoms;
                data_block->dims[0].sams = tsams;
                data_block->dims[0].offs = (char*) toff;
                data_block->dims[0].ints = (char*) tint;

                strcpy(data_block->dims[0].dim_units, tunits);
                strcpy(data_block->dims[0].dim_label, tlabel);

                if (xsams != NULL) free(xsams);
                if (xint != NULL) free(xint);
                if (xoff != NULL) free(xoff);
                if (zval != NULL) free(zval);

                IDAM_LOG(LOG_DEBUG, "\n\tRead f(y,t) data from IDA_DCYT:\n");
                break;
            }

            case IDA_DCTZ: // 2-D TZ data per domain
            {
// that will be f(z,t) data

                data_block->rank = 2;
                data_block->order = 1;
                data_block->data_n = nt * nz;

                data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
                initDimBlock(&data_block->dims[0]);
                initDimBlock(&data_block->dims[1]);

// Z-axis

                data_block->dims[0].dim_n = nz;
                data_block->dims[0].compressed = 1;
                data_block->dims[0].method = 2;
                data_block->dims[0].udoms = (int) nz;
                data_block->dims[0].sams = NULL;
                data_block->dims[0].offs = (char*) zval;
                data_block->dims[0].ints = NULL;

                strcpy(data_block->dims[0].dim_units, zunits);
                strcpy(data_block->dims[0].dim_label, zlabel);

// T axis

                data_block->dims[1].dim_n = nt;
                data_block->dims[1].compressed = 1;
                data_block->dims[1].method = 1;
                data_block->dims[1].udoms = (int) udoms;
                data_block->dims[1].sams = tsams;
                data_block->dims[1].offs = (char*) toff;
                data_block->dims[1].ints = (char*) tint;

                strcpy(data_block->dims[1].dim_units, tunits);
                strcpy(data_block->dims[1].dim_label, tlabel);

                if (xsams != NULL) free(xsams);
                if (xint != NULL) free(xint);
                if (xoff != NULL) free(xoff);
                if (yint != NULL) free(yint);
                if (yoff != NULL) free(yoff);

                IDAM_LOG(LOG_DEBUG, "\n\tRead f(y,t) data from IDA_DCTZ:\n");
                break;
            }

            case IDA_DCZT: // 2-D ZT data per domain
            {

// CHECK **********

                data_block->rank = 2;
                data_block->order = 0;
                data_block->data_n = nt * nz;

                data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
                initDimBlock(&data_block->dims[0]);
                initDimBlock(&data_block->dims[1]);

// Z-axis
///// *********** Error in CALLOC ***********

                data_block->dims[1].dim_n = nz;
                data_block->dims[1].compressed = 1;
                data_block->dims[1].method = 2;
                data_block->dims[1].udoms = (int) nz;
                data_block->dims[1].sams = NULL;
                data_block->dims[1].offs = (char*) zval;
                data_block->dims[1].ints = NULL;

                strcpy(data_block->dims[1].dim_units, zunits);
                strcpy(data_block->dims[1].dim_label, zlabel);

// T axis

                data_block->dims[0].dim_n = nt;
                data_block->dims[0].compressed = 1;
                data_block->dims[0].method = 1;
                data_block->dims[0].udoms = (int) udoms;
                data_block->dims[0].sams = tsams;
                data_block->dims[0].offs = (char*) toff;
                data_block->dims[0].ints = (char*) tint;

                strcpy(data_block->dims[0].dim_units, tunits);
                strcpy(data_block->dims[0].dim_label, tlabel);

                if (xsams != NULL) free(xsams);
                if (xint != NULL) free(xint);
                if (xoff != NULL) free(xoff);
                if (yint != NULL) free(yint);
                if (yoff != NULL) free(yoff);

                IDAM_LOG(LOG_DEBUG, "\n\tRead f(x,t) data from IDA_DCTZ:\n");
                break;
            }

//============================================================= RANK 3 Arrays ================================

            case IDA_DCXY: // 2-D array YX per domain
            {
// that will be f(y,x,t) data

                data_block->rank = 3;
                data_block->order = 2;
                data_block->data_n = maxnx * ny * udoms;

                data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
                initDimBlock(&data_block->dims[0]);
                initDimBlock(&data_block->dims[1]);
                initDimBlock(&data_block->dims[2]);

// T axis

                data_block->dims[2].dim_n = udoms;
                data_block->dims[2].compressed = 1;
                data_block->dims[2].method = 2;
                data_block->dims[2].udoms = (int) udoms;
                data_block->dims[2].sams = NULL;
                data_block->dims[2].offs = (char*) toff;
                data_block->dims[2].ints = NULL;

                strcpy(data_block->dims[2].dim_units, tunits);
                strcpy(data_block->dims[2].dim_label, tlabel);

#ifdef SWAPXY

                // X-axis

                data_block->dims[0].dim_n = maxnx;
                data_block->dims[0].compressed = 1;
                data_block->dims[0].method = 3;
                data_block->dims[0].udoms = (int) maxnx;
                data_block->dims[0].sams = NULL;
                data_block->dims[0].offs = (char*) xoff;
                data_block->dims[0].ints = (char*) xint;

                strcpy(data_block->dims[0].dim_units, xunits);
                strcpy(data_block->dims[0].dim_label, xlabel);

                // Y axis

                data_block->dims[1].dim_n = ny;
                data_block->dims[1].compressed = 1;
                data_block->dims[1].method = 3;
                data_block->dims[1].udoms = (int) ny;
                data_block->dims[1].sams = NULL;
                data_block->dims[1].offs = (char*) yoff;
                data_block->dims[1].ints = (char*) yint;

                strcpy(data_block->dims[1].dim_units, yunits);
                strcpy(data_block->dims[1].dim_label, ylabel);

                // Swap X and Y Data reshaping with pattern 1

                swapRank3(data_block, 1);

#else

                // X-axis

                data_block->dims[1].dim_n = maxnx;
                data_block->dims[1].compressed = 1;
                data_block->dims[1].method = 3;
                data_block->dims[1].udoms = (int) maxnx;
                data_block->dims[1].sams = NULL;
                data_block->dims[1].offs = (char*) xoff;
                data_block->dims[1].ints = (char*) xint;

                strcpy(data_block->dims[1].dim_units, xunits);
                strcpy(data_block->dims[1].dim_label, xlabel);

// Y axis

                data_block->dims[0].dim_n = ny;
                data_block->dims[0].compressed = 1;
                data_block->dims[0].method = 3;
                data_block->dims[0].udoms = (int) ny;
                data_block->dims[0].sams = NULL;
                data_block->dims[0].offs = (char*) yoff;
                data_block->dims[0].ints = (char*) yint;

                strcpy(data_block->dims[0].dim_units, yunits);
                strcpy(data_block->dims[0].dim_label, ylabel);

#endif

                if (tsams != NULL) free(tsams);
                if (tint != NULL) free(tint);
                if (xsams != NULL) free(xsams);
                if (zval != NULL) free(zval);

                IDAM_LOG(LOG_DEBUG, "\n\tRead f(x,y,t) data from IDA_DCXY:\n");
                IDAM_LOG(LOG_DEBUG, "\n\tWARNING: assuming same X-axis for each domain!\n");
                break;
            }


            case IDA_DCYX: // 2-D XY array per domain
            {
// that will be f(x,y,t) data
// but we need to swap indices

                data_block->rank = 3;
                data_block->order = 2;
                data_block->data_n = maxnx * ny * udoms;

                data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
                initDimBlock(&data_block->dims[0]);
                initDimBlock(&data_block->dims[1]);
                initDimBlock(&data_block->dims[2]);

// X-axis

                data_block->dims[0].dim_n = maxnx;
                data_block->dims[0].compressed = 1;
                data_block->dims[0].method = 3;
                data_block->dims[0].udoms = (int) maxnx;
                data_block->dims[0].sams = NULL;
                data_block->dims[0].offs = (char*) xoff;
                data_block->dims[0].ints = (char*) xint;

                strcpy(data_block->dims[0].dim_units, xunits);
                strcpy(data_block->dims[0].dim_label, xlabel);

// Y axis

                data_block->dims[1].dim_n = ny;
                data_block->dims[1].compressed = 1;
                data_block->dims[1].method = 3;
                data_block->dims[1].udoms = (int) ny;
                data_block->dims[1].sams = NULL;
                data_block->dims[1].offs = (char*) yoff;
                data_block->dims[1].ints = (char*) yint;

                strcpy(data_block->dims[1].dim_units, yunits);
                strcpy(data_block->dims[1].dim_label, ylabel);

// T axis

                data_block->dims[2].dim_n = udoms;
                data_block->dims[2].compressed = 1;
                data_block->dims[2].method = 2;
                data_block->dims[2].udoms = (int) udoms;
                data_block->dims[2].sams = NULL;
                data_block->dims[2].offs = (char*) toff;
                data_block->dims[2].ints = NULL;

                strcpy(data_block->dims[2].dim_units, tunits);
                strcpy(data_block->dims[2].dim_label, tlabel);

                if (tsams != NULL) free(tsams);
                if (tint != NULL) free(tint);
                if (xsams != NULL) free(xsams);
                if (zval != NULL) free(zval);

#ifdef SWAPXY

                // Swap X and Y Data reshaping with pattern 2

                swapRank3(data_block, 2);

#endif

                IDAM_LOG(LOG_DEBUG, "\n\tRead f(x,y,t) data from IDA_DCYX:\n");
                IDAM_LOG(LOG_DEBUG, "\n\tWARNING: assuming same X-axis for each domain!\n");
                break;
            }


            case IDA_DCXZ: // 2-D array XZ per domain
            {
// that will be f(x,z,t) data

                data_block->rank = 3;
                data_block->order = 2;
                data_block->data_n = maxnx * nz * udoms;

                data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
                initDimBlock(&data_block->dims[0]);
                initDimBlock(&data_block->dims[1]);
                initDimBlock(&data_block->dims[2]);

// T axis

                data_block->dims[2].dim_n = udoms;
                data_block->dims[2].compressed = 1;
                data_block->dims[2].method = 2;
                data_block->dims[2].udoms = (int) udoms;
                data_block->dims[2].sams = NULL;
                data_block->dims[2].offs = (char*) toff;
                data_block->dims[2].ints = NULL;

                strcpy(data_block->dims[2].dim_units, tunits);
                strcpy(data_block->dims[2].dim_label, tlabel);

#ifdef SWAPXY

                // X-axis

                data_block->dims[0].dim_n = maxnx;
                data_block->dims[0].compressed = 1;
                data_block->dims[0].method = 3;
                data_block->dims[0].udoms = (int) maxnx;
                data_block->dims[0].sams = NULL;
                data_block->dims[0].offs = (char*) xoff;
                data_block->dims[0].ints = (char*) xint;

                strcpy(data_block->dims[0].dim_units, xunits);
                strcpy(data_block->dims[0].dim_label, xlabel);


                // Z axis

                data_block->dims[1].dim_n = nz;
                data_block->dims[1].compressed = 1;
                data_block->dims[1].method = 2;
                data_block->dims[1].udoms = (int) nz;
                data_block->dims[1].sams = NULL;
                data_block->dims[1].offs = (char*) zval;
                data_block->dims[1].ints = NULL;

                strcpy(data_block->dims[1].dim_units, zunits);
                strcpy(data_block->dims[1].dim_label, zlabel);

                // Swap X and Y Data reshaping with pattern 1

                swapRank3(data_block, 1);

#else

                // X-axis

                data_block->dims[1].dim_n = maxnx;
                data_block->dims[1].compressed = 1;
                data_block->dims[1].method = 3;
                data_block->dims[1].udoms = (int) maxnx;
                data_block->dims[1].sams = NULL;
                data_block->dims[1].offs = (char*) xoff;
                data_block->dims[1].ints = (char*) xint;

                strcpy(data_block->dims[1].dim_units, xunits);
                strcpy(data_block->dims[1].dim_label, xlabel);


// Z axis

                data_block->dims[0].dim_n = nz;
                data_block->dims[0].compressed = 1;
                data_block->dims[0].method = 2;
                data_block->dims[0].udoms = (int) nz;
                data_block->dims[0].sams = NULL;
                data_block->dims[0].offs = (char*) zval;
                data_block->dims[0].ints = NULL;

                strcpy(data_block->dims[0].dim_units, zunits);
                strcpy(data_block->dims[0].dim_label, zlabel);

#endif

                if (tsams != NULL) free(tsams);
                if (tint != NULL) free(tint);
                if (xsams != NULL) free(xsams);
                if (yint != NULL) free(yint);
                if (yoff != NULL) free(yoff);

                IDAM_LOG(LOG_DEBUG, "\n\tRead f(x,y,t) data from IDA_DCXZ:\n");
                IDAM_LOG(LOG_DEBUG, "\n\tWARNING: assuming same X-axis for each domain!\n");
                break;
            }


            case IDA_DCZX: // 2-D XY array per domain
            {
// that will be f(x,y,t) data
// but we need to swap indices

                data_block->rank = 3;
                data_block->order = 2;
                data_block->data_n = maxnx * nz * udoms;

                data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
                initDimBlock(&data_block->dims[0]);
                initDimBlock(&data_block->dims[1]);
                initDimBlock(&data_block->dims[2]);

// X-axis

                data_block->dims[0].dim_n = maxnx;
                data_block->dims[0].compressed = 1;
                data_block->dims[0].method = 3;
                data_block->dims[0].udoms = (int) maxnx;
                data_block->dims[0].sams = NULL;
                data_block->dims[0].offs = (char*) xoff;
                data_block->dims[0].ints = (char*) xint;

                strcpy(data_block->dims[0].dim_units, xunits);
                strcpy(data_block->dims[0].dim_label, xlabel);

//Z axis

                data_block->dims[1].dim_n = nz;
                data_block->dims[1].compressed = 1;
                data_block->dims[1].method = 2;
                data_block->dims[1].udoms = (int) nz;
                data_block->dims[1].sams = NULL;
                data_block->dims[1].offs = (char*) zval;
                data_block->dims[1].ints = NULL;

                strcpy(data_block->dims[1].dim_units, zunits);
                strcpy(data_block->dims[1].dim_label, zlabel);

// T axis

                data_block->dims[2].dim_n = udoms;
                data_block->dims[2].compressed = 1;
                data_block->dims[2].method = 2;
                data_block->dims[2].udoms = (int) udoms;
                data_block->dims[2].sams = NULL;
                data_block->dims[2].offs = (char*) toff;
                data_block->dims[2].ints = NULL;

                strcpy(data_block->dims[2].dim_units, tunits);
                strcpy(data_block->dims[2].dim_label, tlabel);

                if (tsams != NULL) free(tsams);
                if (tint != NULL) free(tint);
                if (xsams != NULL) free(xsams);
                if (yint != NULL) free(yint);
                if (yoff != NULL) free(yoff);

#ifdef SWAPXY

                // Swap X and Y Data reshaping with pattern 2

                swapRank3(data_block, 2);

#endif

                IDAM_LOG(LOG_DEBUG, "\n\tRead f(x,y,t) data from IDA_DCZX:\n");
                IDAM_LOG(LOG_DEBUG, "\n\tWARNING: assuming same X-axis for each domain!\n");
                break;
            }


            case IDA_DCYTX: {
// that will be f(x,y,t) data */
// but X=X(T) */

                data_block->rank = 3;
                data_block->order = 2;
                data_block->data_n = maxnx * ny * udoms;

                data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
                initDimBlock(&data_block->dims[0]);
                initDimBlock(&data_block->dims[1]);
                initDimBlock(&data_block->dims[2]);

// X-axis

                data_block->dims[0].dim_n = maxnx;
                data_block->dims[0].compressed = 1;
                data_block->dims[0].method = 3;
                data_block->dims[0].udoms = (int) maxnx;
                data_block->dims[0].sams = NULL;
                data_block->dims[0].offs = (char*) xoff;
                data_block->dims[0].ints = (char*) xint;

                strcpy(data_block->dims[0].dim_units, xunits);
                strcpy(data_block->dims[0].dim_label, xlabel);

// Y axis


                data_block->dims[1].dim_n = ny;
                data_block->dims[1].compressed = 1;
                data_block->dims[1].method = 3;
                data_block->dims[1].udoms = (int) ny;
                data_block->dims[1].sams = NULL;
                data_block->dims[1].offs = (char*) yoff;
                data_block->dims[1].ints = (char*) yint;

                strcpy(data_block->dims[1].dim_units, yunits);
                strcpy(data_block->dims[1].dim_label, ylabel);

// T axis

                data_block->dims[2].dim_n = udoms;
                data_block->dims[2].compressed = 1;
                data_block->dims[2].method = 2;
                data_block->dims[2].udoms = (int) udoms;
                data_block->dims[2].sams = NULL;
                data_block->dims[2].offs = (char*) toff;
                data_block->dims[2].ints = NULL;

                strcpy(data_block->dims[2].dim_units, tunits);
                strcpy(data_block->dims[2].dim_label, tlabel);

                if (tsams != NULL) free(tsams);
                if (tint != NULL) free(tint);
                if (xsams != NULL) free(xsams);
                if (zval != NULL) free(zval);

#ifdef SWAPXY

                // Swap X and Y Data reshaping with pattern 2

                swapRank3(data_block, 2);

#endif

                IDAM_LOG(LOG_DEBUG, "\n\tRead f(x,y,t) data from IDA_DCYTX:\n");
                break;
            }


            case IDA_DCTXY: {
// that will be f(x,y,t) data
// but X=X(T)

                data_block->rank = 3;
                data_block->order = 2;
                data_block->data_n = maxnx * ny * udoms;

                data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
                initDimBlock(&data_block->dims[0]);
                initDimBlock(&data_block->dims[1]);
                initDimBlock(&data_block->dims[2]);

// T axis

                data_block->dims[2].dim_n = udoms;
                data_block->dims[2].compressed = 1;
                data_block->dims[2].method = 2;
                data_block->dims[2].udoms = (int) udoms;
                data_block->dims[2].sams = NULL;
                data_block->dims[2].offs = (char*) toff;
                data_block->dims[2].ints = NULL;

                strcpy(data_block->dims[2].dim_units, tunits);
                strcpy(data_block->dims[2].dim_label, tlabel);

#ifdef SWAPXY

                // X-axis

                data_block->dims[0].dim_n = maxnx;
                data_block->dims[0].compressed = 1;
                data_block->dims[0].method = 3;
                data_block->dims[0].udoms = (int) maxnx;
                data_block->dims[0].sams = NULL;
                data_block->dims[0].offs = (char*) xoff;
                data_block->dims[0].ints = (char*) xint;

                strcpy(data_block->dims[0].dim_units, xunits);
                strcpy(data_block->dims[0].dim_label, xlabel);

                // Y axis

                data_block->dims[1].dim_n = ny;
                data_block->dims[1].compressed = 1;
                data_block->dims[1].method = 3;
                data_block->dims[1].udoms = (int) ny;
                data_block->dims[1].sams = NULL;
                data_block->dims[1].offs = (char*) yoff;
                data_block->dims[1].ints = (char*) yint;

                strcpy(data_block->dims[1].dim_units, yunits);
                strcpy(data_block->dims[1].dim_label, ylabel);

                // Swap X and Y Data reshaping with pattern 1

                swapRank3(data_block, 1);

#else

                // X-axis

                data_block->dims[1].dim_n = maxnx;
                data_block->dims[1].compressed = 1;
                data_block->dims[1].method = 3;
                data_block->dims[1].udoms = (int) maxnx;
                data_block->dims[1].sams = NULL;
                data_block->dims[1].offs = (char*) xoff;
                data_block->dims[1].ints = (char*) xint;

                strcpy(data_block->dims[1].dim_units, xunits);
                strcpy(data_block->dims[1].dim_label, xlabel);

// Y axis

                data_block->dims[0].dim_n = ny;
                data_block->dims[0].compressed = 1;
                data_block->dims[0].method = 3;
                data_block->dims[0].udoms = (int) ny;
                data_block->dims[0].sams = NULL;
                data_block->dims[0].offs = (char*) yoff;
                data_block->dims[0].ints = (char*) yint;

                strcpy(data_block->dims[0].dim_units, yunits);
                strcpy(data_block->dims[0].dim_label, ylabel);

#endif

                if (tsams != NULL) free(tsams);
                if (tint != NULL) free(tint);
                if (xsams != NULL) free(xsams);
                if (zval != NULL) free(zval);

                IDAM_LOG(LOG_DEBUG, "\n\tRead f(x,y,t) data from IDA_DCTXY:\n");
                break;
            }

//==============================================================================================================

            case IDA_DCTX: // 2-D array X(T)T
            {

// that will be f(x(t),t) data */

                data_block->rank = 2;
                data_block->order = 1;
                data_block->data_n = maxnx * udoms;

                data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
                initDimBlock(&data_block->dims[0]);
                initDimBlock(&data_block->dims[1]);

// X-axis

                data_block->dims[0].dim_n = maxnx;
                data_block->dims[0].compressed = 1;
                data_block->dims[0].method = 3;
                data_block->dims[0].udoms = (int) maxnx;
                data_block->dims[0].sams = NULL;
                data_block->dims[0].offs = (char*) xoff;
                data_block->dims[0].ints = (char*) xint;

                strcpy(data_block->dims[0].dim_units, xunits);
                strcpy(data_block->dims[0].dim_label, xlabel);

// T axis

                data_block->dims[1].dim_n = udoms;
                data_block->dims[1].compressed = 1;
                data_block->dims[1].method = 2;
                data_block->dims[1].udoms = (int) udoms;
                data_block->dims[1].sams = NULL;
                data_block->dims[1].offs = (char*) toff;
                data_block->dims[1].ints = NULL;

                strcpy(data_block->dims[1].dim_units, tunits);
                strcpy(data_block->dims[1].dim_label, tlabel);

                if (tsams != NULL) free(tsams);
                if (tint != NULL) free(tint);
                if (xsams != NULL) free(xsams);
                if (yint != NULL) free(yint);
                if (yoff != NULL) free(yoff);
                if (zval != NULL) free(zval);

                IDAM_LOG(LOG_DEBUG, "\n\tRead f(x,t) data from IDA_DCX:\n");
                IDAM_LOG(LOG_DEBUG, "\n\tWARNING: assuming same X-axis for each domain!\n");
                break;
            }

            default: {
                IDAM_LOGF(LOG_DEBUG, "ERROR: Data class %d is not supported!", dclass);
                rerr = -16;
                break;
            }
        }

        if (rerr != 0) break;

//---------------------------------------------------------------------------------------
// End of Error Loop

    } while (0);

//-------------------------------------------------------------------------------------------
// Use requested Cast of Dimensions to DOUBLE?

    IDAM_LOGF(LOG_DEBUG, "Properties: get_datadble = %d", data_block->client_block.get_datadble);
    IDAM_LOGF(LOG_DEBUG, "            get_dimdble  = %d", data_block->client_block.get_dimdble);
    IDAM_LOGF(LOG_DEBUG, "            get_timedble = %d", data_block->client_block.get_timedble);

    if (data_block->client_block.get_dimdble || data_block->client_block.get_timedble) {
        float* foffs, * fints;
        double* doffs = NULL, * dints = NULL;
        for (i = 0; i < data_block->rank; i++) {
            if (data_block->client_block.get_dimdble ||
                (data_block->client_block.get_timedble && i == data_block->order)) {
                data_block->dims[i].data_type = TYPE_DOUBLE;
                if ((foffs = (float*) data_block->dims[i].offs) != NULL) {
                    doffs = (double*) realloc((void*) doffs, data_block->dims[i].udoms * sizeof(double));
                    for (j = 0; j < data_block->dims[i].udoms; j++) doffs[j] = (double) foffs[j];
                    free((void*) data_block->dims[i].offs);
                    data_block->dims[i].offs = (char*) doffs;
                }
                if ((fints = (float*) data_block->dims[i].ints) != NULL) {
                    dints = (double*) realloc((void*) dints, data_block->dims[i].udoms * sizeof(double));
                    for (j = 0; j < data_block->dims[i].udoms; j++) dints[j] = (double) fints[j];
                    free((void*) data_block->dims[i].ints);
                    data_block->dims[i].ints = (char*) dints;
                }
                IDAM_LOGF(LOG_DEBUG, "Changing Dimension %d type to DOUBLE\n", i);
            }
        }
    }

//-------------------------------------------------------------------------------------------
// free local work arrays


    free(ymax);
    free(xmax);
    free(tmax);

//-------------------------------------------------------------------------------------------
// Return XML Meta Data

    if (getbytes) {
        if (rerr != 0) {
            if (metaxml != NULL) free((void*) metaxml);
            metaxml = NULL;
        } else {
            data_block->opaque_type = OPAQUE_TYPE_XML_DOCUMENT;
            data_block->opaque_block = metaxml;
            data_block->opaque_count = nxml;

            IDAM_LOGF(LOG_DEBUG, "%s\n", metaxml);
        }
    }

//---------------------------------------------------------------------------------------
// free the IDA item pointer

    err = ida_free(item);

    return (rerr);
}

void idaclasses(int class, char* label, char* axes, int* datarank, int* timeorder)
{
    int order = -1, rank = 0;
    switch (class) {
        case IDA_DCT: {    // Multiple time points per T domain
            strcpy(label, "DCT: f(t)");
            strcpy(axes, "T");
            rank = 1;
            order = 0;
            break;
        }

        case IDA_DCVAL: {    // single value per T domain
            strcpy(label, "DCVAL: f(t)");
            strcpy(axes, "T");
            order = 0;
            rank = 1;
            break;
        }

        case IDA_DCX: {    // Array in X per T domain: that will be f(x,t) data
            strcpy(label, "DCX: f(x,t)");
            strcpy(axes, "XT");
            rank = 2;
            order = 1;
            break;
        }

        case IDA_DCY: {    // Array in Y per domain: that will be f(y,t) data
            strcpy(label, "DCY: f(y,t)");
            strcpy(axes, "YT");
            rank = 2;
            order = 1;
            break;
        }

        case IDA_DCZ: {    // that will be f(z,t) data
            strcpy(label, "DCZ: f(z,t)");
            strcpy(axes, "ZT");
            rank = 2;
            order = 1;
            break;
        }

        case IDA_DCTY: {    // 2-D TY data per domain that will be f(y,t) data
            strcpy(label, "DCTY: f(y,t)");
            strcpy(axes, "YT");
            rank = 2;
            order = 1;
            break;
        }

        case IDA_DCYT: {    // 2-D YT data per domain
// Here the indices are different to IDA_DCY and IDA_DCYT
// that will be f(y,t) data

            strcpy(label, "DCYT: f(t,y)");
            strcpy(axes, "TY");
            rank = 2;
            order = 0;
            break;
        }

        case IDA_DCTZ: {    // 2-D TZ data per domain that will be f(z,t) data
            strcpy(label, "DCTZ: f(z,t)");
            strcpy(axes, "ZT");
            rank = 2;
            order = 1;
            break;
        }

        case IDA_DCZT: {    // 2-D ZT data per domain
// Here the indices are different to IDA_DCZ and IDA_DCTZ
// that will be f(z,t) data

            strcpy(label, "DCZT: f(t,z)");
            strcpy(axes, "TZ");
            rank = 2;
            order = 0;
            break;
        }

        case IDA_DCXY: {    // 2-D array YX per domain that will be f(x,y,t) data
            strcpy(label, "DCXY: f(x,y,t)");
            strcpy(axes, "XYT");
            rank = 3;
            order = 2;
            break;
        }

        case IDA_DCYX: {    // 2-D XY array per domain that will be f(x,y,t) data
            strcpy(label, "DCYX: f(x,y,t)");
            strcpy(axes, "XYT");
            rank = 3;
            order = 2;
            break;
        }

        case IDA_DCXZ: {    // 2-D array XZ per domain that will be f(x,z,t) data
            strcpy(label, "DCXZ: f(x,z,t)");
            strcpy(axes, "XZT");
            rank = 3;
            order = 2;
            break;
        }

        case IDA_DCZX: {    // 2-D XY array per domain that will be f(x,z,t) data
            strcpy(label, "DCZX: f(x,z,t)");
            strcpy(axes, "XZT");
            rank = 3;
            order = 2;
            break;
        }

        case IDA_DCYTX: {    // that will be f(x,y,t) data but X=X(T)
            strcpy(label, "DCYTX: f(x,y,t)");
            strcpy(axes, "XYT");
            rank = 3;
            order = 2;
            break;
        }

        case IDA_DCTXY: { // that will be f(x,y,t) data but X=X(T)
            strcpy(label, "DCTXY: f(x,y,t)");
            strcpy(axes, "XYT");
            rank = 3;
            order = 2;
            break;
        }

        case IDA_DCTX: {    // 2-D array f(x(t),t) data */
            strcpy(label, "DCTX: f(x(t),t)");
            strcpy(axes, "XT");
            rank = 2;
            order = 1;
            break;
        }

        default: {
            strcpy(label, "Unknown");
            break;
        }
    }

    *datarank = rank;
    *timeorder = order;

    return;
}

#endif
