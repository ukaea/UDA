#include "udaTypes.h"

#include "udaStructs.h"

size_t getSizeOf(int data_type)
{
    switch (data_type) {
        case TYPE_FLOAT:
            return sizeof(float);
        case TYPE_DOUBLE:
            return sizeof(double);
        case TYPE_CHAR:
            return sizeof(char);
        case TYPE_SHORT:
            return sizeof(short);
        case TYPE_INT:
            return sizeof(int);
        case TYPE_LONG:
            return sizeof(long);
        case TYPE_LONG64:
            return sizeof(long long);
        case TYPE_UNSIGNED_CHAR:
            return sizeof(unsigned char);
        case TYPE_UNSIGNED_SHORT:
            return sizeof(unsigned short);
        case TYPE_UNSIGNED_INT:
            return sizeof(unsigned int);
        case TYPE_UNSIGNED_LONG:
            return sizeof(unsigned long);
        case TYPE_UNSIGNED_LONG64:
            return sizeof(unsigned long long);
        case TYPE_COMPLEX:
            return sizeof(COMPLEX);
        case TYPE_DCOMPLEX:
            return sizeof(DCOMPLEX);
        default:
            return 0;
    }
}
