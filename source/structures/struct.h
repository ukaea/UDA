#pragma once

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

} // namespace uda::structures