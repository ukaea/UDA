// User defined type registration, type sizes and the malloc log.
//
// Split out of struct.cpp so that it can be built into libuda_plugins for plugins to use: nothing here records
// errors, and any remaining debug traces compile to nothing in that build (UDA_COMMON_BUILD).

#include "struct.h"

#include <cstdlib>
#include <cstring>
#include <stddef.h>

#ifdef __GNUC__
#  include <strings.h>
#elif defined(_WIN32)
#  include <string.h>
#  define strcasecmp _stricmp
#endif

#include <logging/logging.h>
#include <clientserver/stringUtils.h>
#include <clientserver/udaTypes.h>

#include "accessors.h"

// Malloc log search hint: where the last lookup succeeded. Only an optimisation (searches wrap around), so it is
// harmless for each loaded copy of this code to have its own.
static unsigned int last_malloc_index = 0;
static unsigned int* last_malloc_index_value = &last_malloc_index;  // may be redirected into a GENERAL_STRUCT

void setLastMallocIndexValue(unsigned int* lastMallocIndexValue_in)
{
    last_malloc_index_value = lastMallocIndexValue_in;
    last_malloc_index = *last_malloc_index_value;
}

/** Initialise a LOGMALLOCLIST data structure.
*
* @param str A pointer to a LOGMALLOCLIST data structure instance.
* @return Void.
*/
void initLogMallocList(LOGMALLOCLIST* str)
{
    str->listcount = 0;
    str->listsize = 0;
    str->logmalloc = nullptr;
}

/** Initialise a LOGMALLOC data structure.
*
* @param str A pointer to a LOGMALLOC data structure instance.
* @return Void.
*/
void initLogMalloc(LOGMALLOC* str)
{
    str->count = 0;
    str->rank = 0;
    str->size = 0;
    str->freed = 1;
    str->type[0] = '\0';
    str->heap = nullptr;
    str->shape = nullptr;
}

/** Initialise a LOGSTRUCTLIST data structure.
*
* @return Void.
*/
void initLogStructList(LOGSTRUCTLIST* log_struct_list)
{
    log_struct_list->listcount = 0;
    log_struct_list->listsize = 0;
    log_struct_list->logstruct = nullptr;
}

/** Initialise a LOGSTRUCT data structure.
*
* @param str A pointer to a LOGSTRUCT data structure instance.
* @return Void.
*/
void initLogStruct(LOGSTRUCT* str)
{
    str->id = 0;
    str->type[0] = '\0';
    str->heap = nullptr;
}

/** Initialise a COMPOUNDFIELD data structure.
*
* @param str A pointer to a COMPOUNDFIELD data structure instance.
* @return Void.
*/
void initCompoundField(COMPOUNDFIELD* str)
{
    str->size = 0;
    str->offset = 0;
    str->offpad = 0;
    str->alignment = 0;
    str->atomictype = UDA_TYPE_UNKNOWN;
    str->pointer = 0;
    str->rank = 0;
    str->count = 0;
    str->shape = nullptr;
    memset(str->type, '\0', MAXELEMENTNAME);
    memset(str->name, '\0', MAXELEMENTNAME);
    memset(str->desc, '\0', MAXELEMENTNAME);
}

/** Initialise a USERDEFINEDTYPE data structure.
*
* @param str A pointer to a USERDEFINEDTYPE data structure instance.
* @return Void.
*/
void initUserDefinedType(USERDEFINEDTYPE* str)
{
    str->idamclass = UDA_TYPE_UNKNOWN;
    str->ref_id = 0;
    memset(str->name, '\0', MAXELEMENTNAME);
    memset(str->source, '\0', MAXELEMENTNAME);
    str->imagecount = 0;
    str->image = nullptr;
    str->size = 0;
    str->fieldcount = 0;
    str->compoundfield = nullptr;

}

/** Initialise a USERDEFINEDTYPELIST data structure.
*
* @param str A pointer to a USERDEFINEDTYPELIST data structure instance.
* @return Void.
*/
void initUserDefinedTypeList(USERDEFINEDTYPELIST* str)
{
    str->listCount = 0;
    str->userdefinedtype = nullptr;
}

/** Initialise a GENERAL_BLOCK data structure.
*
* @param str A pointer to a GENERAL_BLOCK data structure instance.
* @return Void.
*/
void initGeneralBlock(GENERAL_BLOCK* str)
{
    str->userdefinedtype = nullptr;
    str->userdefinedtypelist = nullptr;
    str->logmalloclist = nullptr;
    str->lastMallocIndex = 0;
}

/** Add a stack memory location to the LOGMALLOCLIST data structure. These are not freed.
*
* @param stack The memory location.
* @param count The number of elements allocated.
* @param size The size of a single element.
* @param type The name of the type allocated.
* @return void.
*/
void addNonMalloc(LOGMALLOCLIST* logmalloclist, void* stack, int count, size_t size, const char* type)
{
    // Put a non malloc'd memory location on the malloc log flagging it as freed

    if (logmalloclist->listcount + 1 >= logmalloclist->listsize) {
        logmalloclist->logmalloc = (LOGMALLOC*)realloc((void*)logmalloclist->logmalloc,
                                                       (logmalloclist->listsize + GROWMALLOCLIST) * sizeof(LOGMALLOC));
        logmalloclist->listsize = logmalloclist->listsize + GROWMALLOCLIST;
    }

    logmalloclist->logmalloc[logmalloclist->listcount].count = count;
    logmalloclist->logmalloc[logmalloclist->listcount].size = size;
    logmalloclist->logmalloc[logmalloclist->listcount].freed = 1;
    logmalloclist->logmalloc[logmalloclist->listcount].heap = stack;
    strcpy(logmalloclist->logmalloc[logmalloclist->listcount].type, type);

    logmalloclist->logmalloc[logmalloclist->listcount].rank = 0;
    logmalloclist->logmalloc[logmalloclist->listcount].shape = nullptr;

    logmalloclist->listcount++;
}

/** Add a stack memory location to the LOGMALLOCLIST data structure. These are not freed.
*
* @param stack The memory location.
* @param count The number of elements allocated.
* @param size The size of a single element.
* @param type The name of the type allocated.
* @param rank The rank of the allocated array.
* @param shape The shape of the allocated array. Only required when rank > 1.

* @return void.
*/
void
addNonMalloc2(LOGMALLOCLIST* logmalloclist, void* stack, int count, size_t size, const char* type, int rank, int* shape)
{
    // Put a non malloc'd memory location on the malloc log flagging it as freed

    if (logmalloclist->listcount + 1 >= logmalloclist->listsize) {
        logmalloclist->logmalloc = (LOGMALLOC*)realloc((void*)logmalloclist->logmalloc,
                                                       (logmalloclist->listsize + GROWMALLOCLIST) * sizeof(LOGMALLOC));
        logmalloclist->listsize = logmalloclist->listsize + GROWMALLOCLIST;
    }

    logmalloclist->logmalloc[logmalloclist->listcount].count = count;
    logmalloclist->logmalloc[logmalloclist->listcount].size = size;
    logmalloclist->logmalloc[logmalloclist->listcount].freed = 1;
    logmalloclist->logmalloc[logmalloclist->listcount].heap = stack;
    strcpy(logmalloclist->logmalloc[logmalloclist->listcount].type, type);

    logmalloclist->logmalloc[logmalloclist->listcount].rank = rank;
    if (rank > 1) {
        logmalloclist->logmalloc[logmalloclist->listcount].shape = shape;
    } else {
        logmalloclist->logmalloc[logmalloclist->listcount].shape = nullptr;
    }

    logmalloclist->listcount++;
}

/** Add a heap memory location to the LOGMALLOCLIST data structure. These are freed.
*
* @param heap The memory location.
* @param count The number of elements allocated.
* @param size The size of a single element.
* @param type The name of the type allocated.
* @return void.
*/
void addMalloc(LOGMALLOCLIST* logmalloclist, void* heap, int count, size_t size, const char* type)
{
    // Log all Heap allocations for Data from User Defined Structures
    // Grow the list when necessary

    //USERDEFINEDTYPE *udt;

    if (heap == nullptr) return;

    if (logmalloclist->listcount + 1 >= logmalloclist->listsize) {
        logmalloclist->logmalloc = (LOGMALLOC*)realloc((void*)logmalloclist->logmalloc,
                                                       (logmalloclist->listsize + GROWMALLOCLIST) * sizeof(LOGMALLOC));
        logmalloclist->listsize = logmalloclist->listsize + GROWMALLOCLIST;
    }

    logmalloclist->logmalloc[logmalloclist->listcount].count = count;
    logmalloclist->logmalloc[logmalloclist->listcount].size = size;
    logmalloclist->logmalloc[logmalloclist->listcount].freed = 0;
    logmalloclist->logmalloc[logmalloclist->listcount].heap = heap;
    strcpy(logmalloclist->logmalloc[logmalloclist->listcount].type, type);

    logmalloclist->logmalloc[logmalloclist->listcount].rank = 0;
    logmalloclist->logmalloc[logmalloclist->listcount].shape = nullptr;

    logmalloclist->listcount++;
}

/** Add a heap memory location to the LOGMALLOCLIST data structure. These are freed.
*
* @param heap The memory location.
* @param count The number of elements allocated.
* @param size The size of a single element.
* @param type The name of the type allocated.
* @param rank The rank of the allocated array.
* @param shape The shape of the allocated array. Only required when rank > 1.
* @return void.
*/
void
addMalloc2(LOGMALLOCLIST* logmalloclist, void* heap, int count, size_t size, const char* type, int rank, int* shape)
{
    // Log all Heap allocations for Data from User Defined Structures
    // Grow the list when necessary

    if (heap == nullptr) return;

    if (logmalloclist->listcount + 1 >= logmalloclist->listsize) {
        logmalloclist->logmalloc = (LOGMALLOC*)realloc((void*)logmalloclist->logmalloc,
                                                       (logmalloclist->listsize + GROWMALLOCLIST) * sizeof(LOGMALLOC));
        logmalloclist->listsize = logmalloclist->listsize + GROWMALLOCLIST;
    }

    logmalloclist->logmalloc[logmalloclist->listcount].count = count;
    logmalloclist->logmalloc[logmalloclist->listcount].size = size;
    logmalloclist->logmalloc[logmalloclist->listcount].freed = 0;
    logmalloclist->logmalloc[logmalloclist->listcount].heap = heap;
    strcpy(logmalloclist->logmalloc[logmalloclist->listcount].type, type);

    logmalloclist->logmalloc[logmalloclist->listcount].rank = rank;
    if (rank > 1) {
        logmalloclist->logmalloc[logmalloclist->listcount].shape = shape;
    } else {
        logmalloclist->logmalloc[logmalloclist->listcount].shape = nullptr;
    }

    logmalloclist->listcount++;
}

/** Change the logged memory location to a new location (necessary with realloc).
*
* @param old The original logged memory location.
* @param anew The new replacement memory location.
* @param count The number of elements allocated.
* @param size The size of a single element.
* @param type The name of the type allocated.
* @return void.
*/
void changeMalloc(LOGMALLOCLIST* logmalloclist, VOIDTYPE old, void* anew, int count, size_t size, const char* type)
{
    // Change a List Entry
    if (old == 0) {
        addMalloc(logmalloclist, anew, count, size, type);
        return;
    }
    auto target = (VOIDTYPE)((VOIDTYPE*)old);
    for (int i = 0; i < logmalloclist->listcount; i++) {
        auto candidate = (VOIDTYPE)((VOIDTYPE*)logmalloclist->logmalloc[i].heap);
        if (target == candidate) {
            logmalloclist->logmalloc[i].heap = anew;
            logmalloclist->logmalloc[i].freed = 0;
            logmalloclist->logmalloc[i].count = count;
            logmalloclist->logmalloc[i].size = size;
            strcpy(logmalloclist->logmalloc[i].type, type);
            return;
        }
    }
}

/** Change the logged memory location to a new location (necessary with realloc).
*
* @param old The original logged memory location.
* @param anew The new replacement memory location.
* @param count The number of elements allocated.
* @param size The size of a single element.
* @param type The name of the type allocated.
* @return void.
*/
void changeNonMalloc(LOGMALLOCLIST* logmalloclist, void* old, void* anew, int count, size_t size, const char* type)
{
    // Change a non-malloc List Entry

    VOIDTYPE target, candidate;
    if (old == nullptr) {
        addNonMalloc(logmalloclist, anew, count, size, type);
        return;
    }
    target = (VOIDTYPE)((VOIDTYPE*)old);
    for (int i = 0; i < logmalloclist->listcount; i++) {
        candidate = (VOIDTYPE)((VOIDTYPE*)logmalloclist->logmalloc[i].heap);
        if (target == candidate) {
            logmalloclist->logmalloc[i].heap = anew;
            logmalloclist->logmalloc[i].freed = 1;
            logmalloclist->logmalloc[i].count = count;
            logmalloclist->logmalloc[i].size = size;
            strcpy(logmalloclist->logmalloc[i].type, type);
            return;
        }
    }
}

int compare_ulonglong(const void* a, const void* b)
{
    const unsigned long long* da = (const unsigned long long*)a;
    const unsigned long long* db = (const unsigned long long*)b;
    return (int)(*da > *db) - (int)(*da < *db);
}

#ifndef A64
static int compare_ulong(const void *a, const void *b) {
    const unsigned long *da = (const unsigned long *) a;
    const unsigned long *db = (const unsigned long *) b;
    return (int)(*da > *db) - (int)(*da < *db);
}
#endif

int dupCountMallocLog(LOGMALLOCLIST* str)
{
    int sortCount = 0, dupCount = 0;
    int compare_ulonglong(const void*, const void*);
    if (str == nullptr) return 0;
    if (str->listcount <= 1) return 0;
#ifdef A64
    unsigned long long* sorted = (unsigned long long*)malloc(str->listcount * sizeof(unsigned long long));
    for (int i = 0; i < str->listcount; i++) {
        if (str->logmalloc[i].freed == 0) {
            sorted[sortCount++] = (unsigned long long)str->logmalloc[i].heap;
        }
    }
    qsort((void*)sorted, (size_t)sortCount, (size_t)sizeof(unsigned long long), compare_ulonglong);
#else
    unsigned long *sorted = (unsigned long *)malloc(str->listcount*sizeof(unsigned long));
    for (int i = 0; i < str->listcount; i++) {
        if (str->logmalloc[i].freed == 0) {
            sorted[sortCount++] = (unsigned long)str->logmalloc[i].heap;
        }
    }
    qsort((void *)sorted,(size_t)sortCount,(size_t)sizeof(unsigned long), compare_ulong);
#endif
    for (int i = 1; i < sortCount; i++) {
        if (sorted[i] == sorted[i - 1]) dupCount++;
    }
    free(sorted);
    return dupCount;
}

/** Free allocated heap memory but preserve the addresses. There are no arguments.
*
* @return void.
*/
void freeMallocLog(LOGMALLOCLIST* str)
{
    if (str == nullptr) return;
    for (int i = 0; i < str->listcount; i++) {
        if (str->logmalloc[i].freed == 0) {
            if (str->logmalloc[i].heap != nullptr && str->logmalloc[i].count > 0) free(str->logmalloc[i].heap);
            str->logmalloc[i].freed = 1;
            if (str->logmalloc[i].rank > 1 && str->logmalloc[i].shape != nullptr) {
                free(str->logmalloc[i].shape);
                str->logmalloc[i].shape = nullptr;
            }
        }
    }
}

/** Free allocated heap memory and reinitialise a new logmalloclist-> There are no arguments.
*
* @return void.
*/
void freeMallocLogList(LOGMALLOCLIST* str)
{
    if (str == nullptr) return;
    freeMallocLog(str);
    if (str->logmalloc != nullptr) {
        free(str->logmalloc);
    }
    str->logmalloc = nullptr;
    initLogMallocList(str);
}

/** Find the meta data associated with a specific memory location.
*
* @param heap The target memory location.
* @param count The returned allocation count.
* @param size The returned allocation size.
* @param type The returned allocation type.
* @return void.
*/
void findMalloc(LOGMALLOCLIST* logmalloclist, void* heap, int* count, int* size, const char** type)
{
    // Find a specific Heap allocation for Data within User Defined Structures

    VOIDTYPE candidate, target;
    *count = 0;
    *size = 0;
    *type = nullptr;
    if (heap == nullptr) return;

    if ((target = *((VOIDTYPE*)heap)) == 0) return;

    if (last_malloc_index >= (unsigned int)logmalloclist->listcount) {  // Defensive check
        last_malloc_index = 0;
        *last_malloc_index_value = last_malloc_index;
    }

    for (unsigned int i = last_malloc_index; i < (unsigned int)logmalloclist->listcount; i++) {
        candidate = (VOIDTYPE)logmalloclist->logmalloc[i].heap;
        if (target == candidate) {
            *count = logmalloclist->logmalloc[i].count;
            *size = logmalloclist->logmalloc[i].size;
            *type = logmalloclist->logmalloc[i].type;
            last_malloc_index = i;
            *last_malloc_index_value = last_malloc_index;
            return;
        }
    }

    for (unsigned int i = 0; i < last_malloc_index; i++) {
        candidate = (VOIDTYPE)logmalloclist->logmalloc[i].heap;
        if (target == candidate) {
            *count = logmalloclist->logmalloc[i].count;
            *size = logmalloclist->logmalloc[i].size;
            *type = logmalloclist->logmalloc[i].type;
            last_malloc_index = i;
            *last_malloc_index_value = last_malloc_index;
            return;
        }
    }
}

/** Find the meta data associated with a specific memory location.
*
* @param heap The target memory location.
* @param count The returned allocation count.
* @param size The returned allocation size.
* @param type The returned allocation type.
* @param rank The returned rank of the allocated array.
* @param shape The returned shape of the allocated array. Only given when rank > 1.

* @return void.
*/
void
findMalloc2(LOGMALLOCLIST* logmalloclist, void* heap, int* count, int* size, const char** type, int* rank, int** shape)
{
    // Find a specific Heap allocation for Data within User Defined Structures

    VOIDTYPE candidate, target;
    *count = 0;
    *size = 0;
    *type = nullptr;
    *rank = 0;
    *shape = nullptr;
    if (heap == nullptr) return;

    if ((target = *((VOIDTYPE*)heap)) == 0) return;

    if (last_malloc_index >= (unsigned int)logmalloclist->listcount) {  // Defensive check
        last_malloc_index = 0;
        *last_malloc_index_value = last_malloc_index;
    }

    for (unsigned int i = last_malloc_index; i < (unsigned int)logmalloclist->listcount; i++) {
        candidate = (VOIDTYPE)logmalloclist->logmalloc[i].heap;
        if (target == candidate) {
            *count = logmalloclist->logmalloc[i].count;
            *size = logmalloclist->logmalloc[i].size;
            *type = logmalloclist->logmalloc[i].type;
            *rank = logmalloclist->logmalloc[i].rank;
            if (*rank > 1) {
                *shape = logmalloclist->logmalloc[i].shape;
            }
            last_malloc_index = i;    // Start at the current log entry
            *last_malloc_index_value = last_malloc_index;
            return;
        }
    }

    for (unsigned int i = 0; i < last_malloc_index; i++) {   // Start search at the first log entry
        candidate = (VOIDTYPE)logmalloclist->logmalloc[i].heap;
        if (target == candidate) {
            *count = logmalloclist->logmalloc[i].count;
            *size = logmalloclist->logmalloc[i].size;
            *type = logmalloclist->logmalloc[i].type;
            *rank = logmalloclist->logmalloc[i].rank;
            if (*rank > 1) {
                *shape = logmalloclist->logmalloc[i].shape;
            }
            last_malloc_index = i;
            *last_malloc_index_value = last_malloc_index;
            return;
        }
    }
}

/** Add a heap memory location to the LOGSTRUCTLIST data structure. These are freed.
*
* @param heap The memory location.
* @param type The name of the type allocated.
* @return void.
*/
void addStruct(void* heap, const char* type, LOGSTRUCTLIST* log_struct_list)
{
    // Log all dispatched/received Structures
    // Grow the list when necessary

    if (heap == nullptr) return;

    if (log_struct_list->listcount + 1 >= log_struct_list->listsize) {
        log_struct_list->logstruct = (LOGSTRUCT*)realloc((void*)log_struct_list->logstruct,
                                                      (log_struct_list->listsize + GROWMALLOCLIST) * sizeof(LOGSTRUCT));
        log_struct_list->listsize = log_struct_list->listsize + GROWMALLOCLIST;
    }

    log_struct_list->logstruct[log_struct_list->listcount].id = log_struct_list->listcount + 1;
    log_struct_list->logstruct[log_struct_list->listcount].heap = heap;
    strcpy(log_struct_list->logstruct[log_struct_list->listcount].type, type);

    log_struct_list->listcount++;
}

/** Free allocated heap memory and reinitialise a new LOGSTRUCTLIST. There are no arguments.
*
* @return void.
*/
void freeLogStructList(LOGSTRUCTLIST* log_struct_list)
{
    free(log_struct_list->logstruct);
    initLogStructList(log_struct_list);
}

/** Find the meta data associated with a specific Structure.
*
* @param heap The target memory location.
* @param type The returned structure type.
* @return The structure id.
*/
int findStructId(void* heap, char** type, LOGSTRUCTLIST* log_struct_list)
{
    // Find a specific Data Structure

    //VOIDTYPE candidate, target;
    *type = nullptr;
    if (heap == nullptr) return 0;

    for (int i = 0; i < log_struct_list->listcount; i++) {
        if (heap == log_struct_list->logstruct[i].heap) {
            *type = log_struct_list->logstruct[i].type;
            return log_struct_list->logstruct[i].id;
        }
    }
    return 0;
}

/** Find the Heap address and Data Type of a specific Structure.
*
* @param id The structure id.
* @param type The returned structure type.
* @return The heap memory location
*/
void* findStructHeap(int id, char** type, LOGSTRUCTLIST* log_struct_list)
{
    // Find a specific Data Structure

    *type = nullptr;
    if (id == 0) return nullptr;

    for (int i = 0; i < log_struct_list->listcount; i++) {
        if (log_struct_list->logstruct[i].id == id) {
            *type = log_struct_list->logstruct[i].type;
            return log_struct_list->logstruct[i].heap;
        }
    }
    return nullptr;
}

/** Copy a User Defined Structure Definition.
*
* @param old The type definition to be copied.
* @param anew The copy of the type definition.
* @return void.
*/
void copyUserDefinedType(USERDEFINEDTYPE* old, USERDEFINEDTYPE* anew)
{
    USERDEFINEDTYPE udt;
    initUserDefinedType(&udt);
    udt = *old;
    udt.image = (char*)malloc((old->imagecount) * sizeof(char));
    memcpy(udt.image, old->image, old->imagecount);
    udt.compoundfield = (COMPOUNDFIELD*)malloc((old->fieldcount) * sizeof(COMPOUNDFIELD));
    for (int i = 0; i < old->fieldcount; i++) {
        initCompoundField(&udt.compoundfield[i]);
        udt.compoundfield[i] = old->compoundfield[i];
        if (old->compoundfield[i].rank > 0) {
            udt.compoundfield[i].shape = (int*)malloc(old->compoundfield[i].rank * sizeof(int));
            for (int j = 0; j < old->compoundfield[i].rank; j++) {
                udt.compoundfield[i].shape[j] = old->compoundfield[i].shape[j];
            }
        }
    }
    *anew = udt;
}

/** Add a Compound Field type to a structure definition.
*
* @param str The structure definition.
* @param field The Compound field type.
* @return void.
*/
void addCompoundField(USERDEFINEDTYPE* str, COMPOUNDFIELD field)
{
    str->compoundfield = (COMPOUNDFIELD*)realloc((void*)str->compoundfield,
                                                 (str->fieldcount + 1) * sizeof(COMPOUNDFIELD));
    initCompoundField(&str->compoundfield[str->fieldcount]);
    str->compoundfield[str->fieldcount++] = field;
}

/** Add a structure definition to the List of structure types
*
* @param str The list of structure definitions.
* @param type The new definition to add to the list.
* @return void.
*/
void addUserDefinedType(USERDEFINEDTYPELIST* str, USERDEFINEDTYPE type)
{
    str->userdefinedtype = (USERDEFINEDTYPE*)realloc((void*)str->userdefinedtype,
                                                     (str->listCount + 1) * sizeof(USERDEFINEDTYPE));
    initUserDefinedType(&str->userdefinedtype[str->listCount]);
    str->userdefinedtype[str->listCount++] = type;
}

/** Replace the structure definition list with an different structure type.
*
* @param str The list of structure definitions.
* @param typeId The definition list entry to be replaced 
* @param type The definition to add into the list.
* @return void.
*/
void updateUserDefinedType(USERDEFINEDTYPELIST* str, int typeId, USERDEFINEDTYPE type)
{
    str->userdefinedtype[typeId] = type;  // replace existing entry
}

/** Change a structure element's property in the structure definition
*
* @param str The list of structure definitions.
* @param typeId The definition list entry to be modified
* @param element The structure element to be modified
* @param property The structure element's definition property to be modified
* @param value The new property value
* @return void.
*/
void
changeUserDefinedTypeElementProperty(USERDEFINEDTYPELIST* str, int typeId, char* element, char* property, void* value)
{
    USERDEFINEDTYPE* userdefinedtype = str->userdefinedtype; // Target this definition
    for (int i = 0; i < userdefinedtype[typeId].fieldcount; i++) {
        if (!strcmp(userdefinedtype[typeId].compoundfield[i].name, element)) {
            if (!strcmp("atomictype", property)) {
                userdefinedtype[typeId].compoundfield[i].atomictype = *(int*)value;
            } else if (!strcmp("type", property)) {
                strcpy(userdefinedtype[typeId].compoundfield[i].type, (char*)value);
            } else if (!strcmp("name", property)) {
                strcpy(userdefinedtype[typeId].compoundfield[i].name, (char*)value);
            } else if (!strcmp("desc", property)) {
                strcpy(userdefinedtype[typeId].compoundfield[i].desc, (char*)value);
            }
        }
    }
}

/** The number of Structure Definitions or User Defined Types in the structure list
*
* @param str The list of structure definitions.
* @return The count of structured types.
*/
int countUserDefinedType(USERDEFINEDTYPELIST* str)
{
    return str->listCount;   // Number of user defined types
}

/** Free heap from a Compound Field.
*
* @param str The Compound Field.
* @return void.
*/
void freeCompoundField(COMPOUNDFIELD* str)
{
    if (str == nullptr) return;
    free(str->shape);
    str->shape = nullptr;
}

/** Free heap from a User Defined Type.
*
* @param type The User Defined Type.
* @return void.
*/
void freeUserDefinedType(USERDEFINEDTYPE* type)
{
    if (type == nullptr) return;
    for (int i = 0; i < type->fieldcount; i++) {
        freeCompoundField(&type->compoundfield[i]);
    }
    free(type->compoundfield);
    type->compoundfield = nullptr;

    free(type->image);
    type->image = nullptr;
}

/** Free heap from a User Defined Type List.
*
* @param userdefinedtypelist The User Defined Type List.
* @return void.
*/
void freeUserDefinedTypeList(USERDEFINEDTYPELIST* userdefinedtypelist)
{
    if (userdefinedtypelist == nullptr) {
        return;
    }
    if (userdefinedtypelist->listCount == 0) {
        return;
    }
    if (userdefinedtypelist->userdefinedtype == nullptr) {
        return;
    }
    for (int i = 0; i < userdefinedtypelist->listCount; i++) {
        freeUserDefinedType(&userdefinedtypelist->userdefinedtype[i]);
    }
    free(userdefinedtypelist->userdefinedtype);
    initUserDefinedTypeList(userdefinedtypelist);
}

/** The size or byte count of an atomic or structured type
*
* @param type The name of the type
* @return The size in bytes.
*/
size_t getsizeof(USERDEFINEDTYPELIST* userdefinedtypelist, const char* type)
{
    USERDEFINEDTYPE* udt;
    const char* base = type;

    if (!strncmp(type, "const", 5)) {
        const char* p = strrchr(type, ' '); // ignore const
        if (p != nullptr) {
            base = &p[1];
        } else {
            base = &type[6];
        }
    }

    if (!strcasecmp(base, "FLOAT")) return sizeof(float);
    if (!strcasecmp(base, "DOUBLE")) return sizeof(double);
    if (!strcasecmp(base, "CHAR")) return sizeof(char);
    if (!strcasecmp(base, "UNSIGNED CHAR")) return sizeof(unsigned char);
    if (!strcasecmp(base, "UCHAR")) return sizeof(unsigned char);
    if (!strcasecmp(base, "STRING")) return sizeof(char);  // Same as char array but null terminated
    if (!strcasecmp(base, "SHORT")) return sizeof(short);
    if (!strcasecmp(base, "UNSIGNED SHORT")) return sizeof(unsigned short);
    if (!strcasecmp(base, "USHORT")) return sizeof(unsigned short);
    if (!strcasecmp(base, "INT")) return sizeof(int);
    if (!strcasecmp(base, "UNSIGNED INT")) return sizeof(unsigned int);
    if (!strcasecmp(base, "UINT")) return sizeof(unsigned int);
    if (!strcasecmp(base, "LONG")) return sizeof(long);
    if (!strcasecmp(base, "UNSIGNED LONG")) return sizeof(unsigned long);
    if (!strcasecmp(base, "ULONG")) return sizeof(unsigned long);
    if (!strcasecmp(base, "LONG LONG")) return sizeof(long long);
    if (!strcasecmp(base, "UNSIGNED LONG LONG")) return sizeof(unsigned long long);
    if (!strcasecmp(base, "LONG64")) return sizeof(long long);
    if (!strcasecmp(base, "ULONG64")) return sizeof(unsigned long long);
    if (!strcasecmp(base, "COMPLEX")) return 2 * sizeof(float);
    if (!strcasecmp(base, "DCOMPLEX")) return 2 * sizeof(double); // Always aligned on correct byte boundary

    // Search list of User defined types for size

    if ((udt = findUserDefinedType(userdefinedtypelist, base, 0)) != nullptr) {
        return (size_t)udt->size;
    }

    if (strchr(type, '*') != nullptr) {
        return sizeof(void*);
    }

    return 0;
}

/** The value of the IDAM enumeration type for a named regular atomic type
*
* @param type The name of the atomic type
* @return The integer value of the corresponding IDAM enumeration.
*/
int gettypeof(const char* type)
{     // **** const unsigned ....
    if (type == nullptr) return UDA_TYPE_UNKNOWN;
    if (!strcasecmp(type, "FLOAT")) return UDA_TYPE_FLOAT;
    if (!strcasecmp(type, "DOUBLE")) return UDA_TYPE_DOUBLE;
    if (!strcasecmp(type, "CHAR")) return UDA_TYPE_CHAR;
    if (!strcasecmp(type, "SHORT")) return UDA_TYPE_SHORT;
    if (!strcasecmp(type, "INT")) return UDA_TYPE_INT;
    if (!strcasecmp(type, "LONG")) return UDA_TYPE_LONG;
    if (!strcasecmp(type, "LONG64")) return UDA_TYPE_LONG64;
    if (!strcasecmp(type, "LONG LONG"))return UDA_TYPE_LONG64;
    if (!strcasecmp(type, "COMPLEX")) return UDA_TYPE_COMPLEX;
    if (!strcasecmp(type, "DCOMPLEX")) return UDA_TYPE_DCOMPLEX;
    if (!strcasecmp(type, "STRING")) return UDA_TYPE_STRING;
    if (!strcasecmp(type, "VOID")) return UDA_TYPE_VOID;

    if (!strcasecmp(type, "UCHAR")) return UDA_TYPE_UNSIGNED_CHAR;
    if (!strcasecmp(type, "USHORT")) return UDA_TYPE_UNSIGNED_SHORT;
    if (!strcasecmp(type, "UINT")) return UDA_TYPE_UNSIGNED_INT;
    if (!strcasecmp(type, "ULONG")) return UDA_TYPE_UNSIGNED_LONG;
#ifndef __APPLE__
    if (!strcasecmp(type, "ULONG64")) return UDA_TYPE_UNSIGNED_LONG64;
#endif
    if (!strcasecmp(type, "UNSIGNED CHAR")) return UDA_TYPE_UNSIGNED_CHAR;
    if (!strcasecmp(type, "UNSIGNED SHORT")) return UDA_TYPE_UNSIGNED_SHORT;
    if (!strcasecmp(type, "UNSIGNED INT")) return UDA_TYPE_UNSIGNED_INT;
    if (!strcasecmp(type, "UNSIGNED LONG")) return UDA_TYPE_UNSIGNED_LONG;
#ifndef __APPLE__
    if (!strcasecmp(type, "UNSIGNED LONG64")) return UDA_TYPE_UNSIGNED_LONG64;
    if (!strcasecmp(type, "UNSIGNED LONG LONG")) return UDA_TYPE_UNSIGNED_LONG64;
#endif
    return UDA_TYPE_UNKNOWN;  // Means Non Atomic => User defined structure type
}

/** Return structure element alignment byte boundary
*
* @param type The name of the structure atomic type.
* @return number of bytes to align with.
*
* Alignment rules are Architecture Dependent:
* Single byte numbers are aligned at a single byte boundary
* Two byte numbers are aligned with two byte boundaries
* Four byte numbers are aligned on four byte boundaries
* Eight byte numbers are aligned on four byte boundaries if 32 bit architecture and Linux
* or 8 byte boundary if 64 bit or 32 bit and Windows.
* Structures are aligned depending on whichever element has the largest alignment boundary.
*
* Structures between 1 and 4 bytes of data should be padded so that the total structure is 4 bytes.
* Structures between 5 and 8 bytes of data should be padded so that the total structure is 8 bytes.
* Structures between 9 and 16 bytes of data should be padded so that the total structure is 16 bytes.
* Structures greater than 16 bytes should be padded to 16 byte boundary.
*
* Indexing into arrays can be speeded up by making the structure size a power of 2. The compiler can then replace the
* multiply (entry = base address + index*size of structure) with a simple shift operation.
*
*
*/
int getalignmentof(const char* type)
{
    int is32 = (sizeof(void*) == POINTER_SIZE32); // Test architecture
    const char* base = type;

    if (!strncmp(type, "const", 5) || !strncmp(type, "unsigned", 8)) {  // ignore const and unsigned
        const char* p = strrchr(type, ' ');
        if (p != nullptr) {
            base = &p[1];
        } else {
            if (!strncmp(type, "const", 5)) {
                base = &type[6];
            } else {
                base = &type[9];
            }
        }
    }
    if (type == nullptr) return ALIGNMENT;
    if (strchr(base, '*'))return sizeof(void*);  // Pointer type

    if (is32) {
        if (!strcasecmp(base, "FLOAT")) return 4;  // 32 bit architecture
#ifndef WINDOWS
        if (!strcasecmp(base, "DOUBLE")) return 4;
#else
        if (!strcasecmp(base, "DOUBLE")) return 8;
#endif
        if (!strcasecmp(base, "CHAR")) return 1;
        if (!strcasecmp(base, "STRING")) return 1;
        if (!strcasecmp(base, "SHORT")) return 2;
        if (!strcasecmp(base, "INT")) return 4;
        if (!strcasecmp(base, "LONG")) return 4;
        if (!strcasecmp(base, "LONG64")) return 4;
        if (!strcasecmp(base, "LONG LONG")) return 4;
        if (!strcasecmp(base, "UCHAR")) return 1;
        if (!strcasecmp(base, "USHORT")) return 2;
        if (!strcasecmp(base, "UINT")) return 4;
        if (!strcasecmp(base, "ULONG")) return 4;
        if (!strcasecmp(base, "ULONG64")) return 4;
        if (!strcasecmp(base, "COMPLEX")) return 4;
        if (!strcasecmp(base, "DCOMPLEX")) return 4;
        if (!strcasecmp(base, "STRUCTURE")) return 1; // Structures are aligned depending on structure content
    } else {
#ifdef A64
        if (!strcasecmp(base, "FLOAT")) return 4;  // 64 bit architecture
        if (!strcasecmp(base, "DOUBLE")) return 8;
        if (!strcasecmp(base, "CHAR")) return 1;
        if (!strcasecmp(base, "STRING")) return 1;
        if (!strcasecmp(base, "SHORT")) return 2;
        if (!strcasecmp(base, "INT")) return 4;
        if (!strcasecmp(base, "LONG")) return 4;
        if (!strcasecmp(base, "LONG64")) return 8;
        if (!strcasecmp(base, "LONG LONG")) return 8;
        if (!strcasecmp(base, "UCHAR")) return 1;
        if (!strcasecmp(base, "USHORT")) return 2;
        if (!strcasecmp(base, "UINT")) return 4;
        if (!strcasecmp(base, "ULONG")) return 4;
        if (!strcasecmp(base, "ULONG64")) return 8;
        if (!strcasecmp(base, "COMPLEX")) return 4;
        if (!strcasecmp(base, "DCOMPLEX")) return 8;
        if (!strcasecmp(base, "STRUCTURE")) return 1;
#endif
    }
    return ALIGNMENT; // Best Guess!
}

size_t newoffset(size_t offset, const char* type)
{
    int alignment = getalignmentof(type);
    return (offset + ((alignment - (offset % alignment)) % alignment));
}

size_t padding(size_t offset, const char* type)
{
    int alignment = getalignmentof(type);
    return ((alignment - (offset % alignment)) % alignment);
}

/** The name of an atomic type corresponding to a value of the IDAM enumeration type.
*
* @param type The integer value of the type enumeration.
* @return The name of the atomic type.
*/
const char* udaNameType(UDA_TYPE type)
{
    switch (type) {
        case UDA_TYPE_CHAR:
            return "char";
        case UDA_TYPE_SHORT:
            return "short";
        case UDA_TYPE_INT:
            return "int";
        case UDA_TYPE_LONG:
            return "int";
        case UDA_TYPE_LONG64:
            return "long long";
        case UDA_TYPE_FLOAT:
            return "float";
        case UDA_TYPE_DOUBLE:
            return "double";
        case UDA_TYPE_UNSIGNED_CHAR:
            return "unsigned char";
        case UDA_TYPE_UNSIGNED_SHORT:
            return "unsigned short";
        case UDA_TYPE_UNSIGNED_INT:
            return "unsigned int";
        case UDA_TYPE_UNSIGNED_LONG:
            return "unsigned int";
        case UDA_TYPE_UNSIGNED_LONG64:
            return "unsigned long long";
        case UDA_TYPE_STRING:
            return "char";
        default:
            return "unknown";
    }
}

/** The size or byte count of a user defined structured type.
*
* @param str The user defined structure definition.
* @return The size in bytes.
*/
size_t getStructureSize(USERDEFINEDTYPELIST* userdefinedtypelist, USERDEFINEDTYPE* str)
{
    size_t size;
    if (str == nullptr) {
        return 0;
    }

    size_t byteCount = 0;
    size_t space0 = 0;
    int maxAlign = 0;
    size_t offset = 0;

    for (int i = 0; i < str->fieldcount; i++) {

        int alignment;

        if (str->compoundfield[i].pointer) {
            size = sizeof(void*);
            alignment = getalignmentof("*");
        } else {
            size = getsizeof(userdefinedtypelist, str->compoundfield[i].type);
            alignment = getalignmentof(str->compoundfield[i].type);
        }

        size_t space = size * str->compoundfield[i].count;

        if (i != 0) {
            if (str->compoundfield[i].pointer) {
                offset = newoffset(offset + space0, "*");
            } else {
                offset = newoffset(offset + space0, str->compoundfield[i].type);
                // Non-atomic types (structures) will already have been adjusted
            }
        } else {
            offset = 0;
            byteCount = 0;
        }
        byteCount = offset + space;
        if (alignment > maxAlign) {
            maxAlign = alignment;
        }

        space0 = space;  // Retain Previous values
    }

    // Add any additional structure packing to align the structure

    if (maxAlign) {
        byteCount = byteCount + ((maxAlign - (byteCount % maxAlign)) % maxAlign);
    }

    // Return the structure size

    return byteCount;
}

int findUserDefinedTypeId(USERDEFINEDTYPELIST* userdefinedtypelist, const char* name)
{

// Return the List Index key for a Named User Defined Structure

    for (int i = 0; i < userdefinedtypelist->listCount; i++) {
        if (!strcmp(userdefinedtypelist->userdefinedtype[i].name, name)) {
            return i;
        }
    }
#ifdef INCLUDESTRUCTPREFIX
    if (!strncmp(name, "struct ", 7)) {   // search without the struct prefix
        for (i=0; i<userdefinedtypelist->listCount; i++) {
            if (!strcmp(userdefinedtypelist->userdefinedtype[i].name, &name[7])) return(i);
        }
    } else {
        char work[MAXELEMENTNAME+25] = "struct ";  // search with the struct prefix
        strcat(work, name);
        for (i=0; i<userdefinedtypelist->listCount; i++) {
            if (!strcmp(userdefinedtypelist->userdefinedtype[i].name, work)) return(i);
        }
    }
#endif
    return -1;
}

USERDEFINEDTYPE* findUserDefinedType(USERDEFINEDTYPELIST* userdefinedtypelist, const char* name, int ref_id)
{
    // Return the Structure Definition of a Named User Defined Structure

    UDA_LOG(UDA_LOG_DEBUG, "findUserDefinedType: [%s]\n", name);
    UDA_LOG(UDA_LOG_DEBUG, "ref_id: %d\n", ref_id);
    UDA_LOG(UDA_LOG_DEBUG, "listCount: %d\n", userdefinedtypelist->listCount);

    if (name == nullptr) return nullptr;

    if (ref_id > 0 && name[0] != '\0') {
        for (int i = 0; i < userdefinedtypelist->listCount; i++) {
            if (!strcmp(userdefinedtypelist->userdefinedtype[i].name, name) &&
                userdefinedtypelist->userdefinedtype[i].ref_id == ref_id) {
                return (&userdefinedtypelist->userdefinedtype[i]);
            }
        }
        return nullptr;
    }

    if (ref_id == 0 && name[0] != '\0') {

        for (int i = 0; i < userdefinedtypelist->listCount; i++) {
            UDA_LOG(UDA_LOG_DEBUG, "[%2d]: [%s]\n", i, userdefinedtypelist->userdefinedtype[i].name);
            if (!strcmp(userdefinedtypelist->userdefinedtype[i].name, name)) {
                return (&userdefinedtypelist->userdefinedtype[i]);
            }
        }
        return nullptr;
    }

    if (ref_id != 0 && name[0] == '\0') {
        for (int i = 0; i < userdefinedtypelist->listCount; i++) {
            if (userdefinedtypelist->userdefinedtype[i].ref_id == ref_id) {
                return (&userdefinedtypelist->userdefinedtype[i]);
            }
        }
        return nullptr;
    }

    return nullptr;
}

int testUserDefinedType(USERDEFINEDTYPELIST* userdefinedtypelist, USERDEFINEDTYPE* udt)
{
    // Test a Structure Definition is a member of the Structure Type List
    // Return True (1) if the structure definition is found, False (0) otherwise.

    USERDEFINEDTYPE* test;

    if (udt == nullptr) return 0;
    for (int i = 0; i < userdefinedtypelist->listCount; i++) {
        test = &userdefinedtypelist->userdefinedtype[i];
        if (test == udt) {
            return 1;
        }
    }
    return 0;
}

//----------------------------------------------------------------------------------------------------------
/**User defined structure field definition for common types
*
* This is a public function
*
* @param field The user defined structure's field to be populated
* @param name Name of the structure's field.
* @param desc Description of the fields contents
* @param offset Current field byte offset from the start of the structure. Ppdated on return.
* @param type_id Enumerated key indicating the type of data field, e.g. float array
* @return Void
*/
void defineField(COMPOUNDFIELD* field, const char* name, const char* desc, int* offset, unsigned short type_id)
{
    initCompoundField(field);
    strcpy(field->name, name);

    field->pointer = 0;  // default for scalar values
    field->count = 1;

    switch (type_id) {
        case SCALARDOUBLE:
            field->atomictype = UDA_TYPE_DOUBLE;
            strcpy(field->type, "double");
            snprintf(field->desc, MAXELEMENTNAME, "[double %s] %s", name, desc);
            field->size = field->count * sizeof(double);
            break;
        case ARRAYDOUBLE:
            field->atomictype = UDA_TYPE_DOUBLE;
            strcpy(field->type, "double *");
            snprintf(field->desc, MAXELEMENTNAME, "[double *%s] %s", name, desc);
            field->pointer = 1;
            field->size = field->count * sizeof(double*);
            break;
        case SCALARFLOAT:
            field->atomictype = UDA_TYPE_FLOAT;
            strcpy(field->type, "float");
            snprintf(field->desc, MAXELEMENTNAME, "[float %s] %s", name, desc);
            field->size = field->count * sizeof(float);
            break;
        case ARRAYFLOAT:
            field->atomictype = UDA_TYPE_FLOAT;
            strcpy(field->type, "float *");
            snprintf(field->desc, MAXELEMENTNAME, "[float *%s] %s", name, desc);
            field->pointer = 1;
            field->size = field->count * sizeof(float*);
            break;
        case SCALARLONG64:
            field->atomictype = UDA_TYPE_LONG64;
            strcpy(field->type, "long long");
            snprintf(field->desc, MAXELEMENTNAME, "[long long %s] %s", name, desc);
            field->size = field->count * sizeof(long long);
            break;
        case ARRAYLONG64:
            field->atomictype = UDA_TYPE_LONG64;
            strcpy(field->type, "long long *");
            snprintf(field->desc, MAXELEMENTNAME, "[long long *%s] %s", name, desc);
            field->pointer = 1;
            field->size = field->count * sizeof(long long*);
            break;
        case SCALARULONG64:
            field->atomictype = UDA_TYPE_UNSIGNED_LONG64;
            strcpy(field->type, "unsigned long long");
            snprintf(field->desc, MAXELEMENTNAME, "[unsigned long long %s] %s", name, desc);
            field->size = field->count * sizeof(unsigned long long);
            break;
        case ARRAYULONG64:
            field->atomictype = UDA_TYPE_UNSIGNED_LONG64;
            strcpy(field->type, "unsigned long long *");
            snprintf(field->desc, MAXELEMENTNAME, "[unsigned long long *%s] %s", name, desc);
            field->pointer = 1;
            field->size = field->count * sizeof(unsigned long long*);
            break;
        case SCALARINT:
            field->atomictype = UDA_TYPE_INT;
            strcpy(field->type, "int");
            snprintf(field->desc, MAXELEMENTNAME, "[int %s] %s", name, desc);
            field->size = field->count * sizeof(int);
            break;
        case ARRAYINT:
            field->atomictype = UDA_TYPE_INT;
            strcpy(field->type, "int *");
            snprintf(field->desc, MAXELEMENTNAME, "[int *%s] %s", name, desc);
            field->pointer = 1;
            field->size = field->count * sizeof(int*);
            break;
        case SCALARUINT:
            field->atomictype = UDA_TYPE_UNSIGNED_INT;
            strcpy(field->type, "unsigned int");
            snprintf(field->desc, MAXELEMENTNAME, "[unsigned int %s] %s", name, desc);
            field->size = field->count * sizeof(unsigned int);
            break;
        case ARRAYUINT:
            field->atomictype = UDA_TYPE_UNSIGNED_INT;
            strcpy(field->type, "unsigned int *");
            snprintf(field->desc, MAXELEMENTNAME, "[unsigned int *%s] %s", name, desc);
            field->pointer = 1;
            field->size = field->count * sizeof(unsigned int*);
            break;
        case SCALARSHORT:
            field->atomictype = UDA_TYPE_SHORT;
            strcpy(field->type, "short");
            snprintf(field->desc, MAXELEMENTNAME, "[short %s] %s", name, desc);
            field->size = field->count * sizeof(short);
            break;
        case ARRAYSHORT:
            field->atomictype = UDA_TYPE_SHORT;
            strcpy(field->type, "short *");
            snprintf(field->desc, MAXELEMENTNAME, "[short *%s] %s", name, desc);
            field->pointer = 1;
            field->size = field->count * sizeof(short*);
            break;
        case SCALARUSHORT:
            field->atomictype = UDA_TYPE_UNSIGNED_SHORT;
            strcpy(field->type, "unsigned short");
            snprintf(field->desc, MAXELEMENTNAME, "[unsigned short %s] %s", name, desc);
            field->size = field->count * sizeof(unsigned short);
            break;
        case ARRAYUSHORT:
            field->atomictype = UDA_TYPE_UNSIGNED_SHORT;
            strcpy(field->type, "unsigned short *");
            snprintf(field->desc, MAXELEMENTNAME, "[unsigned short *%s] %s", name, desc);
            field->pointer = 1;
            field->size = field->count * sizeof(unsigned short*);
            break;
        case SCALARCHAR:
            field->atomictype = UDA_TYPE_CHAR;
            strcpy(field->type, "char");
            snprintf(field->desc, MAXELEMENTNAME, "[char %s] %s", name, desc);
            field->size = field->count * sizeof(char);
            break;
        case ARRAYCHAR:
            field->atomictype = UDA_TYPE_CHAR;
            strcpy(field->type, "char *");
            snprintf(field->desc, MAXELEMENTNAME, "[char *%s] %s", name, desc);
            field->pointer = 1;
            field->size = field->count * sizeof(char*);
            break;
        case SCALARSTRING:
            field->atomictype = UDA_TYPE_STRING;
            strcpy(field->type, "STRING");
            snprintf(field->desc, MAXELEMENTNAME, "[char *%s] %s", name, desc);
            field->pointer = 1;
            field->size = field->count * sizeof(char*);
            field->offset = (int)newoffset((size_t)*offset, "char *"); // must be an explicit char pointer (STRING Convention!)
            field->offpad = (int)padding((size_t)*offset, "char *");
            field->alignment = getalignmentof("char *");
            break;
        case ARRAYSTRING:
            //Bug Fix dgm 07Jul2014: atomictype was missing!
            field->atomictype = UDA_TYPE_STRING;
            strcpy(field->type, "STRING *");
            snprintf(field->desc, MAXELEMENTNAME, "[char **%s] %s", name, desc);
            field->pointer = 1;
            field->size = field->count * sizeof(char**);
            break;
        case ARRAYVOID:
            field->atomictype = UDA_TYPE_VOID;
            strcpy(field->type, "void *");
            snprintf(field->desc, MAXELEMENTNAME, "[void *%s] %s", name, desc);
            field->pointer = 1;
            field->size = field->count * sizeof(void*);
            break;
        case SCALARUCHAR:
            field->atomictype = UDA_TYPE_UNSIGNED_CHAR;
            strcpy(field->type, "unsigned char");
            snprintf(field->desc, MAXELEMENTNAME, "[char %s] %s", name, desc);
            field->size = field->count * sizeof(unsigned char);
            break;
        case ARRAYUCHAR:
            field->atomictype = UDA_TYPE_UNSIGNED_CHAR;
            strcpy(field->type, "unsigned char *");
            snprintf(field->desc, MAXELEMENTNAME, "[unsigned char *%s] %s", name, desc);
            field->pointer = 1;
            field->size = field->count * sizeof(unsigned char*);
            break;
    }

    field->rank = 0;
    field->shape = nullptr;

    if (type_id != SCALARSTRING) {
        field->offset = (int)newoffset(*offset, field->type);
        field->offpad = (int)padding(*offset, field->type);
        field->alignment = getalignmentof(field->type);
    }

    *offset = field->offset + field->size; // Next Offset
}
