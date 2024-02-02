#pragma once

#include "genStructs.h"

void defineField(COMPOUNDFIELD* field, const char* name, const char* desc, int* offset,
                 unsigned short type_id, int rank, int* shape);

void defineUserTypeField(COMPOUNDFIELD* field, const char* name, const char* desc, int* offset, int rank, int* shape,
                         USERDEFINEDTYPE* user_type, bool is_pointer);