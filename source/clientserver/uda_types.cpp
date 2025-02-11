#include <uda/types.h>

#include "uda_structs.h"

size_t getSizeOf(UDA_TYPE data_type)
{
    switch (data_type) {
        case UDA_TYPE_FLOAT:
            return sizeof(float);
        case UDA_TYPE_DOUBLE:
            return sizeof(double);
        case UDA_TYPE_CHAR:
            return sizeof(char);
        case UDA_TYPE_SHORT:
            return sizeof(short);
        case UDA_TYPE_INT:
            return sizeof(int);
        case UDA_TYPE_LONG:
            return sizeof(long);
        case UDA_TYPE_LONG64:
            return sizeof(long long);
        case UDA_TYPE_UNSIGNED_CHAR:
            return sizeof(unsigned char);
        case UDA_TYPE_UNSIGNED_SHORT:
            return sizeof(unsigned short);
        case UDA_TYPE_UNSIGNED_INT:
            return sizeof(unsigned int);
        case UDA_TYPE_UNSIGNED_LONG:
            return sizeof(unsigned long);
        case UDA_TYPE_UNSIGNED_LONG64:
            return sizeof(unsigned long long);
        case UDA_TYPE_STRING:
            return sizeof(char);
        case UDA_TYPE_COMPLEX:
            return sizeof(uda::client_server::Complex);
        case UDA_TYPE_DCOMPLEX:
            return sizeof(uda::client_server::DComplex);
        case UDA_TYPE_CAPNP:
            return sizeof(char);
        default:
            return 0;
    }
}

size_t getPtrSizeOf(UDA_TYPE data_type)
{
    switch (data_type) {
        case UDA_TYPE_FLOAT:
            return sizeof(float*);
        case UDA_TYPE_DOUBLE:
            return sizeof(double*);
        case UDA_TYPE_CHAR:
            return sizeof(char*);
        case UDA_TYPE_SHORT:
            return sizeof(short*);
        case UDA_TYPE_INT:
            return sizeof(int*);
        case UDA_TYPE_LONG:
            return sizeof(long*);
        case UDA_TYPE_LONG64:
            return sizeof(long long*);
        case UDA_TYPE_UNSIGNED_CHAR:
            return sizeof(unsigned char*);
        case UDA_TYPE_UNSIGNED_SHORT:
            return sizeof(unsigned short*);
        case UDA_TYPE_UNSIGNED_INT:
            return sizeof(unsigned int*);
        case UDA_TYPE_UNSIGNED_LONG:
            return sizeof(unsigned long*);
        case UDA_TYPE_UNSIGNED_LONG64:
            return sizeof(unsigned long long*);
        case UDA_TYPE_STRING:
            return sizeof(char*);
        case UDA_TYPE_COMPLEX:
            return sizeof(uda::client_server::Complex*);
        case UDA_TYPE_DCOMPLEX:
            return sizeof(uda::client_server::DComplex*);
        default:
            return 0;
    }
}