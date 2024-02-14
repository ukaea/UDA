#pragma once

#include "genStructs.h"

namespace uda::structures
{

void defineField(CompoundField* field, const char* name, const char* desc, int* offset, unsigned short type_id,
                 int rank, int* shape);

void defineUserTypeField(CompoundField* field, const char* name, const char* desc, int* offset, int rank, int* shape,
                         UserDefinedType* user_type, bool is_pointer);

} // namespace uda::structures