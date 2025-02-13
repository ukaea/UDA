#pragma once

#include "genStructs.h"

namespace uda::structures
{

void defineField(CompoundField* field, const char* name, const char* desc, int* offset, unsigned short type_id,
                 int rank, int* shape, bool is_pointer, bool is_scalar);

void defineUserTypeField(CompoundField* field, const char* name, const char* desc, int* offset, int rank, int* shape,
                         UserDefinedType* user_type, bool is_pointer);

void print_image(const char* image, int imagecount);

int regularise_vlen_structures(UserDefinedTypeList* user_defined_type_list, LogMallocList* log_malloc_list, NTree* tree,
    const char* target, unsigned int count);

int uda_regularise_vlen_data(NTree* tree, UserDefinedTypeList* user_defined_type_list);

char** parse_target(const char* target, int* ntargets);

NTree* find_ntree_structure_component2(LogMallocList* log_malloc_list, NTree* c_tree, const char* target, const char** lastname);

NTree* find_ntree_structure_component(LogMallocList* log_malloc_list, NTree* tree, const char* target);

NTree* find_ntree_structure_definition(NTree* tree, const char* target);

int get_node_structure_data_count(LogMallocList* log_malloc_list, NTree* tree);

NTree* find_ntree_structure(LogMallocList* log_malloc_list, NTree* tree, const char* target);

} // namespace uda::structures