//==============================================================================================================
// Example: Send a user defined structure named mystruct of type MYSTRUCT
//
// Static Master list of all known user defined types from header files and data bindings to XML Schema.
// This list is extended at run time when unknown types are encountered (e.g. from netCDF4 files)
//
// UserDefinedTypeList user_defined_type_list = getOpaqueStructurefromDatabase or ...
//
// pointer to the Data structure with measurement data etc.
//
// MYSTRUCT *mystruct = &data;
//
// Fetch the structure Definition from the Master List of all known structures
//
// UserDefinedType *udtype = udaFindUserDefinedType("MYSTRUCT");
//
// Send the Data
//
// rc = xdr_user_defined_type_data(xdrs, udtype, (void *)&mystruct);
//
// Arrays of User Defined Structures are ported using a special structure names SArray ....
//
//==============================================================================================================
// Example: Receive a user defined structure of unknown type
//
// void *mystruct;  // pointer to the new Data structure with measurement data etc.
//
// Create an empty structure definition
//
// UserDefinedType udtype;
// init_user_defined_type(&udtype);
//
// Receive the Data and its structure definition
//
// rc = xdr_user_defined_type_data(xdrs, &udtype, (void *)&mystruct);
//
//
//==============================================================================================================
#include "struct.h"

#include "accessors.h"
#include "clientserver/protocolXML2.h"
#include "genStructs.h"

#include <cstdlib>
#include <stddef.h>
#include <boost/algorithm/string/join.hpp>
#include <gsl/span>

#ifdef __GNUC__
#  include <strings.h>
#elif defined(_WIN32)
#  include <string.h>
#  define strcasecmp _stricmp
#endif

#include "clientserver/errorLog.h"
#include "clientserver/protocolXML2Put.h"
#include "common/stringUtils.h"
#include "clientserver/xdrlib.h"
#include "logging/logging.h"

#include "xdrUserDefinedData.h"
#include <uda/structured.h>

#if defined(SERVERBUILD)
#  include "server/udaServer.h"
#endif

using namespace uda::client_server;
using namespace uda::logging;
using namespace uda::structures;

static unsigned int last_malloc_index = 0; // Malloc Log search index last value
static unsigned int* last_malloc_index_value = &last_malloc_index; // Preserve Malloc Log search index last value in GENERAL_STRUCT
static NTree* full_ntree = nullptr;

NTREE* udaGetFullNTree()
{
    return static_cast<NTREE*>(full_ntree);
}

void udaSetFullNTree(NTREE* c_tree)
{
    full_ntree = static_cast<NTree*>(c_tree);
}

void udaSetLastMallocIndexValue(unsigned int* lastMallocIndexValue_in)
{
    last_malloc_index_value = lastMallocIndexValue_in;
    last_malloc_index = *last_malloc_index_value;
}

void udaResetLastMallocIndex()
{
    last_malloc_index = 0;
    last_malloc_index_value = &last_malloc_index;
}

//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------

/** Initialise a SArray data structure.
 *
 * @param str A pointer to a SArray data structure instance.
 * @return Void.
 */
void uda::structures::initSArray(SArray* str)
{
    str->count = 0;
    str->rank = 0;
    str->shape = nullptr;
    str->data = nullptr;
    str->type[0] = '\0';
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
void udaAddNTreeList(LOGMALLOCLIST* c_log_malloc_list, NTREE* node, NTREELIST* ntree_list)
{
    auto tree = static_cast<NTree*>(node);
    auto list = static_cast<NTreeList*>(ntree_list);

    VOIDTYPE old = (VOIDTYPE)list->forrest;
    list->forrest = (NTree*)realloc((void*)list->forrest, (++list->listCount) * sizeof(NTree*));
    udaChangeMalloc(c_log_malloc_list, old, (void*)list->forrest, list->listCount, sizeof(NTree*), "NTree *");
    list->forrest[list->listCount] = *tree;
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

    int branch;
    if (child == nullptr || parent == nullptr) {
        return;
    }
    child->parent = parent;           // Identify the parent of the child node
    branch = parent->branches;        // Number of existing siblings
    parent->children[branch] = child; // Append the new node
    parent->branches++;               // Update the count of children
}

/** Free an NTree node together with the array of child nodes.
 *
 * @param tree A NTree node with or without a set of child nodes
 * @return Void.
 */
void udaFreeNTreeNode(NTREE* c_ntree)
{
    auto tree = static_cast<NTree*>(c_ntree);

    if (tree == nullptr) {
        return;
    }
    if (tree->branches > 0 && tree->children != nullptr) {
        for (int i = 0; i < tree->branches; i++) {
            udaFreeNTreeNode(tree->children[i]);
        }
        free(tree->children);
    }
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
    int len;
    void* old = (void*)*image;
    char* anew = nullptr;
    if (line == nullptr || line[0] == '\0') {
        return;
    }
    len = (int)strlen(line);
    anew = (char*)realloc((void*)old, (*imagecount + len + 1) * sizeof(char));
    strcpy(&anew[*imagecount], line);
    *imagecount = *imagecount + len + 1;
    *image = anew;
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
    // Insert values with #define names
    int len, lstr;
    char work[StringLength];
    char *p1, *p2, *p3;

    if (buffer[0] != '\t' || buffer[0] != ' ') {
        strcpy(expand, "\t"); // Tab out the structure contents
    } else {
        expand[0] = '\0';
    }
    len = (int)strlen(expand);

    if ((p1 = strchr(buffer, '[')) != nullptr) { // Array
        strncat(expand, buffer, p1 - buffer + 1);
        expand[len + (int)(p1 - buffer + 1)] = '\0';
        len = (int)strlen(expand);
        do {
            if ((p2 = strchr(buffer, ']')) != nullptr) {
                lstr = (int)(p2 - p1 - 1);
                strncpy(work, &p1[1], lstr);
                work[lstr] = '\0';
                if (is_number(work)) { // hard coded size
                    strncat(expand, &p1[1], p2 - &p1[1] + 1);
                    len = len + (int)(p2 - &p1[1] + 1);
                    expand[len] = '\0';
                    len = (int)strlen(expand);
                } else {
                    for (int j = 0; j < defCount; j++) {
                        if (!strcmp((char*)defnames[j], work)) {
                            snprintf(work, StringLength, " = %d]", defvalues[j]); // Array size
                            strncat(expand, &p1[1], p2 - &p1[1]);
                            len = len + (int)(p2 - &p1[1]);
                            expand[len] = '\0';
                            strcat(expand, work);
                            len = (int)strlen(expand);
                            break;
                        }
                    }
                }
            }
            p3 = p2;
        } while (p2 != nullptr && (p1 = strchr(p2, '[')) != nullptr);
        strcat(expand, &p3[1]);
    } else {
        strcat(expand, buffer);
    }
}

/** Initialise a LogMallocList data structure.
 *
 * @param str A pointer to a LogMallocList data structure instance.
 * @return Void.
 */
void uda::structures::init_log_malloc_list(LogMallocList* str)
{
    str->listcount = 0;
    str->listsize = 0;
    str->logmalloc = nullptr;
}

/** Initialise a LogMalloc data structure.
 *
 * @param str A pointer to a LogMalloc data structure instance.
 * @return Void.
 */
void uda::structures::init_log_malloc(LogMalloc* str)
{
    str->count = 0;
    str->rank = 0;
    str->size = 0;
    str->freed = 1;
    str->type[0] = '\0';
    str->heap = nullptr;
    str->shape = nullptr;
}

/** Initialise a LogStructList data structure.
 *
 * @return Void.
 */
void uda::structures::init_log_struct_list(LogStructList* log_struct_list)
{
    log_struct_list->listcount = 0;
    log_struct_list->listsize = 0;
    log_struct_list->logstruct = nullptr;
}

/** Initialise a LogStruct data structure.
 *
 * @param str A pointer to a LogStruct data structure instance.
 * @return Void.
 */
void uda::structures::init_log_struct(LogStruct* str)
{
    str->id = 0;
    str->type[0] = '\0';
    str->heap = nullptr;
}

/** Initialise a CompoundField data structure.
 *
 * @param str A pointer to a CompoundField data structure instance.
 * @return Void.
 */
void uda::structures::init_compound_field(CompoundField* str)
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

/** Initialise a UserDefinedType data structure.
 *
 * @param str A pointer to a UserDefinedType data structure instance.
 * @return Void.
 */
void uda::structures::init_user_defined_type(UserDefinedType* user_defined_type)
{
    user_defined_type->idamclass = UDA_TYPE_UNKNOWN;
    user_defined_type->ref_id = 0;
    memset(user_defined_type->name, '\0', MAXELEMENTNAME);
    memset(user_defined_type->source, '\0', MAXELEMENTNAME);
    user_defined_type->imagecount = 0;
    user_defined_type->image = nullptr;
    user_defined_type->size = 0;
    user_defined_type->fieldcount = 0;
    user_defined_type->compoundfield = nullptr;
}

/** Initialise a UserDefinedTypeList data structure.
 *
 * @param str A pointer to a UserDefinedTypeList data structure instance.
 * @return Void.
 */
void uda::structures::init_user_defined_type_list(UserDefinedTypeList* str)
{
    str->listCount = 0;
    str->userdefinedtype = nullptr;
}

/** Initialise a GeneralBlock data structure.
 *
 * @param str A pointer to a GeneralBlock data structure instance.
 * @return Void.
 */
void init_general_block(GeneralBlock* str)
{
    str->userdefinedtype = nullptr;
    str->userdefinedtypelist = nullptr;
    str->logmalloclist = nullptr;
    str->lastMallocIndex = 0;
}

/** Print the Contents of a CompoundField data structure.
 *
 * @param fd A File Descriptor.
 * @param str A CompoundField data structure instance.
 * @return Void.
 */
void print_compound_field(CompoundField str)
{
    UDA_LOG(UDA_LOG_DEBUG, "CompoundField Contents")
    UDA_LOG(UDA_LOG_DEBUG, "name     : {}", str.name)
    UDA_LOG(UDA_LOG_DEBUG, "type     : {}", str.type)
    UDA_LOG(UDA_LOG_DEBUG, "desc     : {}", str.desc)
    UDA_LOG(UDA_LOG_DEBUG, "Atomic type id : {}", str.atomictype)
    UDA_LOG(UDA_LOG_DEBUG, "pointer  : {}", str.pointer)
    UDA_LOG(UDA_LOG_DEBUG, "size     : {}", str.size)
    UDA_LOG(UDA_LOG_DEBUG, "offset   : {}", str.offset)
    UDA_LOG(UDA_LOG_DEBUG, "offpad   : {}", str.offpad)
    UDA_LOG(UDA_LOG_DEBUG, "alignment: {}", str.alignment)
    UDA_LOG(UDA_LOG_DEBUG, "rank     : {}", str.rank)
    UDA_LOG(UDA_LOG_DEBUG, "count    : {}", str.count)

    if (str.shape != nullptr && str.rank > 0) {
        gsl::span span{ str.shape, (size_t)str.rank };
        std::vector<std::string> shape_strings{ span.size() };
        std::transform(span.begin(), span.end(), shape_strings.begin(), [](int i) { return std::to_string(i); });
        std::string shape_string = boost::join(shape_strings, ",");
        UDA_LOG(UDA_LOG_DEBUG, "shape    : [{}]", shape_string)
    }
}

/** Print the Tabulated Contents of a CompoundField data structure.
 *
 * @param fd A File Descriptor.
 * @param str A CompoundField data structure instance.
 * @return Void.
 */
void print_compound_field_table(CompoundField str)
{
    UDA_LOG(UDA_LOG_DEBUG, "\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}", str.name, str.type, str.pointer, str.size,
            str.count, str.offset, str.offpad, str.alignment);
}

/** Print the Contents of a UserDefinedType data structure.
 *
 * @param fd A File Descriptor.
 * @param str A UserDefinedType data structure instance.
 * @return Void.
 */
void print_user_defined_type(UserDefinedType str)
{
    UDA_LOG(UDA_LOG_DEBUG, "UserDefinedType Contents");
    UDA_LOG(UDA_LOG_DEBUG, "name        : {}", str.name);
    UDA_LOG(UDA_LOG_DEBUG, "source      : {}", str.source);
    UDA_LOG(UDA_LOG_DEBUG, "ID Reference: {}", str.ref_id);
    UDA_LOG(UDA_LOG_DEBUG, "size        : {}", str.size);
    UDA_LOG(UDA_LOG_DEBUG, "fieldcount  : {}", str.fieldcount);

    printImage(str.image, str.imagecount);
    UDA_LOG(UDA_LOG_DEBUG, "");

    if (str.compoundfield != nullptr) {
        for (int i = 0; i < str.fieldcount; i++) {
            print_compound_field(str.compoundfield[i]);
        }
    }
    UDA_LOG(UDA_LOG_DEBUG, "");
}

/** Print the Tabulated Contents of a UserDefinedType data structure.
 *
 * @param fd A File Descriptor.
 * @param str A UserDefinedType data structure instance.
 * @return Void.
 */
void print_user_defined_type_table(UserDefinedTypeList* user_defined_type_list, UserDefinedType str)
{
    UDA_LOG(UDA_LOG_DEBUG, "UserDefinedType name: {} size: {} [{}] fieldcount: {} ref_id: {} ", str.name, str.size,
            udaGetStructureSize(user_defined_type_list, &str), str.fieldcount, str.ref_id);
    if (str.compoundfield != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG,
                "\t                Item\t            type\tpointer\tsize\tcount\toffset\toffpad\talignment\n");
        for (int i = 0; i < str.fieldcount; i++) {
            print_compound_field_table(str.compoundfield[i]);
        }
    }
}

/** Print the Tabulated Contents of a UserDefinedType data structure with Zero Sized elements.
 *
 * @param fd A File Descriptor.
 * @param str A UserDefinedType data structure instance.
 * @return Void.
 */
void print_zero_sized_user_defined_type_table(UserDefinedType str)
{
    int size1 = 0, size2 = 0;
    UDA_LOG(UDA_LOG_DEBUG, "UserDefinedType name: {} size: {} fieldcount {}", str.name, str.size, str.fieldcount);
    if (str.compoundfield != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG,
                "\t                Item\t            type\tpointer\tsize\tcount\toffset\toffpad\talignment\n");
        for (int i = 0; i < str.fieldcount; i++) {
            if (str.compoundfield[i].size > 0) {
                continue;
            }
            print_compound_field_table(str.compoundfield[i]);
            if (str.compoundfield[i].pointer) {
                size1 = size1 + str.compoundfield[i].size;
            } else {
                size1 = size1 + str.compoundfield[i].size * str.compoundfield[i].count;
            }
            size1 = size1 + str.compoundfield[i].offpad;
        }
        size2 = str.compoundfield[str.fieldcount - 1].offset;
        if (str.compoundfield[str.fieldcount - 1].pointer) {
            size2 = size2 + str.compoundfield[str.fieldcount - 1].size;
        } else {
            size2 = size2 + str.compoundfield[str.fieldcount - 1].size * str.compoundfield[str.fieldcount - 1].count;
        }
    }
    UDA_LOG(UDA_LOG_DEBUG, "[{}][{}]", size1, size2);
}

/** Print the Contents of a UserDefinedTypeList data structure.
 *
 * @param fd A File Descriptor.
 * @param str A UserDefinedTypeList data structure instance.
 * @return Void.
 */
void print_user_defined_type_list(UserDefinedTypeList str)
{
    UDA_LOG(UDA_LOG_DEBUG, "UserDefinedTypeList Contents");
    UDA_LOG(UDA_LOG_DEBUG, "listCount  : {}", str.listCount);
    for (int i = 0; i < str.listCount; i++) {
        print_user_defined_type(str.userdefinedtype[i]);
    }
    UDA_LOG(UDA_LOG_DEBUG, "");
}

/** Print the Tabulated Contents of a UserDefinedTypeList data structure.
 *
 * @param fd A File Descriptor.
 * @param str A UserDefinedTypeList data structure instance.
 * @return Void.
 */
void uda::structures::print_user_defined_type_list_table(UserDefinedTypeList str)
{
    UDA_LOG(UDA_LOG_DEBUG, "UserDefinedTypeList Contents");
    UDA_LOG(UDA_LOG_DEBUG, "listCount  : {}", str.listCount);
    for (int i = 0; i < str.listCount; i++) {
        print_user_defined_type_table(&str, str.userdefinedtype[i]);
    }
    UDA_LOG(UDA_LOG_DEBUG, "\n");
}

/** Print the Tabulated Contents of a UserDefinedTypeList data structure where the size is zero.
 *
 * @param fd A File Descriptor.
 * @param str A UserDefinedTypeList data structure instance.
 * @return Void.
 */
void print_zero_sized_user_defined_type_list_table(UserDefinedTypeList str)
{
    UDA_LOG(UDA_LOG_DEBUG, "Zero Size UserDefinedTypeList Contents");
    UDA_LOG(UDA_LOG_DEBUG, "listCount  : {}", str.listCount);
    for (int i = 0; i < str.listCount; i++) {
        print_zero_sized_user_defined_type_table(str.userdefinedtype[i]);
    }
    UDA_LOG(UDA_LOG_DEBUG, "\n");
}

/** Print the Contents of a LogMalloc data structure.
 *
 * @param fd A File Descriptor.
 * @param str A LogMalloc data structure instance.
 * @return Void.
 */
void print_malloc_log(LogMalloc str)
{
    UDA_LOG(UDA_LOG_DEBUG, "{}\t{}\t{}\t{}\t{}", (void*)str.heap, str.count, str.size, str.freed, str.type);
    if (str.rank > 1 && str.shape != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "\trank {} shape [{}", str.rank, str.shape[0]);
        for (int i = 1; i < str.rank; i++) {
            UDA_LOG(UDA_LOG_DEBUG, ",{}", str.shape[i]);
        }
        UDA_LOG(UDA_LOG_DEBUG, "]");
    }
}

/** Print the Contents of the Global LogMallocList data structure.
 *
 * @param fd A File Descriptor.
 * @return Void.
 */
void print_malloc_log_list(const LogMallocList* log_malloc_list)
{
    UDA_LOG(UDA_LOG_DEBUG, "MALLOC LOG List Contents");
    UDA_LOG(UDA_LOG_DEBUG, "listCount  : {}", log_malloc_list->listcount);
    UDA_LOG(UDA_LOG_DEBUG, "Address\t\tCount\tSize\tFreed\tType");
    for (int i = 0; i < log_malloc_list->listcount; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "[{}]  ", i);
        print_malloc_log(log_malloc_list->logmalloc[i]);
    }
    UDA_LOG(UDA_LOG_DEBUG, "\n");
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
void udaAddNonMalloc(LOGMALLOCLIST* c_log_malloc_list, void* stack, int count, size_t size, const char* type)
{
    auto log_malloc_list = static_cast<LogMallocList*>(c_log_malloc_list);

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

    log_malloc_list->logmalloc[log_malloc_list->listcount].rank = 0;
    log_malloc_list->logmalloc[log_malloc_list->listcount].shape = nullptr;

    log_malloc_list->listcount++;
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
void udaAddNonMalloc2(LOGMALLOCLIST* c_log_malloc_list, void* stack, int count, size_t size, const char* type, int rank,
                      int* shape)
{
    auto log_malloc_list = static_cast<LogMallocList*>(c_log_malloc_list);

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
void udaAddMalloc(LOGMALLOCLIST* c_log_malloc_list, void* heap, int count, size_t size, const char* type)
{
    auto log_malloc_list = static_cast<LogMallocList*>(c_log_malloc_list);

    // Log all Heap allocations for Data from User Defined Structures
    // Grow the list when necessary

    // UserDefinedType *udt;

    if (heap == nullptr) {
        return;
    }

    if (log_malloc_list->listcount + 1 >= log_malloc_list->listsize) {
        log_malloc_list->logmalloc = (LogMalloc*)realloc(
            (void*)log_malloc_list->logmalloc, (log_malloc_list->listsize + GROWMALLOCLIST) * sizeof(LogMalloc));
        log_malloc_list->listsize = log_malloc_list->listsize + GROWMALLOCLIST;
    }

    log_malloc_list->logmalloc[log_malloc_list->listcount].count = count;
    log_malloc_list->logmalloc[log_malloc_list->listcount].size = size;
    log_malloc_list->logmalloc[log_malloc_list->listcount].freed = 0;
    log_malloc_list->logmalloc[log_malloc_list->listcount].heap = heap;
    strcpy(log_malloc_list->logmalloc[log_malloc_list->listcount].type, type);

    log_malloc_list->logmalloc[log_malloc_list->listcount].rank = 0;
    log_malloc_list->logmalloc[log_malloc_list->listcount].shape = nullptr;

    log_malloc_list->listcount++;
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
void udaAddMalloc2(LOGMALLOCLIST* c_log_malloc_list, void* heap, int count, size_t size, const char* type, int rank,
                   int* shape)
{
    auto log_malloc_list = static_cast<LogMallocList*>(c_log_malloc_list);

    // Log all Heap allocations for Data from User Defined Structures
    // Grow the list when necessary

    if (heap == nullptr) {
        return;
    }

    if (log_malloc_list->listcount + 1 >= log_malloc_list->listsize) {
        log_malloc_list->logmalloc = (LogMalloc*)realloc(
            (void*)log_malloc_list->logmalloc, (log_malloc_list->listsize + GROWMALLOCLIST) * sizeof(LogMalloc));
        log_malloc_list->listsize = log_malloc_list->listsize + GROWMALLOCLIST;
    }

    log_malloc_list->logmalloc[log_malloc_list->listcount].count = count;
    log_malloc_list->logmalloc[log_malloc_list->listcount].size = size;
    log_malloc_list->logmalloc[log_malloc_list->listcount].freed = 0;
    log_malloc_list->logmalloc[log_malloc_list->listcount].heap = heap;
    strcpy(log_malloc_list->logmalloc[log_malloc_list->listcount].type, type);

    log_malloc_list->logmalloc[log_malloc_list->listcount].rank = rank;
    if (rank > 1) {
        log_malloc_list->logmalloc[log_malloc_list->listcount].shape = shape;
    } else {
        log_malloc_list->logmalloc[log_malloc_list->listcount].shape = nullptr;
    }

    log_malloc_list->listcount++;
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
void udaChangeMalloc(LOGMALLOCLIST* c_log_malloc_list, VOIDTYPE old, void* anew, int count, size_t size,
                     const char* type)
{
    auto log_malloc_list = static_cast<LogMallocList*>(c_log_malloc_list);

    // Change a List Entry
    if (old == 0) {
        udaAddMalloc(log_malloc_list, anew, count, size, type);
        return;
    }
    auto target = (VOIDTYPE)((VOIDTYPE*)old);
    for (int i = 0; i < log_malloc_list->listcount; i++) {
        auto candidate = (VOIDTYPE)((VOIDTYPE*)log_malloc_list->logmalloc[i].heap);
        if (target == candidate) {
            log_malloc_list->logmalloc[i].heap = anew;
            log_malloc_list->logmalloc[i].freed = 0;
            log_malloc_list->logmalloc[i].count = count;
            log_malloc_list->logmalloc[i].size = size;
            strcpy(log_malloc_list->logmalloc[i].type, type);
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
void udaChangeNonMalloc(LOGMALLOCLIST* c_log_malloc_list, void* old, void* anew, int count, size_t size,
                        const char* type)
{
    auto log_malloc_list = static_cast<LogMallocList*>(c_log_malloc_list);

    // Change a non-malloc List Entry

    VOIDTYPE target, candidate;
    if (old == nullptr) {
        udaAddNonMalloc(log_malloc_list, anew, count, size, type);
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
void udaFreeMallocLog(LOGMALLOCLIST* c_str)
{
    auto str = static_cast<LogMallocList*>(c_str);

    if (str == nullptr) {
        return;
    }
    for (int i = 0; i < str->listcount; i++) {
        if (str->logmalloc[i].freed == 0) {
            if (str->logmalloc[i].heap != nullptr && str->logmalloc[i].count > 0) {
                free(str->logmalloc[i].heap);
            }
            str->logmalloc[i].freed = 1;
            if (str->logmalloc[i].rank > 1 && str->logmalloc[i].shape != nullptr) {
                free(str->logmalloc[i].shape);
                str->logmalloc[i].shape = nullptr;
            }
        }
    }
}

/** Free allocated heap memory and reinitialise a new log_malloc_list-> There are no arguments.
 *
 * @return void.
 */
void udaFreeMallocLogList(LOGMALLOCLIST* c_str)
{
    auto str = static_cast<LogMallocList*>(c_str);

    if (str == nullptr) {
        return;
    }
    udaFreeMallocLog(str);
    if (str->logmalloc != nullptr) {
        free(str->logmalloc);
    }
    str->logmalloc = nullptr;
    init_log_malloc_list(str);
}

/** Find the meta data associated with a specific memory location.
 *
 * @param heap The target memory location.
 * @param count The returned allocation count.
 * @param size The returned allocation size.
 * @param type The returned allocation type.
 * @return void.
 */
void udaFindMalloc(LOGMALLOCLIST* c_log_malloc_list, void* heap, int* count, int* size, const char** type)
{
    auto log_malloc_list = static_cast<LogMallocList*>(c_log_malloc_list);

    // Find a specific Heap allocation for Data within User Defined Structures

    VOIDTYPE candidate, target;
    *count = 0;
    *size = 0;
    *type = nullptr;
    if (heap == nullptr) {
        return;
    }

    if ((target = *((VOIDTYPE*)heap)) == 0) {
        return;
    }

    if (last_malloc_index >= (unsigned int)log_malloc_list->listcount) { // Defensive check
        last_malloc_index = 0;
        *last_malloc_index_value = last_malloc_index;
    }

    for (unsigned int i = last_malloc_index; i < (unsigned int)log_malloc_list->listcount; i++) {
        candidate = (VOIDTYPE)log_malloc_list->logmalloc[i].heap;
        if (target == candidate) {
            *count = log_malloc_list->logmalloc[i].count;
            *size = log_malloc_list->logmalloc[i].size;
            *type = log_malloc_list->logmalloc[i].type;
            last_malloc_index = i;
            *last_malloc_index_value = last_malloc_index;
            return;
        }
    }

    for (unsigned int i = 0; i < last_malloc_index; i++) {
        candidate = (VOIDTYPE)log_malloc_list->logmalloc[i].heap;
        if (target == candidate) {
            *count = log_malloc_list->logmalloc[i].count;
            *size = log_malloc_list->logmalloc[i].size;
            *type = log_malloc_list->logmalloc[i].type;
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
void udaFindMalloc2(LOGMALLOCLIST* c_log_malloc_list, void* heap, int* count, int* size, const char** type, int* rank,
                    int** shape)
{
    auto log_malloc_list = static_cast<LogMallocList*>(c_log_malloc_list);

    // Find a specific Heap allocation for Data within User Defined Structures

    VOIDTYPE candidate, target;
    *count = 0;
    *size = 0;
    *type = nullptr;
    *rank = 0;
    *shape = nullptr;
    if (heap == nullptr) {
        return;
    }

    if ((target = *((VOIDTYPE*)heap)) == 0) {
        return;
    }

    if (last_malloc_index >= (unsigned int)log_malloc_list->listcount) { // Defensive check
        last_malloc_index = 0;
        *last_malloc_index_value = last_malloc_index;
    }

    for (unsigned int i = last_malloc_index; i < (unsigned int)log_malloc_list->listcount; i++) {
        candidate = (VOIDTYPE)log_malloc_list->logmalloc[i].heap;
        if (target == candidate) {
            *count = log_malloc_list->logmalloc[i].count;
            *size = log_malloc_list->logmalloc[i].size;
            *type = log_malloc_list->logmalloc[i].type;
            *rank = log_malloc_list->logmalloc[i].rank;
            if (*rank > 1) {
                *shape = log_malloc_list->logmalloc[i].shape;
            }
            last_malloc_index = i; // Start at the current log entry
            *last_malloc_index_value = last_malloc_index;
            return;
        }
    }

    for (unsigned int i = 0; i < last_malloc_index; i++) { // Start search at the first log entry
        candidate = (VOIDTYPE)log_malloc_list->logmalloc[i].heap;
        if (target == candidate) {
            *count = log_malloc_list->logmalloc[i].count;
            *size = log_malloc_list->logmalloc[i].size;
            *type = log_malloc_list->logmalloc[i].type;
            *rank = log_malloc_list->logmalloc[i].rank;
            if (*rank > 1) {
                *shape = log_malloc_list->logmalloc[i].shape;
            }
            last_malloc_index = i;
            *last_malloc_index_value = last_malloc_index;
            return;
        }
    }
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

/** Free allocated heap memory and reinitialise a new LogStructList. There are no arguments.
 *
 * @return void.
 */
void udaFreeLogStructList(LOGSTRUCTLIST* c_log_struct_list)
{
    auto log_struct_list = static_cast<LogStructList*>(c_log_struct_list);

    free(log_struct_list->logstruct);
    init_log_struct_list(log_struct_list);
}

/** Find the meta data associated with a specific Structure.
 *
 * @param heap The target memory location.
 * @param type The returned structure type.
 * @return The structure id.
 */
int udaFindStructId(void* heap, char** type, LOGSTRUCTLIST* c_log_struct_list)
{
    auto log_struct_list = static_cast<LogStructList*>(c_log_struct_list);

    // Find a specific Data Structure

    // VOIDTYPE candidate, target;
    *type = nullptr;
    if (heap == nullptr) {
        return 0;
    }

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
void* udaFindStructHeap(int id, char** type, LOGSTRUCTLIST* c_log_struct_list)
{
    auto log_struct_list = static_cast<LogStructList*>(c_log_struct_list);

    // Find a specific Data Structure

    *type = nullptr;
    if (id == 0) {
        return nullptr;
    }

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

/** Copy the Master User Defined Structure Definition List.
 *
 * @param anew The copy of the type definition list.
 * @return void.
 */
#if defined(SERVERBUILD)
void uda::structures::copy_user_defined_type_list(UserDefinedTypeList** anew,
                                                  const UserDefinedTypeList* parseduserdefinedtypelist)
{
    UserDefinedTypeList* list = (UserDefinedTypeList*)malloc(sizeof(UserDefinedTypeList));
    init_user_defined_type_list(list);
    list->listCount = parseduserdefinedtypelist->listCount; // Copy the standard set of structure definitions
    list->userdefinedtype = (UserDefinedType*)malloc(parseduserdefinedtypelist->listCount * sizeof(UserDefinedType));

    for (int i = 0; i < list->listCount; i++) {
        UserDefinedType usertypeOld = parseduserdefinedtypelist->userdefinedtype[i];
        UserDefinedType usertypeNew;
        init_user_defined_type(&usertypeNew);
        usertypeNew = usertypeOld;
        usertypeNew.image =
            (char*)malloc(usertypeOld.imagecount * sizeof(char)); // Copy pointer type (prevents double free)
        memcpy(usertypeNew.image, usertypeOld.image, usertypeOld.imagecount);

        usertypeNew.compoundfield = (CompoundField*)malloc(usertypeOld.fieldcount * sizeof(CompoundField));

        for (int j = 0; j < usertypeOld.fieldcount; j++) {
            init_compound_field(&usertypeNew.compoundfield[j]);
            usertypeNew.compoundfield[j] = usertypeOld.compoundfield[j];
            if (usertypeOld.compoundfield[j].rank > 0) {
                usertypeNew.compoundfield[j].shape = (int*)malloc(usertypeOld.compoundfield[j].rank * sizeof(int));

                for (int k = 0; k < usertypeOld.compoundfield[j].rank; k++) {
                    usertypeNew.compoundfield[j].shape[k] = usertypeOld.compoundfield[j].shape[k];
                }
            }
        }
        list->userdefinedtype[i] = usertypeNew;
    }

    *anew = list;
}
#else

void uda::structures::copy_user_defined_type_list(UserDefinedTypeList** anew,
                                                  const UserDefinedTypeList* parseduserdefinedtypelist)
{
    UDA_LOG(UDA_LOG_DEBUG, "Not SERVERBUILD - UserDefinedTypeList is not allocated");
}

#endif

/** Create the Initial User Defined Structure Definition List.
 *
 * @param anew The initial type definition list.
 * @return void.
 */
void uda::structures::get_initial_user_defined_type_list(UserDefinedTypeList** anew)
{
    auto list = (UserDefinedTypeList*)malloc(sizeof(UserDefinedTypeList));
    init_user_defined_type_list(list);

    UserDefinedType usertype;
    CompoundField field;

    int offset = 0;

    //----------------------------------------------------------------------------------------------------------------
    // SArray

    init_user_defined_type(&usertype); // New structure definition
    init_compound_field(&field);

    strcpy(usertype.name, "SArray");
    strcpy(usertype.source, "get_initial_user_defined_type_list");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(SArray); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;
    usertype.fieldcount = 0;

    offset = 0;

    defineField(&field, "count", "Number of data array elements", &offset, UDA_TYPE_INT, 0, nullptr, false, true);
    add_compound_field(&usertype, field);
    defineField(&field, "rank", "Rank of the data array", &offset, UDA_TYPE_INT, 0, nullptr, false, true);
    add_compound_field(&usertype, field);
    defineField(&field, "shape", "Shape of the data array", &offset, UDA_TYPE_INT, 0, nullptr, true, false);
    add_compound_field(&usertype, field);
    defineField(&field, "data", "Location of the Structure Array", &offset, UDA_TYPE_VOID, 0, nullptr, true, false);
    add_compound_field(&usertype, field);

    init_compound_field(&field);
    strcpy(field.name, "type");
    field.atomictype = UDA_TYPE_STRING;
    strcpy(field.type, "STRING");
    strcpy(field.desc, "The Structure Array Element's type name (Must be Unique)");
    field.pointer = 0;
    field.count = MAXELEMENTNAME;
    field.rank = 1;
    field.shape = (int*)malloc(field.rank * sizeof(int)); // Needed when rank >= 1
    field.shape[0] = field.count;
    field.size = field.count * sizeof(char);
    field.offset = udaNewoffset(offset, field.type);
    field.offpad = udaPadding(offset, field.type);
    field.alignment = udaGetalignmentof(field.type);
    offset = field.offset + field.size;
    add_compound_field(&usertype, field);

    add_user_defined_type(list, usertype);

    //----------------------------------------------------------------------------------------------------------------
    // EnumMember

    init_user_defined_type(&usertype); // New structure definition
    strcpy(usertype.name, "EnumMember");
    strcpy(usertype.source, "EnumMember structure: for labels and values");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(EnumMember); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;
    usertype.fieldcount = 0;

    offset = 0;

    init_compound_field(&field);
    strcpy(field.name, "name");
    field.atomictype = UDA_TYPE_STRING;
    strcpy(field.type, "STRING"); // convert atomic type to a string label
    strcpy(field.desc, "The ENUM label");
    field.pointer = 0;
    field.count = MAXELEMENTNAME;
    field.rank = 1;
    field.shape = (int*)malloc(field.rank * sizeof(int)); // Needed when rank >= 1
    field.shape[0] = field.count;
    field.size = field.count * sizeof(char);
    field.offset = offsetof(EnumMember, name);
    offset = field.offset + field.size;
    field.offpad = udaPadding(offset, field.type);
    field.alignment = udaGetalignmentof(field.type);
    add_compound_field(&usertype, field);

    init_compound_field(&field);
    defineField(&field, "value", "The ENUM value", &offset, UDA_TYPE_LONG64, 0, nullptr, false, true);
    add_compound_field(&usertype, field);

    add_user_defined_type(list, usertype);

    //----------------------------------------------------------------------------------------------------------------
    // EnumList

    init_user_defined_type(&usertype); // New structure definition
    strcpy(usertype.name, "EnumList");
    strcpy(usertype.source, "Array of ENUM values with properties");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(EnumMember); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;
    usertype.fieldcount = 0;

    offset = 0;

    init_compound_field(&field);
    strcpy(field.name, "name");
    field.atomictype = UDA_TYPE_STRING;
    strcpy(field.type, "STRING"); // convert atomic type to a string label
    strcpy(field.desc, "The ENUM name");
    field.pointer = 0;
    field.count = MAXELEMENTNAME;
    field.rank = 1;
    field.shape = (int*)malloc(field.rank * sizeof(int)); // Needed when rank >= 1
    field.shape[0] = field.count;
    field.size = field.count * sizeof(char);
    field.offset = offsetof(EnumList, name);
    offset = field.offset + field.size;
    field.offpad = udaPadding(offset, field.type);
    field.alignment = udaGetalignmentof(field.type);
    add_compound_field(&usertype, field);

    init_compound_field(&field);
    defineField(&field, "type", "The ENUM base integer atomic type", &offset, UDA_TYPE_INT, 0, nullptr, false, true);
    add_compound_field(&usertype, field);

    init_compound_field(&field);
    defineField(&field, "count", "The number of ENUM values", &offset, UDA_TYPE_INT, 0, nullptr, false, true);
    add_compound_field(&usertype, field);

    init_compound_field(&field);
    strcpy(field.name, "enummember");
    field.atomictype = UDA_TYPE_UNKNOWN;
    strcpy(field.type, "EnumMember");
    strcpy(field.desc, "The ENUM list members: labels and value");
    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = nullptr;
    field.size = sizeof(EnumMember*);
    field.offset = offsetof(EnumList, enummember); // Different to newoffset
    offset = field.offset + field.size;
    field.offpad = udaPadding(offset, field.type);
    field.alignment = udaGetalignmentof(field.type);
    add_compound_field(&usertype, field);

    // defineField(&field, "data", "Generalised data pointer for all integer type arrays", &offset, UDA_ARRAY_VOID);
    // UDA_ARRAY_VOID doesn't work - not implemented in the middleware!
    // Naming the field "data" hits a bug and garbage is returned!
    // Don't know the correct type until the structure is used!!!! - so cannot pre-define!
    // Make the necessary changes to the structure definition when EnumList is used or
    // Convert data to standard unsigned long64

    init_compound_field(&field);
    defineField(&field, "enumarray", "Data with this enumerated type", &offset, UDA_TYPE_LONG64, 0,
                nullptr, true, false); // Data need to be converted to this type
    add_compound_field(&usertype, field);
    init_compound_field(&field);
    defineField(&field, "enumarray_rank", "The rank of arraydata", &offset, UDA_TYPE_INT, 0, nullptr, false, true);
    add_compound_field(&usertype, field);
    init_compound_field(&field);
    defineField(&field, "enumarray_count", "The count of arraydata", &offset, UDA_TYPE_INT, 0, nullptr, false, true);
    add_compound_field(&usertype, field);
    init_compound_field(&field);
    defineField(&field, "enumarray_shape", "The shape of arraydata", &offset, UDA_TYPE_INT, 0, nullptr, true, false);
    add_compound_field(&usertype, field);

    add_user_defined_type(list, usertype);

    *anew = list;
}

/** Add a Compound Field type to a structure definition.
 *
 * @param str The structure definition.
 * @param field The Compound field type.
 * @return void.
 */
void uda::structures::add_compound_field(UserDefinedType* user_defined_type, CompoundField compound_field)
{
    user_defined_type->compoundfield = (CompoundField*)realloc(
        (void*)user_defined_type->compoundfield, (user_defined_type->fieldcount + 1) * sizeof(CompoundField));
    init_compound_field(&user_defined_type->compoundfield[user_defined_type->fieldcount]);
    user_defined_type->compoundfield[user_defined_type->fieldcount++] = compound_field;
}

/** Add a structure definition to the List of structure types
 *
 * @param str The list of structure definitions.
 * @param type The new definition to add to the list.
 * @return void.
 */
void uda::structures::add_user_defined_type(UserDefinedTypeList* user_defined_type_list,
                                            UserDefinedType user_defined_type)
{
    user_defined_type_list->userdefinedtype =
        (UserDefinedType*)realloc((void*)user_defined_type_list->userdefinedtype,
                                  (user_defined_type_list->listCount + 1) * sizeof(UserDefinedType));
    init_user_defined_type(&user_defined_type_list->userdefinedtype[user_defined_type_list->listCount]);
    user_defined_type_list->userdefinedtype[user_defined_type_list->listCount++] = user_defined_type;
}

/** Replace the structure definition list with an different structure type.
 *
 * @param str The list of structure definitions.
 * @param typeId The definition list entry to be replaced
 * @param type The definition to add into the list.
 * @return void.
 */
void uda::structures::update_user_defined_type(UserDefinedTypeList* user_defined_type_list, int typeId,
                                               UserDefinedType user_defined_type)
{
    user_defined_type_list->userdefinedtype[typeId] = user_defined_type; // replace existing entry
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
void udaChangeUserDefinedTypeElementProperty(USERDEFINEDTYPELIST* c_user_defined_type_list, int typeId, char* element,
                                             char* property, void* value)
{
    auto user_defined_type_list = static_cast<UserDefinedTypeList*>(c_user_defined_type_list);

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

/** Free heap from a Compound Field.
 *
 * @param str The Compound Field.
 * @return void.
 */
void udaFreeCompoundField(COMPOUNDFIELD* c_compound_field)
{
    auto compound_field = static_cast<CompoundField*>(c_compound_field);
    if (compound_field == nullptr) {
        return;
    }
    free(compound_field->shape);
    compound_field->shape = nullptr;
}

/** Free heap from a User Defined Type.
 *
 * @param type The User Defined Type.
 * @return void.
 */
void udaFreeUserDefinedType(USERDEFINEDTYPE* c_user_defined_type)
{
    auto user_defined_type = static_cast<UserDefinedType*>(c_user_defined_type);
    if (user_defined_type == nullptr) {
        return;
    }
    for (int i = 0; i < user_defined_type->fieldcount; i++) {
        udaFreeCompoundField(&user_defined_type->compoundfield[i]);
    }
    free(user_defined_type->compoundfield);
    user_defined_type->compoundfield = nullptr;

    free(user_defined_type->image);
    user_defined_type->image = nullptr;
}

/** Free heap from a User Defined Type List.
 *
 * @param user_defined_type_list The User Defined Type List.
 * @return void.
 */
void udaFreeUserDefinedTypeList(USERDEFINEDTYPELIST* c_user_defined_type_list)
{
    auto user_defined_type_list = static_cast<UserDefinedTypeList*>(c_user_defined_type_list);
    if (user_defined_type_list == nullptr) {
        return;
    }
    if (user_defined_type_list->listCount == 0) {
        return;
    }
    if (user_defined_type_list->userdefinedtype == nullptr) {
        return;
    }
    for (int i = 0; i < user_defined_type_list->listCount; i++) {
        udaFreeUserDefinedType(&user_defined_type_list->userdefinedtype[i]);
    }
    free(user_defined_type_list->userdefinedtype);
    init_user_defined_type_list(user_defined_type_list);
}

/** The size or byte count of an atomic or structured type
 *
 * @param type The name of the type
 * @return The size in bytes.
 */
size_t udaGetsizeof(USERDEFINEDTYPELIST* c_user_defined_type_list, const char* type)
{
    const char* base = type;

    if (!strncmp(type, "const", 5)) {
        const char* p = strrchr(type, ' '); // ignore const
        if (p != nullptr) {
            base = &p[1];
        } else {
            base = &type[6];
        }
    }

    if (!strcasecmp(base, "FLOAT")) {
        return sizeof(float);
    }
    if (!strcasecmp(base, "DOUBLE")) {
        return sizeof(double);
    }
    if (!strcasecmp(base, "CHAR")) {
        return sizeof(char);
    }
    if (!strcasecmp(base, "UNSIGNED CHAR")) {
        return sizeof(unsigned char);
    }
    if (!strcasecmp(base, "UCHAR")) {
        return sizeof(unsigned char);
    }
    if (!strcasecmp(base, "STRING")) {
        return sizeof(char); // Same as char array but null terminated
    }
    if (!strcasecmp(base, "SHORT")) {
        return sizeof(short);
    }
    if (!strcasecmp(base, "UNSIGNED SHORT")) {
        return sizeof(unsigned short);
    }
    if (!strcasecmp(base, "USHORT")) {
        return sizeof(unsigned short);
    }
    if (!strcasecmp(base, "INT")) {
        return sizeof(int);
    }
    if (!strcasecmp(base, "UNSIGNED INT")) {
        return sizeof(unsigned int);
    }
    if (!strcasecmp(base, "UINT")) {
        return sizeof(unsigned int);
    }
    if (!strcasecmp(base, "LONG")) {
        return sizeof(long);
    }
    if (!strcasecmp(base, "UNSIGNED LONG")) {
        return sizeof(unsigned long);
    }
    if (!strcasecmp(base, "ULONG")) {
        return sizeof(unsigned long);
    }
    if (!strcasecmp(base, "LONG LONG")) {
        return sizeof(long long);
    }
    if (!strcasecmp(base, "UNSIGNED LONG LONG")) {
        return sizeof(unsigned long long);
    }
    if (!strcasecmp(base, "LONG64")) {
        return sizeof(long long);
    }
    if (!strcasecmp(base, "ULONG64")) {
        return sizeof(unsigned long long);
    }
    if (!strcasecmp(base, "COMPLEX")) {
        return 2 * sizeof(float);
    }
    if (!strcasecmp(base, "DCOMPLEX")) {
        return 2 * sizeof(double); // Always aligned on correct byte boundary
    }

    // Search list of User defined types for size

    USERDEFINEDTYPE* c_udt;
    if ((c_udt = udaFindUserDefinedType(c_user_defined_type_list, base, 0)) != nullptr) {
        auto udt = static_cast<UserDefinedType*>(c_udt);
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
int udaGettypeof(const char* type)
{ // **** const unsigned ....
    if (type == nullptr) {
        return UDA_TYPE_UNKNOWN;
    }
    if (!strcasecmp(type, "FLOAT")) {
        return UDA_TYPE_FLOAT;
    }
    if (!strcasecmp(type, "DOUBLE")) {
        return UDA_TYPE_DOUBLE;
    }
    if (!strcasecmp(type, "CHAR")) {
        return UDA_TYPE_CHAR;
    }
    if (!strcasecmp(type, "SHORT")) {
        return UDA_TYPE_SHORT;
    }
    if (!strcasecmp(type, "INT")) {
        return UDA_TYPE_INT;
    }
    if (!strcasecmp(type, "LONG")) {
        return UDA_TYPE_LONG;
    }
    if (!strcasecmp(type, "LONG64")) {
        return UDA_TYPE_LONG64;
    }
    if (!strcasecmp(type, "LONG LONG")) {
        return UDA_TYPE_LONG64;
    }
    if (!strcasecmp(type, "COMPLEX")) {
        return UDA_TYPE_COMPLEX;
    }
    if (!strcasecmp(type, "DCOMPLEX")) {
        return UDA_TYPE_DCOMPLEX;
    }
    if (!strcasecmp(type, "STRING")) {
        return UDA_TYPE_STRING;
    }
    if (!strcasecmp(type, "VOID")) {
        return UDA_TYPE_VOID;
    }

    if (!strcasecmp(type, "UCHAR")) {
        return UDA_TYPE_UNSIGNED_CHAR;
    }
    if (!strcasecmp(type, "USHORT")) {
        return UDA_TYPE_UNSIGNED_SHORT;
    }
    if (!strcasecmp(type, "UINT")) {
        return UDA_TYPE_UNSIGNED_INT;
    }
    if (!strcasecmp(type, "ULONG")) {
        return UDA_TYPE_UNSIGNED_LONG;
    }
#ifndef __APPLE__
    if (!strcasecmp(type, "ULONG64")) {
        return UDA_TYPE_UNSIGNED_LONG64;
    }
#endif
    if (!strcasecmp(type, "UNSIGNED CHAR")) {
        return UDA_TYPE_UNSIGNED_CHAR;
    }
    if (!strcasecmp(type, "UNSIGNED SHORT")) {
        return UDA_TYPE_UNSIGNED_SHORT;
    }
    if (!strcasecmp(type, "UNSIGNED INT")) {
        return UDA_TYPE_UNSIGNED_INT;
    }
    if (!strcasecmp(type, "UNSIGNED LONG")) {
        return UDA_TYPE_UNSIGNED_LONG;
    }
#ifndef __APPLE__
    if (!strcasecmp(type, "UNSIGNED LONG64")) {
        return UDA_TYPE_UNSIGNED_LONG64;
    }
    if (!strcasecmp(type, "UNSIGNED LONG LONG")) {
        return UDA_TYPE_UNSIGNED_LONG64;
    }
#endif
    return UDA_TYPE_UNKNOWN; // Means Non Atomic => User defined structure type
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
int udaGetalignmentof(const char* type)
{
    int is32 = (sizeof(void*) == POINTER_SIZE32); // Test architecture
    const char* base = type;

    if (!strncmp(type, "const", 5) || !strncmp(type, "unsigned", 8)) { // ignore const and unsigned
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
    if (type == nullptr) {
        return ALIGNMENT;
    }
    if (strchr(base, '*')) {
        return sizeof(void*); // Pointer type
    }

    if (is32) {
        if (!strcasecmp(base, "FLOAT")) {
            return 4; // 32 bit architecture
        }
#ifndef WINDOWS
        if (!strcasecmp(base, "DOUBLE")) {
            return 4;
        }
#else
        if (!strcasecmp(base, "DOUBLE")) {
            return 8;
        }
#endif
        if (!strcasecmp(base, "CHAR")) {
            return 1;
        }
        if (!strcasecmp(base, "STRING")) {
            return 1;
        }
        if (!strcasecmp(base, "SHORT")) {
            return 2;
        }
        if (!strcasecmp(base, "INT")) {
            return 4;
        }
        if (!strcasecmp(base, "LONG")) {
            return 4;
        }
        if (!strcasecmp(base, "LONG64")) {
            return 4;
        }
        if (!strcasecmp(base, "LONG LONG")) {
            return 4;
        }
        if (!strcasecmp(base, "UCHAR")) {
            return 1;
        }
        if (!strcasecmp(base, "USHORT")) {
            return 2;
        }
        if (!strcasecmp(base, "UINT")) {
            return 4;
        }
        if (!strcasecmp(base, "ULONG")) {
            return 4;
        }
        if (!strcasecmp(base, "ULONG64")) {
            return 4;
        }
        if (!strcasecmp(base, "COMPLEX")) {
            return 4;
        }
        if (!strcasecmp(base, "DCOMPLEX")) {
            return 4;
        }
        if (!strcasecmp(base, "STRUCTURE")) {
            return 1; // Structures are aligned depending on structure content
        }
    } else {
#ifdef A64
        if (!strcasecmp(base, "FLOAT")) {
            return 4; // 64 bit architecture
        }
        if (!strcasecmp(base, "DOUBLE")) {
            return 8;
        }
        if (!strcasecmp(base, "CHAR")) {
            return 1;
        }
        if (!strcasecmp(base, "STRING")) {
            return 1;
        }
        if (!strcasecmp(base, "SHORT")) {
            return 2;
        }
        if (!strcasecmp(base, "INT")) {
            return 4;
        }
        if (!strcasecmp(base, "LONG")) {
            return 4;
        }
        if (!strcasecmp(base, "LONG64")) {
            return 8;
        }
        if (!strcasecmp(base, "LONG LONG")) {
            return 8;
        }
        if (!strcasecmp(base, "UCHAR")) {
            return 1;
        }
        if (!strcasecmp(base, "USHORT")) {
            return 2;
        }
        if (!strcasecmp(base, "UINT")) {
            return 4;
        }
        if (!strcasecmp(base, "ULONG")) {
            return 4;
        }
        if (!strcasecmp(base, "ULONG64")) {
            return 8;
        }
        if (!strcasecmp(base, "COMPLEX")) {
            return 4;
        }
        if (!strcasecmp(base, "DCOMPLEX")) {
            return 8;
        }
        if (!strcasecmp(base, "STRUCTURE")) {
            return 1;
        }
#endif
    }
    return ALIGNMENT; // Best Guess!
}

size_t udaNewoffset(size_t offset, const char* type)
{
    int alignment = udaGetalignmentof(type);
    return (offset + ((alignment - (offset % alignment)) % alignment));
}

size_t udaPadding(size_t offset, const char* type)
{
    int alignment = udaGetalignmentof(type);
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
size_t udaGetStructureSize(USERDEFINEDTYPELIST* c_user_defined_type_list, USERDEFINEDTYPE* c_user_defined_type)
{
    auto user_defined_type_list = static_cast<UserDefinedTypeList*>(c_user_defined_type_list);
    auto user_defined_type = static_cast<UserDefinedType*>(c_user_defined_type);

    size_t size;
    if (user_defined_type == nullptr) {
        return 0;
    }

    size_t byteCount = 0;
    size_t space0 = 0;
    int maxAlign = 0;
    size_t offset = 0;

    for (int i = 0; i < user_defined_type->fieldcount; i++) {

        int alignment;

        if (user_defined_type->compoundfield[i].pointer) {
            size = sizeof(void*);
            alignment = udaGetalignmentof("*");
        } else {
            size = udaGetsizeof(user_defined_type_list, user_defined_type->compoundfield[i].type);
            alignment = udaGetalignmentof(user_defined_type->compoundfield[i].type);
        }

        size_t space = size * user_defined_type->compoundfield[i].count;

        if (i != 0) {
            if (user_defined_type->compoundfield[i].pointer) {
                offset = udaNewoffset(offset + space0, "*");
            } else {
                offset = udaNewoffset(offset + space0, user_defined_type->compoundfield[i].type);
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

        space0 = space; // Retain Previous values
    }

    // Add any additional structure packing to align the structure

    if (maxAlign) {
        byteCount = byteCount + ((maxAlign - (byteCount % maxAlign)) % maxAlign);
    }

    // Return the structure size

    return byteCount;
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

//==============================================================================================================
// Functions to Send or Receive Data contained in User Defined Structures

// Send or Receive the Data Structure

// Recursive Send/Receive Individual User Defined Structure Elements.
// Structure elements may be primitive (atomic) types or other User defined types
// The structure Size has two possible sizes if it contains pointers.
// Pointer size changes from 4 to 8 bytes if hardware architecture changes from 32 to 64 bits.
// Hardware architecture also affects offset values.
// All atomic types have standard byte lengths.
// Assume 32 bit architecture as the default and pass additional values for 64 bit systems
// The type, length and size of pointer types is passed prior to sending.
// Pointer type, length and size are recorded in a malloc log.
// The count of data structures to be received is passed ...
//

int uda::structures::xdrAtomicData(LogMallocList* log_malloc_list, XDR* xdrs, const char* type, int count, int size,
                                   char** data)
{
    int type_id = udaGettypeof(type);
    char* d;
    if (xdrs->x_op == XDR_DECODE) {
        d = (char*)malloc(count * size);
        udaAddMalloc(log_malloc_list, (void*)d, count, size, type);
        *data = d;
    } else {
        d = *data;
    }
#ifdef DEBUG2
    if (xdrs->x_op != XDR_DECODE) {
        int rc;
        switch (type_id) {
            case (UDA_TYPE_FLOAT):
                for (rc = 0; rc < count; rc++) {
                    fprintf(stdout, "{} ", *((float*)d + rc));
                }
                break;
            case (UDA_TYPE_DOUBLE):
                for (rc = 0; rc < count; rc++) {
                    fprintf(stdout, "{} ", *((double*)d + rc));
                }
                break;
            case (UDA_TYPE_COMPLEX):
                for (rc = 0; rc < 2 * count; rc++) {
                    fprintf(stdout, "{} ", *((float*)d + rc));
                }
                break;
            case (UDA_TYPE_INT):
                for (rc = 0; rc < count; rc++) {
                    fprintf(stdout, "{} ", *((int*)d + rc));
                }
                break;
        }
        fprintf(stdout, "\n");
    }
#endif
#ifndef DEBUG2
    switch (type_id) {
        case UDA_TYPE_FLOAT:
            return (xdr_vector(xdrs, d, count, sizeof(float), (xdrproc_t)xdr_float));
        case UDA_TYPE_DOUBLE:
            return (xdr_vector(xdrs, d, count, sizeof(double), (xdrproc_t)xdr_double));
        case UDA_TYPE_COMPLEX:
            return (xdr_vector(xdrs, d, 2 * count, sizeof(float), (xdrproc_t)xdr_float));
        case UDA_TYPE_DCOMPLEX:
            return (xdr_vector(xdrs, d, 2 * count, sizeof(double), (xdrproc_t)xdr_double));

        case UDA_TYPE_CHAR:
            return (xdr_vector(xdrs, d, count, sizeof(char), (xdrproc_t)xdr_char));
        case UDA_TYPE_SHORT:
            return (xdr_vector(xdrs, d, count, sizeof(short), (xdrproc_t)xdr_short));
        case UDA_TYPE_INT:
            return (xdr_vector(xdrs, d, count, sizeof(int), (xdrproc_t)xdr_int));
        case UDA_TYPE_LONG64:
            return (xdr_vector(xdrs, d, count, sizeof(long long), (xdrproc_t)xdr_int64_t));

        case UDA_TYPE_UNSIGNED_CHAR:
            return (xdr_vector(xdrs, d, count, sizeof(unsigned char), (xdrproc_t)xdr_u_char));
        case UDA_TYPE_UNSIGNED_SHORT:
            return (xdr_vector(xdrs, d, count, sizeof(unsigned short), (xdrproc_t)xdr_u_short));
        case UDA_TYPE_UNSIGNED_INT:
            return (xdr_vector(xdrs, d, count, sizeof(unsigned int), (xdrproc_t)xdr_u_int));
#  ifndef __APPLE__
        case UDA_TYPE_UNSIGNED_LONG64:
            return (xdr_vector(xdrs, d, count, sizeof(unsigned long long), (xdrproc_t)xdr_uint64_t));
#  endif
    }
#  ifdef DEBUG2
    if (xdrs->x_op == XDR_DECODE) {
        int rc;
        switch (type_id) {
            case (UDA_TYPE_FLOAT):
                for (rc = 0; rc < count; rc++) {
                    fprintf(stdout, "{} ", *((float*)d + rc));
                }
                break;
            case (UDA_TYPE_DOUBLE):
                for (rc = 0; rc < count; rc++) {
                    fprintf(stdout, "{} ", *((double*)d + rc));
                }
                break;
            case (UDA_TYPE_COMPLEX):
                for (rc = 0; rc < 2 * count; rc++) {
                    fprintf(stdout, "{} ", *((float*)d + rc));
                }
                break;
            case (UDA_TYPE_INT):
                for (rc = 0; rc < count; rc++) {
                    fprintf(stdout, "{} ", *((int*)d + rc));
                }
                break;
        }
        fprintf(stdout, "\n");
    }
#  endif
#endif

    return 0;
}

// Send/Receive Array of Structures

int uda::client_server::xdr_user_defined_type_data(XDR* xdrs, LogMallocList* log_malloc_list,
                                                   UserDefinedTypeList* user_defined_type_list,
                                                   UserDefinedType* user_defined_type, void** data, int protocolVersion,
                                                   bool xdr_stdio_flag, LogStructList* log_struct_list,
                                                   int malloc_source)
{
    int rc;

    init_log_struct_list(log_struct_list); // Initialise Linked List Structure Log

    if (xdrs->x_op == XDR_DECODE) {

        NTree* dataNTree = nullptr;

        if (!xdr_stdio_flag) {
            rc = xdrrec_skiprecord(xdrs); // Receiving
        } else {
            rc = 1;
        }

        rc = rc &&
             xdr_user_defined_type(xdrs, user_defined_type_list, user_defined_type); // User Defined Type Definitions
        rc = rc &&
             xdrUserDefinedData(xdrs, log_malloc_list, log_struct_list, user_defined_type_list, user_defined_type, data,
                                1, 0, nullptr, 0, &dataNTree, protocolVersion, malloc_source); // Data within Structures

        udaSetFullNTree(dataNTree); // Copy to Global
    } else {

        if (user_defined_type == nullptr) {
            add_error(ErrorType::Code, "udaXDRUserDefinedTypeData", 999,
                      "No User Defined Type passed - cannot send!");
            return 0;
        }

        rc = xdr_user_defined_type(xdrs, user_defined_type_list, user_defined_type); // User Defined Type Definitions
        rc = rc &&
             xdrUserDefinedData(xdrs, log_malloc_list, log_struct_list, user_defined_type_list, user_defined_type, data,
                                1, 0, nullptr, 0, nullptr, protocolVersion, malloc_source); // Data within Structures

        if (!xdr_stdio_flag) {
            rc = rc && xdrrec_endofrecord(xdrs, 1);
        }
    }

    udaFreeLogStructList(log_struct_list); // Free Linked List Structure Log heap

    return rc;
}

int udaFindUserDefinedTypeId(USERDEFINEDTYPELIST* c_user_defined_type_list, const char* name)
{
    auto user_defined_type_list = static_cast<UserDefinedTypeList*>(c_user_defined_type_list);

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

USERDEFINEDTYPE* udaFindUserDefinedType(USERDEFINEDTYPELIST* c_user_defined_type_list, const char* name, int ref_id)
{
    auto user_defined_type_list = static_cast<UserDefinedTypeList*>(c_user_defined_type_list);

    // Return the Structure Definition of a Named User Defined Structure

    UDA_LOG(UDA_LOG_DEBUG, "udaFindUserDefinedType: [{}]", name);
    UDA_LOG(UDA_LOG_DEBUG, "ref_id: {}", ref_id);
    UDA_LOG(UDA_LOG_DEBUG, "listCount: {}", user_defined_type_list->listCount);

    if (name == nullptr) {
        return nullptr;
    }

    if (ref_id > 0 && name[0] != '\0') {
        for (int i = 0; i < user_defined_type_list->listCount; i++) {
            if (!strcmp(user_defined_type_list->userdefinedtype[i].name, name) &&
                user_defined_type_list->userdefinedtype[i].ref_id == ref_id) {
                return (&user_defined_type_list->userdefinedtype[i]);
            }
        }
        return nullptr;
    }

    if (ref_id == 0 && name[0] != '\0') {

        for (int i = 0; i < user_defined_type_list->listCount; i++) {
            UDA_LOG(UDA_LOG_DEBUG, "[{}]: [{}]", i, user_defined_type_list->userdefinedtype[i].name);
            if (!strcmp(user_defined_type_list->userdefinedtype[i].name, name)) {
                return (&user_defined_type_list->userdefinedtype[i]);
            }
        }
        return nullptr;
    }

    if (ref_id != 0 && name[0] == '\0') {
        for (int i = 0; i < user_defined_type_list->listCount; i++) {
            if (user_defined_type_list->userdefinedtype[i].ref_id == ref_id) {
                return (&user_defined_type_list->userdefinedtype[i]);
            }
        }
        return nullptr;
    }

    return nullptr;
}

int udaTestUserDefinedType(USERDEFINEDTYPELIST* c_user_defined_type_list, USERDEFINEDTYPE* udt)
{
    auto user_defined_type_list = static_cast<UserDefinedTypeList*>(c_user_defined_type_list);

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

//==============================================================================================================
// Functions to Send or Receive Definitions of User Defined Structure

bool_t xdr_compoundfield(XDR* xdrs, CompoundField* str)
{

    // Send/Receive individual compound types
    int rc;

    rc = xdr_int(xdrs, &str->size);
    rc = rc && xdr_int(xdrs, &str->offset);
    rc = rc && xdr_int(xdrs, &str->offpad);
    rc = rc && xdr_int(xdrs, &str->alignment);
    rc = rc && xdr_int(xdrs, &str->atomictype);
    rc = rc && xdr_int(xdrs, &str->pointer);
    rc = rc && xdr_int(xdrs, &str->rank);
    rc = rc && xdr_int(xdrs, &str->count);
    rc = rc && wrap_xdr_string(xdrs, (char*)str->type, MAXELEMENTNAME - 1);
    rc = rc && wrap_xdr_string(xdrs, (char*)str->name, MAXELEMENTNAME - 1);
    rc = rc && wrap_xdr_string(xdrs, (char*)str->desc, MAXELEMENTNAME - 1);
    if (!rc) {
        return 0;
    }
    if (str->rank > 1) {
        // Only necessary to pass shape if the rank > 1
        if (rc && xdrs->x_op == XDR_DECODE) {
            // Receiving an array so allocate Heap for it then initialise
            str->shape = (int*)malloc(str->rank * sizeof(int));
            for (int i = 0; i < str->rank; i++) {
                str->shape[i] = 0;
            }
        }
        rc = rc && xdr_vector(xdrs, (char*)str->shape, (u_int)str->rank, sizeof(int), (xdrproc_t)xdr_int);
    } else {
        str->shape = nullptr;
    }
    return rc;
}

bool_t uda::client_server::xdr_user_defined_type(XDR* xdrs, UserDefinedTypeList* user_defined_type_list,
                                                 UserDefinedType* str)
{
    // Send/Receive a single user defined type

    int rc, adjust = 0;

    rc = xdr_int(xdrs, &str->idamclass);
    rc = rc && wrap_xdr_string(xdrs, (char*)str->name, MAXELEMENTNAME - 1);
    rc = rc && wrap_xdr_string(xdrs, (char*)str->source, MAXELEMENTNAME - 1);
    rc = rc && xdr_int(xdrs, &str->ref_id);
    rc = rc && xdr_int(xdrs, &str->size); // Size determined on the server side: client must recalculate
    rc = rc && xdr_int(xdrs, &str->imagecount);
    rc = rc && xdr_int(xdrs, &str->fieldcount);

    print_user_defined_type(*str);

    if (xdrs->x_op == XDR_DECODE) { // Receiving an array so allocate Heap for it then initialise
        if (str->imagecount > 0) {
            str->image = (char*)malloc(str->imagecount * sizeof(char));
        } else {
            str->image = nullptr;
        }
        if (str->fieldcount > 0) {
            str->compoundfield = (CompoundField*)malloc(str->fieldcount * sizeof(CompoundField));
            for (int i = 0; i < str->fieldcount; i++) {
                init_compound_field(&str->compoundfield[i]);
            }
        } else {
            str->compoundfield = nullptr;
        }
    }

    if (str->imagecount > 0) {
        rc = rc && xdr_vector(xdrs, str->image, (u_int)str->imagecount, sizeof(char), (xdrproc_t)xdr_char);
    }

    for (int i = 0; i < str->fieldcount; i++) {
        rc = rc && xdr_compoundfield(xdrs, &str->compoundfield[i]);

        // Check for necessary adjustment to offset, offpad, alignment and size when receiving structure
        // definition elements - because of architecture changes and alignment differences
        // Enables a cast to a specific data type
        // Offsets from netCDF files are comapct with no filler bytes so will not generally map to a structure type

        if (xdrs->x_op == XDR_DECODE && !adjust && rc) {
            size_t size;
            int alignment;
            if (str->compoundfield[i].pointer) {
                size = sizeof(void*);
            } else {
                // Non-atomic types (structures) will already have been adjusted
                size = udaGetsizeof(user_defined_type_list, str->compoundfield[i].type);
            }
            alignment = udaGetalignmentof(str->compoundfield[i].type);
            // Adjustment required
            adjust = (alignment != str->compoundfield[i].alignment || size != (size_t)str->compoundfield[i].size);
        }
    }

    // Make necessary adjustments

    if (adjust) {
        int space, space0 = 0, size, offset = 0, offpad, alignment, byteCount = 0, maxAlign = 0;
        for (int i = 0; i < str->fieldcount; i++) {

            if (str->compoundfield[i].pointer) {
                size = sizeof(void*);
                alignment = udaGetalignmentof("*");
            } else {
                size = udaGetsizeof(user_defined_type_list, str->compoundfield[i].type);
                alignment = udaGetalignmentof(str->compoundfield[i].type);
            }

            space = size * str->compoundfield[i].count;

            if (i != 0) {
                if (str->compoundfield[i].pointer) {
                    offpad = udaPadding(offset + space0, "*");
                    offset = udaNewoffset(offset + space0, "*");
                } else {
                    offpad = udaPadding(offset + space0, str->compoundfield[i].type);
                    offset = udaNewoffset(
                        offset + space0,
                        str->compoundfield[i].type); // Non-atomic types (structures) will already have been adjusted
                }
            } else {
                offpad = 0;
                offset = 0;
                byteCount = 0;
            }

            str->compoundfield[i].size = size;
            str->compoundfield[i].offset = offset;
            str->compoundfield[i].offpad = offpad;
            str->compoundfield[i].alignment = alignment;

            byteCount = offset + space;
            if (alignment > maxAlign) {
                maxAlign = alignment;
            }

            space0 = space; // Retain Previous values
        }

        // Add any additional structure packing to align the structure

        byteCount = byteCount + ((maxAlign - (byteCount % maxAlign)) % maxAlign);

        // Update the structure size

        str->size = byteCount;

        print_user_defined_type(*str);
    }

    return rc;
}

bool_t uda::client_server::xdr_user_defined_type_list(XDR* xdrs, UserDefinedTypeList* str, bool xdr_stdio_flag)
{
    // Send/Receive the list of userdefined types

    int rc = 1;

    if (!xdr_stdio_flag) {
        if (xdrs->x_op == XDR_DECODE) {
            rc = xdrrec_skiprecord(xdrs);
        }
    } else {
        rc = 1;
    }

    rc = rc && xdr_int(xdrs, &str->listCount);

    UDA_LOG(UDA_LOG_DEBUG, "xdr_userdefinedtypelist: rc = {}, listCount = {}", rc, str->listCount);

    if (!rc || str->listCount == 0) {
        return rc;
    }

    if (xdrs->x_op == XDR_DECODE) { // Receiving array so allocate Heap for it then initialise
        str->userdefinedtype = (UserDefinedType*)malloc(str->listCount * sizeof(UserDefinedType));
        for (int i = 0; i < str->listCount; i++) {
            init_user_defined_type(&str->userdefinedtype[i]);
        }
    }

    for (int i = 0; i < str->listCount; i++) {
        rc = rc && xdr_user_defined_type(xdrs, str, &str->userdefinedtype[i]);
    }

    if (!xdr_stdio_flag) {
        if (xdrs->x_op == XDR_ENCODE) {
            rc = rc && xdrrec_endofrecord(xdrs, 1);
        }
    }

    return rc;
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
    if (data == nullptr || count == 0) {
        UDA_LOG(UDA_LOG_DEBUG, "{}: null", label);
        return;
    }
    switch (atomictype) {
        case UDA_TYPE_FLOAT: {
            float* d = (float*)data;
            if (count > 1) {
                UDA_LOG(UDA_LOG_DEBUG, "{}:", label);
                for (int i = 0; i < count; i++) {
                    UDA_LOG(UDA_LOG_DEBUG, "[{}]   {}", i, d[i]);
                }
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "{}: {}", label, d[0]);
            }
            return;
        }
        case UDA_TYPE_DOUBLE: {
            double* d = (double*)data;
            if (count > 1) {
                UDA_LOG(UDA_LOG_DEBUG, "{}:", label);
                for (int i = 0; i < count; i++) {
                    UDA_LOG(UDA_LOG_DEBUG, "[{}]   {}", i, d[i]);
                }
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "{}: {}", label, d[0]);
            }
            return;
        }
        case UDA_TYPE_SHORT: {
            short* d = (short*)data;
            if (count > 1) {
                UDA_LOG(UDA_LOG_DEBUG, "{}:", label);
                for (int i = 0; i < count; i++) {
                    UDA_LOG(UDA_LOG_DEBUG, "[{}]   {}", i, d[i]);
                }
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "{}: {}", label, d[0]);
            }
            return;
        }
        case UDA_TYPE_INT:
        case UDA_TYPE_LONG: {
            int* d = (int*)data;
            if (count > 1) {
                UDA_LOG(UDA_LOG_DEBUG, "{}:", label);
                for (int i = 0; i < count; i++) {
                    UDA_LOG(UDA_LOG_DEBUG, "[{}]   {}", i, d[i]);
                }
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "{}: {}", label, d[0]);
            }
            return;
        }
        case UDA_TYPE_LONG64: {
            long long* d = (long long*)data;
            if (count > 1) {
                UDA_LOG(UDA_LOG_DEBUG, "{}:", label);
                for (int i = 0; i < count; i++) {
                    UDA_LOG(UDA_LOG_DEBUG, "[{}]   {}", i, d[i]);
                }
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "{}: {}", label, d[0]);
            }
            return;
        }
        case UDA_TYPE_UNSIGNED_CHAR: {
            unsigned char* d = (unsigned char*)data;
            if (count > 1) {
                UDA_LOG(UDA_LOG_DEBUG, "{}:", label);
                for (int i = 0; i < count; i++) {
                    UDA_LOG(UDA_LOG_DEBUG, "[{}]   {}", i, d[i]);
                }
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "{}: {}", label, d[0]);
            }
            return;
        }
        case UDA_TYPE_UNSIGNED_SHORT: {
            unsigned short* d = (unsigned short*)data;
            if (count > 1) {
                UDA_LOG(UDA_LOG_DEBUG, "{}:", label);
                for (int i = 0; i < count; i++) {
                    UDA_LOG(UDA_LOG_DEBUG, "[{}]   {}", i, d[i]);
                }
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "{}: {}", label, d[0]);
            }
            return;
        }
        case UDA_TYPE_UNSIGNED_INT:
        case UDA_TYPE_UNSIGNED_LONG: {
            unsigned int* d = (unsigned int*)data;
            if (count > 1) {
                UDA_LOG(UDA_LOG_DEBUG, "{}:", label);
                for (int i = 0; i < count; i++) {
                    UDA_LOG(UDA_LOG_DEBUG, "[{}]   {}", i, d[i]);
                }
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "{}: {}", label, d[0]);
            }
            return;
        }
        case UDA_TYPE_UNSIGNED_LONG64: {
            unsigned long long* d = (unsigned long long*)data;
            if (count > 1) {
                UDA_LOG(UDA_LOG_DEBUG, "{}:", label);
                for (int i = 0; i < count; i++) {
                    UDA_LOG(UDA_LOG_DEBUG, "[{}]   {}", i, d[i]);
                }
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "{}: {}", label, d[0]);
            }
            return;
        }
        case UDA_TYPE_STRING: {
            char* d = (char*)data;
            UDA_LOG(UDA_LOG_DEBUG, "{}: {}", label, d);
            return;
        }
        case UDA_TYPE_CHAR: {
            char* d = (char*)data;
            UDA_LOG(UDA_LOG_DEBUG, "{}: {}", label, d);
            return;
        }
    }
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
void udaPrintAtomicType(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree, const char* target)
{
    auto tree = static_cast<NTree*>(c_tree);

    UserDefinedType* user_defined_type = tree->userdefinedtype;
    char* p;
    void* data;
    int fieldcount = tree->userdefinedtype->fieldcount;
    for (int i = 0; i < fieldcount; i++) {
        if (!strcmp(user_defined_type->compoundfield[i].name, target)) {
            if (user_defined_type->compoundfield[i].atomictype != UDA_TYPE_UNKNOWN) {
                p = (char*)tree->data;
                if (user_defined_type->compoundfield[i].pointer ||
                    !strcmp(user_defined_type->compoundfield[i].type, "STRING *")) { // Strings are an exception!
                    int count, size;
                    const char* type;
                    data = (void*)*((VOIDTYPE*)&p[user_defined_type->compoundfield[i].offset]);
                    if (data == nullptr) {
                        UDA_LOG(UDA_LOG_DEBUG, "{}: null", target);
                        return;
                    }
                    udaFindMalloc(c_log_malloc_list, &data, &count, &size, &type);
                    if (count > 0) {
                        udaPrintAtomicData(data, udaGettypeof(type), count, target);
                    }
                } else {
                    data = (void*)&p[user_defined_type->compoundfield[i].offset];
                    udaPrintAtomicData(data, user_defined_type->compoundfield[i].atomictype,
                                       user_defined_type->compoundfield[i].count, target);
                }
            } else {
                UDA_LOG(UDA_LOG_ERROR, "ERROR: {} is Not of Atomic Type", target);
            }
            return;
        }
    }
    UDA_LOG(UDA_LOG_ERROR, "ERROR: {} is Not located in the current Tree Node", target);
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
    auto tree = static_cast<NTree*>(c_tree);

    UserDefinedType* user_defined_type = tree->userdefinedtype;
    int fieldcount = tree->userdefinedtype->fieldcount;
    for (int i = 0; i < fieldcount; i++) {
        if (!strcmp(user_defined_type->compoundfield[i].name, target)) {
            print_compound_field(user_defined_type->compoundfield[i]);
            UDA_LOG(UDA_LOG_DEBUG, "{}[ {} ]", target, user_defined_type->compoundfield[i].count);
        }
    }
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
COMPOUNDFIELD* udaGetNodeStructureComponent(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree, const char* target)
{
    UserDefinedType* user_defined_type;
    int fieldcount;
    const char* lastname;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }

    // Locate the Node with a Structure Component
    c_tree = udaFindNTreeStructureComponent2(c_log_malloc_list, c_tree, target, &lastname);

    if (c_tree != nullptr) {
        auto tree = static_cast<NTree*>(c_tree);
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

/** Return a Pointer to a string array containing the hierarchical names of structure components.
 *
 * @param target The name of a User Defined Structure element (case sensitive).
 * @param ntargets A returned count of the number of names in the returned list.
 * @return the list of structure component names.
 */
char** udaParseTarget(const char* target, int* ntargets)
{
    char** targetlist = nullptr;
    char *buffer = nullptr, *work, *p;
    buffer = (char*)malloc((strlen(target) + 1) * sizeof(char));
    strcpy(buffer, target);
    work = buffer;
    *ntargets = 0;
    if (((p = strchr(work, '.')) != nullptr) || (p = strchr(work, '/')) != nullptr) {
        p[0] = '\0';
        targetlist = (char**)realloc((void*)targetlist, (*ntargets + 1) * sizeof(char*));
        targetlist[0] = (char*)malloc((strlen(work) + 1) * sizeof(char));
        strcpy(targetlist[0], work);
        work = &p[1];
        *ntargets = 1;
        do {
            targetlist = (char**)realloc((void*)targetlist, (*ntargets + 1) * sizeof(char*));
            if (((p = strchr(work, '.')) != nullptr) || (p = strchr(work, '/')) != nullptr) {
                p[0] = '\0';
                targetlist[*ntargets] = (char*)malloc((strlen(work) + 1) * sizeof(char));
                strcpy(targetlist[*ntargets], work);
                work = &p[1];
            } else {
                targetlist[*ntargets] = (char*)malloc((strlen(work) + 1) * sizeof(char));
                strcpy(targetlist[*ntargets], work);
            }
            *ntargets = *ntargets + 1;
        } while (p != nullptr);
    }
    if (*ntargets == 0) { // Return the target even if not hierarchical
        *ntargets = 1;
        targetlist = (char**)malloc(sizeof(char*));
        targetlist[0] = buffer;
    }
    if (*ntargets > 1 && buffer != nullptr) {
        free(buffer);
    }
    return targetlist;
}

/** Print the Contents of a tree node to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return Void
 */
void udaPrintNode(NTREE* c_tree)
{
    auto tree = static_cast<NTree*>(c_tree);

    if (tree == nullptr) {
        tree = full_ntree;
    }
    UDA_LOG(UDA_LOG_DEBUG, "NTree Node Contents");
    UDA_LOG(UDA_LOG_DEBUG, "Name    : {} ", tree->name);
    UDA_LOG(UDA_LOG_DEBUG, "Branches: {} ", tree->branches);
#ifdef A64
    UDA_LOG(UDA_LOG_DEBUG, "Parent  : {}   ({}) ", (void*)tree->parent, (UVOIDTYPE)tree->parent);
    for (int i = 0; i < tree->branches; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "Children[{}]: {}   ({}) ", i, (void*)tree->children[i],
                (UVOIDTYPE)tree->children[i]);
    }
#else
    UDA_LOG(UDA_LOG_DEBUG, "Parent  : {}   ({}) ", (void*)tree->parent, (UVOIDTYPE)tree->parent);
    for (int i = 0; i < tree->branches; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "Children[{}]: {}   ({}) ", i, (void*)tree->children[i], (UVOIDTYPE)tree->children[i]);
    }
#endif
    print_user_defined_type(*tree->userdefinedtype);
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
    NTREE* tree = nullptr;
    if (target[0] != '\0') {
        if ((tree = udaFindNTreeStructureDefinition(tree, target)) == nullptr) {
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
    if (target[0] != '\0') {
        if ((c_tree = udaFindNTreeStructureDefinition(c_tree, target)) == nullptr) {
            UDA_LOG(UDA_LOG_DEBUG, "the Structure Definition for {} could not be Found", target);
            return;
        }
        auto tree = static_cast<NTree*>(c_tree);
        printImage(tree->userdefinedtype->image, tree->userdefinedtype->imagecount);
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
    auto tree = static_cast<NTree*>(c_tree);
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
    auto tree = static_cast<NTree*>(c_tree);
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
    auto tree = static_cast<NTree*>(c_tree);
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
    auto tree = static_cast<NTree*>(c_tree);
    return tree->userdefinedtype->size;
}

/** Return a pointer to a Tree Nodes's Data Structure Array element.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @param index The array index
 * @return a Pointer to a Structure Array element.
 */
void* udaGetNodeStructureArrayData(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree, int index)
{
    char* p;
    if (index < 0) {
        add_error(ErrorType::Code, "udaGetNodeStructureArrayData", 999, "The Tree Node array index < 0");
        return nullptr;
    }
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    if (udaGetNodeStructureDataCount(c_log_malloc_list, c_tree) < (index + 1)) {
        add_error(ErrorType::Code, "udaGetNodeStructureArrayData", 999,
                  "The Tree Node array index > allocated array dimension");
        return nullptr;
    }
    auto tree = static_cast<NTree*>(c_tree);
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
void* udaGetNodeStructureComponentArrayData(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree, const char* target,
                                            int structureindex, int componentindex)
{
    int offset, count, size;
    char* p;
    char* pp;
    const char* type;
    if (structureindex < 0) {
        add_error(ErrorType::Code, "udaGetNodeStructureComponentArrayData", 999,
                  "The Tree Node Structure array index < 0");
    }
    if (componentindex < 0) {
        add_error(ErrorType::Code, "udaGetNodeStructureComponentArrayData", 999,
                  "The Tree Node Structure Component array index < 0");
        return nullptr;
    }
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }

    if ((pp = (char*)udaGetNodeStructureArrayData(c_log_malloc_list, c_tree, structureindex)) == nullptr) {
        return nullptr;
    }

    auto tree = static_cast<NTree*>(c_tree);
    for (int i = 0; i < tree->userdefinedtype->fieldcount; i++) {
        if (!strcmp(tree->userdefinedtype->compoundfield[i].name, target)) {
            offset = tree->userdefinedtype->compoundfield[i].offset;
            if (tree->userdefinedtype->compoundfield[i].pointer) {
                p = (char*)*((VOIDTYPE*)&pp[offset]); // Data Element from the single Structure Array Element
                udaFindMalloc(c_log_malloc_list, p, &count, &size, &type);
            } else {
                p = &pp[offset];
                size = tree->userdefinedtype->compoundfield[i].size;
                count = tree->userdefinedtype->compoundfield[i].count;
            }
            if (size == 0) {
                return nullptr;
            }
            if (count <= componentindex) {
                add_error(ErrorType::Code, "udaGetNodeStructureComponentArrayData", 999,
                          "The Tree Node Structure Component array index > allocated array dimension");
                return nullptr;
            }
            return (void*)&p[componentindex * size];
        }
    }
    add_error(ErrorType::Code, "udaGetNodeStructureComponentArrayData", 999,
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
    auto tree = static_cast<NTree*>(c_tree);
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
    auto tree = static_cast<NTree*>(c_tree);
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
    auto tree = static_cast<NTree*>(c_tree);
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
    auto tree = static_cast<NTree*>(c_tree);
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
    auto tree = static_cast<NTree*>(c_tree);
    return tree->data;
}

/** Return a Count of Structured Component Types attached to a tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the Count of Structured types.
 */

int udaGetNodeStructureCount(NTREE* c_tree)
{
    int count = 0;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    auto tree = static_cast<NTree*>(c_tree);
    for (int i = 0; i < tree->userdefinedtype->fieldcount; i++) {
        if (tree->userdefinedtype->compoundfield[i].atomictype == UDA_TYPE_UNKNOWN) {
            count++;
        }
    }
    return count;
}

/** Return a Count of Atomic Component Types attached to a tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the Count of Atomic types.
 */

int udaGetNodeAtomicCount(NTREE* c_tree)
{
    int count = 0;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    auto tree = static_cast<NTree*>(c_tree);
    for (int i = 0; i < tree->userdefinedtype->fieldcount; i++) {
        if (tree->userdefinedtype->compoundfield[i].atomictype != UDA_TYPE_UNKNOWN) {
            count++;
        }
    }
    return count;
}

/** Return a List of Structure component Names attached to a tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of Structure names.
 */

char** udaGetNodeStructureNames(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree)
{
    int count;
    char** names;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    if ((count = udaGetNodeStructureCount(c_tree)) == 0) {
        return nullptr;
    }
    names = (char**)malloc(count * sizeof(char*));
    udaAddMalloc(c_log_malloc_list, (void*)names, count, sizeof(char*), "char *");
    count = 0;
    auto tree = static_cast<NTree*>(c_tree);
    for (int i = 0; i < tree->userdefinedtype->fieldcount; i++) {
        if (tree->userdefinedtype->compoundfield[i].atomictype == UDA_TYPE_UNKNOWN) {
            names[count++] = tree->userdefinedtype->compoundfield[i].name;
        }
    }
    return names;
}

/** Return a List of Atomic component Names attached to a tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of Atomic element names.
 */

char** udaGetNodeAtomicNames(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree)
{
    int count;
    char** names;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    if ((count = udaGetNodeAtomicCount(c_tree)) == 0) {
        return nullptr;
    }
    names = (char**)malloc(count * sizeof(char*));
    udaAddMalloc(c_log_malloc_list, (void*)names, count, sizeof(char*), "char *");

    auto tree = static_cast<NTree*>(c_tree);
    count = 0;
    for (int i = 0; i < tree->userdefinedtype->fieldcount; i++) {
        if (tree->userdefinedtype->compoundfield[i].atomictype != UDA_TYPE_UNKNOWN) {
            names[count++] = tree->userdefinedtype->compoundfield[i].name;
        }
    }
    return names;
}

/** Return a List of Structure Component Type Names attached to a tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of Structure Type names.
 */

char** udaGetNodeStructureTypes(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree)
{
    int count;
    char** names;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    if ((count = udaGetNodeStructureCount(c_tree)) == 0) {
        return nullptr;
    }
    names = (char**)malloc(count * sizeof(char*));
    udaAddMalloc(c_log_malloc_list, (void*)names, count, sizeof(char*), "char *");

    auto tree = static_cast<NTree*>(c_tree);
    count = 0;
    for (int i = 0; i < tree->userdefinedtype->fieldcount; i++) {
        if (tree->userdefinedtype->compoundfield[i].atomictype == UDA_TYPE_UNKNOWN) {
            names[count++] = tree->userdefinedtype->compoundfield[i].type;
        }
    }
    return names;
}

/** Return a List of Atomic Component Type Names attached to a tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of Atomic Type names.
 */

char** udaGetNodeAtomicTypes(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree)
{
    int count;
    char** names;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    if ((count = udaGetNodeAtomicCount(c_tree)) == 0) {
        return nullptr;
    }
    names = (char**)malloc(count * sizeof(char*));
    udaAddMalloc(c_log_malloc_list, (void*)names, count, sizeof(char*), "char *");

    auto tree = static_cast<NTree*>(c_tree);
    count = 0;
    for (int i = 0; i < tree->userdefinedtype->fieldcount; i++) {
        if (tree->userdefinedtype->compoundfield[i].atomictype != UDA_TYPE_UNKNOWN) {
            names[count++] = tree->userdefinedtype->compoundfield[i].type;
        }
    }
    return names;
}

/** Return a List of Structure Component Pointer property attached to a tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of Structure Pointer Properties.
 */

int* udaGetNodeStructurePointers(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree)
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
    udaAddMalloc(c_log_malloc_list, (void*)pointers, count, sizeof(int), "int");

    auto tree = static_cast<NTree*>(c_tree);
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

int* udaGetNodeAtomicPointers(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree)
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
    udaAddMalloc(c_log_malloc_list, (void*)pointers, count, sizeof(int), "int");

    auto tree = static_cast<NTree*>(c_tree);
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

int* udaGetNodeStructureRank(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree)
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
    udaAddMalloc(c_log_malloc_list, (void*)ranks, count, sizeof(int), "int");

    auto tree = static_cast<NTree*>(c_tree);
    count = 0;
    for (int i = 0; i < tree->userdefinedtype->fieldcount; i++) {
        if (tree->userdefinedtype->compoundfield[i].atomictype == UDA_TYPE_UNKNOWN) {
            if (!tree->userdefinedtype->compoundfield[i].pointer) {
                ranks[count] = tree->userdefinedtype->compoundfield[i].rank;
            } else {
                if ((data = (char*)tree->data) == nullptr) {
                    return nullptr;
                }
                udaFindMalloc2(c_log_malloc_list, &data[tree->userdefinedtype->compoundfield[i].offset], &count0, &size,
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

int* udaGetNodeAtomicRank(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree)
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
    udaAddMalloc(c_log_malloc_list, (void*)ranks, count, sizeof(int), "int");

    auto tree = static_cast<NTree*>(c_tree);
    count = 0;
    for (int i = 0; i < tree->userdefinedtype->fieldcount; i++) {
        if (tree->userdefinedtype->compoundfield[i].atomictype != UDA_TYPE_UNKNOWN) {
            if (!tree->userdefinedtype->compoundfield[i].pointer) {
                ranks[count] = tree->userdefinedtype->compoundfield[i].rank;
            } else {
                if ((data = (char*)tree->data) == nullptr) {
                    return nullptr;
                }
                udaFindMalloc2(c_log_malloc_list, &data[tree->userdefinedtype->compoundfield[i].offset], &count0, &size,
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
int** udaGetNodeStructureShape(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree)
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
    udaAddMalloc(c_log_malloc_list, (void*)shapes, count, sizeof(int*), "int *");

    auto tree = static_cast<NTree*>(c_tree);
    count = 0;
    for (int i = 0; i < tree->userdefinedtype->fieldcount; i++) {
        if (tree->userdefinedtype->compoundfield[i].atomictype == UDA_TYPE_UNKNOWN) {
            if (!tree->userdefinedtype->compoundfield[i].pointer) {
                shapes[count] = tree->userdefinedtype->compoundfield[i].shape;
            } else {
                if ((data = (char*)tree->data) == nullptr) {
                    return nullptr;
                }
                udaFindMalloc2(c_log_malloc_list, &data[tree->userdefinedtype->compoundfield[i].offset], &count0, &size,
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
int** udaGetNodeAtomicShape(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree)
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
    udaAddMalloc(c_log_malloc_list, (void*)shapes, count, sizeof(int*), "int *");

    auto tree = static_cast<NTree*>(c_tree);
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
                udaFindMalloc2(c_log_malloc_list, ptr, &count0, &size, &type, &rank, &shape);
                shapes[count] = shape;
                if (shape == 0 && (rank < 2)) {
                    shape = (int*)malloc(sizeof(int)); // Assume rank 1
                    udaAddMalloc(c_log_malloc_list, (void*)shape, 1, sizeof(int), "int");
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

void udaPrintNodeNames(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree)
{
    auto tree = static_cast<NTree*>(c_tree);

    int namecount;
    char** namelist;
    char** typelist;
    if (tree == nullptr) {
        tree = full_ntree;
    }

    UDA_LOG(UDA_LOG_DEBUG, "\nData Node Structure Names and Types");
    namecount = udaGetNodeStructureCount(tree);                   // Count of all local data structures
    namelist = udaGetNodeStructureNames(c_log_malloc_list, tree); // Names
    typelist = udaGetNodeStructureTypes(c_log_malloc_list, tree); // Types
    UDA_LOG(UDA_LOG_DEBUG, "Structure Count {}", namecount);
    if (namecount > 0) {
        UDA_LOG(UDA_LOG_DEBUG, "  #\tName\tType");
        for (int i = 0; i < namecount; i++) {
            UDA_LOG(UDA_LOG_DEBUG, "[{}]\t{}\t{}", i, namelist[i], typelist[i]);
        }
    }
    UDA_LOG(UDA_LOG_DEBUG, "\nData Node Atomic Names and Types");
    namecount = udaGetNodeAtomicCount(tree);                   // Count of all local atomic data
    namelist = udaGetNodeAtomicNames(c_log_malloc_list, tree); // Names
    typelist = udaGetNodeAtomicTypes(c_log_malloc_list, tree); // Types
    UDA_LOG(UDA_LOG_DEBUG, "Atomic Count {}", namecount);
    if (namecount > 0) {
        UDA_LOG(UDA_LOG_DEBUG, "  #\tName\tType");
        for (int i = 0; i < namecount; i++) {
            UDA_LOG(UDA_LOG_DEBUG, "[{}]\t{}\t{}", i, namelist[i], typelist[i]);
        }
    }
}

/** Print the Atomic Data from a data node to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return Void
 */
void udaPrintNodeAtomic(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree)
{
    auto tree = static_cast<NTree*>(c_tree);

    int namecount;
    char** namelist;
    if (tree == nullptr) {
        tree = full_ntree;
    }
    namecount = udaGetNodeAtomicCount(tree);                   // Count of all local atomic data
    namelist = udaGetNodeAtomicNames(c_log_malloc_list, tree); // Names
    for (int i = 0; i < namecount; i++) {
        udaPrintAtomicType(c_log_malloc_list, tree, namelist[i]);
    }
}

/** Return the number of User Defined Type Structure Definition Components attached to this tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the number of User Defined Type Structure Definition Components.
 */
int udaGetNodeStructureComponentCount(NTREE* c_tree)
{
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    auto tree = static_cast<NTree*>(c_tree);
    return tree->userdefinedtype->fieldcount;
}

/** Return a List of User Defined Type Structure Definition Components Names attached to this tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of User Defined Type Structure Definition Component names.
 */
char** udaGetNodeStructureComponentNames(NTREE* c_tree)
{
    int count;
    char** names;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    auto tree = static_cast<NTree*>(c_tree);
    count = tree->userdefinedtype->fieldcount;
    if (count == 0) {
        return nullptr;
    }
    names = (char**)malloc(count * sizeof(char*));
    for (int i = 0; i < count; i++) {
        names[i] = (char*)tree->userdefinedtype->compoundfield[i].name;
    }
    return names;
}

/** Return a List of User Defined Type Structure Definition Components Types attached to this tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of User Defined Type Structure Definition Component Types.
 */
char** udaGetNodeStructureComponentTypes(NTREE* c_tree)
{
    int count;
    char** names;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    auto tree = static_cast<NTree*>(c_tree);
    count = tree->userdefinedtype->fieldcount;
    if (count == 0) {
        return nullptr;
    }
    names = (char**)malloc(count * sizeof(char*));
    for (int i = 0; i < count; i++) {
        names[i] = (char*)tree->userdefinedtype->compoundfield[i].type;
    }
    return names;
}

/** Return a List of User Defined Type Structure Definition Components Descriptions attached to this tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of User Defined Type Structure Definition Component Descriptions.
 */
char** udaGetNodeStructureComponentDescriptions(NTREE* c_tree)
{
    int count;
    char** names;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    auto tree = static_cast<NTree*>(c_tree);
    count = tree->userdefinedtype->fieldcount;
    if (count == 0) {
        return nullptr;
    }
    names = (char**)malloc(count * sizeof(char*));
    for (int i = 0; i < count; i++) {
        names[i] = (char*)tree->userdefinedtype->compoundfield[i].desc;
    }
    return names;
}

/** Return the Count of User Defined Structure Component Data array elements attached to this tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @param target The name of a User Defined Structure definition.
 * @return the Count of User Defined Structure Component Data Array elements.
 */

int udaGetNodeStructureComponentDataCount(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree, const char* target)
{
    const char* lastname;
    UserDefinedType* user_defined_type;
    int count = 0, size, fieldcount;
    const char* type;
    char* data;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    c_tree = udaFindNTreeStructureComponent2(c_log_malloc_list, c_tree, target,
                                             &lastname); // Identify node and component name
    if (c_tree == nullptr) {
        return 0;
    }

    auto tree = static_cast<NTree*>(c_tree);

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
                udaFindMalloc(c_log_malloc_list, &data[user_defined_type->compoundfield[i].offset], &count, &size,
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
int udaGetNodeStructureComponentDataRank(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree, const char* target)
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
    c_tree = udaFindNTreeStructureComponent2(c_log_malloc_list, c_tree, target,
                                             &lastname); // Identify node and component name
    if (c_tree == nullptr) {
        return 0;
    }
    auto tree = static_cast<NTree*>(c_tree);
    user_defined_type = tree->userdefinedtype;
    fieldcount = tree->userdefinedtype->fieldcount;
    for (int i = 0; i < fieldcount; i++) {
        if (!strcmp(user_defined_type->compoundfield[i].name, lastname)) {
            if (user_defined_type->compoundfield[i].pointer) {
                if ((data = (char*)tree->data) == nullptr) {
                    return 0;
                }
                udaFindMalloc2(c_log_malloc_list, &data[tree->userdefinedtype->compoundfield[i].offset], &count, &size,
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
int* udaGetNodeStructureComponentDataShape(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree, const char* target)
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
    c_tree = udaFindNTreeStructureComponent2(c_log_malloc_list, c_tree, target,
                                             &lastname); // Identify node and component name
    if (c_tree == nullptr) {
        return nullptr;
    }
    auto tree = static_cast<NTree*>(c_tree);
    user_defined_type = tree->userdefinedtype;
    fieldcount = tree->userdefinedtype->fieldcount;
    for (int i = 0; i < fieldcount; i++) {
        if (!strcmp(user_defined_type->compoundfield[i].name, lastname)) {
            if (user_defined_type->compoundfield[i].pointer) {
                if ((data = (char*)tree->data) == nullptr) {
                    return 0;
                }
                udaFindMalloc2(c_log_malloc_list, &data[tree->userdefinedtype->compoundfield[i].offset], &count, &size,
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
int udaGetNodeStructureComponentDataIsPointer(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree, const char* target)
{
    const char* lastname;
    UserDefinedType* user_defined_type;
    int ispointer = 0, fieldcount;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    c_tree = udaFindNTreeStructureComponent2(c_log_malloc_list, c_tree, target,
                                             &lastname); // Identify node and component name
    if (c_tree == nullptr) {
        return 0;
    }
    auto tree = static_cast<NTree*>(c_tree);
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
int udaGetNodeStructureComponentDataSize(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree, const char* target)
{
    const char* lastname;
    UserDefinedType* user_defined_type;
    int count, size = 0, fieldcount;
    const char* type;
    char* data;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    c_tree = udaFindNTreeStructureComponent2(c_log_malloc_list, c_tree, target,
                                             &lastname); // Identify node and component name
    if (c_tree == nullptr) {
        return 0;
    }
    auto tree = static_cast<NTree*>(c_tree);
    user_defined_type = tree->userdefinedtype;
    fieldcount = tree->userdefinedtype->fieldcount;
    for (int i = 0; i < fieldcount; i++) {
        if (!strcmp(user_defined_type->compoundfield[i].name, lastname)) {
            if (user_defined_type->compoundfield[i].pointer) {
                if ((data = (char*)tree->data) == nullptr) {
                    break;
                }
                udaFindMalloc(c_log_malloc_list, &data[user_defined_type->compoundfield[i].offset], &count, &size,
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
const char* udaGetNodeStructureComponentDataDataType(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree,
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
    c_tree = udaFindNTreeStructureComponent2(c_log_malloc_list, c_tree, target,
                                             &lastname); // Identify node and component name
    if (c_tree == nullptr) {
        return "unknown";
    }
    auto tree = static_cast<NTree*>(c_tree);
    user_defined_type = tree->userdefinedtype;
    fieldcount = tree->userdefinedtype->fieldcount;
    for (int i = 0; i < fieldcount; i++) {
        if (!strcmp(user_defined_type->compoundfield[i].name, lastname)) {
            if (user_defined_type->compoundfield[i].pointer) {
                if ((data = (char*)tree->data) == nullptr) {
                    break;
                }
                udaFindMalloc(c_log_malloc_list, &data[user_defined_type->compoundfield[i].offset], &count, &size,
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
void* udaGetNodeStructureComponentData(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree, const char* target)
{
    const char* lastname;
    UserDefinedType* user_defined_type;
    int offset, fieldcount;
    char* p;
    void* data = nullptr;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    c_tree = udaFindNTreeStructureComponent2(c_log_malloc_list, c_tree, target,
                                             &lastname); // Identify node and component name
    if (c_tree == nullptr) {
        return nullptr;
    }

    auto tree = static_cast<NTree*>(c_tree);

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
void udaPrintNodeStructureComponentData(NTREE* c_tree, LOGMALLOCLIST* c_log_malloc_list,
                                        USERDEFINEDTYPELIST* c_user_defined_type_list, const char* target)
{
    auto user_defined_type_list = static_cast<UserDefinedTypeList*>(c_user_defined_type_list);

    int namecount, count;
    const char* type;
    const char* lastname;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }

    NTREE* node = udaFindNTreeStructureComponent2(c_log_malloc_list, c_tree, target, &lastname); // Locate the Node
    if (c_tree == nullptr) {
        return;
    }

    count = udaGetNodeStructureComponentDataCount(c_log_malloc_list, node, lastname);   // Array Size
    type = udaGetNodeStructureComponentDataDataType(c_log_malloc_list, node, lastname); // Type

    if (count > 0) {
        UDA_LOG(UDA_LOG_DEBUG, "[{}] Data Count {}   Type {}", target, count, type);
        UDA_LOG(UDA_LOG_DEBUG, "Data Values");
        if (!strcmp(type, "float")) {
            auto s = (float*)udaGetNodeStructureComponentData(c_log_malloc_list, node, lastname);
            for (int i = 0; i < count; i++) {
                UDA_LOG(UDA_LOG_DEBUG, "[{}] {}", i, s[i]);
            }
            return;
        }
        if (!strcmp(type, "int")) {
            auto s = (int*)udaGetNodeStructureComponentData(c_log_malloc_list, node, lastname);
            for (int i = 0; i < count; i++) {
                UDA_LOG(UDA_LOG_DEBUG, "[{}] {}", i, s[i]);
            }
            return;
        }
        if (!strcmp(type, "STRING")) {
            auto s = (char*)udaGetNodeStructureComponentData(c_log_malloc_list, node, lastname);
            UDA_LOG(UDA_LOG_DEBUG, "{}", s);
            return;
        }

        USERDEFINEDTYPE* c_user_defined_type;
        if ((c_user_defined_type = udaFindUserDefinedType(c_user_defined_type_list, type, 0)) != nullptr) {
            auto user_defined_type = static_cast<UserDefinedType*>(c_user_defined_type);

            int firstpass = 1, offset, namecount2;
            char** namelist2;
            NTree temp;
            initNTree(&temp);
            void* str = nullptr;
            void* data = nullptr;
            void* olddata = nullptr;
            char* p = (char*)udaGetNodeStructureComponentData(c_log_malloc_list, node, lastname); // Structure Array
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
                            static_cast<UserDefinedType*>(udaFindUserDefinedType(user_defined_type_list, type, 0));
                        if (firstpass) {
                            udaAddNonMalloc(c_log_malloc_list, data, 1, user_defined_type->compoundfield[i].size, type);
                            firstpass = 0;
                        } else {
                            udaChangeNonMalloc(c_log_malloc_list, olddata, data, 1,
                                               user_defined_type->compoundfield[i].size, type);
                        }
                        olddata = data;

                        namecount2 = udaGetNodeStructureComponentCount(&temp); // Count of structure elements
                        namelist2 = udaGetNodeStructureComponentNames(&temp);  // List of structure element names
                        UDA_LOG(UDA_LOG_DEBUG, "Data Count {}   Type {}", namecount2, type);

                        for (int k = 0; k < namecount2; k++) {
                            udaPrintNodeStructureComponentData(&temp, c_log_malloc_list, user_defined_type_list,
                                                               namelist2[k]);
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
void udaPrintNodeStructure(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree)
{
    int count, acount, scount, kstart = 1;
    char **anamelist, **snamelist;
    void* data;

    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }

    acount = udaGetNodeAtomicCount(c_tree); // Count of the Tree Node Structure atomic type components
    anamelist = udaGetNodeAtomicNames(c_log_malloc_list, c_tree);
    scount = udaGetNodeStructureCount(c_tree); // Count of the Tree Node Structure structure type components
    snamelist = udaGetNodeStructureNames(c_log_malloc_list, c_tree);
    count = udaGetNodeStructureDataCount(c_log_malloc_list, c_tree); // Count of the Tree Node Structure Array elements

    NTREE* node = c_tree; // Start at the base node: all other structure array elements are sibling nodes
    auto tree = static_cast<NTree*>(c_tree);

    for (int j = 0; j < count; j++) {

        UDA_LOG(UDA_LOG_DEBUG, "{} contents:", tree->userdefinedtype->name);

        data = udaGetNodeStructureArrayData(c_log_malloc_list, tree, j); // Loop over Structure Array Elements

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
                add_error(ErrorType::Code, "udaPrintNodeStructure", 999, "Structure Array element Node not Found!");
                return;
            }
        }

        for (int i = 0; i < acount; i++) {
            udaPrintAtomicType(c_log_malloc_list, node, anamelist[i]); // Print Atomic Components
        }

        for (int i = 0; i < scount; i++) { // Print Structured Components

            // Structured components must be children of this node.
            NTREE* node2;
            if ((node2 = udaFindNTreeStructure(c_log_malloc_list, node, snamelist[i])) != nullptr) {
                udaPrintNodeStructure(c_log_malloc_list, node2);
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
float* udaCastNodeStructureComponentDatatoFloat(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree, const char* target)
{
    int count;
    const char* type;
    const char* lastname;
    float* data;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }

    NTREE* node = udaFindNTreeStructureComponent2(c_log_malloc_list, c_tree, target, &lastname);
    if (c_tree == nullptr) {
        return nullptr;
    }

    count = udaGetNodeStructureComponentDataCount(c_log_malloc_list, node, lastname);
    type = udaGetNodeStructureComponentDataDataType(c_log_malloc_list, node, lastname);

    if (!strcmp(type, "float")) {
        return ((float*)udaGetNodeStructureComponentData(c_log_malloc_list, node, lastname));
    }

    if (count == 0) {
        return nullptr;
    }

    data = (float*)malloc(count * sizeof(float));
    if (!strcmp(type, "double")) {
        double* s = (double*)udaGetNodeStructureComponentData(c_log_malloc_list, node, lastname);
        for (int i = 0; i < count; i++) {
            data[i] = (float)s[i];
        }
        return data;
    }
    if (!strcmp(type, "int")) {
        int* s = (int*)udaGetNodeStructureComponentData(c_log_malloc_list, node, lastname);
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
double* udaCastNodeStructureComponentDatatoDouble(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree, const char* target)
{
    int count;
    const char* type;
    const char* lastname;
    double* data;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }

    NTREE* node = udaFindNTreeStructureComponent2(c_log_malloc_list, c_tree, target, &lastname);
    if (c_tree == nullptr) {
        return nullptr;
    }

    count = udaGetNodeStructureComponentDataCount(c_log_malloc_list, node, lastname);
    type = udaGetNodeStructureComponentDataDataType(c_log_malloc_list, node, lastname);

    if (!strcmp(type, "double")) {
        return (double*)udaGetNodeStructureComponentData(c_log_malloc_list, node, lastname);
    }

    if (count == 0) {
        return nullptr;
    }

    data = (double*)malloc(count * sizeof(double));
    if (!strcmp(type, "float")) {
        float* s = (float*)udaGetNodeStructureComponentData(c_log_malloc_list, node, lastname);
        for (int i = 0; i < count; i++) {
            data[i] = (double)s[i];
        }
        return data;
    }
    if (!strcmp(type, "int")) {
        int* s = (int*)udaGetNodeStructureComponentData(c_log_malloc_list, node, lastname);
        for (int i = 0; i < count; i++) {
            data[i] = (double)s[i];
        }
        return data;
    }

    return nullptr;
}

//---------------------------------------------------------------------------------------------
// Tree Branch Family: Whole tree is in scope

/** Initialise a NTree data structure.
 *
 * @param str A pointer to a NTree data structure instance.
 * @return Void.
 */
void uda::structures::initNTree(NTree* str)
{
    str->branches = 0;
    str->name[0] = '\0';
    str->userdefinedtype = nullptr;
    str->data = nullptr;
    str->parent = nullptr;
    str->children = nullptr;
}

/** Initialise the Global NTree list structure.
 *
 * @return Void.
 */
void initNTreeList(NTreeList* ntree_list)
{
    ntree_list->listCount = 0;
    ntree_list->forrest = nullptr;
}

/** Print the Contents of a tree node to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return Void
 */
void udaPrintNTree2(NTREE* c_tree)
{
    auto tree = static_cast<NTree*>(c_tree);

    if (tree == nullptr) {
        tree = full_ntree;
    }
    UDA_LOG(UDA_LOG_DEBUG, "\nNTree Node Contents");
    UDA_LOG(UDA_LOG_DEBUG, "Name    : {}", tree->name);
    UDA_LOG(UDA_LOG_DEBUG, "Type    : {}", tree->userdefinedtype->name);
#ifdef A64
    UDA_LOG(UDA_LOG_DEBUG, "Parent  : {}", (UVOIDTYPE)tree->parent);
#else
    UDA_LOG(UDA_LOG_DEBUG, "Parent  : {}", (UVOIDTYPE)tree->parent);
#endif
    UDA_LOG(UDA_LOG_DEBUG, "Children: {}", tree->branches);
#ifdef A64
    for (int i = 0; i < tree->branches; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "[{}]: {}", i, (UVOIDTYPE)tree->children[i]);
    }
#else
    for (int i = 0; i < tree->branches; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "[{}]: {}", i, (UVOIDTYPE)tree->children[i]);
    }
#endif
    for (int i = 0; i < tree->branches; i++) {
        udaPrintNTree2(tree->children[i]);
    }
}

/** Print the Contents of a tree node to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return Void
 */
void udaPrintNTree(NTREE* c_tree, USERDEFINEDTYPELIST* c_user_defined_type_list)
{
    auto tree = static_cast<NTree*>(c_tree);

    if (tree == nullptr) {
        tree = full_ntree;
    }
    UDA_LOG(UDA_LOG_DEBUG, "--------------------------------------------------------------------");
#ifdef A64
    UDA_LOG(UDA_LOG_DEBUG, "\nNTree Node {} ({}) Contents", (UVOIDTYPE)tree, (UVOIDTYPE)tree);
#else
    UDA_LOG(UDA_LOG_DEBUG, "\nNTree Node {} ({}) Contents", (UVOIDTYPE)tree, (UVOIDTYPE)tree);
#endif
    UDA_LOG(UDA_LOG_DEBUG, "Name: {}", tree->name);
    UDA_LOG(UDA_LOG_DEBUG, "Children: {}", tree->branches);
    auto user_defined_type_list = static_cast<UserDefinedTypeList*>(c_user_defined_type_list);
    print_user_defined_type_table(user_defined_type_list, *tree->userdefinedtype);
    for (int i = 0; i < tree->branches; i++) {
        udaPrintNTree(tree->children[i], c_user_defined_type_list);
    }
}

/** Print Details of the tree node List to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return Void
 */
void udaPrintNTreeList(NTREE* c_tree)
{
    auto tree = static_cast<NTree*>(c_tree);

    if (tree == nullptr) {
        tree = full_ntree;
    }
#ifdef A64
    UDA_LOG(UDA_LOG_DEBUG, "{}\t({})\t{}\t{}\t{}", (UVOIDTYPE)tree, (UVOIDTYPE)tree, tree->name,
            tree->userdefinedtype->name, tree->branches);
#else
    UDA_LOG(UDA_LOG_DEBUG, "{}\t({})\t{}\t{}\t{}", (UVOIDTYPE)tree, (UVOIDTYPE)tree, tree->name,
            tree->userdefinedtype->name, tree->branches);
#endif
    for (int i = 0; i < tree->branches; i++) {
        udaPrintNTreeList(tree->children[i]);
    }
}

/** Return a Count of User Defined Type Tree Nodes from and including the passed tree node.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the Count of Tree Nodes.
 */
int udaGetNTreeStructureCount(NTREE* c_tree)
{
    int count = 1;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }
    auto tree = static_cast<NTree*>(c_tree);
    if (tree->branches == 0) {
        return count;
    }
    for (int i = 0; i < tree->branches; i++) {
        if (i == 0 ||
            strcmp(tree->children[i]->userdefinedtype->name, tree->children[i - 1]->userdefinedtype->name) != 0) {
            count = count + udaGetNTreeStructureCount(tree->children[i]);
        }
    }
    return count;
}

/** Return a List of User Defined Type Structure Names attached to this tree branch.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of User Defined Type Structure names.
 */
char** udaGetNTreeStructureNames(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree)
{
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }

    auto names = (char**)malloc(sizeof(char*));
    udaAddMalloc(c_log_malloc_list, (void*)names, 1, sizeof(char*), "char *");

    auto tree = static_cast<NTree*>(c_tree);

    names[0] = tree->name;

    if (tree->branches == 0) {
        return names;
    }

    int count = 1;
    for (int i = 0; i < tree->branches; i++) {
        if (i == 0 ||
            strcmp(tree->children[i]->userdefinedtype->name, tree->children[i - 1]->userdefinedtype->name) != 0) {
            int childcount = udaGetNTreeStructureCount(tree->children[i]);
            VOIDTYPE old = (VOIDTYPE)names;
            names = (char**)realloc((void*)names, (count + childcount) * sizeof(char*));
            udaChangeMalloc(c_log_malloc_list, old, (void*)names, (count + childcount), sizeof(char*), "char *");
            char** childnames = udaGetNTreeStructureNames(c_log_malloc_list, tree->children[i]);
            for (int j = 0; j < childcount; j++) {
                names[count + j] = childnames[j];
            }
            count += childcount;
        }
    }
    return names;
}

/** Return a List of User Defined Type Structure Type Names attached to this tree branch.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of User Defined Type Structure Type names.
 */
char** udaGetNTreeStructureTypes(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree)
{
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }

    auto names = (char**)malloc(sizeof(char*));
    udaAddMalloc(c_log_malloc_list, (void*)names, 1, sizeof(char*), "char *");

    auto tree = static_cast<NTree*>(c_tree);

    names[0] = tree->userdefinedtype->name;
    if (tree->branches == 0) {
        return names;
    }

    int count = 1;
    for (int i = 0; i < tree->branches; i++) {
        if (i == 0 ||
            strcmp(tree->children[i]->userdefinedtype->name, tree->children[i - 1]->userdefinedtype->name) != 0) {
            int childcount = udaGetNTreeStructureCount(tree->children[i]);
            VOIDTYPE old = (VOIDTYPE)names;
            names = (char**)realloc((void*)names, (count + childcount) * sizeof(char*));
            udaChangeMalloc(c_log_malloc_list, old, (void*)names, (count + childcount), sizeof(char*), "char *");
            char** childnames = udaGetNTreeStructureTypes(c_log_malloc_list, tree->children[i]);
            for (int j = 0; j < childcount; j++) {
                names[count + j] = childnames[j];
            }
            count = count + childcount;
        }
    }
    return names;
}

/** Print the Names and Types of all Data Structures to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return Void
 */
void udaPrintNTreeStructureNames(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree)
{
    auto tree = static_cast<NTree*>(c_tree);

    int namecount;
    char **namelist, **typelist;
    if (tree == nullptr) {
        tree = full_ntree;
    }
#ifdef A64
    UDA_LOG(UDA_LOG_DEBUG, "\nData Tree {} Structure Names and Types", (UVOIDTYPE)tree);
#else
    UDA_LOG(UDA_LOG_DEBUG, "\nData Tree {} Structure Names and Types", (UVOIDTYPE)tree);
#endif
    namecount = udaGetNTreeStructureCount(tree);                   // Count of all Tree Nodes
    namelist = udaGetNTreeStructureNames(c_log_malloc_list, tree); // Names of all user defined data structures
    typelist = udaGetNTreeStructureTypes(c_log_malloc_list, tree); // Types of all user defined data structures
    UDA_LOG(UDA_LOG_DEBUG, "Total Structure Count {}", namecount);
    UDA_LOG(UDA_LOG_DEBUG, "  #\tName\tType");
    for (int i = 0; i < namecount; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "[{}]\t{}\t{}", i, namelist[i], typelist[i]);
    }
}

/** Return the total number of User Defined Type Structure Definition Components attached to this tree branch.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the number of User Defined Type Structure Definition Components.
 */
int udaGetNTreeStructureComponentCount(NTREE* c_tree)
{
    int count;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }

    auto tree = static_cast<NTree*>(c_tree);

    count = tree->userdefinedtype->fieldcount;

    for (int i = 0; i < tree->branches; i++) {
        if (i == 0 ||
            strcmp(tree->children[i]->userdefinedtype->name, tree->children[i - 1]->userdefinedtype->name) != 0) {
            count = count + udaGetNTreeStructureComponentCount(tree->children[i]);
        }
    }
    return count;
}

/** Return a List of User Defined Type Structure Definition Components Names attached to this tree branch.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of User Defined Type Structure Definition Component names.
 */
char** udaGetNTreeStructureComponentNames(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree)
{
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }

    auto tree = static_cast<NTree*>(c_tree);

    int count = tree->userdefinedtype->fieldcount;
    if (count == 0) {
        return nullptr;
    }

    char** names = udaGetNodeStructureComponentNames(tree);

    for (int i = 0; i < tree->branches; i++) {
        if (i == 0 ||
            strcmp(tree->children[i]->userdefinedtype->name, tree->children[i - 1]->userdefinedtype->name) != 0) {
            int childcount = udaGetNTreeStructureComponentCount(tree->children[i]);
            auto old = (VOIDTYPE)names;
            names = (char**)realloc((void*)names, (count + childcount) * sizeof(char*));
            udaChangeMalloc(c_log_malloc_list, old, (void*)names, (count + childcount), sizeof(char*), "char *");
            char** childnames = udaGetNTreeStructureComponentNames(c_log_malloc_list, tree->children[i]);
            for (int j = 0; j < childcount; j++) {
                names[count + j] = childnames[j];
            }
            count += childcount;
        }
    }
    return names;
}

/** Return a List of User Defined Type Structure Definition Components Types attached to this tree branch.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of User Defined Type Structure Definition Component Types.
 */
char** udaGetNTreeStructureComponentTypes(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree)
{
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }

    auto tree = static_cast<NTree*>(c_tree);

    int count = tree->userdefinedtype->fieldcount;
    if (count == 0) {
        return nullptr;
    }

    char** names = udaGetNodeStructureComponentTypes(tree);

    for (int i = 0; i < tree->branches; i++) {
        if (i == 0 ||
            strcmp(tree->children[i]->userdefinedtype->name, tree->children[i - 1]->userdefinedtype->name) != 0) {
            int childcount = udaGetNTreeStructureComponentCount(tree->children[i]);
            auto old = (VOIDTYPE)names;
            names = (char**)realloc((void*)names, (count + childcount) * sizeof(char*));
            udaChangeMalloc(c_log_malloc_list, old, (void*)names, (count + childcount), sizeof(char*), "char *");
            char** childnames = udaGetNTreeStructureComponentTypes(c_log_malloc_list, tree->children[i]);
            for (int j = 0; j < childcount; j++) {
                names[count + j] = childnames[j];
            }
            count += childcount;
        }
    }
    return names;
}

/** Return a List of User Defined Type Structure Definition Components Descriptions attached to this tree branch.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of User Defined Type Structure Definition Component Descriptions.
 */
char** udaGetNTreeStructureComponentDescriptions(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree)
{
    auto tree = static_cast<NTree*>(c_tree);

    if (tree == nullptr) {
        tree = static_cast<NTree*>(udaGetFullNTree());
    }

    int count = tree->userdefinedtype->fieldcount;
    if (count == 0) {
        return nullptr;
    }

    char** names = udaGetNodeStructureComponentDescriptions(tree);

    for (int i = 0; i < tree->branches; i++) {
        if (i == 0 ||
            strcmp(tree->children[i]->userdefinedtype->name, tree->children[i - 1]->userdefinedtype->name) != 0) {
            int childcount = udaGetNTreeStructureComponentCount(tree->children[i]);
            auto old = (VOIDTYPE)names;
            names = (char**)realloc((void*)names, (count + childcount) * sizeof(char*));
            udaChangeMalloc(c_log_malloc_list, old, (void*)names, (count + childcount), sizeof(char*), "char *");
            char** childnames = udaGetNTreeStructureComponentDescriptions(c_log_malloc_list, tree->children[i]);
            for (int j = 0; j < childcount; j++) {
                names[count + j] = childnames[j];
            }
            count += childcount;
        }
    }
    return names;
}

/** Print the Names and Types of all Data Elements to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return Void
 */
void udaPrintNTreeStructureComponentNames(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree)
{
    auto tree = static_cast<NTree*>(c_tree);

    int namecount;
    char **namelist, **typelist, **desclist;
    if (tree == nullptr) {
        tree = full_ntree;
    }
    UDA_LOG(UDA_LOG_DEBUG, "\nData Tree Structure Component Names, Types and Descriptions");
    namecount = udaGetNTreeStructureComponentCount(tree);                   // Count of all Tree Nodes
    namelist = udaGetNTreeStructureComponentNames(c_log_malloc_list, tree); // Names of all structure elements
    typelist = udaGetNTreeStructureComponentTypes(c_log_malloc_list, tree); // Types of all structure elements
    desclist =
        udaGetNTreeStructureComponentDescriptions(c_log_malloc_list, tree); // Descriptions of all structure elements
    UDA_LOG(UDA_LOG_DEBUG, "Total Structure Component Count {}", namecount);
    UDA_LOG(UDA_LOG_DEBUG, "  #\tName\tType\tDescription");
    for (int i = 0; i < namecount; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "[{}]\t{}\t{}\t{}", i, namelist[i], typelist[i], desclist[i]);
    }
}

void udaGetNodeStructureComponentDataShape_f(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree, const char* target,
                                             int* shape_f)
{
    int rank = udaGetNodeStructureComponentDataRank(c_log_malloc_list, c_tree, target);
    for (int i = 0; i < MAXRANK; i++) {
        shape_f[i] = 0;
    }
    if (rank > 1 && rank <= MAXRANK) {
        int* shape = udaGetNodeStructureComponentDataShape(c_log_malloc_list, c_tree, target);
        if (shape != nullptr) {
            for (int i = 0; i < rank; i++) {
                shape_f[i] = shape[i];
            }
        }
    } else {
        shape_f[0] = udaGetNodeStructureComponentDataCount(c_log_malloc_list, c_tree, target);
    }
}

void udaGetNodeStructureComponentShortData_f(LOGMALLOCLIST* c_log_malloc_list, NTREE* node, const char* target,
                                             short* data_f)
{
    auto data = (short*)udaGetNodeStructureComponentData(c_log_malloc_list, node, target);
    int count = udaGetNodeStructureComponentDataCount(c_log_malloc_list, node, target);
    for (int i = 0; i < count; i++) {
        data_f[i] = data[i];
    }
}

void udaGetNodeStructureComponentFloatData_f(LOGMALLOCLIST* c_log_malloc_list, NTREE* node, const char* target,
                                             float* data_f)
{
    float* data = (float*)udaGetNodeStructureComponentData(c_log_malloc_list, node, target);
    int count = udaGetNodeStructureComponentDataCount(c_log_malloc_list, node, target);
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

short* udaCastNodeStructureComponentDatatoShort(LOGMALLOCLIST* c_log_malloc_list, NTREE* c_tree, const char* target)
{
    int count;
    const char* type;
    const char* lastname;
    short* data;
    if (c_tree == nullptr) {
        c_tree = udaGetFullNTree();
    }

    NTREE* node = udaFindNTreeStructureComponent2(c_log_malloc_list, c_tree, target, &lastname);
    if (node == nullptr) {
        return nullptr;
    }

    count = udaGetNodeStructureComponentDataCount(c_log_malloc_list, node, lastname);
    type = udaGetNodeStructureComponentDataDataType(c_log_malloc_list, node, lastname);

    if (!strcmp(type, "short")) {
        return (short*)udaGetNodeStructureComponentData(c_log_malloc_list, node, lastname);
    }

    if (count == 0) {
        return nullptr;
    }

    data = (short*)malloc(count * sizeof(short));
    if (!strcmp(type, "double")) {
        double* s = (double*)udaGetNodeStructureComponentData(c_log_malloc_list, node, lastname);
        for (int i = 0; i < count; i++) {
            data[i] = (short)s[i];
        }
        return data;
    }
    if (!strcmp(type, "float")) {
        float* s = (float*)udaGetNodeStructureComponentData(c_log_malloc_list, node, lastname);
        for (int i = 0; i < count; i++) {
            data[i] = (short)s[i];
        }
        return data;
    }
    if (!strcmp(type, "int")) {
        int* s = (int*)udaGetNodeStructureComponentData(c_log_malloc_list, node, lastname);
        for (int i = 0; i < count; i++) {
            data[i] = (short)s[i];
        }
        return data;
    }
    if (!strcmp(type, "unsigned int")) {
        unsigned int* s = (unsigned int*)udaGetNodeStructureComponentData(c_log_malloc_list, node, lastname);
        for (int i = 0; i < count; i++) {
            data[i] = (short)s[i];
        }
        return data;
    }
    if (!strcmp(type, "unsigned short")) {
        unsigned short* s = (unsigned short*)udaGetNodeStructureComponentData(c_log_malloc_list, node, lastname);
        for (int i = 0; i < count; i++) {
            data[i] = (short)s[i];
        }
        return data;
    }
    return nullptr;
}

void udaCastNodeStructureComponentDatatoShort_f(LOGMALLOCLIST* c_log_malloc_list, NTREE* node, const char* target,
                                                short* data_f)
{
    short* data = udaCastNodeStructureComponentDatatoShort(c_log_malloc_list, node, target);
    if (data != nullptr) {
        int count = udaGetNodeStructureComponentDataCount(c_log_malloc_list, node, target);
        for (int i = 0; i < count; i++) {
            data_f[i] = data[i];
        }
        free(data);
    }
}

void udaCastNodeStructureComponentDatatoFloat_f(LOGMALLOCLIST* c_log_malloc_list, NTREE* node, const char* target,
                                                float* data_f)
{
    float* data = udaCastNodeStructureComponentDatatoFloat(c_log_malloc_list, node, target);
    if (data != nullptr) {
        int count = udaGetNodeStructureComponentDataCount(c_log_malloc_list, node, target);
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
