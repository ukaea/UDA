#pragma once

#include <string>

#include "genStructs.h"

namespace uda::structures
{

/** Initialise a LogMallocList data structure.
 *
 * @param str A pointer to a LogMallocList data structure instance.
 * @return void.
 */
void init_log_malloc_list(LogMallocList* str);

/** Initialise a LogMalloc data structure.
 *
 * @param str A pointer to a LogMalloc data structure instance.
 * @return void.
 */
void init_log_malloc(LogMalloc* str);

/** Initialise a LogStructList data structure.
 *
 * @return void.
 */
void init_log_struct_list(LogStructList* logstructlist);

/** Initialise a LogStruct data structure.
 *
 * @param str A pointer to a LogStruct data structure instance.
 * @return void.
 */
void init_log_struct(LogStruct* str);

/** Initialise a CompoundField data structure.
 *
 * @param str A pointer to a CompoundField data structure instance.
 * @return void.
 */
void init_compound_field(CompoundField* str);

/** Initialise a UserDefinedType data structure.
 *
 * @param str A pointer to a UserDefinedType data structure instance.
 * @return void.
 */
void init_user_defined_type(UserDefinedType* str);

/** Initialise a UserDefinedTypeList data structure.
 *
 * @param str A pointer to a UserDefinedTypeList data structure instance.
 * @return void.
 */
void init_user_defined_type_list(UserDefinedTypeList* str);

/** Add a Compound Field type to a structure definition.
 *
 * @param str The structure definition.
 * @param field The Compound field type.
 * @return void.
 */
void add_compound_field(UserDefinedType* str, CompoundField field);

/** Add a structure definition to the List of structure types
 *
 * @param str The list of structure definitions.
 * @param type The new definition to add to the list.
 * @return void.
 */
void add_user_defined_type(UserDefinedTypeList* str, UserDefinedType type);

/** Replace/Update the structure definition list with an different structure type.
 *
 * @param str The list of structure definitions.
 * @param typeId The definition list entry to be replaced/updated
 * @param type The definition to add into the list.
 * @return void.
 */
void update_user_defined_type(UserDefinedTypeList* str, int typeId, UserDefinedType type);

void add_malloc(LogMallocList* log_malloc_list, void* heap, int count, size_t size, const char* type);

void add_malloc2(LogMallocList* log_malloc_list, void* heap, int count, size_t size, const char* type, int rank, int* shape);

size_t get_size_of(UserDefinedTypeList* user_defined_type_list, const char* type);

USERDEFINEDTYPE* find_user_defined_type(UserDefinedTypeList* user_defined_type_list, const char* name, int ref_id);

void add_non_malloc(LogMallocList* log_malloc_list, void* stack, int count, size_t size, const char* type);

void find_malloc(LogMallocList* log_malloc_list, void* heap, int* count, int* size, const char** type);

void find_malloc2(LogMallocList* log_malloc_list, void* heap, int* count, int* size, const char** type, int* rank, int** shape);

void change_malloc(LogMallocList* log_malloc_list, VOIDTYPE old, void* anew, int count, size_t size,
                     const char* type);

int find_struct_id(LogStructList* log_struct_list, void* heap, char** type);

void* find_struct_heap(LogStructList* log_struct_list, int id, char** type);

int get_type_of(std::string type);

void add_ntree(NTree* parent, NTree* child);

void free_malloc_log_list(LogMallocList* str);

void free_malloc_log(LogMallocList* log_malloc_list);

void set_full_ntree(NTree* tree);

void reset_last_malloc_index();

int get_alignment_of(const char* type);

size_t new_offset(size_t offset, const char* type);

size_t padding(size_t offset, const char* type);

size_t get_structure_size(UserDefinedTypeList* user_defined_type_list, UserDefinedType* str);

void add_ntree_list(LogMallocList* log_malloc_list, NTree* node, NTreeList* ntree_list);

void free_ntree_node(NTree* tree);

void print_type_count(NTree* c_tree, const char* target);

NTree* get_full_ntree();

void set_last_malloc_index_value(unsigned int* last_malloc_index_value);

void print_node(NTree* tree);

void print_atomic_data(void* data, int atomictype, int count, const char* label);

void print_node_atomic(LogMallocList* log_malloc_list, NTree* tree);

void print_node_names(LogMallocList* log_malloc_list, NTree* tree);

void print_atomic_type(LogMallocList* log_malloc_list, NTree* tree, const char* target);

void print_ntree2(NTree* tree);

void print_ntree(UserDefinedTypeList* user_defined_type_list, NTree* tree);

void print_ntree_list(NTree* tree);

void print_ntree_structure_names(LogMallocList* log_malloc_list, NTree* tree);

void print_ntree_structure_component_names(LogMallocList* log_malloc_list, NTree* tree);

void add_image(char** image, int* imagecount, const char* line);

void expand_image(char* buffer, char defnames[MAXELEMENTS][MAXELEMENTNAME], int* defvalues, int defCount,
                    char* expand);

int get_node_structure_count(NTree* tree);
char** get_node_structure_names(LogMallocList* log_malloc_list, NTree* tree);
char** get_node_structure_types(LogMallocList* log_malloc_list, NTree* tree);

int get_node_atomic_count(NTree* tree);
char** get_node_atomic_names(LogMallocList* log_malloc_list, NTree* tree);
char** get_node_atomic_types(LogMallocList* log_malloc_list, NTree* tree);

int get_ntree_structure_count(NTree* tree);
char** get_ntree_structure_names(LogMallocList* log_malloc_list, NTree* tree);
char** get_ntree_structure_types(LogMallocList* log_malloc_list, NTree* tree);

int get_ntree_structure_component_count(NTree* tree);
char** get_ntree_structure_component_names(LogMallocList* log_malloc_list, NTree* tree);
char** get_ntree_structure_component_types(LogMallocList* log_malloc_list, NTree* tree);
char** get_ntree_structure_component_descriptions(LogMallocList* log_malloc_list, NTree* tree);

int get_node_structure_component_count(NTree* tree);
char** get_node_structure_component_names(NTree* tree);
char** get_node_structure_component_types(NTree* tree);
char** get_node_structure_component_descriptions(NTree* tree);

void free_user_defined_type_list(UserDefinedTypeList* user_defined_type_list);
void free_log_struct_list(LogStructList* log_struct_list);
void free_log_malloc_list(LogMallocList* log_malloc_list);

} // namespace uda::structures