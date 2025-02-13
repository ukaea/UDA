#include <uda/structured.h>

#include <logging/logging.h>

#include "structures/struct.h"
#include "structures/genStructs.h"
#include "structures/accessors.h"
#include "client2/thread_client.hpp"
#include "clientserver/error_log.h"
#include "common/string_utils.h"

using namespace uda::structures;
using namespace uda::logging;
using namespace uda::client_server;
using namespace uda::common;

NTREE* udaGetFullNTree()
{
    return get_full_ntree();
}

void udaSetFullNTree(NTREE* c_tree)
{
    auto* tree = static_cast<NTree*>(c_tree);
    set_full_ntree(tree);
}

void udaSetLastMallocIndexValue(unsigned int* last_malloc_index_value)
{
    set_last_malloc_index_value(last_malloc_index_value);
}

void udaResetLastMallocIndex()
{
    reset_last_malloc_index();
}

/** Print the Contents of a SArray data structure.
 *
 * @param fd A File Descriptor.
 * @param str A SArray data structure instance.
 * @return Void.
 */
void udaPrintSarray(SArray str)
{
    UDA_LOG(UDA_LOG_DEBUG, "SArray Contents");
    UDA_LOG(UDA_LOG_DEBUG, "Type : {}", str.type);
    UDA_LOG(UDA_LOG_DEBUG, "Rank : {}", str.rank);
    UDA_LOG(UDA_LOG_DEBUG, "Count: {}", str.count);
    if (str.rank > 0 && str.shape != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "shape  : [{}", str.shape[0]);
        for (int i = 1; i < str.rank; i++) {
            if (i < str.rank - 1) {
                UDA_LOG(UDA_LOG_DEBUG, "{},", str.shape[i]);
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "{}", str.shape[i]);
            }
        }
        UDA_LOG(UDA_LOG_DEBUG, "]");
    }
    UDA_LOG(UDA_LOG_DEBUG, "");
}

/** Add an NTree List entry.
 *
 * @param node A NTree node to add.
 * @return Void.
 */
void udaAddNTreeList(NTREE* node, NTREELIST* ntree_list)
{
    auto* tree = static_cast<NTree*>(node);
    auto* list = static_cast<NTreeList*>(ntree_list);

    const auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();

    add_ntree_list(log_malloc_list, tree, list);
}

/** Add an NTree node to an array of child nodes.
 *
 * @param parent A NTree node with a set of child nodes
 * @param child A NTree node to add to the existing set of child nodes
 * @return Void.
 */
void udaAddNTree(NTREE* c_parent, NTREE* c_child)
{
    auto child = static_cast<NTree*>(c_child);
    auto parent = static_cast<NTree*>(c_parent);

    return add_ntree(parent, child);
}

/** Free an NTree node together with the array of child nodes.
 *
 * @param tree A NTree node with or without a set of child nodes
 * @return Void.
 */
void udaFreeNTreeNode(NTREE* c_ntree)
{
    auto* tree = static_cast<NTree*>(c_ntree);
    free_ntree_node(tree);
}

/** Add a new image line to the existing image.
 *
 * @param image A block of bytes used to record structure definition image data.
 * @param imagecount The current count of bytes used to record the present image.
 * @param line A new image line to add to the existing image.
 * @return Both image and image count are updated on return.
 */
void udaAddImage(char** image, int* imagecount, const char* line)
{
    add_image(image, imagecount, line);
}

/** Expand an image line that contains header defines and include the numerical value
 *
 * @param buffer An image line to be expanded
 * @param defnames An array of define names
 * @param defvalues An array of define values
 * @param defCount The number of define names and values
 * @param expand A pre-allocated array of char to be used to receive the expanded buffer string.
 * @return expand An expanded Image line.
 */
void udaExpandImage(char* buffer, char defnames[MAXELEMENTS][MAXELEMENTNAME], int* defvalues, int defCount,
                    char* expand)
{
    expand_image(buffer, defnames, defvalues, defCount, expand);
}

//==============================================================================================================
// Utility Functions

/** Add a stack memory location to the LogMallocList data structure. These are not freed.
 *
 * @param stack The memory location.
 * @param count The number of elements allocated.
 * @param size The size of a single element.
 * @param type The name of the type allocated.
 * @return void.
 */
void udaAddNonMalloc(void* stack, int count, size_t size, const char* type)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();

    add_non_malloc(log_malloc_list, stack, count, size, type);
}

/** Add a stack memory location to the LogMallocList data structure. These are not freed.
*
* @param stack The memory location.
* @param count The number of elements allocated.
* @param size The size of a single element.
* @param type The name of the type allocated.
* @param rank The rank of the allocated array.
* @param shape The shape of the allocated array. Only required when rank > 1.

* @return void.
*/
void udaAddNonMalloc2(void* stack, int count, size_t size, const char* type, int rank,
                      int* shape)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();

    // Put a non malloc'd memory location on the malloc log flagging it as freed

    if (log_malloc_list->listcount + 1 >= log_malloc_list->listsize) {
        log_malloc_list->logmalloc = (LogMalloc*)realloc(
            (void*)log_malloc_list->logmalloc, (log_malloc_list->listsize + GROWMALLOCLIST) * sizeof(LogMalloc));
        log_malloc_list->listsize = log_malloc_list->listsize + GROWMALLOCLIST;
    }

    log_malloc_list->logmalloc[log_malloc_list->listcount].count = count;
    log_malloc_list->logmalloc[log_malloc_list->listcount].size = size;
    log_malloc_list->logmalloc[log_malloc_list->listcount].freed = 1;
    log_malloc_list->logmalloc[log_malloc_list->listcount].heap = stack;
    strcpy(log_malloc_list->logmalloc[log_malloc_list->listcount].type, type);

    log_malloc_list->logmalloc[log_malloc_list->listcount].rank = rank;
    if (rank > 1) {
        log_malloc_list->logmalloc[log_malloc_list->listcount].shape = shape;
    } else {
        log_malloc_list->logmalloc[log_malloc_list->listcount].shape = nullptr;
    }

    log_malloc_list->listcount++;
}

/** Add a heap memory location to the LogMallocList data structure. These are freed.
 *
 * @param heap The memory location.
 * @param count The number of elements allocated.
 * @param size The size of a single element.
 * @param type The name of the type allocated.
 * @return void.
 */
void udaAddMalloc(void* heap, int count, size_t size, const char* type)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();

    add_malloc(log_malloc_list, heap, count, size, type);
}

/** Add a heap memory location to the LogMallocList data structure. These are freed.
 *
 * @param heap The memory location.
 * @param count The number of elements allocated.
 * @param size The size of a single element.
 * @param type The name of the type allocated.
 * @param rank The rank of the allocated array.
 * @param shape The shape of the allocated array. Only required when rank > 1.
 * @return void.
 */
void udaAddMalloc2(void* heap, int count, size_t size, const char* type, int rank,
                   int* shape)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();

    add_malloc2(log_malloc_list, heap, count, size, type, rank, shape);
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
void udaChangeMalloc(VOIDTYPE old, void* anew, int count, size_t size,
                     const char* type)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();

    change_malloc(log_malloc_list, old, anew, count, size, type);
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
void udaChangeNonMalloc(void* old, void* anew, int count, size_t size,
                        const char* type)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();

    // Change a non-malloc List Entry

    VOIDTYPE target, candidate;
    if (old == nullptr) {
        udaAddNonMalloc(anew, count, size, type);
        return;
    }
    target = (VOIDTYPE)((VOIDTYPE*)old);
    for (int i = 0; i < log_malloc_list->listcount; i++) {
        candidate = (VOIDTYPE)((VOIDTYPE*)log_malloc_list->logmalloc[i].heap);
        if (target == candidate) {
            log_malloc_list->logmalloc[i].heap = anew;
            log_malloc_list->logmalloc[i].freed = 1;
            log_malloc_list->logmalloc[i].count = count;
            log_malloc_list->logmalloc[i].size = size;
            strcpy(log_malloc_list->logmalloc[i].type, type);
            return;
        }
    }
}

/** Check allocated heap memory for Duplicates - Isolate to prevent double free error.
 *
 * @return void.
 */
#ifdef A64

int compare_ulonglong(const void* a, const void* b)
{
    const unsigned long long* da = (const unsigned long long*)a;
    const unsigned long long* db = (const unsigned long long*)b;
    return (int)(*da > *db) - (int)(*da < *db);
}

#else
static int compare_ulong(const void* a, const void* b)
{
    const unsigned long* da = (const unsigned long*)a;
    const unsigned long* db = (const unsigned long*)b;
    return (int)(*da > *db) - (int)(*da < *db);
}
#endif

int udaDupCountMallocLog(LOGMALLOCLIST* c_str)
{
    auto str = static_cast<LogMallocList*>(c_str);

    int sortCount = 0, dupCount = 0;
    int compare_ulonglong(const void*, const void*);
    if (str == nullptr) {
        return 0;
    }
    if (str->listcount <= 1) {
        return 0;
    }
#ifdef A64
    unsigned long long* sorted = (unsigned long long*)malloc(str->listcount * sizeof(unsigned long long));
    for (int i = 0; i < str->listcount; i++) {
        if (str->logmalloc[i].freed == 0) {
            sorted[sortCount++] = (unsigned long long)str->logmalloc[i].heap;
        }
    }
    qsort((void*)sorted, (size_t)sortCount, (size_t)sizeof(unsigned long long), compare_ulonglong);
#else
    unsigned long* sorted = (unsigned long*)malloc(str->listcount * sizeof(unsigned long));
    for (int i = 0; i < str->listcount; i++) {
        if (str->logmalloc[i].freed == 0) {
            sorted[sortCount++] = (unsigned long)str->logmalloc[i].heap;
        }
    }
    qsort((void*)sorted, (size_t)sortCount, (size_t)sizeof(unsigned long), compare_ulong);
#endif
    for (int i = 1; i < sortCount; i++) {
        if (sorted[i] == sorted[i - 1]) {
            dupCount++;
        }
    }
    free(sorted);
    return dupCount;
}

/** Free allocated heap memory but preserve the addresses. There are no arguments.
 *
 * @return void.
 */
void udaFreeMallocLog()
{
    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();

    free_malloc_log(log_malloc_list);
}

/** Find the meta data associated with a specific memory location.
 *
 * @param heap The target memory location.
 * @param count The returned allocation count.
 * @param size The returned allocation size.
 * @param type The returned allocation type.
 * @return void.
 */
void udaFindMalloc(void* heap, int* count, int* size, const char** type)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();

    return find_malloc(log_malloc_list, heap, count, size, type);
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
void udaFindMalloc2(void* heap, int* count, int* size, const char** type, int* rank,
                    int** shape)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();

    return find_malloc2(log_malloc_list, heap, count, size, type, rank, shape);
}

/** Add a heap memory location to the LogStructList data structure. These are freed.
 *
 * @param heap The memory location.
 * @param type The name of the type allocated.
 * @return void.
 */
void udaAddStruct(void* heap, const char* type, LOGSTRUCTLIST* c_log_struct_list)
{
    auto log_struct_list = static_cast<LogStructList*>(c_log_struct_list);

    // Log all dispatched/received Structures
    // Grow the list when necessary

    if (heap == nullptr) {
        return;
    }

    if (log_struct_list->listcount + 1 >= log_struct_list->listsize) {
        log_struct_list->logstruct = (LogStruct*)realloc(
            (void*)log_struct_list->logstruct, (log_struct_list->listsize + GROWMALLOCLIST) * sizeof(LogStruct));
        log_struct_list->listsize = log_struct_list->listsize + GROWMALLOCLIST;
    }

    log_struct_list->logstruct[log_struct_list->listcount].id = log_struct_list->listcount + 1;
    log_struct_list->logstruct[log_struct_list->listcount].heap = heap;
    strcpy(log_struct_list->logstruct[log_struct_list->listcount].type, type);

    log_struct_list->listcount++;
}

/** Find the meta data associated with a specific Structure.
 *
 * @param heap The target memory location.
 * @param type The returned structure type.
 * @return The structure id.
 */
int udaFindStructId(void* heap, char** type)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto* log_struct_list = instance.log_struct_list();

    return find_struct_id(log_struct_list, heap, type);
}

/** Find the Heap address and Data Type of a specific Structure.
 *
 * @param id The structure id.
 * @param type The returned structure type.
 * @return The heap memory location
 */
void* udaFindStructHeap(int id, char** type)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto* log_struct_list = instance.log_struct_list();

    return find_struct_heap(log_struct_list, id, type);
}

/** Copy a User Defined Structure Definition.
 *
 * @param old The type definition to be copied.
 * @param anew The copy of the type definition.
 * @return void.
 */
void udaCopyUserDefinedType(USERDEFINEDTYPE* c_old, USERDEFINEDTYPE* c_anew)
{
    auto old = static_cast<UserDefinedType*>(c_old);
    auto anew = static_cast<UserDefinedType*>(c_anew);

    UserDefinedType udt;
    init_user_defined_type(&udt);
    udt = *old;
    udt.image = (char*)malloc((old->imagecount) * sizeof(char));
    memcpy(udt.image, old->image, old->imagecount);
    udt.compoundfield = (CompoundField*)malloc((old->fieldcount) * sizeof(CompoundField));
    for (int i = 0; i < old->fieldcount; i++) {
        init_compound_field(&udt.compoundfield[i]);
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


/** Change a structure element's property in the structure definition
 *
 * @param str The list of structure definitions.
 * @param typeId The definition list entry to be modified
 * @param element The structure element to be modified
 * @param property The structure element's definition property to be modified
 * @param value The new property value
 * @return void.
 */
void udaChangeUserDefinedTypeElementProperty(int typeId, char* element,
                                             char* property, void* value)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto* user_defined_type_list = instance.user_defined_type_list();

    UserDefinedType* user_defined_type = user_defined_type_list->userdefinedtype; // Target this definition
    for (int i = 0; i < user_defined_type[typeId].fieldcount; i++) {
        if (!strcmp(user_defined_type[typeId].compoundfield[i].name, element)) {
            if (!strcmp("atomictype", property)) {
                user_defined_type[typeId].compoundfield[i].atomictype = *(int*)value;
            } else if (!strcmp("type", property)) {
                strcpy(user_defined_type[typeId].compoundfield[i].type, (char*)value);
            } else if (!strcmp("name", property)) {
                strcpy(user_defined_type[typeId].compoundfield[i].name, (char*)value);
            } else if (!strcmp("desc", property)) {
                strcpy(user_defined_type[typeId].compoundfield[i].desc, (char*)value);
            }
        }
    }
}

/** The number of Structure Definitions or User Defined Types in the structure list
 *
 * @param str The list of structure definitions.
 * @return The count of structured types.
 */
int udaCountUserDefinedType(USERDEFINEDTYPELIST* c_user_defined_type_list)
{
    auto user_defined_type_list = static_cast<UserDefinedTypeList*>(c_user_defined_type_list);
    return user_defined_type_list->listCount; // Number of user defined types
}

/** The size or byte count of an atomic or structured type
 *
 * @param type The name of the type
 * @return The size in bytes.
 */
size_t udaGetsizeof(const char* type)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto* user_defined_type_list = instance.user_defined_type_list();

    return get_size_of(user_defined_type_list, type);
}

/** The value of the IDAM enumeration type for a named regular atomic type
 *
 * @param type The name of the atomic type
 * @return The integer value of the corresponding IDAM enumeration.
 */
int udaGettypeof(const char* type)
{
    if (type == nullptr) {
        return UDA_TYPE_UNKNOWN;
    }
    return get_type_of(type);
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
int udaGetalignmentof(const char* type) {
    return get_alignment_of(type);
}

size_t udaNewoffset(size_t offset, const char* type) {
    return new_offset(offset, type);
}

size_t udaPadding(size_t offset, const char* type) {
    return padding(offset, type);
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
size_t udaGetStructureSize(USERDEFINEDTYPE* c_user_defined_type) {
    auto& instance = uda::client::ThreadClient::instance();
    auto* user_defined_type_list = instance.user_defined_type_list();

    auto user_defined_type = static_cast<UserDefinedType*>(c_user_defined_type);
    return get_structure_size(user_defined_type_list, user_defined_type);
}

/** Print an error message.
 *
 * @param fd The output file descriptor pointer.
 * @param warning Print a warning message rather than an error message.
 * @param line The line number where the error occured.
 * @param file The file name where the error occured.
 * @param msg The message to print.
 * @return The size in bytes.
 */
void udaPrintError(int warning, int line, char* file, char* msg)
{
    if (warning) {
        UDA_LOG(UDA_LOG_DEBUG, "WARNING: line {}, file {}\n{}", line, file, msg);
    } else {
        UDA_LOG(UDA_LOG_ERROR, "ERROR: line {}, file {}\n{}", line, file, msg);
    }
}

int udaFindUserDefinedTypeId(const char* name)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto* user_defined_type_list = instance.user_defined_type_list();

    // Return the List Index key for a Named User Defined Structure

    for (int i = 0; i < user_defined_type_list->listCount; i++) {
        if (!strcmp(user_defined_type_list->userdefinedtype[i].name, name)) {
            return i;
        }
    }
#ifdef INCLUDESTRUCTPREFIX
    if (!strncmp(name, "struct ", 7)) { // search without the struct prefix
        for (i = 0; i < user_defined_type_list->listCount; i++) {
            if (!strcmp(user_defined_type_list->userdefinedtype[i].name, &name[7])) {
                return (i);
            }
        }
    } else {
        char work[MAXELEMENTNAME + 25] = "struct "; // search with the struct prefix
        strcat(work, name);
        for (i = 0; i < user_defined_type_list->listCount; i++) {
            if (!strcmp(user_defined_type_list->userdefinedtype[i].name, work)) {
                return (i);
            }
        }
    }
#endif
    return -1;
}

USERDEFINEDTYPE* udaFindUserDefinedType(const char* name, int ref_id)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto* user_defined_type_list = instance.user_defined_type_list();

    return find_user_defined_type(user_defined_type_list, name, ref_id);
}

int udaTestUserDefinedType(USERDEFINEDTYPE* udt)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto* user_defined_type_list = instance.user_defined_type_list();

    // Test a Structure Definition is a member of the Structure Type List
    // Return True (1) if the structure definition is found, False (0) otherwise.

    UserDefinedType* test;

    if (udt == nullptr) {
        return 0;
    }
    for (int i = 0; i < user_defined_type_list->listCount; i++) {
        test = &user_defined_type_list->userdefinedtype[i];
        if (test == udt) {
            return 1;
        }
    }
    return 0;
}

//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
// Client Utility functions operating on Linked List N-Tree

/** Print the data from an array of Atomic Type to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param data A pointer to the data.
 * @param atomictype The name of a Atomic type.
 * @param count The array element count.
 * @param label A label to print before the value.
 * @return Void
 */
void udaPrintAtomicData(void* data, int atomictype, int count, const char* label)
{
    print_atomic_data(data, atomictype, count, label);
}

/** Print the data from a named array of Atomic Type from a given tree node to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param tree A pointer to a tree node.
 * @param target The name of a User Defined Structure type.
 * @return Void
 *
 * \todo {When the structure is an array, either print data from a single array element or print data from
 * all structure elements}
 */
void udaPrintAtomicType(NTREE* c_tree, const char* target)
{
    auto* tree = static_cast<NTree*>(c_tree);
    const auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();

    print_atomic_type(log_malloc_list, tree, target);
}

/** Print the Count of elements of a named data array from a given tree node to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param tree A pointer to a tree node.
 * @param target The name of a Structure element.
 * @return Void
 */
void udaPrintTypeCount(NTREE* c_tree, const char* target)
{
    auto* tree = static_cast<NTree*>(c_tree);
    print_type_count(tree, target);
}

// The compound field element of the structure definition contains the count, rank and shape details
// of non-pointer data.
// Pointer data have a count from the malloc log.

//---------------------------------------------------------------------------------------------
// Node Data Structure Component

/** Return a pointer to a User Defined Structure Component Structure.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @param target The name of a Structure Element.
 * @return the Structure Element Definition Structure.
 */
COMPOUNDFIELD* udaGetNodeStructureComponent(NTREE* c_tree, const char* target)
{
    UserDefinedType* user_defined_type;
    int fieldcount;
    const char* lastname;

    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }

    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();
    auto* tree = static_cast<NTree*>(c_tree);

    // Locate the Node with a Structure Component
    tree = find_ntree_structure_component2(log_malloc_list, tree, target, &lastname);

    if (tree != nullptr) {
        user_defined_type = tree->userdefinedtype;
        fieldcount = tree->userdefinedtype->fieldcount;
        for (int i = 0; i < fieldcount; i++) {
            if (!strcmp(user_defined_type->compoundfield[i].name, target)) {
                return &user_defined_type->compoundfield[i];
            }
        }
    }
    return nullptr;
}

//---------------------------------------------------------------------------------------------
// Tree Node Family: Single tree node is in scope



/** Print the Contents of a tree node to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return Void
 */
void udaPrintNode(NTREE* c_tree)
{
    auto* tree = static_cast<NTree*>(c_tree);
    print_node(tree);
}

/** Print the Contents of a tree node with the specified User Defined Structure name to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param target The name of a User Defined Structure type. If an null string is passed, the structure
 *  of the root node is used.
 * @return Void
 */
void udaPrintNodeStructureDefinition(const char* target)
{
    NTree* tree = nullptr;
    if (target[0] != '\0') {
        if ((tree = find_ntree_structure_definition(tree, target)) == nullptr) {
            UDA_LOG(UDA_LOG_DEBUG, "the Structure Definition for {} could not be Found", target);
            return;
        }
    }
    udaPrintNode(tree);
}

/** Print an Image of the Named Structure Definition to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param target The name of a User Defined Structure type.
 * @return Void
 */

void udaPrintNodeStructureImage(const char* target)
{
    NTREE* c_tree = nullptr;
    auto* tree = static_cast<NTree*>(c_tree);

    if (target[0] != '\0') {
        if ((tree = find_ntree_structure_definition(tree, target)) == nullptr) {
            UDA_LOG(UDA_LOG_DEBUG, "the Structure Definition for {} could not be Found", target);
            return;
        }
        print_image(tree->userdefinedtype->image, tree->userdefinedtype->imagecount);
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "no Structure Definition name was given!");
    }
}

/** Return a Pointer to the User Defined Type Structure of the data attached to this tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the User Defined Type Structure Definition.
 */

USERDEFINEDTYPE* udaGetNodeUserDefinedType(NTREE* c_tree)
{
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    auto* tree = static_cast<NTree*>(c_tree);
    return tree->userdefinedtype;
}

/** Return the name of the User Defined Type Structure.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the name of the User Defined Type Structure.
 */
char* udaGetNodeStructureName(NTREE* c_tree)
{
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    auto* tree = static_cast<NTree*>(c_tree);
    return tree->name;
}

/** Return the Type of the User Defined Type Structure.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the Type of the User Defined Type Structure.
 */
char* udaGetNodeStructureType(NTREE* c_tree)
{
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    auto* tree = static_cast<NTree*>(c_tree);
    return tree->userdefinedtype->name;
}

/** Return the Size of the User Defined Type Structure.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the Size (Bytes) of the User Defined Type Structure.
 */
int udaGetNodeStructureSize(NTREE* c_tree)
{
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    auto* tree = static_cast<NTree*>(c_tree);
    return tree->userdefinedtype->size;
}

/** Return a pointer to a Tree Nodes's Data Structure Array element.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @param index The array index
 * @return a Pointer to a Structure Array element.
 */
void* udaGetNodeStructureArrayData(NTREE* c_tree, int index)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto& error_stack = instance.error_stack();
    auto* log_malloc_list = instance.log_malloc_list();

    char* p;
    if (index < 0) {
        add_error(error_stack, ErrorType::Code, "udaGetNodeStructureArrayData", 999, "The Tree Node array index < 0");
        return nullptr;
    }
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    
    auto* tree = static_cast<NTree*>(c_tree);
    
    if (get_node_structure_data_count(log_malloc_list, tree) < (index + 1)) {
        add_error(error_stack, ErrorType::Code, "udaGetNodeStructureArrayData", 999,
                  "The Tree Node array index > allocated array dimension");
        return nullptr;
    }
    p = (char*)tree->data;
    return (void*)&p[index * tree->userdefinedtype->size];
}

/** Return a pointer to a Component Data Structure Array element.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @param target The name of the Structure Array element.
 * @param structureindex The Array index
 * @param componentindex The structure element index
 * @return a Pointer to a Component Structure Array element.
 */
void* udaGetNodeStructureComponentArrayData(NTREE* c_tree, const char* target,
                                            int structureindex, int componentindex)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto& error_stack = instance.error_stack();

    int offset, count, size;
    char* p;
    char* pp;
    const char* type;
    if (structureindex < 0) {
        add_error(error_stack, ErrorType::Code, "udaGetNodeStructureComponentArrayData", 999,
                  "The Tree Node Structure array index < 0");
    }
    if (componentindex < 0) {
        add_error(error_stack, ErrorType::Code, "udaGetNodeStructureComponentArrayData", 999,
                  "The Tree Node Structure Component array index < 0");
        return nullptr;
    }
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }

    if ((pp = (char*)udaGetNodeStructureArrayData(c_tree, structureindex)) == nullptr) {
        return nullptr;
    }

    auto* tree = static_cast<NTree*>(c_tree);
    for (int i = 0; i < tree->userdefinedtype->fieldcount; i++) {
        if (!strcmp(tree->userdefinedtype->compoundfield[i].name, target)) {
            offset = tree->userdefinedtype->compoundfield[i].offset;
            if (tree->userdefinedtype->compoundfield[i].pointer) {
                p = (char*)*((VOIDTYPE*)&pp[offset]); // Data Element from the single Structure Array Element
                udaFindMalloc(p, &count, &size, &type);
            } else {
                p = &pp[offset];
                size = tree->userdefinedtype->compoundfield[i].size;
                count = tree->userdefinedtype->compoundfield[i].count;
            }
            if (size == 0) {
                return nullptr;
            }
            if (count <= componentindex) {
                add_error(error_stack, ErrorType::Code, "udaGetNodeStructureComponentArrayData", 999,
                          "The Tree Node Structure Component array index > allocated array dimension");
                return nullptr;
            }
            return (void*)&p[componentindex * size];
        }
    }
    add_error(error_stack, ErrorType::Code, "udaGetNodeStructureComponentArrayData", 999,
              "The named Tree Node Structure Component array is not a member of the Data structure");
    return nullptr;
}


/** Return the count of child User Defined Type Structures (elements of this structure).
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the name of the User Defined Type Structure.
 */
int udaGetNodeChildrenCount(NTREE* c_tree)
{
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    auto* tree = static_cast<NTree*>(c_tree);
    return tree->branches;
}

/** Return a Child Node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @param child A integer index identifying which child from the child array to return
 * @return the Child Node.
 */
NTREE* udaGetNodeChild(NTREE* c_tree, int child)
{
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    auto* tree = static_cast<NTree*>(c_tree);
    if (child < 0 || child >= tree->branches) {
        return nullptr;
    }
    return tree->children[child];
}

/** Return a Child Node'd ID (Branch index value).
 *
 * @param tree A pointer to a Parent tree node. If nullptr the root node is assumed.
 * @param child A ipointer to a Child tree node.
 * @return the Child Node's ID.
 */
int udaGetNodeChildId(NTREE* c_tree, NTREE* child)
{
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    if (child == nullptr) {
        return -1;
    }
    auto* tree = static_cast<NTree*>(c_tree);
    if (tree->branches == 0) {
        return -1;
    }
    for (int i = 0; i < tree->branches; i++) {
        if (tree->children[i] == child) {
            return i;
        }
    }
    return -1;
}

/** Return the parent Node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the Parent Node.
 */
NTREE* udaGetNodeParent(NTREE* c_tree)
{
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    auto* tree = static_cast<NTree*>(c_tree);
    return tree->parent;
}

/** Return the Data pointer.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return a Pointer to the Data.
 */
void* udaGetNodeData(NTREE* c_tree)
{
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    auto* tree = static_cast<NTree*>(c_tree);
    return tree->data;
}

/** Return a Count of Structured Component Types attached to a tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the Count of Structured types.
 */

int udaGetNodeStructureCount(NTREE* c_tree)
{
    auto* tree = static_cast<NTree*>(c_tree);
    return get_node_structure_count(tree);
}

/** Return a Count of Atomic Component Types attached to a tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the Count of Atomic types.
 */

int udaGetNodeAtomicCount(NTREE* c_tree)
{
    auto* tree = static_cast<NTree*>(c_tree);
    return get_node_atomic_count(tree);
}

/** Return a List of Structure component Names attached to a tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of Structure names.
 */

char** udaGetNodeStructureNames(NTREE* c_tree)
{
    auto* tree = static_cast<NTree*>(c_tree);
    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();

    return get_node_structure_names(log_malloc_list, tree);
}

/** Return a List of Atomic component Names attached to a tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of Atomic element names.
 */

char** udaGetNodeAtomicNames(NTREE* c_tree)
{
    auto* tree = static_cast<NTree*>(c_tree);
    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();

    return get_node_atomic_names(log_malloc_list, tree);
}

/** Return a List of Structure Component Type Names attached to a tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of Structure Type names.
 */

char** udaGetNodeStructureTypes(NTREE* c_tree)
{
    auto* tree = static_cast<NTree*>(c_tree);
    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();

    return get_node_structure_types(log_malloc_list, tree);
}

/** Return a List of Atomic Component Type Names attached to a tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of Atomic Type names.
 */

char** udaGetNodeAtomicTypes(NTREE* c_tree)
{
    auto* tree = static_cast<NTree*>(c_tree);
    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();

    return get_node_atomic_types(log_malloc_list, tree);
}

/** Return a List of Structure Component Pointer property attached to a tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of Structure Pointer Properties.
 */

int* udaGetNodeStructurePointers(NTREE* c_tree)
{
    int count;
    int* pointers;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    if ((count = udaGetNodeStructureCount(c_tree)) == 0) {
        return nullptr;
    }
    pointers = (int*)malloc(count * sizeof(int));
    udaAddMalloc(pointers, count, sizeof(int), "int");

    auto* tree = static_cast<NTree*>(c_tree);
    count = 0;
    for (int i = 0; i < tree->userdefinedtype->fieldcount; i++) {
        if (tree->userdefinedtype->compoundfield[i].atomictype == UDA_TYPE_UNKNOWN) {
            pointers[count] = tree->userdefinedtype->compoundfield[i].pointer;
            count++;
        }
    }
    return pointers;
}

/** Return a List of Atomic Component Pointer property attached to a tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of Atomic Pointer Properties.
 */

int* udaGetNodeAtomicPointers(NTREE* c_tree)
{
    int count;
    int* pointers;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    if ((count = udaGetNodeAtomicCount(c_tree)) == 0) {
        return nullptr;
    }
    pointers = (int*)malloc(count * sizeof(int));
    udaAddMalloc(pointers, count, sizeof(int), "int");

    auto* tree = static_cast<NTree*>(c_tree);
    count = 0;
    for (int i = 0; i < tree->userdefinedtype->fieldcount; i++) {
        if (tree->userdefinedtype->compoundfield[i].atomictype != UDA_TYPE_UNKNOWN) {
            pointers[count] = tree->userdefinedtype->compoundfield[i].pointer;
            // if (!strcmp(tree->userdefinedtype->compoundfield[i].type, "STRING *")) pointers[count] = 1;
            count++;
        }
    }
    return pointers;
}

/** Return a List of Rank values of the Structure Components attached to a tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of Structure Ranks.
 */

int* udaGetNodeStructureRank(NTREE* c_tree)
{
    int count, count0, size, rank;
    int *ranks, *shape;
    const char* type;
    char* data;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    if ((count = udaGetNodeStructureCount(c_tree)) == 0) {
        return nullptr;
    }
    ranks = (int*)malloc(count * sizeof(int));
    udaAddMalloc(ranks, count, sizeof(int), "int");

    auto* tree = static_cast<NTree*>(c_tree);
    count = 0;
    for (int i = 0; i < tree->userdefinedtype->fieldcount; i++) {
        if (tree->userdefinedtype->compoundfield[i].atomictype == UDA_TYPE_UNKNOWN) {
            if (!tree->userdefinedtype->compoundfield[i].pointer) {
                ranks[count] = tree->userdefinedtype->compoundfield[i].rank;
            } else {
                if ((data = (char*)tree->data) == nullptr) {
                    return nullptr;
                }
                udaFindMalloc2(&data[tree->userdefinedtype->compoundfield[i].offset], &count0, &size,
                               &type, &rank, &shape);
                ranks[count] = rank;
            }
            count++;
        }
    }
    return ranks;
}

/** Return a List of Rank values of the Atomic Components attached to a tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of Atomic Ranks.
 */

int* udaGetNodeAtomicRank(NTREE* c_tree)
{
    int count, count0, size, rank;
    int *ranks, *shape;
    const char* type;
    char* data;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    if ((count = udaGetNodeAtomicCount(c_tree)) == 0) {
        return nullptr;
    }
    ranks = (int*)malloc(count * sizeof(int));
    udaAddMalloc(ranks, count, sizeof(int), "int");

    auto* tree = static_cast<NTree*>(c_tree);
    count = 0;
    for (int i = 0; i < tree->userdefinedtype->fieldcount; i++) {
        if (tree->userdefinedtype->compoundfield[i].atomictype != UDA_TYPE_UNKNOWN) {
            if (!tree->userdefinedtype->compoundfield[i].pointer) {
                ranks[count] = tree->userdefinedtype->compoundfield[i].rank;
            } else {
                if ((data = (char*)tree->data) == nullptr) {
                    return nullptr;
                }
                udaFindMalloc2(&data[tree->userdefinedtype->compoundfield[i].offset], &count0, &size,
                               &type, &rank, &shape);
                ranks[count] = rank;
            }
            count++;
        }
    }
    return ranks;
}

/** Return a List of Shape Arrays of the Structure Components attached to a tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of Structure Shape Arrays.
 */
int** udaGetNodeStructureShape(NTREE* c_tree)
{
    int count, count0, size, rank;
    int* shape;
    int** shapes;
    const char* type;
    char* data;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    if ((count = udaGetNodeStructureCount(c_tree)) == 0) {
        return nullptr;
    }
    shapes = (int**)malloc(count * sizeof(int*));
    udaAddMalloc(shapes, count, sizeof(int*), "int *");

    auto* tree = static_cast<NTree*>(c_tree);
    count = 0;
    for (int i = 0; i < tree->userdefinedtype->fieldcount; i++) {
        if (tree->userdefinedtype->compoundfield[i].atomictype == UDA_TYPE_UNKNOWN) {
            if (!tree->userdefinedtype->compoundfield[i].pointer) {
                shapes[count] = tree->userdefinedtype->compoundfield[i].shape;
            } else {
                if ((data = (char*)tree->data) == nullptr) {
                    return nullptr;
                }
                udaFindMalloc2(&data[tree->userdefinedtype->compoundfield[i].offset], &count0, &size,
                               &type, &rank, &shape);
                shapes[count] = shape;
            }
            count++;
        }
    }
    return (shapes);
}

/** Return a List of Shape Arrays of the Atomic Components attached to a tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of Atomic Shape Arrays.
 */
int** udaGetNodeAtomicShape(NTREE* c_tree)
{
    int count, count0, size, rank;
    const char* type;
    char* data;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    if ((count = udaGetNodeAtomicCount(c_tree)) == 0) {
        return nullptr;
    }
    int** shapes = (int**)malloc(count * sizeof(int*));
    udaAddMalloc(shapes, count, sizeof(int*), "int *");

    auto* tree = static_cast<NTree*>(c_tree);
    count = 0;
    for (int i = 0; i < tree->userdefinedtype->fieldcount; i++) {
        if (tree->userdefinedtype->compoundfield[i].atomictype != UDA_TYPE_UNKNOWN) {
            if (!tree->userdefinedtype->compoundfield[i].pointer) {
                shapes[count] = tree->userdefinedtype->compoundfield[i].shape;
                if (shapes[count] == nullptr &&
                    tree->userdefinedtype->compoundfield[i].rank < 2) { // Not passed so create
                    shapes[count] = (int*)malloc(sizeof(int));
                    shapes[count][0] = tree->userdefinedtype->compoundfield[i].count;
                    tree->userdefinedtype->compoundfield[i].shape = shapes[count];
                }
            } else {
                if ((data = (char*)tree->data) == nullptr) {
                    return nullptr;
                }
                int* shape;
                void* ptr = &data[tree->userdefinedtype->compoundfield[i].offset];
                udaFindMalloc2(ptr, &count0, &size, &type, &rank, &shape);
                shapes[count] = shape;
                if (shape == 0 && (rank < 2)) {
                    shape = (int*)malloc(sizeof(int)); // Assume rank 1
                    udaAddMalloc(shape, 1, sizeof(int), "int");
                    shape[0] = count0;
                    shapes[count] =
                        shape; // Pass back the length of the scalar or rank 1 array from the malloc log query
                }
            }
            count++;
        }
    }
    return shapes;
}

/** Print the Names and Types of all Node Data Elements to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return Void
 */

void udaPrintNodeNames(NTREE* c_tree)
{
    auto* tree = static_cast<NTree*>(c_tree);
    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();

    print_node_names(log_malloc_list, tree);
}

/** Print the Atomic Data from a data node to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return Void
 */
void udaPrintNodeAtomic(NTREE* c_tree)
{
    auto* tree = static_cast<NTree*>(c_tree);
    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();

    print_node_atomic(log_malloc_list, tree);
}

/** Return the number of User Defined Type Structure Definition Components attached to this tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the number of User Defined Type Structure Definition Components.
 */
int udaGetNodeStructureComponentCount(NTREE* c_tree)
{
    auto* tree = static_cast<NTree*>(c_tree);
    return get_node_structure_component_count(tree);
}

/** Return a List of User Defined Type Structure Definition Components Names attached to this tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of User Defined Type Structure Definition Component names.
 */
char** udaGetNodeStructureComponentNames(NTREE* c_tree)
{
    auto* tree = static_cast<NTree*>(c_tree);
    return get_node_structure_component_names(tree);
}

/** Return a List of User Defined Type Structure Definition Components Types attached to this tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of User Defined Type Structure Definition Component Types.
 */
char** udaGetNodeStructureComponentTypes(NTREE* c_tree)
{
    auto* tree = static_cast<NTree*>(c_tree);
    return get_node_structure_component_types(tree);
}

/** Return a List of User Defined Type Structure Definition Components Descriptions attached to this tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of User Defined Type Structure Definition Component Descriptions.
 */
char** udaGetNodeStructureComponentDescriptions(NTREE* c_tree)
{
    auto* tree = static_cast<NTree*>(c_tree);
    return get_node_structure_component_descriptions(tree);
}

/** Return the Count of User Defined Structure Component Data array elements attached to this tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @param target The name of a User Defined Structure definition.
 * @return the Count of User Defined Structure Component Data Array elements.
 */

int udaGetNodeStructureComponentDataCount(NTREE* c_tree, const char* target)
{
    const char* lastname;
    UserDefinedType* user_defined_type;
    int count = 0, size, fieldcount;
    const char* type;
    char* data;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }

    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();

    auto* tree = static_cast<NTree*>(c_tree);

    // Identify node and component name
    tree = find_ntree_structure_component2(log_malloc_list, tree, target, &lastname);
    if (tree == nullptr) {
        return 0;
    }

    // dgm 05Aug2015        structure and first component share the same address
    if (!strcmp(tree->name, lastname)) {
        return tree->parent->branches;
    }

    user_defined_type = tree->userdefinedtype;
    fieldcount = tree->userdefinedtype->fieldcount;
    for (int i = 0; i < fieldcount; i++) {
        if (!strcmp(user_defined_type->compoundfield[i].name, lastname)) {
            if (user_defined_type->compoundfield[i].pointer) {
                if ((data = (char*)tree->data) == nullptr) {
                    break;
                }
                udaFindMalloc(&data[user_defined_type->compoundfield[i].offset], &count, &size,
                              &type);
                break;
            } else {
                count = user_defined_type->compoundfield[i].count;
                break;
            }
        }
    }
    return count;
}

/** Return the Rank of User Defined Structure Component Data array attached to this tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @param target The name of a User Defined Structure definition.
 * @return the Rank of User Defined Structure Component Data array.
 */
int udaGetNodeStructureComponentDataRank(NTREE* c_tree, const char* target)
{
    const char* lastname;
    char* data;
    const char* type;
    UserDefinedType* user_defined_type;
    int rank = 0, fieldcount, count, size;
    int* shape;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }

    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();
    auto* tree = static_cast<NTree*>(c_tree);

    // Identify node and component name
    tree = find_ntree_structure_component2(log_malloc_list, tree, target, &lastname);
    if (tree == nullptr) {
        return 0;
    }

    user_defined_type = tree->userdefinedtype;
    fieldcount = tree->userdefinedtype->fieldcount;
    for (int i = 0; i < fieldcount; i++) {
        if (!strcmp(user_defined_type->compoundfield[i].name, lastname)) {
            if (user_defined_type->compoundfield[i].pointer) {
                if ((data = (char*)tree->data) == nullptr) {
                    return 0;
                }
                udaFindMalloc2(&data[tree->userdefinedtype->compoundfield[i].offset], &count, &size,
                               &type, &rank, &shape);
                if (count == 0) {
                    rank = 0;
                }
            } else {
                rank = user_defined_type->compoundfield[i].rank;
            }
            break;
        }
    }
    return rank;
}

/** Return the Shape array of the User Defined Structure Component Data array attached to this tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @param target The name of a User Defined Structure definition.
 * @return the Shape array of length Rank of the User Defined Structure Component Data array.
 */
int* udaGetNodeStructureComponentDataShape(NTREE* c_tree, const char* target)
{
    const char* lastname;
    char* data;
    const char* type;
    UserDefinedType* user_defined_type;
    int fieldcount, count, size, rank;
    int* shape = nullptr;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }

    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();
    auto* tree = static_cast<NTree*>(c_tree);

    // Identify node and component name
    tree = find_ntree_structure_component2(log_malloc_list, tree, target, &lastname);
    if (tree == nullptr) {
        return nullptr;
    }
    user_defined_type = tree->userdefinedtype;
    fieldcount = tree->userdefinedtype->fieldcount;
    for (int i = 0; i < fieldcount; i++) {
        if (!strcmp(user_defined_type->compoundfield[i].name, lastname)) {
            if (user_defined_type->compoundfield[i].pointer) {
                if ((data = (char*)tree->data) == nullptr) {
                    return 0;
                }
                udaFindMalloc2(&data[tree->userdefinedtype->compoundfield[i].offset], &count, &size,
                               &type, &rank, &shape);
                if (count == 0) {
                    shape = nullptr;
                }
            } else {
                shape = user_defined_type->compoundfield[i].shape;
            }
            break;
        }
    }
    return shape;
}

/** Return True (1) if the User Defined Structure Component Data array, attached to this tree node,
 * is a pointer type. Returns False (0) otherwise.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @param target The name of a User Defined Structure definition.
 * @return the value 1 if the User Defined Structure Component Data array is a pointer type.
 */
int udaGetNodeStructureComponentDataIsPointer(NTREE* c_tree, const char* target)
{
    const char* lastname;
    UserDefinedType* user_defined_type;
    int ispointer = 0, fieldcount;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }

    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();
    auto* tree = static_cast<NTree*>(c_tree);

    // Identify node and component name
    tree = find_ntree_structure_component2(log_malloc_list, tree, target, &lastname);
    if (tree == nullptr) {
        return 0;
    }
    user_defined_type = tree->userdefinedtype;
    fieldcount = tree->userdefinedtype->fieldcount;
    for (int i = 0; i < fieldcount; i++) {
        if (!strcmp(user_defined_type->compoundfield[i].name, lastname)) {
            ispointer = user_defined_type->compoundfield[i].pointer;
            break;
        }
    }
    return ispointer;
}

/** Return the Size of a User Defined Structure Component.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @param target The name of a User Defined Structure Component.
 * @return the Size of the User Defined Structure Component.
 */
int udaGetNodeStructureComponentDataSize(NTREE* c_tree, const char* target)
{
    const char* lastname;
    UserDefinedType* user_defined_type;
    int count, size = 0, fieldcount;
    const char* type;
    char* data;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }

    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();
    auto* tree = static_cast<NTree*>(c_tree);

    // Identify node and component name
    tree = find_ntree_structure_component2(log_malloc_list, tree, target, &lastname);
    if (tree == nullptr) {
        return 0;
    }
    user_defined_type = tree->userdefinedtype;
    fieldcount = tree->userdefinedtype->fieldcount;
    for (int i = 0; i < fieldcount; i++) {
        if (!strcmp(user_defined_type->compoundfield[i].name, lastname)) {
            if (user_defined_type->compoundfield[i].pointer) {
                if ((data = (char*)tree->data) == nullptr) {
                    break;
                }
                udaFindMalloc(&data[user_defined_type->compoundfield[i].offset], &count, &size,
                              &type);
                break;
            } else {
                size = user_defined_type->compoundfield[i].size;
                break;
            }
        }
    }
    return size;
}

/** Return the Type Name of a User Defined Structure Component.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @param target The name of a User Defined Structure Component.
 * @return the Type Name of the User Defined Structure Component.
 */
const char* udaGetNodeStructureComponentDataDataType(NTREE* c_tree,
                                                     const char* target)
{
    const char* lastname;
    UserDefinedType* user_defined_type;
    int count, size, fieldcount;
    const char* type = nullptr;
    char* data;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }

    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();
    auto* tree = static_cast<NTree*>(c_tree);

    // Identify node and component name
    tree = find_ntree_structure_component2(log_malloc_list, tree, target, &lastname);
    if (tree == nullptr) {
        return "unknown";
    }
    user_defined_type = tree->userdefinedtype;
    fieldcount = tree->userdefinedtype->fieldcount;
    for (int i = 0; i < fieldcount; i++) {
        if (!strcmp(user_defined_type->compoundfield[i].name, lastname)) {
            if (user_defined_type->compoundfield[i].pointer) {
                if ((data = (char*)tree->data) == nullptr) {
                    break;
                }
                udaFindMalloc(&data[user_defined_type->compoundfield[i].offset], &count, &size,
                              &type);
                break;
            } else {
                type = user_defined_type->compoundfield[i].type;
                break;
            }
        }
    }
    return type;
}

/** Return a pointer to a User Defined Structure Component's data.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @param target The name of a User Defined Structure Component.
 * @return the User Defined Structure Component's data.
 */
void* udaGetNodeStructureComponentData(NTREE* c_tree, const char* target)
{
    const char* lastname;
    UserDefinedType* user_defined_type;
    int offset, fieldcount;
    char* p;
    void* data = nullptr;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }

    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();
    auto* tree = static_cast<NTree*>(c_tree);

    // Identify node and component name
    tree = find_ntree_structure_component2(log_malloc_list, tree, target, &lastname);
    if (tree == nullptr) {
        return nullptr;
    }

    if ((strchr(target, '.') != nullptr || strchr(target, '/') != nullptr) && !strcmp(tree->name, lastname)) {
        return tree->data;
    }

    if (strcmp(tree->name, lastname) == 0 && strcmp(target, lastname) == 0) {
        return tree->data;
    }

    user_defined_type = tree->userdefinedtype;
    fieldcount = tree->userdefinedtype->fieldcount;
    for (int i = 0; i < fieldcount; i++) {
        if (!strcmp(user_defined_type->compoundfield[i].name, lastname)) {
            p = (char*)tree->data;
            offset = user_defined_type->compoundfield[i].offset;
            if (user_defined_type->compoundfield[i].pointer) {
                data = (void*)*((VOIDTYPE*)&p[offset]);
            } else {
                data = (void*)&p[offset];
            }
            break;
        }
    }
    return data;
}

/** Print a User Defined Structure Component's data.
 *
 * @param fd File Descriptor
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @param target The name of a User Defined Structure Component.
 * @return void.
 */
void udaPrintNodeStructureComponentData(NTREE* c_tree, const char* target)
{
    int namecount, count;
    const char* type;
    const char* lastname;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }

    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();
    auto* tree = static_cast<NTree*>(c_tree);

    NTree* node = find_ntree_structure_component2(log_malloc_list, tree, target, &lastname); // Locate the Node
    if (node == nullptr) {
        return;
    }

    count = udaGetNodeStructureComponentDataCount(node, lastname);   // Array Size
    type = udaGetNodeStructureComponentDataDataType(node, lastname); // Type

    if (count > 0) {
        UDA_LOG(UDA_LOG_DEBUG, "[{}] Data Count {}   Type {}", target, count, type);
        UDA_LOG(UDA_LOG_DEBUG, "Data Values");
        if (!strcmp(type, "float")) {
            auto s = (float*)udaGetNodeStructureComponentData(node, lastname);
            for (int i = 0; i < count; i++) {
                UDA_LOG(UDA_LOG_DEBUG, "[{}] {}", i, s[i]);
            }
            return;
        }
        if (!strcmp(type, "int")) {
            auto s = (int*)udaGetNodeStructureComponentData(node, lastname);
            for (int i = 0; i < count; i++) {
                UDA_LOG(UDA_LOG_DEBUG, "[{}] {}", i, s[i]);
            }
            return;
        }
        if (!strcmp(type, "STRING")) {
            auto s = (char*)udaGetNodeStructureComponentData(node, lastname);
            UDA_LOG(UDA_LOG_DEBUG, "{}", s);
            return;
        }

        USERDEFINEDTYPE* c_user_defined_type;
        if ((c_user_defined_type = udaFindUserDefinedType(type, 0)) != nullptr) {
            auto user_defined_type = static_cast<UserDefinedType*>(c_user_defined_type);

            int firstpass = 1, offset, namecount2;
            char** namelist2;
            NTree temp;
            initNTree(&temp);
            void* str = nullptr;
            void* data = nullptr;
            void* olddata = nullptr;
            char* p = (char*)udaGetNodeStructureComponentData(node, lastname); // Structure Array
            char* pp = nullptr;
            namecount = user_defined_type->fieldcount; // Count of sub-structure elements
            UDA_LOG(UDA_LOG_DEBUG, "Data Count {}   Type {}", namecount, type);
            for (int j = 0; j < count; j++) {
                str = (void*)&p[j * user_defined_type->size];
                pp = (char*)str;
                for (int i = 0; i < namecount; i++) {
                    offset = user_defined_type->compoundfield[i].offset;
                    type = user_defined_type->compoundfield[i].type;
                    UDA_LOG(UDA_LOG_DEBUG, "[{}]   Type {}   Name {}", i, type,
                            user_defined_type->compoundfield[i].name);

                    if (user_defined_type->compoundfield[i].pointer) {
                        data = (void*)*((VOIDTYPE*)&pp[offset]); // Data Element from the single Structure Array Element
                    } else {
                        data = (void*)&pp[offset];
                    }
                    if (data == nullptr) {
                        continue;
                    }
                    if (user_defined_type->compoundfield[i].atomictype != UDA_TYPE_UNKNOWN) {
                        udaPrintAtomicData(data, user_defined_type->compoundfield[i].atomictype,
                                           user_defined_type->compoundfield[i].count, lastname);
                    } else {

                        temp.data = data;
                        strcpy(temp.name, user_defined_type->compoundfield[i].name);
                        temp.userdefinedtype =
                            static_cast<UserDefinedType*>(udaFindUserDefinedType(type, 0));
                        if (firstpass) {
                            udaAddNonMalloc(data, 1, user_defined_type->compoundfield[i].size, type);
                            firstpass = 0;
                        } else {
                            udaChangeNonMalloc(olddata, data, 1,
                                               user_defined_type->compoundfield[i].size, type);
                        }
                        olddata = data;

                        namecount2 = udaGetNodeStructureComponentCount(&temp); // Count of structure elements
                        namelist2 = udaGetNodeStructureComponentNames(&temp);  // List of structure element names
                        UDA_LOG(UDA_LOG_DEBUG, "Data Count {}   Type {}", namecount2, type);

                        for (int k = 0; k < namecount2; k++) {
                            udaPrintNodeStructureComponentData(&temp, namelist2[k]);
                        }
                    }
                }
            }
            return;
        }
    }
}

/** Print a Data Structure's Contents.
 *
 * @param fd File Descriptor
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return void.
 */
void udaPrintNodeStructure(NTREE* c_tree)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto& error_stack = instance.error_stack();
    auto* log_malloc_list = instance.log_malloc_list();

    int count, acount, scount, kstart = 1;
    char **anamelist, **snamelist;
    void* data;

    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }

    auto* tree = static_cast<NTree*>(c_tree);

    acount = udaGetNodeAtomicCount(c_tree); // Count of the Tree Node Structure atomic type components
    anamelist = udaGetNodeAtomicNames(c_tree);
    scount = udaGetNodeStructureCount(c_tree); // Count of the Tree Node Structure structure type components
    snamelist = udaGetNodeStructureNames(c_tree);
    count = get_node_structure_data_count(log_malloc_list, tree); // Count of the Tree Node Structure Array elements

    NTree* node = tree; // Start at the base node: all other structure array elements are sibling nodes

    for (int j = 0; j < count; j++) {

        UDA_LOG(UDA_LOG_DEBUG, "{} contents:", tree->userdefinedtype->name);

        data = udaGetNodeStructureArrayData(tree, j); // Loop over Structure Array Elements

        // Find the next structure array element node - it must be a sibling node
        // Nodes are ordered so start at the previous node
        // Nodes must be of the same type and heap address must match

        if (j > 0) {
            node = nullptr;
            for (int k = kstart; k < tree->parent->branches; k++) {
                if (!strcmp(tree->parent->children[k]->name, tree->name) && (tree->parent->children[k]->data == data)) {
                    node = tree->parent->children[k];
                    kstart = k + 1; // Next Start from the next sibling node
                }
            }
            if (node == nullptr) {
                add_error(error_stack, ErrorType::Code, "udaPrintNodeStructure", 999, "Structure Array element Node not Found!");
                return;
            }
        }

        for (int i = 0; i < acount; i++) {
            udaPrintAtomicType(node, anamelist[i]); // Print Atomic Components
        }

        for (int i = 0; i < scount; i++) { // Print Structured Components

            // Structured components must be children of this node.
            NTree* node2;
            if ((node2 = find_ntree_structure(log_malloc_list, node, snamelist[i])) != nullptr) {
                udaPrintNodeStructure(node2);
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "{}: null", snamelist[i]);
            }
        }
    }
}

/** Return a pointer to a User Defined Structure Component's data cast to FLOAT.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @param target The name of a User Defined Structure Component.
 * @return the User Defined Structure Component's data cast to float.
 */
float* udaCastNodeStructureComponentDatatoFloat(NTREE* c_tree, const char* target)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();

    int count;
    const char* type;
    const char* lastname;
    float* data;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }

    auto* tree = static_cast<NTree*>(c_tree);

    NTree* node = find_ntree_structure_component2(log_malloc_list, tree, target, &lastname);
    if (node == nullptr) {
        return nullptr;
    }

    count = udaGetNodeStructureComponentDataCount(node, lastname);
    type = udaGetNodeStructureComponentDataDataType(node, lastname);

    if (!strcmp(type, "float")) {
        return ((float*)udaGetNodeStructureComponentData(node, lastname));
    }

    if (count == 0) {
        return nullptr;
    }

    data = (float*)malloc(count * sizeof(float));
    if (!strcmp(type, "double")) {
        double* s = (double*)udaGetNodeStructureComponentData(node, lastname);
        for (int i = 0; i < count; i++) {
            data[i] = (float)s[i];
        }
        return data;
    }
    if (!strcmp(type, "int")) {
        int* s = (int*)udaGetNodeStructureComponentData(node, lastname);
        for (int i = 0; i < count; i++) {
            data[i] = (float)s[i];
        }
        return data;
    }

    return nullptr;
}

/** Return a pointer to a User Defined Structure Component's data cast to DOUBLE.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @param target The name of a User Defined Structure Component.
 * @return the User Defined Structure Component's data cast to float.
 */
double* udaCastNodeStructureComponentDatatoDouble(NTREE* c_tree, const char* target)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();

    int count;
    const char* type;
    const char* lastname;
    double* data;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }

    auto* tree = static_cast<NTree*>(c_tree);

    NTree* node = find_ntree_structure_component2(log_malloc_list, tree, target, &lastname);
    if (node == nullptr) {
        return nullptr;
    }

    count = udaGetNodeStructureComponentDataCount(node, lastname);
    type = udaGetNodeStructureComponentDataDataType(node, lastname);

    if (!strcmp(type, "double")) {
        return (double*)udaGetNodeStructureComponentData(node, lastname);
    }

    if (count == 0) {
        return nullptr;
    }

    data = (double*)malloc(count * sizeof(double));
    if (!strcmp(type, "float")) {
        float* s = (float*)udaGetNodeStructureComponentData(node, lastname);
        for (int i = 0; i < count; i++) {
            data[i] = (double)s[i];
        }
        return data;
    }
    if (!strcmp(type, "int")) {
        int* s = (int*)udaGetNodeStructureComponentData(node, lastname);
        for (int i = 0; i < count; i++) {
            data[i] = (double)s[i];
        }
        return data;
    }

    return nullptr;
}

/** Print the Contents of a tree node to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return Void
 */
void udaPrintNTree2(NTREE* c_tree)
{
    auto* tree = static_cast<NTree*>(c_tree);
    print_ntree2(tree);
}

/** Print the Contents of a tree node to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return Void
 */
void udaPrintNTree(NTREE* c_tree)
{
    auto* tree = static_cast<NTree*>(c_tree);
    auto& instance = uda::client::ThreadClient::instance();
    auto* user_defined_type_list = instance.user_defined_type_list();
    print_ntree(user_defined_type_list, tree);
}

/** Print Details of the tree node List to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return Void
 */
void udaPrintNTreeList(NTREE* c_tree)
{
    auto* tree = static_cast<NTree*>(c_tree);
    print_ntree_list(tree);
}

/** Return a Count of User Defined Type Tree Nodes from and including the passed tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the Count of Tree Nodes.
 */
int udaGetNTreeStructureCount(NTREE* c_tree)
{
    auto* tree = static_cast<NTree*>(c_tree);

    return get_ntree_structure_count(tree);
}

/** Return a List of User Defined Type Structure Names attached to this tree branch.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of User Defined Type Structure names.
 */
char** udaGetNTreeStructureNames(NTREE* c_tree)
{
    auto* tree = static_cast<NTree*>(c_tree);
    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();

    return get_ntree_structure_names(log_malloc_list, tree);
}

/** Return a List of User Defined Type Structure Type Names attached to this tree branch.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of User Defined Type Structure Type names.
 */
char** udaGetNTreeStructureTypes(NTREE* c_tree)
{
    auto* tree = static_cast<NTree*>(c_tree);
    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();

    return get_ntree_structure_types(log_malloc_list, tree);
}

/** Print the Names and Types of all Data Structures to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return Void
 */
void udaPrintNTreeStructureNames(NTREE* c_tree)
{
    auto* tree = static_cast<NTree*>(c_tree);
    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();

    print_ntree_structure_names(log_malloc_list, tree);
}

/** Return the total number of User Defined Type Structure Definition Components attached to this tree branch.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the number of User Defined Type Structure Definition Components.
 */
int udaGetNTreeStructureComponentCount(NTREE* c_tree)
{
    auto* tree = static_cast<NTree*>(c_tree);

    return get_ntree_structure_component_count(tree);
}

/** Return a List of User Defined Type Structure Definition Components Names attached to this tree branch.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of User Defined Type Structure Definition Component names.
 */
char** udaGetNTreeStructureComponentNames(NTREE* c_tree)
{
    auto* tree = static_cast<NTree*>(c_tree);
    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();

    return get_ntree_structure_component_names(log_malloc_list, tree);
}

/** Return a List of User Defined Type Structure Definition Components Types attached to this tree branch.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of User Defined Type Structure Definition Component Types.
 */
char** udaGetNTreeStructureComponentTypes(NTREE* c_tree)
{
    auto* tree = static_cast<NTree*>(c_tree);
    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();

    return get_ntree_structure_component_types(log_malloc_list, tree);
}

/** Return a List of User Defined Type Structure Definition Components Descriptions attached to this tree branch.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of User Defined Type Structure Definition Component Descriptions.
 */
char** udaGetNTreeStructureComponentDescriptions(NTREE* c_tree)
{
    auto* tree = static_cast<NTree*>(c_tree);
    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();

    return get_ntree_structure_component_descriptions(log_malloc_list, tree);
}

/** Print the Names and Types of all Data Elements to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return Void
 */
void udaPrintNTreeStructureComponentNames(NTREE* c_tree)
{
    auto* tree = static_cast<NTree*>(c_tree);
    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();

    print_ntree_structure_component_names(log_malloc_list, tree);
}

void udaGetNodeStructureComponentDataShape_f(NTREE* c_tree, const char* target,
                                             int* shape_f)
{
    int rank = udaGetNodeStructureComponentDataRank(c_tree, target);
    for (int i = 0; i < MAXRANK; i++) {
        shape_f[i] = 0;
    }
    if (rank > 1 && rank <= MAXRANK) {
        int* shape = udaGetNodeStructureComponentDataShape(c_tree, target);
        if (shape != nullptr) {
            for (int i = 0; i < rank; i++) {
                shape_f[i] = shape[i];
            }
        }
    } else {
        shape_f[0] = udaGetNodeStructureComponentDataCount(c_tree, target);
    }
}

void udaGetNodeStructureComponentShortData_f(NTREE* node, const char* target,
                                             short* data_f)
{
    auto data = (short*)udaGetNodeStructureComponentData(node, target);
    int count = udaGetNodeStructureComponentDataCount(node, target);
    for (int i = 0; i < count; i++) {
        data_f[i] = data[i];
    }
}

void udaGetNodeStructureComponentFloatData_f(NTREE* node, const char* target,
                                             float* data_f)
{
    float* data = (float*)udaGetNodeStructureComponentData(node, target);
    int count = udaGetNodeStructureComponentDataCount(node, target);
    for (int i = 0; i < count; i++) {
        data_f[i] = data[i];
    }
}

void udaDereferenceShortData(short* data_c, int count, short* data_f)
{
    for (int i = 0; i < count; i++) {
        data_f[i] = data_c[i];
    }
}

void udaDereferenceFloatData(float* data_c, int count, float* data_f)
{
    for (int i = 0; i < count; i++) {
        data_f[i] = data_c[i];
    }
}

short* udaCastNodeStructureComponentDatatoShort(NTREE* c_tree, const char* target)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();
    
    int count;
    const char* type;
    const char* lastname;
    short* data;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }

    auto* tree = static_cast<NTree*>(c_tree);
    
    NTREE* node = find_ntree_structure_component2(log_malloc_list, tree, target, &lastname);
    if (node == nullptr) {
        return nullptr;
    }

    count = udaGetNodeStructureComponentDataCount(node, lastname);
    type = udaGetNodeStructureComponentDataDataType(node, lastname);

    if (!strcmp(type, "short")) {
        return (short*)udaGetNodeStructureComponentData(node, lastname);
    }

    if (count == 0) {
        return nullptr;
    }

    data = (short*)malloc(count * sizeof(short));
    if (!strcmp(type, "double")) {
        double* s = (double*)udaGetNodeStructureComponentData(node, lastname);
        for (int i = 0; i < count; i++) {
            data[i] = (short)s[i];
        }
        return data;
    }
    if (!strcmp(type, "float")) {
        float* s = (float*)udaGetNodeStructureComponentData(node, lastname);
        for (int i = 0; i < count; i++) {
            data[i] = (short)s[i];
        }
        return data;
    }
    if (!strcmp(type, "int")) {
        int* s = (int*)udaGetNodeStructureComponentData(node, lastname);
        for (int i = 0; i < count; i++) {
            data[i] = (short)s[i];
        }
        return data;
    }
    if (!strcmp(type, "unsigned int")) {
        unsigned int* s = (unsigned int*)udaGetNodeStructureComponentData(node, lastname);
        for (int i = 0; i < count; i++) {
            data[i] = (short)s[i];
        }
        return data;
    }
    if (!strcmp(type, "unsigned short")) {
        unsigned short* s = (unsigned short*)udaGetNodeStructureComponentData(node, lastname);
        for (int i = 0; i < count; i++) {
            data[i] = (short)s[i];
        }
        return data;
    }
    return nullptr;
}

void udaCastNodeStructureComponentDatatoShort_f(NTREE* node, const char* target,
                                                short* data_f)
{
    short* data = udaCastNodeStructureComponentDatatoShort(node, target);
    if (data != nullptr) {
        int count = udaGetNodeStructureComponentDataCount(node, target);
        for (int i = 0; i < count; i++) {
            data_f[i] = data[i];
        }
        free(data);
    }
}

void udaCastNodeStructureComponentDatatoFloat_f(NTREE* node, const char* target,
                                                float* data_f)
{
    float* data = udaCastNodeStructureComponentDatatoFloat(node, target);
    if (data != nullptr) {
        int count = udaGetNodeStructureComponentDataCount(node, target);
        for (int i = 0; i < count; i++) {
            data_f[i] = data[i];
        }
        free(data);
    }
}

void udaAddStructureField(USERDEFINEDTYPE* user_type, const char* name, const char* desc, UDA_TYPE data_type,
                          bool is_pointer, int rank, int* shape, size_t offset)
{
    CompoundField field;
    init_compound_field(&field);

    strcpy(field.name, name);
    field.atomictype = data_type;
    if (data_type == UDA_TYPE_STRING) {
        strcpy(field.type, "STRING");
    } else {
        strcpy(field.type, udaNameType(data_type));
    }
    if (is_pointer) {
        strcpy(&field.type[strlen(field.type)], " *");
    }
    strcpy(field.desc, desc);
    field.pointer = is_pointer;
    field.rank = rank;
    field.count = 1;
    if (shape != nullptr) {
        field.shape = (int*)malloc(field.rank * sizeof(int));
        for (int i = 0; i < rank; ++i) {
            field.shape[i] = shape[i];
            field.count *= shape[i];
        }
    }
    field.size = is_pointer ? (int)getPtrSizeOf(data_type) : (field.count * (int)getSizeOf(data_type));
    field.offset = (int)offset;
    field.offpad = (int)udaPadding(offset, field.type);
    field.alignment = udaGetalignmentof(field.type);

    add_compound_field(static_cast<UserDefinedType*>(user_type), field);
}

NTREE* udaFindNTreeStructureDefinition(NTREE* ntree, const char* target) {
    auto* tree = static_cast<NTree*>(ntree);

    return find_ntree_structure_definition(tree, target);
}

NTREE* udaFindNTreeStructureComponent(NTREE* ntree, const char* target) {
    const auto& instance = uda::client::ThreadClient::instance();
    auto* log_malloc_list = instance.log_malloc_list();
    auto* tree = static_cast<NTree*>(ntree);

    return find_ntree_structure_component(log_malloc_list, tree, target);
}