#pragma once

#include "genStructs.h"

/** Initialise a LOGMALLOCLIST data structure.
 *
 * @param str A pointer to a LOGMALLOCLIST data structure instance.
 * @return void.
 */
void initLogMallocList(LOGMALLOCLIST* str);

/** Initialise a LOGMALLOC data structure.
 *
 * @param str A pointer to a LOGMALLOC data structure instance.
 * @return void.
 */
void initLogMalloc(LOGMALLOC* str);

/** Initialise a LOGSTRUCTLIST data structure.
 *
 * @return void.
 */
void initLogStructList(LOGSTRUCTLIST* logstructlist);

/** Initialise a LOGSTRUCT data structure.
 *
 * @param str A pointer to a LOGSTRUCT data structure instance.
 * @return void.
 */
void initLogStruct(LOGSTRUCT* str);

/** Initialise a COMPOUNDFIELD data structure.
 *
 * @param str A pointer to a COMPOUNDFIELD data structure instance.
 * @return void.
 */
void initCompoundField(COMPOUNDFIELD* str);

/** Initialise a USERDEFINEDTYPE data structure.
 *
 * @param str A pointer to a USERDEFINEDTYPE data structure instance.
 * @return void.
 */
void initUserDefinedType(USERDEFINEDTYPE* str);

/** Initialise a USERDEFINEDTYPELIST data structure.
 *
 * @param str A pointer to a USERDEFINEDTYPELIST data structure instance.
 * @return void.
 */
void initUserDefinedTypeList(USERDEFINEDTYPELIST* str);

/** Add a Compound Field type to a structure definition.
 *
 * @param str The structure definition.
 * @param field The Compound field type.
 * @return void.
 */
void udaAddCompoundField(USERDEFINEDTYPE* str, COMPOUNDFIELD field);

/** Add a structure definition to the List of structure types
 *
 * @param str The list of structure definitions.
 * @param type The new definition to add to the list.
 * @return void.
 */
void udaAddUserDefinedType(USERDEFINEDTYPELIST* str, USERDEFINEDTYPE type);

/** Replace/Update the structure definition list with an different structure type.
 *
 * @param str The list of structure definitions.
 * @param typeId The definition list entry to be replaced/updated
 * @param type The definition to add into the list.
 * @return void.
 */
void udaUpdateUserDefinedType(USERDEFINEDTYPELIST* str, int typeId, USERDEFINEDTYPE type);