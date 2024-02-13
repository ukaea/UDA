//==============================================================================================================
// Example: Send a user defined structure named mystruct of type MYSTRUCT
//
// Static Master list of all known user defined types from header files and data bindings to XML Schema.
// This list is extended at run time when unknown types are encountered (e.g. from netCDF4 files)
//
// USERDEFINEDTYPELIST userdefinedtypelist = getOpaqueStructurefromDatabase or ...
//
// pointer to the Data structure with measurement data etc.
//
// MYSTRUCT *mystruct = &data;
//
// Fetch the structure Definition from the Master List of all known structures
//
// USERDEFINEDTYPE *udtype = udaFindUserDefinedType("MYSTRUCT");
//
// Send the Data
//
// rc = xdr_user_defined_type_data(xdrs, udtype, (void *)&mystruct);
//
// Arrays of User Defined Structures are ported using a special structure names SARRAY ....
//
//==============================================================================================================
// Example: Receive a user defined structure of unknown type
//
// void *mystruct;  // pointer to the new Data structure with measurement data etc.
//
// Create an empty structure definition
//
// USERDEFINEDTYPE udtype;
// initUserDefinedType(&udtype);
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

#ifdef __GNUC__

#  include <strings.h>

#elif defined(_WIN32)
#  include <string.h>
#  define strcasecmp _stricmp
#endif

#include "clientserver/errorLog.h"
#include "clientserver/protocolXML2Put.h"
#include "clientserver/stringUtils.h"
#include "clientserver/xdrlib.h"
#include "logging/logging.h"

#include "xdrUserDefinedData.h"
#include <uda/structured.h>

#if defined(SERVERBUILD)
#  include "server/udaServer.h"
#endif

using namespace uda::client_server;
using namespace uda::logging;

static unsigned int last_malloc_index = 0; // Malloc Log search index last value
static unsigned int* last_malloc_index_value =
    &last_malloc_index; // Preserve Malloc Log search index last value in GENERAL_STRUCT
static NTREE* full_ntree = nullptr;

NTREE* udaGetFullNTree()
{
    return full_ntree;
}

void udaSetFullNTree(NTREE* ntree)
{
    full_ntree = ntree;
}

void udaSetLastMallocIndexValue(unsigned int* lastMallocIndexValue_in)
{
    last_malloc_index_value = lastMallocIndexValue_in;
    last_malloc_index = *last_malloc_index_value;
}

//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------

/** Initialise a SARRAY data structure.
 *
 * @param str A pointer to a SARRAY data structure instance.
 * @return Void.
 */
void initSArray(SARRAY* str)
{
    str->count = 0;
    str->rank = 0;
    str->shape = nullptr;
    str->data = nullptr;
    str->type[0] = '\0';
}

/** Print the Contents of a SARRAY data structure.
 *
 * @param fd A File Descriptor.
 * @param str A SARRAY data structure instance.
 * @return Void.
 */
void udaPrintSarray(SARRAY str)
{
    UDA_LOG(UDA_LOG_DEBUG, "SARRAY Contents\n");
    UDA_LOG(UDA_LOG_DEBUG, "Type : %s\n", str.type);
    UDA_LOG(UDA_LOG_DEBUG, "Rank : %d\n", str.rank);
    UDA_LOG(UDA_LOG_DEBUG, "Count: %d\n", str.count);
    if (str.rank > 0 && str.shape != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "shape  : [%d", str.shape[0]);
        for (int i = 1; i < str.rank; i++) {
            if (i < str.rank - 1) {
                UDA_LOG(UDA_LOG_DEBUG, "%d,", str.shape[i]);
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "%d", str.shape[i]);
            }
        }
        UDA_LOG(UDA_LOG_DEBUG, "]\n");
    }
    UDA_LOG(UDA_LOG_DEBUG, "\n");
}

/** Add an NTREE List entry.
 *
 * @param node A NTREE node to add.
 * @return Void.
 */
void udaAddNTreeList(LOGMALLOCLIST* logmalloclist, NTREE* node, NTREELIST* ntree_list)
{
    VOIDTYPE old = (VOIDTYPE)ntree_list->forrest;
    ntree_list->forrest = (NTREE*)realloc((void*)ntree_list->forrest, (++ntree_list->listCount) * sizeof(NTREE*));
    udaChangeMalloc(logmalloclist, old, (void*)ntree_list->forrest, ntree_list->listCount, sizeof(NTREE*), "NTREE *");
    ntree_list->forrest[ntree_list->listCount] = *node;
}

/** Add an NTREE node to an array of child nodes.
 *
 * @param parent A NTREE node with a set of child nodes
 * @param child A NTREE node to add to the existing set of child nodes
 * @return Void.
 */
void udaAddNTree(NTREE* parent, NTREE* child)
{
    int branch;
    if (child == nullptr || parent == nullptr) {
        return;
    }
    child->parent = parent;           // Identify the parent of the child node
    branch = parent->branches;        // Number of existing siblings
    parent->children[branch] = child; // Append the new node
    parent->branches++;               // Update the count of children
}

/** Free an NTREE node together with the array of child nodes.
 *
 * @param ntree A NTREE node with or without a set of child nodes
 * @return Void.
 */
void udaFreeNTreeNode(NTREE* ntree)
{
    if (ntree == nullptr) {
        return;
    }
    if (ntree->branches > 0 && ntree->children != nullptr) {
        for (int i = 0; i < ntree->branches; i++) {
            udaFreeNTreeNode(ntree->children[i]);
        }
        free(ntree->children);
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
    char work[STRING_LENGTH];
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
                            snprintf(work, STRING_LENGTH, " = %d]", defvalues[j]); // Array size
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

/** Print the Contents of a COMPOUNDFIELD data structure.
 *
 * @param fd A File Descriptor.
 * @param str A COMPOUNDFIELD data structure instance.
 * @return Void.
 */
void udaPrintCompoundField(COMPOUNDFIELD str)
{
    UDA_LOG(UDA_LOG_DEBUG, "COMPOUNDFIELD Contents\n");
    UDA_LOG(UDA_LOG_DEBUG, "name     : %s\n", str.name);
    UDA_LOG(UDA_LOG_DEBUG, "type     : %s\n", str.type);
    UDA_LOG(UDA_LOG_DEBUG, "desc     : %s\n", str.desc);
    UDA_LOG(UDA_LOG_DEBUG, "Atomic type id : %d\n", str.atomictype);
    UDA_LOG(UDA_LOG_DEBUG, "pointer  : %d\n", str.pointer);
    UDA_LOG(UDA_LOG_DEBUG, "size     : %d\n", str.size);
    UDA_LOG(UDA_LOG_DEBUG, "offset   : %d\n", str.offset);
    UDA_LOG(UDA_LOG_DEBUG, "offpad   : %d\n", str.offpad);
    UDA_LOG(UDA_LOG_DEBUG, "alignment: %d\n", str.alignment);
    UDA_LOG(UDA_LOG_DEBUG, "rank     : %d\n", str.rank);
    UDA_LOG(UDA_LOG_DEBUG, "count    : %d\n", str.count);
    if (str.rank > 0 && str.shape != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "shape    : [", str.shape[0]);
        for (int i = 0; i < str.rank; i++) {
            if (i < str.rank - 1) {
                udaLog(UDA_LOG_DEBUG, "%d,", str.shape[i]);
            } else {
                udaLog(UDA_LOG_DEBUG, "%d", str.shape[i]);
            }
        }
        udaLog(UDA_LOG_DEBUG, "]\n");
    }
}

/** Print the Tabulated Contents of a COMPOUNDFIELD data structure.
 *
 * @param fd A File Descriptor.
 * @param str A COMPOUNDFIELD data structure instance.
 * @return Void.
 */
void udaPrintCompoundFieldTable(COMPOUNDFIELD str)
{
    UDA_LOG(UDA_LOG_DEBUG, "\t%20s\t%16s\t%d\t%d\t%d\t%d\t%d\t%d\n", str.name, str.type, str.pointer, str.size,
            str.count, str.offset, str.offpad, str.alignment);
}

/** Print the Contents of a USERDEFINEDTYPE data structure.
 *
 * @param fd A File Descriptor.
 * @param str A USERDEFINEDTYPE data structure instance.
 * @return Void.
 */
void udaPrintUserDefinedType(USERDEFINEDTYPE str)
{
    UDA_LOG(UDA_LOG_DEBUG, "USERDEFINEDTYPE Contents\n");
    UDA_LOG(UDA_LOG_DEBUG, "name        : %s\n", str.name);
    UDA_LOG(UDA_LOG_DEBUG, "source      : %s\n", str.source);
    UDA_LOG(UDA_LOG_DEBUG, "ID Reference: %d\n", str.ref_id);
    UDA_LOG(UDA_LOG_DEBUG, "size        : %d\n", str.size);
    UDA_LOG(UDA_LOG_DEBUG, "fieldcount  : %d\n", str.fieldcount);

    printImage(str.image, str.imagecount);
    UDA_LOG(UDA_LOG_DEBUG, "\n");

    if (str.compoundfield != nullptr) {
        for (int i = 0; i < str.fieldcount; i++) {
            udaPrintCompoundField(str.compoundfield[i]);
        }
    }
    UDA_LOG(UDA_LOG_DEBUG, "\n");
}

/** Print the Tabulated Contents of a USERDEFINEDTYPE data structure.
 *
 * @param fd A File Descriptor.
 * @param str A USERDEFINEDTYPE data structure instance.
 * @return Void.
 */
void udaPrintUserDefinedTypeTable(USERDEFINEDTYPELIST* userdefinedtypelist, USERDEFINEDTYPE str)
{
    UDA_LOG(UDA_LOG_DEBUG, "USERDEFINEDTYPE name: %s size: %d [%d] fieldcount: %d ref_id: %d \n", str.name, str.size,
            udaGetStructureSize(userdefinedtypelist, &str), str.fieldcount, str.ref_id);
    if (str.compoundfield != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG,
                "\t                Item\t            type\tpointer\tsize\tcount\toffset\toffpad\talignment\n");
        for (int i = 0; i < str.fieldcount; i++) {
            udaPrintCompoundFieldTable(str.compoundfield[i]);
        }
    }
}

/** Print the Tabulated Contents of a USERDEFINEDTYPE data structure with Zero Sized elements.
 *
 * @param fd A File Descriptor.
 * @param str A USERDEFINEDTYPE data structure instance.
 * @return Void.
 */
void udaPrintZeroSizedUserDefinedTypeTable(USERDEFINEDTYPE str)
{
    int size1 = 0, size2 = 0;
    UDA_LOG(UDA_LOG_DEBUG, "USERDEFINEDTYPE name: %s size: %d fieldcount %d\n", str.name, str.size, str.fieldcount);
    if (str.compoundfield != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG,
                "\t                Item\t            type\tpointer\tsize\tcount\toffset\toffpad\talignment\n");
        for (int i = 0; i < str.fieldcount; i++) {
            if (str.compoundfield[i].size > 0) {
                continue;
            }
            udaPrintCompoundFieldTable(str.compoundfield[i]);
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
    UDA_LOG(UDA_LOG_DEBUG, "[%d][%d]\n", size1, size2);
}

/** Print the Contents of a USERDEFINEDTYPELIST data structure.
 *
 * @param fd A File Descriptor.
 * @param str A USERDEFINEDTYPELIST data structure instance.
 * @return Void.
 */
void udaPrintUserDefinedTypeList(USERDEFINEDTYPELIST str)
{
    UDA_LOG(UDA_LOG_DEBUG, "USERDEFINEDTYPELIST Contents\n");
    UDA_LOG(UDA_LOG_DEBUG, "listCount  : %d\n", str.listCount);
    for (int i = 0; i < str.listCount; i++) {
        udaPrintUserDefinedType(str.userdefinedtype[i]);
    }
    UDA_LOG(UDA_LOG_DEBUG, "\n");
}

/** Print the Tabulated Contents of a USERDEFINEDTYPELIST data structure.
 *
 * @param fd A File Descriptor.
 * @param str A USERDEFINEDTYPELIST data structure instance.
 * @return Void.
 */
void printUserDefinedTypeListTable(USERDEFINEDTYPELIST str)
{
    UDA_LOG(UDA_LOG_DEBUG, "USERDEFINEDTYPELIST Contents\n");
    UDA_LOG(UDA_LOG_DEBUG, "listCount  : %d\n", str.listCount);
    for (int i = 0; i < str.listCount; i++) {
        udaPrintUserDefinedTypeTable(&str, str.userdefinedtype[i]);
    }
    UDA_LOG(UDA_LOG_DEBUG, "\n\n");
}

/** Print the Tabulated Contents of a USERDEFINEDTYPELIST data structure where the size is zero.
 *
 * @param fd A File Descriptor.
 * @param str A USERDEFINEDTYPELIST data structure instance.
 * @return Void.
 */
void udaPrintZeroSizedUserDefinedTypeListTable(USERDEFINEDTYPELIST str)
{
    UDA_LOG(UDA_LOG_DEBUG, "Zero Size USERDEFINEDTYPELIST Contents\n");
    UDA_LOG(UDA_LOG_DEBUG, "listCount  : %d\n", str.listCount);
    for (int i = 0; i < str.listCount; i++) {
        udaPrintZeroSizedUserDefinedTypeTable(str.userdefinedtype[i]);
    }
    UDA_LOG(UDA_LOG_DEBUG, "\n\n");
}

/** Print the Contents of a LOGMALLOC data structure.
 *
 * @param fd A File Descriptor.
 * @param str A LOGMALLOC data structure instance.
 * @return Void.
 */
void udaPrintMallocLog(LOGMALLOC str)
{
    UDA_LOG(UDA_LOG_DEBUG, "%p\t%d\t%d\t%d\t%s\n", (void*)str.heap, str.count, str.size, str.freed, str.type);
    if (str.rank > 1 && str.shape != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "\trank %d shape [%d", str.rank, str.shape[0]);
        for (int i = 1; i < str.rank; i++) {
            UDA_LOG(UDA_LOG_DEBUG, ",%d", str.shape[i]);
        }
        UDA_LOG(UDA_LOG_DEBUG, "]\n");
    }
}

/** Print the Contents of the Global LOGMALLOCLIST data structure.
 *
 * @param fd A File Descriptor.
 * @return Void.
 */
void udaPrintMallocLogList(const LOGMALLOCLIST* logmalloclist)
{
    UDA_LOG(UDA_LOG_DEBUG, "MALLOC LOG List Contents\n");
    UDA_LOG(UDA_LOG_DEBUG, "listCount  : %d\n", logmalloclist->listcount);
    UDA_LOG(UDA_LOG_DEBUG, "Address\t\tCount\tSize\tFreed\tType\n");
    for (int i = 0; i < logmalloclist->listcount; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "[%3d]  ", i);
        udaPrintMallocLog(logmalloclist->logmalloc[i]);
    }
    UDA_LOG(UDA_LOG_DEBUG, "\n\n");
}

//==============================================================================================================
// Utility Functions

/** Add a stack memory location to the LOGMALLOCLIST data structure. These are not freed.
 *
 * @param stack The memory location.
 * @param count The number of elements allocated.
 * @param size The size of a single element.
 * @param type The name of the type allocated.
 * @return void.
 */
void udaAddNonMalloc(LOGMALLOCLIST* logmalloclist, void* stack, int count, size_t size, const char* type)
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
void udaAddNonMalloc2(LOGMALLOCLIST* logmalloclist, void* stack, int count, size_t size, const char* type, int rank,
                      int* shape)
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
void udaAddMalloc(LOGMALLOCLIST* logmalloclist, void* heap, int count, size_t size, const char* type)
{
    // Log all Heap allocations for Data from User Defined Structures
    // Grow the list when necessary

    // USERDEFINEDTYPE *udt;

    if (heap == nullptr) {
        return;
    }

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
void udaAddMalloc2(LOGMALLOCLIST* logmalloclist, void* heap, int count, size_t size, const char* type, int rank,
                   int* shape)
{
    // Log all Heap allocations for Data from User Defined Structures
    // Grow the list when necessary

    if (heap == nullptr) {
        return;
    }

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
void udaChangeMalloc(LOGMALLOCLIST* logmalloclist, VOIDTYPE old, void* anew, int count, size_t size, const char* type)
{
    // Change a List Entry
    if (old == 0) {
        udaAddMalloc(logmalloclist, anew, count, size, type);
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
void udaChangeNonMalloc(LOGMALLOCLIST* logmalloclist, void* old, void* anew, int count, size_t size, const char* type)
{
    // Change a non-malloc List Entry

    VOIDTYPE target, candidate;
    if (old == nullptr) {
        udaAddNonMalloc(logmalloclist, anew, count, size, type);
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

int udaDupCountMallocLog(LOGMALLOCLIST* str)
{
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
void udaFreeMallocLog(LOGMALLOCLIST* str)
{
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

/** Free allocated heap memory and reinitialise a new logmalloclist-> There are no arguments.
 *
 * @return void.
 */
void udaFreeMallocLogList(LOGMALLOCLIST* str)
{
    if (str == nullptr) {
        return;
    }
    udaFreeMallocLog(str);
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
void udaFindMalloc(LOGMALLOCLIST* logmalloclist, void* heap, int* count, int* size, const char** type)
{
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

    if (last_malloc_index >= (unsigned int)logmalloclist->listcount) { // Defensive check
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
void udaFindMalloc2(LOGMALLOCLIST* logmalloclist, void* heap, int* count, int* size, const char** type, int* rank,
                    int** shape)
{
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

    if (last_malloc_index >= (unsigned int)logmalloclist->listcount) { // Defensive check
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
            last_malloc_index = i; // Start at the current log entry
            *last_malloc_index_value = last_malloc_index;
            return;
        }
    }

    for (unsigned int i = 0; i < last_malloc_index; i++) { // Start search at the first log entry
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
void udaAddStruct(void* heap, const char* type, LOGSTRUCTLIST* log_struct_list)
{
    // Log all dispatched/received Structures
    // Grow the list when necessary

    if (heap == nullptr) {
        return;
    }

    if (log_struct_list->listcount + 1 >= log_struct_list->listsize) {
        log_struct_list->logstruct = (LOGSTRUCT*)realloc(
            (void*)log_struct_list->logstruct, (log_struct_list->listsize + GROWMALLOCLIST) * sizeof(LOGSTRUCT));
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
void udaFreeLogStructList(LOGSTRUCTLIST* log_struct_list)
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
int udaFindStructId(void* heap, char** type, LOGSTRUCTLIST* log_struct_list)
{
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
void* udaFindStructHeap(int id, char** type, LOGSTRUCTLIST* log_struct_list)
{
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
void udaCopyUserDefinedType(USERDEFINEDTYPE* old, USERDEFINEDTYPE* anew)
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

/** Copy the Master User Defined Structure Definition List.
 *
 * @param anew The copy of the type definition list.
 * @return void.
 */
#if defined(SERVERBUILD)
void udaCopyUserDefinedTypeList(USERDEFINEDTYPELIST** anew, const USERDEFINEDTYPELIST* parseduserdefinedtypelist)
{
    USERDEFINEDTYPELIST* list = (USERDEFINEDTYPELIST*)malloc(sizeof(USERDEFINEDTYPELIST));
    initUserDefinedTypeList(list);
    list->listCount = parseduserdefinedtypelist->listCount; // Copy the standard set of structure definitions
    list->userdefinedtype = (USERDEFINEDTYPE*)malloc(parseduserdefinedtypelist->listCount * sizeof(USERDEFINEDTYPE));

    for (int i = 0; i < list->listCount; i++) {
        USERDEFINEDTYPE usertypeOld = parseduserdefinedtypelist->userdefinedtype[i];
        USERDEFINEDTYPE usertypeNew;
        initUserDefinedType(&usertypeNew);
        usertypeNew = usertypeOld;
        usertypeNew.image =
            (char*)malloc(usertypeOld.imagecount * sizeof(char)); // Copy pointer type (prevents double free)
        memcpy(usertypeNew.image, usertypeOld.image, usertypeOld.imagecount);

        usertypeNew.compoundfield = (COMPOUNDFIELD*)malloc(usertypeOld.fieldcount * sizeof(COMPOUNDFIELD));

        for (int j = 0; j < usertypeOld.fieldcount; j++) {
            initCompoundField(&usertypeNew.compoundfield[j]);
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

void udaCopyUserDefinedTypeList(USERDEFINEDTYPELIST** anew, const USERDEFINEDTYPELIST* parseduserdefinedtypelist)
{
    UDA_LOG(UDA_LOG_DEBUG, "Not SERVERBUILD - USERDEFINEDTYPELIST is not allocated\n");
}

#endif

/** Create the Initial User Defined Structure Definition List.
 *
 * @param anew The initial type definition list.
 * @return void.
 */
void udaGetInitialUserDefinedTypeList(USERDEFINEDTYPELIST** anew)
{
    auto list = (USERDEFINEDTYPELIST*)malloc(sizeof(USERDEFINEDTYPELIST));
    initUserDefinedTypeList(list);

    USERDEFINEDTYPE usertype;
    COMPOUNDFIELD field;

    int offset = 0;

    //----------------------------------------------------------------------------------------------------------------
    // SARRAY

    initUserDefinedType(&usertype); // New structure definition
    initCompoundField(&field);

    strcpy(usertype.name, "SARRAY");
    strcpy(usertype.source, "udaGetInitialUserDefinedTypeList");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(SARRAY); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    offset = 0;

    defineField(&field, "count", "Number of data array elements", &offset, SCALARINT, 0, nullptr);
    udaAddCompoundField(&usertype, field);
    defineField(&field, "rank", "Rank of the data array", &offset, SCALARINT, 0, nullptr);
    udaAddCompoundField(&usertype, field);
    defineField(&field, "shape", "Shape of the data array", &offset, ARRAYINT, 0, nullptr);
    udaAddCompoundField(&usertype, field);
    defineField(&field, "data", "Location of the Structure Array", &offset, ARRAYVOID, 0, nullptr);
    udaAddCompoundField(&usertype, field);

    initCompoundField(&field);
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
    udaAddCompoundField(&usertype, field);

    udaAddUserDefinedType(list, usertype);

    //----------------------------------------------------------------------------------------------------------------
    // ENUMMEMBER

    initUserDefinedType(&usertype); // New structure definition
    strcpy(usertype.name, "ENUMMEMBER");
    strcpy(usertype.source, "ENUMMEMBER structure: for labels and values");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(ENUMMEMBER); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    offset = 0;

    initCompoundField(&field);
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
    field.offset = offsetof(ENUMMEMBER, name);
    offset = field.offset + field.size;
    field.offpad = udaPadding(offset, field.type);
    field.alignment = udaGetalignmentof(field.type);
    udaAddCompoundField(&usertype, field);

    initCompoundField(&field);
    defineField(&field, "value", "The ENUM value", &offset, SCALARLONG64, 0, nullptr);
    udaAddCompoundField(&usertype, field);

    udaAddUserDefinedType(list, usertype);

    //----------------------------------------------------------------------------------------------------------------
    // ENUMLIST

    initUserDefinedType(&usertype); // New structure definition
    strcpy(usertype.name, "ENUMLIST");
    strcpy(usertype.source, "Array of ENUM values with properties");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(ENUMLIST); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    offset = 0;

    initCompoundField(&field);
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
    field.offset = offsetof(ENUMLIST, name);
    offset = field.offset + field.size;
    field.offpad = udaPadding(offset, field.type);
    field.alignment = udaGetalignmentof(field.type);
    udaAddCompoundField(&usertype, field);

    initCompoundField(&field);
    defineField(&field, "type", "The ENUM base integer atomic type", &offset, SCALARINT, 0, nullptr);
    udaAddCompoundField(&usertype, field);

    initCompoundField(&field);
    defineField(&field, "count", "The number of ENUM values", &offset, SCALARINT, 0, nullptr);
    udaAddCompoundField(&usertype, field);

    initCompoundField(&field);
    strcpy(field.name, "enummember");
    field.atomictype = UDA_TYPE_UNKNOWN;
    strcpy(field.type, "ENUMMEMBER");
    strcpy(field.desc, "The ENUM list members: labels and value");
    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = nullptr;
    field.size = sizeof(ENUMMEMBER*);
    field.offset = offsetof(ENUMLIST, enummember); // Different to newoffset
    offset = field.offset + field.size;
    field.offpad = udaPadding(offset, field.type);
    field.alignment = udaGetalignmentof(field.type);
    udaAddCompoundField(&usertype, field);

    // defineField(&field, "data", "Generalised data pointer for all integer type arrays", &offset, ARRAYVOID);
    // ARRAYVOID doesn't work - not implemented in the middleware!
    // Naming the field "data" hits a bug and garbage is returned!
    // Don't know the correct type until the structure is used!!!! - so cannot pre-define!
    // Make the necessary changes to the structure definition when ENUMLIST is used or
    // Convert data to standard unsigned long64

    initCompoundField(&field);
    defineField(&field, "enumarray", "Data with this enumerated type", &offset, ARRAYULONG64, 0,
                nullptr); // Data need to be converted to this type
    udaAddCompoundField(&usertype, field);
    initCompoundField(&field);
    defineField(&field, "enumarray_rank", "The rank of arraydata", &offset, SCALARINT, 0, nullptr);
    udaAddCompoundField(&usertype, field);
    initCompoundField(&field);
    defineField(&field, "enumarray_count", "The count of arraydata", &offset, SCALARINT, 0, nullptr);
    udaAddCompoundField(&usertype, field);
    initCompoundField(&field);
    defineField(&field, "enumarray_shape", "The shape of arraydata", &offset, ARRAYINT, 0, nullptr);
    udaAddCompoundField(&usertype, field);

    udaAddUserDefinedType(list, usertype);

    *anew = list;
}

/** Add a Compound Field type to a structure definition.
 *
 * @param str The structure definition.
 * @param field The Compound field type.
 * @return void.
 */
void udaAddCompoundField(USERDEFINEDTYPE* str, COMPOUNDFIELD field)
{
    str->compoundfield =
        (COMPOUNDFIELD*)realloc((void*)str->compoundfield, (str->fieldcount + 1) * sizeof(COMPOUNDFIELD));
    initCompoundField(&str->compoundfield[str->fieldcount]);
    str->compoundfield[str->fieldcount++] = field;
}

/** Add a structure definition to the List of structure types
 *
 * @param str The list of structure definitions.
 * @param type The new definition to add to the list.
 * @return void.
 */
void udaAddUserDefinedType(USERDEFINEDTYPELIST* str, USERDEFINEDTYPE type)
{
    str->userdefinedtype =
        (USERDEFINEDTYPE*)realloc((void*)str->userdefinedtype, (str->listCount + 1) * sizeof(USERDEFINEDTYPE));
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
void udaUpdateUserDefinedType(USERDEFINEDTYPELIST* str, int typeId, USERDEFINEDTYPE type)
{
    str->userdefinedtype[typeId] = type; // replace existing entry
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
void udaChangeUserDefinedTypeElementProperty(USERDEFINEDTYPELIST* str, int typeId, char* element, char* property,
                                             void* value)
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
int udaCountUserDefinedType(USERDEFINEDTYPELIST* str)
{
    return str->listCount; // Number of user defined types
}

/** Free heap from a Compound Field.
 *
 * @param str The Compound Field.
 * @return void.
 */
void udaFreeCompoundField(COMPOUNDFIELD* str)
{
    if (str == nullptr) {
        return;
    }
    free(str->shape);
    str->shape = nullptr;
}

/** Free heap from a User Defined Type.
 *
 * @param type The User Defined Type.
 * @return void.
 */
void udaFreeUserDefinedType(USERDEFINEDTYPE* type)
{
    if (type == nullptr) {
        return;
    }
    for (int i = 0; i < type->fieldcount; i++) {
        udaFreeCompoundField(&type->compoundfield[i]);
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
void udaFreeUserDefinedTypeList(USERDEFINEDTYPELIST* userdefinedtypelist)
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
        udaFreeUserDefinedType(&userdefinedtypelist->userdefinedtype[i]);
    }
    free(userdefinedtypelist->userdefinedtype);
    initUserDefinedTypeList(userdefinedtypelist);
}

/** The size or byte count of an atomic or structured type
 *
 * @param type The name of the type
 * @return The size in bytes.
 */
size_t udaGetsizeof(USERDEFINEDTYPELIST* userdefinedtypelist, const char* type)
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

    if ((udt = udaFindUserDefinedType(userdefinedtypelist, base, 0)) != nullptr) {
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
size_t udaGetStructureSize(USERDEFINEDTYPELIST* userdefinedtypelist, USERDEFINEDTYPE* str)
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
            alignment = udaGetalignmentof("*");
        } else {
            size = udaGetsizeof(userdefinedtypelist, str->compoundfield[i].type);
            alignment = udaGetalignmentof(str->compoundfield[i].type);
        }

        size_t space = size * str->compoundfield[i].count;

        if (i != 0) {
            if (str->compoundfield[i].pointer) {
                offset = udaNewoffset(offset + space0, "*");
            } else {
                offset = udaNewoffset(offset + space0, str->compoundfield[i].type);
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
        UDA_LOG(UDA_LOG_DEBUG, "WARNING: line %d, file %s\n%s\n", line, file, msg);
    } else {
        UDA_LOG(UDA_LOG_ERROR, "ERROR: line %d, file %s\n%s\n", line, file, msg);
    }
}

//==============================================================================================================
// Functions to Send or Receive Data contained in User Defined Structures

// Send or Receive the Data Structure

// Recursive Send/Receive Individual User Defined Structure Elements
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

int xdrAtomicData(LOGMALLOCLIST* logmalloclist, XDR* xdrs, const char* type, int count, int size, char** data)
{
    int type_id = udaGettypeof(type);
    char* d;
    if (xdrs->x_op == XDR_DECODE) {
        d = (char*)malloc(count * size);
        udaAddMalloc(logmalloclist, (void*)d, count, size, type);
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
                    fprintf(stdout, "%f ", *((float*)d + rc));
                }
                break;
            case (UDA_TYPE_DOUBLE):
                for (rc = 0; rc < count; rc++) {
                    fprintf(stdout, "%f ", *((double*)d + rc));
                }
                break;
            case (UDA_TYPE_COMPLEX):
                for (rc = 0; rc < 2 * count; rc++) {
                    fprintf(stdout, "%f ", *((float*)d + rc));
                }
                break;
            case (UDA_TYPE_INT):
                for (rc = 0; rc < count; rc++) {
                    fprintf(stdout, "%d ", *((int*)d + rc));
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
                    fprintf(stdout, "%f ", *((float*)d + rc));
                }
                break;
            case (UDA_TYPE_DOUBLE):
                for (rc = 0; rc < count; rc++) {
                    fprintf(stdout, "%f ", *((double*)d + rc));
                }
                break;
            case (UDA_TYPE_COMPLEX):
                for (rc = 0; rc < 2 * count; rc++) {
                    fprintf(stdout, "%f ", *((float*)d + rc));
                }
                break;
            case (UDA_TYPE_INT):
                for (rc = 0; rc < count; rc++) {
                    fprintf(stdout, "%d ", *((int*)d + rc));
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

int uda::client_server::xdr_user_defined_type_data(XDR* xdrs, LOGMALLOCLIST* logmalloclist,
                                               USERDEFINEDTYPELIST* userdefinedtypelist,
                                               USERDEFINEDTYPE* userdefinedtype, void** data, int protocolVersion,
                                               bool xdr_stdio_flag, LOGSTRUCTLIST* log_struct_list, int malloc_source)
{
    int rc;

    initLogStructList(log_struct_list); // Initialise Linked List Structure Log

    if (xdrs->x_op == XDR_DECODE) {

        NTREE* dataNTree = nullptr;

        if (!xdr_stdio_flag) {
            rc = xdrrec_skiprecord(xdrs); // Receiving
        } else {
            rc = 1;
        }

        rc = rc && xdr_user_defined_type(xdrs, userdefinedtypelist, userdefinedtype); // User Defined Type Definitions
        rc = rc &&
             xdrUserDefinedData(xdrs, logmalloclist, log_struct_list, userdefinedtypelist, userdefinedtype, data, 1, 0,
                                nullptr, 0, &dataNTree, protocolVersion, malloc_source); // Data within Structures

        udaSetFullNTree(dataNTree); // Copy to Global
    } else {

        if (userdefinedtype == nullptr) {
            add_error(UDA_CODE_ERROR_TYPE, "udaXDRUserDefinedTypeData", 999,
                      "No User Defined Type passed - cannot send!");
            return 0;
        }

        rc = xdr_user_defined_type(xdrs, userdefinedtypelist, userdefinedtype); // User Defined Type Definitions
        rc = rc &&
             xdrUserDefinedData(xdrs, logmalloclist, log_struct_list, userdefinedtypelist, userdefinedtype, data, 1, 0,
                                nullptr, 0, nullptr, protocolVersion, malloc_source); // Data within Structures

        if (!xdr_stdio_flag) {
            rc = rc && xdrrec_endofrecord(xdrs, 1);
        }
    }

    udaFreeLogStructList(log_struct_list); // Free Linked List Structure Log heap

    return rc;
}

int udaFindUserDefinedTypeId(USERDEFINEDTYPELIST* userdefinedtypelist, const char* name)
{

    // Return the List Index key for a Named User Defined Structure

    for (int i = 0; i < userdefinedtypelist->listCount; i++) {
        if (!strcmp(userdefinedtypelist->userdefinedtype[i].name, name)) {
            return i;
        }
    }
#ifdef INCLUDESTRUCTPREFIX
    if (!strncmp(name, "struct ", 7)) { // search without the struct prefix
        for (i = 0; i < userdefinedtypelist->listCount; i++) {
            if (!strcmp(userdefinedtypelist->userdefinedtype[i].name, &name[7])) {
                return (i);
            }
        }
    } else {
        char work[MAXELEMENTNAME + 25] = "struct "; // search with the struct prefix
        strcat(work, name);
        for (i = 0; i < userdefinedtypelist->listCount; i++) {
            if (!strcmp(userdefinedtypelist->userdefinedtype[i].name, work)) {
                return (i);
            }
        }
    }
#endif
    return -1;
}

USERDEFINEDTYPE* udaFindUserDefinedType(USERDEFINEDTYPELIST* userdefinedtypelist, const char* name, int ref_id)
{
    // Return the Structure Definition of a Named User Defined Structure

    UDA_LOG(UDA_LOG_DEBUG, "udaFindUserDefinedType: [%s]\n", name);
    UDA_LOG(UDA_LOG_DEBUG, "ref_id: %d\n", ref_id);
    UDA_LOG(UDA_LOG_DEBUG, "listCount: %d\n", userdefinedtypelist->listCount);

    if (name == nullptr) {
        return nullptr;
    }

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

int udaTestUserDefinedType(USERDEFINEDTYPELIST* userdefinedtypelist, USERDEFINEDTYPE* udt)
{
    // Test a Structure Definition is a member of the Structure Type List
    // Return True (1) if the structure definition is found, False (0) otherwise.

    USERDEFINEDTYPE* test;

    if (udt == nullptr) {
        return 0;
    }
    for (int i = 0; i < userdefinedtypelist->listCount; i++) {
        test = &userdefinedtypelist->userdefinedtype[i];
        if (test == udt) {
            return 1;
        }
    }
    return 0;
}

//==============================================================================================================
// Functions to Send or Receive Definitions of User Defined Structure

bool_t xdr_compoundfield(XDR* xdrs, COMPOUNDFIELD* str)
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

bool_t uda::client_server::xdr_user_defined_type(XDR* xdrs, USERDEFINEDTYPELIST* userdefinedtypelist,
                                               USERDEFINEDTYPE* str)
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

    udaPrintUserDefinedType(*str);

    if (xdrs->x_op == XDR_DECODE) { // Receiving an array so allocate Heap for it then initialise
        if (str->imagecount > 0) {
            str->image = (char*)malloc(str->imagecount * sizeof(char));
        } else {
            str->image = nullptr;
        }
        if (str->fieldcount > 0) {
            str->compoundfield = (COMPOUNDFIELD*)malloc(str->fieldcount * sizeof(COMPOUNDFIELD));
            for (int i = 0; i < str->fieldcount; i++) {
                initCompoundField(&str->compoundfield[i]);
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
                size = udaGetsizeof(userdefinedtypelist, str->compoundfield[i].type);
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
                size = udaGetsizeof(userdefinedtypelist, str->compoundfield[i].type);
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

        udaPrintUserDefinedType(*str);
    }

    return rc;
}

bool_t uda::client_server::xdr_user_defined_type_list(XDR* xdrs, USERDEFINEDTYPELIST* str, bool xdr_stdio_flag)
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

    UDA_LOG(UDA_LOG_DEBUG, "xdr_userdefinedtypelist: rc = %d, listCount = %d\n", rc, str->listCount);

    if (!rc || str->listCount == 0) {
        return rc;
    }

    if (xdrs->x_op == XDR_DECODE) { // Receiving array so allocate Heap for it then initialise
        str->userdefinedtype = (USERDEFINEDTYPE*)malloc(str->listCount * sizeof(USERDEFINEDTYPE));
        for (int i = 0; i < str->listCount; i++) {
            initUserDefinedType(&str->userdefinedtype[i]);
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
        UDA_LOG(UDA_LOG_DEBUG, "%40s: null\n", label);
        return;
    }
    switch (atomictype) {
        case UDA_TYPE_FLOAT: {
            float* d = (float*)data;
            if (count > 1) {
                UDA_LOG(UDA_LOG_DEBUG, "%40s:\n", label);
                for (int i = 0; i < count; i++) {
                    UDA_LOG(UDA_LOG_DEBUG, "[%d]   %f\n", i, d[i]);
                }
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "%40s: %f\n", label, d[0]);
            }
            return;
        }
        case UDA_TYPE_DOUBLE: {
            double* d = (double*)data;
            if (count > 1) {
                UDA_LOG(UDA_LOG_DEBUG, "%40s:\n", label);
                for (int i = 0; i < count; i++) {
                    UDA_LOG(UDA_LOG_DEBUG, "[%d]   %f\n", i, d[i]);
                }
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "%40s: %f\n", label, d[0]);
            }
            return;
        }
        case UDA_TYPE_SHORT: {
            short* d = (short*)data;
            if (count > 1) {
                UDA_LOG(UDA_LOG_DEBUG, "%40s:\n", label);
                for (int i = 0; i < count; i++) {
                    UDA_LOG(UDA_LOG_DEBUG, "[%d]   %d\n", i, d[i]);
                }
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "%40s: %d\n", label, d[0]);
            }
            return;
        }
        case UDA_TYPE_INT:
        case UDA_TYPE_LONG: {
            int* d = (int*)data;
            if (count > 1) {
                UDA_LOG(UDA_LOG_DEBUG, "%40s:\n", label);
                for (int i = 0; i < count; i++) {
                    UDA_LOG(UDA_LOG_DEBUG, "[%d]   %d\n", i, d[i]);
                }
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "%40s: %d\n", label, d[0]);
            }
            return;
        }
        case UDA_TYPE_LONG64: {
            long long* d = (long long*)data;
            if (count > 1) {
                UDA_LOG(UDA_LOG_DEBUG, "%40s:\n", label);
                for (int i = 0; i < count; i++) {
                    UDA_LOG(UDA_LOG_DEBUG, "[%d]   %lld\n", i, d[i]);
                }
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "%40s: %lld\n", label, d[0]);
            }
            return;
        }
        case UDA_TYPE_UNSIGNED_CHAR: {
            unsigned char* d = (unsigned char*)data;
            if (count > 1) {
                UDA_LOG(UDA_LOG_DEBUG, "%40s:\n", label);
                for (int i = 0; i < count; i++) {
                    UDA_LOG(UDA_LOG_DEBUG, "[%d]   %u\n", i, d[i]);
                }
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "%40s: %d\n", label, d[0]);
            }
            return;
        }
        case UDA_TYPE_UNSIGNED_SHORT: {
            unsigned short* d = (unsigned short*)data;
            if (count > 1) {
                UDA_LOG(UDA_LOG_DEBUG, "%40s:\n", label);
                for (int i = 0; i < count; i++) {
                    UDA_LOG(UDA_LOG_DEBUG, "[%d]   %u\n", i, d[i]);
                }
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "%40s: %d\n", label, d[0]);
            }
            return;
        }
        case UDA_TYPE_UNSIGNED_INT:
        case UDA_TYPE_UNSIGNED_LONG: {
            unsigned int* d = (unsigned int*)data;
            if (count > 1) {
                UDA_LOG(UDA_LOG_DEBUG, "%40s:\n", label);
                for (int i = 0; i < count; i++) {
                    UDA_LOG(UDA_LOG_DEBUG, "[%d]   %u\n", i, d[i]);
                }
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "%40s: %d\n", label, d[0]);
            }
            return;
        }
        case UDA_TYPE_UNSIGNED_LONG64: {
            unsigned long long* d = (unsigned long long*)data;
            if (count > 1) {
                UDA_LOG(UDA_LOG_DEBUG, "%40s:\n", label);
                for (int i = 0; i < count; i++) {
                    UDA_LOG(UDA_LOG_DEBUG, "[%d]   %llu\n", i, d[i]);
                }
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "%40s: %llu\n", label, d[0]);
            }
            return;
        }
        case UDA_TYPE_STRING: {
            char* d = (char*)data;
            UDA_LOG(UDA_LOG_DEBUG, "%40s: %s\n", label, d);
            return;
        }
        case UDA_TYPE_CHAR: {
            char* d = (char*)data;
            UDA_LOG(UDA_LOG_DEBUG, "%40s: %s\n", label, d);
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
void udaPrintAtomicType(LOGMALLOCLIST* logmalloclist, NTREE* tree, const char* target)
{
    USERDEFINEDTYPE* userdefinedtype = tree->userdefinedtype;
    char* p;
    void* data;
    int fieldcount = tree->userdefinedtype->fieldcount;
    for (int i = 0; i < fieldcount; i++) {
        if (!strcmp(userdefinedtype->compoundfield[i].name, target)) {
            if (userdefinedtype->compoundfield[i].atomictype != UDA_TYPE_UNKNOWN) {
                p = (char*)tree->data;
                if (userdefinedtype->compoundfield[i].pointer ||
                    !strcmp(userdefinedtype->compoundfield[i].type, "STRING *")) { // Strings are an exception!
                    int count, size;
                    const char* type;
                    data = (void*)*((VOIDTYPE*)&p[userdefinedtype->compoundfield[i].offset]);
                    if (data == nullptr) {
                        UDA_LOG(UDA_LOG_DEBUG, "%40s: null\n", target);
                        return;
                    }
                    udaFindMalloc(logmalloclist, &data, &count, &size, &type);
                    if (count > 0) {
                        udaPrintAtomicData(data, udaGettypeof(type), count, target);
                    }
                } else {
                    data = (void*)&p[userdefinedtype->compoundfield[i].offset];
                    udaPrintAtomicData(data, userdefinedtype->compoundfield[i].atomictype,
                                       userdefinedtype->compoundfield[i].count, target);
                }
            } else {
                UDA_LOG(UDA_LOG_ERROR, "ERROR: %s is Not of Atomic Type\n", target);
            }
            return;
        }
    }
    UDA_LOG(UDA_LOG_ERROR, "ERROR: %s is Not located in the current Tree Node\n", target);
}

/** Print the Count of elements of a named data array from a given tree node to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param ntree A pointer to a tree node.
 * @param target The name of a Structure element.
 * @return Void
 */
void udaPrintTypeCount(NTREE* ntree, const char* target)
{
    USERDEFINEDTYPE* userdefinedtype = ntree->userdefinedtype;
    int fieldcount = ntree->userdefinedtype->fieldcount;
    for (int i = 0; i < fieldcount; i++) {
        if (!strcmp(userdefinedtype->compoundfield[i].name, target)) {
            udaPrintCompoundField(userdefinedtype->compoundfield[i]);
            UDA_LOG(UDA_LOG_DEBUG, "%s[ %d ]\n", target, userdefinedtype->compoundfield[i].count);
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
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @param target The name of a Structure Element.
 * @return the Structure Element Definition Structure.
 */
COMPOUNDFIELD* getNodeStructureComponent(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target)
{
    USERDEFINEDTYPE* userdefinedtype;
    int fieldcount;
    const char* lastname;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }

    // Locate the Node with a Structure Component
    ntree = udaFindNTreeStructureComponent2(logmalloclist, ntree, target, &lastname);

    if (ntree != nullptr) {
        userdefinedtype = ntree->userdefinedtype;
        fieldcount = ntree->userdefinedtype->fieldcount;
        for (int i = 0; i < fieldcount; i++) {
            if (!strcmp(userdefinedtype->compoundfield[i].name, target)) {
                return &userdefinedtype->compoundfield[i];
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
void udaPrintNode(NTREE* tree)
{
    if (tree == nullptr) {
        tree = full_ntree;
    }
    UDA_LOG(UDA_LOG_DEBUG, "NTREE Node Contents\n");
    UDA_LOG(UDA_LOG_DEBUG, "Name    : %s \n", tree->name);
    UDA_LOG(UDA_LOG_DEBUG, "Branches: %d \n", tree->branches);
#ifdef A64
    UDA_LOG(UDA_LOG_DEBUG, "Parent  : %p   (%llx) \n", (void*)tree->parent, (UVOIDTYPE)tree->parent);
    for (int i = 0; i < tree->branches; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "Children[%d]: %p   (%llx) \n", i, (void*)tree->children[i],
                (UVOIDTYPE)tree->children[i]);
    }
#else
    UDA_LOG(UDA_LOG_DEBUG, "Parent  : %p   (%x) \n", (void*)tree->parent, (UVOIDTYPE)tree->parent);
    for (int i = 0; i < tree->branches; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "Children[%d]: %p   (%x) \n", i, (void*)tree->children[i], (UVOIDTYPE)tree->children[i]);
    }
#endif
    udaPrintUserDefinedType(*tree->userdefinedtype);
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
    NTREE* ntree = nullptr;
    if (target[0] != '\0') {
        if ((ntree = udaFindNTreeStructureDefinition(ntree, target)) == nullptr) {
            UDA_LOG(UDA_LOG_DEBUG, "the Structure Definition for %s could not be Found\n", target);
            return;
        }
    }
    udaPrintNode(ntree);
}

/** Print an Image of the Named Structure Definition to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param target The name of a User Defined Structure type.
 * @return Void
 */

void udaPrintNodeStructureImage(const char* target)
{
    NTREE* ntree = nullptr;
    if (target[0] != '\0') {
        if ((ntree = udaFindNTreeStructureDefinition(ntree, target)) == nullptr) {
            UDA_LOG(UDA_LOG_DEBUG, "the Structure Definition for %s could not be Found\n", target);
            return;
        }
        printImage(ntree->userdefinedtype->image, ntree->userdefinedtype->imagecount);
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "no Structure Definition name was given!\n");
    }
}

/** Return a Pointer to the User Defined Type Structure of the data attached to this tree node.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the User Defined Type Structure Definition.
 */

USERDEFINEDTYPE* udaGetNodeUserDefinedType(NTREE* ntree)
{
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    return ntree->userdefinedtype;
}

/** Return the name of the User Defined Type Structure.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the name of the User Defined Type Structure.
 */
char* udaGetNodeStructureName(NTREE* ntree)
{
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    return ntree->name;
}

/** Return the Type of the User Defined Type Structure.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the Type of the User Defined Type Structure.
 */
char* udaGetNodeStructureType(NTREE* ntree)
{
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    return ntree->userdefinedtype->name;
}

/** Return the Size of the User Defined Type Structure.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the Size (Bytes) of the User Defined Type Structure.
 */
int udaGetNodeStructureSize(NTREE* ntree)
{
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    return ntree->userdefinedtype->size;
}

/** Return a pointer to a Tree Nodes's Data Structure Array element.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @param index The array index
 * @return a Pointer to a Structure Array element.
 */
void* udaGetNodeStructureArrayData(LOGMALLOCLIST* logmalloclist, NTREE* ntree, int index)
{
    char* p;
    if (index < 0) {
        add_error(UDA_CODE_ERROR_TYPE, "udaGetNodeStructureArrayData", 999, "The Tree Node array index < 0");
        return nullptr;
    }
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    if (udaGetNodeStructureDataCount(logmalloclist, ntree) < (index + 1)) {
        add_error(UDA_CODE_ERROR_TYPE, "udaGetNodeStructureArrayData", 999,
                  "The Tree Node array index > allocated array dimension");
        return nullptr;
    }
    p = (char*)ntree->data;
    return (void*)&p[index * ntree->userdefinedtype->size];
}

/** Return a pointer to a Component Data Structure Array element.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @param target The name of the Structure Array element.
 * @param structureindex The Array index
 * @param componentindex The structure element index
 * @return a Pointer to a Component Structure Array element.
 */
void* udaGetNodeStructureComponentArrayData(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target,
                                            int structureindex, int componentindex)
{
    int offset, count, size;
    char* p;
    char* pp;
    const char* type;
    if (structureindex < 0) {
        add_error(UDA_CODE_ERROR_TYPE, "udaGetNodeStructureComponentArrayData", 999,
                  "The Tree Node Structure array index < 0");
    }
    if (componentindex < 0) {
        add_error(UDA_CODE_ERROR_TYPE, "udaGetNodeStructureComponentArrayData", 999,
                  "The Tree Node Structure Component array index < 0");
        return nullptr;
    }
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }

    if ((pp = (char*)udaGetNodeStructureArrayData(logmalloclist, ntree, structureindex)) == nullptr) {
        return nullptr;
    }

    for (int i = 0; i < ntree->userdefinedtype->fieldcount; i++) {
        if (!strcmp(ntree->userdefinedtype->compoundfield[i].name, target)) {
            offset = ntree->userdefinedtype->compoundfield[i].offset;
            if (ntree->userdefinedtype->compoundfield[i].pointer) {
                p = (char*)*((VOIDTYPE*)&pp[offset]); // Data Element from the single Structure Array Element
                udaFindMalloc(logmalloclist, p, &count, &size, &type);
            } else {
                p = &pp[offset];
                size = ntree->userdefinedtype->compoundfield[i].size;
                count = ntree->userdefinedtype->compoundfield[i].count;
            }
            if (size == 0) {
                return nullptr;
            }
            if (count <= componentindex) {
                add_error(UDA_CODE_ERROR_TYPE, "udaGetNodeStructureComponentArrayData", 999,
                          "The Tree Node Structure Component array index > allocated array dimension");
                return nullptr;
            }
            return (void*)&p[componentindex * size];
        }
    }
    add_error(UDA_CODE_ERROR_TYPE, "udaGetNodeStructureComponentArrayData", 999,
              "The named Tree Node Structure Component array is not a member of the Data structure");
    return nullptr;
}

/** Return the count of child User Defined Type Structures (elements of this structure).
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the name of the User Defined Type Structure.
 */
int udaGetNodeChildrenCount(NTREE* ntree)
{
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    return ntree->branches;
}

/** Return a Child Node.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @param child A integer index identifying which child from the child array to return
 * @return the Child Node.
 */
NTREE* udaGetNodeChild(NTREE* ntree, int child)
{
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    if (child < 0 || child >= ntree->branches) {
        return nullptr;
    }
    return ntree->children[child];
}

/** Return a Child Node'd ID (Branch index value).
 *
 * @param ntree A pointer to a Parent tree node. If nullptr the root node is assumed.
 * @param child A ipointer to a Child tree node.
 * @return the Child Node's ID.
 */
int udaGetNodeChildId(NTREE* ntree, NTREE* child)
{
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    if (child == nullptr) {
        return -1;
    }
    if (ntree->branches == 0) {
        return -1;
    }
    for (int i = 0; i < ntree->branches; i++) {
        if (ntree->children[i] == child) {
            return i;
        }
    }
    return -1;
}

/** Return a Pointer to the children of this tree node.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the Array of children.
 */

NTREE** udaGetNodeChildren(NTREE* ntree)
{
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    return ntree->children;
}

/** Return the parent Node.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the Parent Node.
 */
NTREE* udaGetNodeParent(NTREE* ntree)
{
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    return ntree->parent;
}

/** Return the Data pointer.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return a Pointer to the Data.
 */
void* udaGetNodeData(NTREE* ntree)
{
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    return ntree->data;
}

/** Return a Count of Structured Component Types attached to a tree node.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the Count of Structured types.
 */

int udaGetNodeStructureCount(NTREE* ntree)
{
    int count = 0;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    for (int i = 0; i < ntree->userdefinedtype->fieldcount; i++) {
        if (ntree->userdefinedtype->compoundfield[i].atomictype == UDA_TYPE_UNKNOWN) {
            count++;
        }
    }
    return count;
}

/** Return a Count of Atomic Component Types attached to a tree node.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the Count of Atomic types.
 */

int udaGetNodeAtomicCount(NTREE* ntree)
{
    int count = 0;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    for (int i = 0; i < ntree->userdefinedtype->fieldcount; i++) {
        if (ntree->userdefinedtype->compoundfield[i].atomictype != UDA_TYPE_UNKNOWN) {
            count++;
        }
    }
    return count;
}

/** Return a List of Structure component Names attached to a tree node.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of Structure names.
 */

char** udaGetNodeStructureNames(LOGMALLOCLIST* logmalloclist, NTREE* ntree)
{
    int count;
    char** names;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    if ((count = udaGetNodeStructureCount(ntree)) == 0) {
        return nullptr;
    }
    names = (char**)malloc(count * sizeof(char*));
    udaAddMalloc(logmalloclist, (void*)names, count, sizeof(char*), "char *");
    count = 0;
    for (int i = 0; i < ntree->userdefinedtype->fieldcount; i++) {
        if (ntree->userdefinedtype->compoundfield[i].atomictype == UDA_TYPE_UNKNOWN) {
            names[count++] = ntree->userdefinedtype->compoundfield[i].name;
        }
    }
    return names;
}

/** Return a List of Atomic component Names attached to a tree node.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of Atomic element names.
 */

char** udaGetNodeAtomicNames(LOGMALLOCLIST* logmalloclist, NTREE* ntree)
{
    int count;
    char** names;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    if ((count = udaGetNodeAtomicCount(ntree)) == 0) {
        return nullptr;
    }
    names = (char**)malloc(count * sizeof(char*));
    udaAddMalloc(logmalloclist, (void*)names, count, sizeof(char*), "char *");

    count = 0;
    for (int i = 0; i < ntree->userdefinedtype->fieldcount; i++) {
        if (ntree->userdefinedtype->compoundfield[i].atomictype != UDA_TYPE_UNKNOWN) {
            names[count++] = ntree->userdefinedtype->compoundfield[i].name;
        }
    }
    return names;
}

/** Return a List of Structure Component Type Names attached to a tree node.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of Structure Type names.
 */

char** udaGetNodeStructureTypes(LOGMALLOCLIST* logmalloclist, NTREE* ntree)
{
    int count;
    char** names;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    if ((count = udaGetNodeStructureCount(ntree)) == 0) {
        return nullptr;
    }
    names = (char**)malloc(count * sizeof(char*));
    udaAddMalloc(logmalloclist, (void*)names, count, sizeof(char*), "char *");
    count = 0;
    for (int i = 0; i < ntree->userdefinedtype->fieldcount; i++) {
        if (ntree->userdefinedtype->compoundfield[i].atomictype == UDA_TYPE_UNKNOWN) {
            names[count++] = ntree->userdefinedtype->compoundfield[i].type;
        }
    }
    return names;
}

/** Return a List of Atomic Component Type Names attached to a tree node.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of Atomic Type names.
 */

char** udaGetNodeAtomicTypes(LOGMALLOCLIST* logmalloclist, NTREE* ntree)
{
    int count;
    char** names;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    if ((count = udaGetNodeAtomicCount(ntree)) == 0) {
        return nullptr;
    }
    names = (char**)malloc(count * sizeof(char*));
    udaAddMalloc(logmalloclist, (void*)names, count, sizeof(char*), "char *");
    count = 0;
    for (int i = 0; i < ntree->userdefinedtype->fieldcount; i++) {
        if (ntree->userdefinedtype->compoundfield[i].atomictype != UDA_TYPE_UNKNOWN) {
            names[count++] = ntree->userdefinedtype->compoundfield[i].type;
        }
    }
    return names;
}

/** Return a List of Structure Component Pointer property attached to a tree node.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of Structure Pointer Properties.
 */

int* udaGetNodeStructurePointers(LOGMALLOCLIST* logmalloclist, NTREE* ntree)
{
    int count;
    int* pointers;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    if ((count = udaGetNodeStructureCount(ntree)) == 0) {
        return nullptr;
    }
    pointers = (int*)malloc(count * sizeof(int));
    udaAddMalloc(logmalloclist, (void*)pointers, count, sizeof(int), "int");
    count = 0;
    for (int i = 0; i < ntree->userdefinedtype->fieldcount; i++) {
        if (ntree->userdefinedtype->compoundfield[i].atomictype == UDA_TYPE_UNKNOWN) {
            pointers[count] = ntree->userdefinedtype->compoundfield[i].pointer;
            count++;
        }
    }
    return pointers;
}

/** Return a List of Atomic Component Pointer property attached to a tree node.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of Atomic Pointer Properties.
 */

int* udaGetNodeAtomicPointers(LOGMALLOCLIST* logmalloclist, NTREE* ntree)
{
    int count;
    int* pointers;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    if ((count = udaGetNodeAtomicCount(ntree)) == 0) {
        return nullptr;
    }
    pointers = (int*)malloc(count * sizeof(int));
    udaAddMalloc(logmalloclist, (void*)pointers, count, sizeof(int), "int");
    count = 0;
    for (int i = 0; i < ntree->userdefinedtype->fieldcount; i++) {
        if (ntree->userdefinedtype->compoundfield[i].atomictype != UDA_TYPE_UNKNOWN) {
            pointers[count] = ntree->userdefinedtype->compoundfield[i].pointer;
            // if (!strcmp(ntree->userdefinedtype->compoundfield[i].type, "STRING *")) pointers[count] = 1;
            count++;
        }
    }
    return pointers;
}

/** Return a List of Rank values of the Structure Components attached to a tree node.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of Structure Ranks.
 */

int* udaGetNodeStructureRank(LOGMALLOCLIST* logmalloclist, NTREE* ntree)
{
    int count, count0, size, rank;
    int *ranks, *shape;
    const char* type;
    char* data;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    if ((count = udaGetNodeStructureCount(ntree)) == 0) {
        return nullptr;
    }
    ranks = (int*)malloc(count * sizeof(int));
    udaAddMalloc(logmalloclist, (void*)ranks, count, sizeof(int), "int");
    count = 0;
    for (int i = 0; i < ntree->userdefinedtype->fieldcount; i++) {
        if (ntree->userdefinedtype->compoundfield[i].atomictype == UDA_TYPE_UNKNOWN) {
            if (!ntree->userdefinedtype->compoundfield[i].pointer) {
                ranks[count] = ntree->userdefinedtype->compoundfield[i].rank;
            } else {
                if ((data = (char*)ntree->data) == nullptr) {
                    return nullptr;
                }
                udaFindMalloc2(logmalloclist, &data[ntree->userdefinedtype->compoundfield[i].offset], &count0, &size,
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
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of Atomic Ranks.
 */

int* udaGetNodeAtomicRank(LOGMALLOCLIST* logmalloclist, NTREE* ntree)
{
    int count, count0, size, rank;
    int *ranks, *shape;
    const char* type;
    char* data;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    if ((count = udaGetNodeAtomicCount(ntree)) == 0) {
        return nullptr;
    }
    ranks = (int*)malloc(count * sizeof(int));
    udaAddMalloc(logmalloclist, (void*)ranks, count, sizeof(int), "int");
    count = 0;
    for (int i = 0; i < ntree->userdefinedtype->fieldcount; i++) {
        if (ntree->userdefinedtype->compoundfield[i].atomictype != UDA_TYPE_UNKNOWN) {
            if (!ntree->userdefinedtype->compoundfield[i].pointer) {
                ranks[count] = ntree->userdefinedtype->compoundfield[i].rank;
            } else {
                if ((data = (char*)ntree->data) == nullptr) {
                    return nullptr;
                }
                udaFindMalloc2(logmalloclist, &data[ntree->userdefinedtype->compoundfield[i].offset], &count0, &size,
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
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of Structure Shape Arrays.
 */
int** udaGetNodeStructureShape(LOGMALLOCLIST* logmalloclist, NTREE* ntree)
{
    int count, count0, size, rank;
    int* shape;
    int** shapes;
    const char* type;
    char* data;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    if ((count = udaGetNodeStructureCount(ntree)) == 0) {
        return nullptr;
    }
    shapes = (int**)malloc(count * sizeof(int*));
    udaAddMalloc(logmalloclist, (void*)shapes, count, sizeof(int*), "int *");
    count = 0;
    for (int i = 0; i < ntree->userdefinedtype->fieldcount; i++) {
        if (ntree->userdefinedtype->compoundfield[i].atomictype == UDA_TYPE_UNKNOWN) {
            if (!ntree->userdefinedtype->compoundfield[i].pointer) {
                shapes[count] = ntree->userdefinedtype->compoundfield[i].shape;
            } else {
                if ((data = (char*)ntree->data) == nullptr) {
                    return nullptr;
                }
                udaFindMalloc2(logmalloclist, &data[ntree->userdefinedtype->compoundfield[i].offset], &count0, &size,
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
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of Atomic Shape Arrays.
 */
int** udaGetNodeAtomicShape(LOGMALLOCLIST* logmalloclist, NTREE* ntree)
{
    int count, count0, size, rank;
    const char* type;
    char* data;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    if ((count = udaGetNodeAtomicCount(ntree)) == 0) {
        return nullptr;
    }
    int** shapes = (int**)malloc(count * sizeof(int*));
    udaAddMalloc(logmalloclist, (void*)shapes, count, sizeof(int*), "int *");
    count = 0;
    for (int i = 0; i < ntree->userdefinedtype->fieldcount; i++) {
        if (ntree->userdefinedtype->compoundfield[i].atomictype != UDA_TYPE_UNKNOWN) {
            if (!ntree->userdefinedtype->compoundfield[i].pointer) {
                shapes[count] = ntree->userdefinedtype->compoundfield[i].shape;
                if (shapes[count] == nullptr &&
                    ntree->userdefinedtype->compoundfield[i].rank < 2) { // Not passed so create
                    shapes[count] = (int*)malloc(sizeof(int));
                    shapes[count][0] = ntree->userdefinedtype->compoundfield[i].count;
                    ntree->userdefinedtype->compoundfield[i].shape = shapes[count];
                }
            } else {
                if ((data = (char*)ntree->data) == nullptr) {
                    return nullptr;
                }
                int* shape;
                void* ptr = &data[ntree->userdefinedtype->compoundfield[i].offset];
                udaFindMalloc2(logmalloclist, ptr, &count0, &size, &type, &rank, &shape);
                shapes[count] = shape;
                if (shape == 0 && (rank < 2)) {
                    shape = (int*)malloc(sizeof(int)); // Assume rank 1
                    udaAddMalloc(logmalloclist, (void*)shape, 1, sizeof(int), "int");
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

void udaPrintNodeNames(LOGMALLOCLIST* logmalloclist, NTREE* tree)
{
    int namecount;
    char** namelist;
    char** typelist;
    if (tree == nullptr) {
        tree = full_ntree;
    }

    UDA_LOG(UDA_LOG_DEBUG, "\nData Node Structure Names and Types\n");
    namecount = udaGetNodeStructureCount(tree);               // Count of all local data structures
    namelist = udaGetNodeStructureNames(logmalloclist, tree); // Names
    typelist = udaGetNodeStructureTypes(logmalloclist, tree); // Types
    UDA_LOG(UDA_LOG_DEBUG, "Structure Count %d\n", namecount);
    if (namecount > 0) {
        UDA_LOG(UDA_LOG_DEBUG, "  #\tName\tType\n");
        for (int i = 0; i < namecount; i++) {
            UDA_LOG(UDA_LOG_DEBUG, "[%2d]\t%s\t%s\n", i, namelist[i], typelist[i]);
        }
    }
    UDA_LOG(UDA_LOG_DEBUG, "\nData Node Atomic Names and Types\n");
    namecount = udaGetNodeAtomicCount(tree);               // Count of all local atomic data
    namelist = udaGetNodeAtomicNames(logmalloclist, tree); // Names
    typelist = udaGetNodeAtomicTypes(logmalloclist, tree); // Types
    UDA_LOG(UDA_LOG_DEBUG, "Atomic Count %d\n", namecount);
    if (namecount > 0) {
        UDA_LOG(UDA_LOG_DEBUG, "  #\tName\tType\n");
        for (int i = 0; i < namecount; i++) {
            UDA_LOG(UDA_LOG_DEBUG, "[%2d]\t%s\t%s\n", i, namelist[i], typelist[i]);
        }
    }
}

/** Print the Atomic Data from a data node to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return Void
 */
void udaPrintNodeAtomic(LOGMALLOCLIST* logmalloclist, NTREE* tree)
{
    int namecount;
    char** namelist;
    if (tree == nullptr) {
        tree = full_ntree;
    }
    namecount = udaGetNodeAtomicCount(tree);               // Count of all local atomic data
    namelist = udaGetNodeAtomicNames(logmalloclist, tree); // Names
    for (int i = 0; i < namecount; i++) {
        udaPrintAtomicType(logmalloclist, tree, namelist[i]);
    }
}

/** Return the number of User Defined Type Structure Definition Components attached to this tree node.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the number of User Defined Type Structure Definition Components.
 */
int udaGetNodeStructureComponentCount(NTREE* ntree)
{
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    return ntree->userdefinedtype->fieldcount;
}

/** Return a List of User Defined Type Structure Definition Components Names attached to this tree node.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of User Defined Type Structure Definition Component names.
 */
char** udaGetNodeStructureComponentNames(NTREE* ntree)
{
    int count;
    char** names;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    count = ntree->userdefinedtype->fieldcount;
    if (count == 0) {
        return nullptr;
    }
    names = (char**)malloc(count * sizeof(char*));
    for (int i = 0; i < count; i++) {
        names[i] = (char*)ntree->userdefinedtype->compoundfield[i].name;
    }
    return names;
}

/** Return a List of User Defined Type Structure Definition Components Types attached to this tree node.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of User Defined Type Structure Definition Component Types.
 */
char** udaGetNodeStructureComponentTypes(NTREE* ntree)
{
    int count;
    char** names;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    count = ntree->userdefinedtype->fieldcount;
    if (count == 0) {
        return nullptr;
    }
    names = (char**)malloc(count * sizeof(char*));
    for (int i = 0; i < count; i++) {
        names[i] = (char*)ntree->userdefinedtype->compoundfield[i].type;
    }
    return names;
}

/** Return a List of User Defined Type Structure Definition Components Descriptions attached to this tree node.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of User Defined Type Structure Definition Component Descriptions.
 */
char** udaGetNodeStructureComponentDescriptions(NTREE* ntree)
{
    int count;
    char** names;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    count = ntree->userdefinedtype->fieldcount;
    if (count == 0) {
        return nullptr;
    }
    names = (char**)malloc(count * sizeof(char*));
    for (int i = 0; i < count; i++) {
        names[i] = (char*)ntree->userdefinedtype->compoundfield[i].desc;
    }
    return names;
}

/** Return the Count of User Defined Structure Component Data array elements attached to this tree node.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @param target The name of a User Defined Structure definition.
 * @return the Count of User Defined Structure Component Data Array elements.
 */

int udaGetNodeStructureComponentDataCount(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target)
{
    const char* lastname;
    USERDEFINEDTYPE* userdefinedtype;
    int count = 0, size, fieldcount;
    const char* type;
    char* data;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    ntree =
        udaFindNTreeStructureComponent2(logmalloclist, ntree, target, &lastname); // Identify node and component name
    if (ntree == nullptr) {
        return 0;
    }

    // dgm 05Aug2015        structure and first component share the same address
    if (!strcmp(ntree->name, lastname)) {
        return ntree->parent->branches;
    }

    userdefinedtype = ntree->userdefinedtype;
    fieldcount = ntree->userdefinedtype->fieldcount;
    for (int i = 0; i < fieldcount; i++) {
        if (!strcmp(userdefinedtype->compoundfield[i].name, lastname)) {
            if (userdefinedtype->compoundfield[i].pointer) {
                if ((data = (char*)ntree->data) == nullptr) {
                    break;
                }
                udaFindMalloc(logmalloclist, &data[userdefinedtype->compoundfield[i].offset], &count, &size, &type);
                break;
            } else {
                count = userdefinedtype->compoundfield[i].count;
                break;
            }
        }
    }
    return count;
}

/** Return the Rank of User Defined Structure Component Data array attached to this tree node.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @param target The name of a User Defined Structure definition.
 * @return the Rank of User Defined Structure Component Data array.
 */
int udaGetNodeStructureComponentDataRank(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target)
{
    const char* lastname;
    char* data;
    const char* type;
    USERDEFINEDTYPE* userdefinedtype;
    int rank = 0, fieldcount, count, size;
    int* shape;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    ntree =
        udaFindNTreeStructureComponent2(logmalloclist, ntree, target, &lastname); // Identify node and component name
    if (ntree == nullptr) {
        return 0;
    }
    userdefinedtype = ntree->userdefinedtype;
    fieldcount = ntree->userdefinedtype->fieldcount;
    for (int i = 0; i < fieldcount; i++) {
        if (!strcmp(userdefinedtype->compoundfield[i].name, lastname)) {
            if (userdefinedtype->compoundfield[i].pointer) {
                if ((data = (char*)ntree->data) == nullptr) {
                    return 0;
                }
                udaFindMalloc2(logmalloclist, &data[ntree->userdefinedtype->compoundfield[i].offset], &count, &size,
                               &type, &rank, &shape);
                if (count == 0) {
                    rank = 0;
                }
            } else {
                rank = userdefinedtype->compoundfield[i].rank;
            }
            break;
        }
    }
    return rank;
}

/** Return the Shape array of the User Defined Structure Component Data array attached to this tree node.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @param target The name of a User Defined Structure definition.
 * @return the Shape array of length Rank of the User Defined Structure Component Data array.
 */
int* udaGetNodeStructureComponentDataShape(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target)
{
    const char* lastname;
    char* data;
    const char* type;
    USERDEFINEDTYPE* userdefinedtype;
    int fieldcount, count, size, rank;
    int* shape = nullptr;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    ntree =
        udaFindNTreeStructureComponent2(logmalloclist, ntree, target, &lastname); // Identify node and component name
    if (ntree == nullptr) {
        return nullptr;
    }
    userdefinedtype = ntree->userdefinedtype;
    fieldcount = ntree->userdefinedtype->fieldcount;
    for (int i = 0; i < fieldcount; i++) {
        if (!strcmp(userdefinedtype->compoundfield[i].name, lastname)) {
            if (userdefinedtype->compoundfield[i].pointer) {
                if ((data = (char*)ntree->data) == nullptr) {
                    return 0;
                }
                udaFindMalloc2(logmalloclist, &data[ntree->userdefinedtype->compoundfield[i].offset], &count, &size,
                               &type, &rank, &shape);
                if (count == 0) {
                    shape = nullptr;
                }
            } else {
                shape = userdefinedtype->compoundfield[i].shape;
            }
            break;
        }
    }
    return shape;
}

/** Return True (1) if the User Defined Structure Component Data array, attached to this tree node,
 * is a pointer type. Returns False (0) otherwise.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @param target The name of a User Defined Structure definition.
 * @return the value 1 if the User Defined Structure Component Data array is a pointer type.
 */
int udaGetNodeStructureComponentDataIsPointer(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target)
{
    const char* lastname;
    USERDEFINEDTYPE* userdefinedtype;
    int ispointer = 0, fieldcount;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    ntree =
        udaFindNTreeStructureComponent2(logmalloclist, ntree, target, &lastname); // Identify node and component name
    if (ntree == nullptr) {
        return 0;
    }
    userdefinedtype = ntree->userdefinedtype;
    fieldcount = ntree->userdefinedtype->fieldcount;
    for (int i = 0; i < fieldcount; i++) {
        if (!strcmp(userdefinedtype->compoundfield[i].name, lastname)) {
            ispointer = userdefinedtype->compoundfield[i].pointer;
            break;
        }
    }
    return ispointer;
}

/** Return the Size of a User Defined Structure Component.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @param target The name of a User Defined Structure Component.
 * @return the Size of the User Defined Structure Component.
 */
int udaGetNodeStructureComponentDataSize(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target)
{
    const char* lastname;
    USERDEFINEDTYPE* userdefinedtype;
    int count, size = 0, fieldcount;
    const char* type;
    char* data;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    ntree =
        udaFindNTreeStructureComponent2(logmalloclist, ntree, target, &lastname); // Identify node and component name
    if (ntree == nullptr) {
        return 0;
    }
    userdefinedtype = ntree->userdefinedtype;
    fieldcount = ntree->userdefinedtype->fieldcount;
    for (int i = 0; i < fieldcount; i++) {
        if (!strcmp(userdefinedtype->compoundfield[i].name, lastname)) {
            if (userdefinedtype->compoundfield[i].pointer) {
                if ((data = (char*)ntree->data) == nullptr) {
                    break;
                }
                udaFindMalloc(logmalloclist, &data[userdefinedtype->compoundfield[i].offset], &count, &size, &type);
                break;
            } else {
                size = userdefinedtype->compoundfield[i].size;
                break;
            }
        }
    }
    return size;
}

/** Return the Type Name of a User Defined Structure Component.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @param target The name of a User Defined Structure Component.
 * @return the Type Name of the User Defined Structure Component.
 */
const char* udaGetNodeStructureComponentDataDataType(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target)
{
    const char* lastname;
    USERDEFINEDTYPE* userdefinedtype;
    int count, size, fieldcount;
    const char* type = nullptr;
    char* data;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    ntree =
        udaFindNTreeStructureComponent2(logmalloclist, ntree, target, &lastname); // Identify node and component name
    if (ntree == nullptr) {
        return "unknown";
    }
    userdefinedtype = ntree->userdefinedtype;
    fieldcount = ntree->userdefinedtype->fieldcount;
    for (int i = 0; i < fieldcount; i++) {
        if (!strcmp(userdefinedtype->compoundfield[i].name, lastname)) {
            if (userdefinedtype->compoundfield[i].pointer) {
                if ((data = (char*)ntree->data) == nullptr) {
                    break;
                }
                udaFindMalloc(logmalloclist, &data[userdefinedtype->compoundfield[i].offset], &count, &size, &type);
                break;
            } else {
                type = userdefinedtype->compoundfield[i].type;
                break;
            }
        }
    }
    return type;
}

/** Return a pointer to a User Defined Structure Component's data.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @param target The name of a User Defined Structure Component.
 * @return the User Defined Structure Component's data.
 */
void* udaGetNodeStructureComponentData(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target)
{
    const char* lastname;
    USERDEFINEDTYPE* userdefinedtype;
    int offset, fieldcount;
    char* p;
    void* data = nullptr;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    ntree =
        udaFindNTreeStructureComponent2(logmalloclist, ntree, target, &lastname); // Identify node and component name
    if (ntree == nullptr) {
        return nullptr;
    }

    if ((strchr(target, '.') != nullptr || strchr(target, '/') != nullptr) && !strcmp(ntree->name, lastname)) {
        return ntree->data;
    }

    if (strcmp(ntree->name, lastname) == 0 && strcmp(target, lastname) == 0) {
        return ntree->data;
    }

    userdefinedtype = ntree->userdefinedtype;
    fieldcount = ntree->userdefinedtype->fieldcount;
    for (int i = 0; i < fieldcount; i++) {
        if (!strcmp(userdefinedtype->compoundfield[i].name, lastname)) {
            p = (char*)ntree->data;
            offset = userdefinedtype->compoundfield[i].offset;
            if (userdefinedtype->compoundfield[i].pointer) {
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
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @param target The name of a User Defined Structure Component.
 * @return void.
 */
void udaPrintNodeStructureComponentData(NTREE* ntree, LOGMALLOCLIST* logmalloclist,
                                        USERDEFINEDTYPELIST* userdefinedtypelist, const char* target)
{
    NTREE* node;
    USERDEFINEDTYPE* userdefinedtype;
    int namecount, count;
    const char* type;
    const char* lastname;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }

    node = udaFindNTreeStructureComponent2(logmalloclist, ntree, target, &lastname); // Locate the Node
    if (ntree == nullptr) {
        return;
    }

    count = udaGetNodeStructureComponentDataCount(logmalloclist, node, lastname);   // Array Size
    type = udaGetNodeStructureComponentDataDataType(logmalloclist, node, lastname); // Type

    if (count > 0) {
        UDA_LOG(UDA_LOG_DEBUG, "[%s] Data Count %d   Type %s\n", target, count, type);
        UDA_LOG(UDA_LOG_DEBUG, "Data Values\n");
        if (!strcmp(type, "float")) {
            auto s = (float*)udaGetNodeStructureComponentData(logmalloclist, node, lastname);
            for (int i = 0; i < count; i++) {
                UDA_LOG(UDA_LOG_DEBUG, "[%d] %f\n", i, s[i]);
            }
            return;
        }
        if (!strcmp(type, "int")) {
            auto s = (int*)udaGetNodeStructureComponentData(logmalloclist, node, lastname);
            for (int i = 0; i < count; i++) {
                UDA_LOG(UDA_LOG_DEBUG, "[%d] %d\n", i, s[i]);
            }
            return;
        }
        if (!strcmp(type, "STRING")) {
            auto s = (char*)udaGetNodeStructureComponentData(logmalloclist, node, lastname);
            UDA_LOG(UDA_LOG_DEBUG, "%s\n", s);
            return;
        }
        if ((userdefinedtype = udaFindUserDefinedType(userdefinedtypelist, type, 0)) != nullptr) {
            int firstpass = 1, offset, namecount2;
            char** namelist2;
            NTREE temp;
            initNTree(&temp);
            void* str = nullptr;
            void* data = nullptr;
            void* olddata = nullptr;
            char* p = (char*)udaGetNodeStructureComponentData(logmalloclist, node, lastname); // Structure Array
            char* pp = nullptr;
            namecount = userdefinedtype->fieldcount; // Count of sub-structure elements
            UDA_LOG(UDA_LOG_DEBUG, "Data Count %d   Type %s\n", namecount, type);
            for (int j = 0; j < count; j++) {
                str = (void*)&p[j * userdefinedtype->size];
                pp = (char*)str;
                for (int i = 0; i < namecount; i++) {
                    offset = userdefinedtype->compoundfield[i].offset;
                    type = userdefinedtype->compoundfield[i].type;
                    UDA_LOG(UDA_LOG_DEBUG, "[%d]   Type %s   Name %s\n", i, type,
                            userdefinedtype->compoundfield[i].name);

                    if (userdefinedtype->compoundfield[i].pointer) {
                        data = (void*)*((VOIDTYPE*)&pp[offset]); // Data Element from the single Structure Array Element
                    } else {
                        data = (void*)&pp[offset];
                    }
                    if (data == nullptr) {
                        continue;
                    }
                    if (userdefinedtype->compoundfield[i].atomictype != UDA_TYPE_UNKNOWN) {
                        udaPrintAtomicData(data, userdefinedtype->compoundfield[i].atomictype,
                                           userdefinedtype->compoundfield[i].count, lastname);
                    } else {

                        temp.data = data;
                        strcpy(temp.name, userdefinedtype->compoundfield[i].name);
                        temp.userdefinedtype = udaFindUserDefinedType(userdefinedtypelist, type, 0);
                        if (firstpass) {
                            udaAddNonMalloc(logmalloclist, data, 1, userdefinedtype->compoundfield[i].size, type);
                            firstpass = 0;
                        } else {
                            udaChangeNonMalloc(logmalloclist, olddata, data, 1, userdefinedtype->compoundfield[i].size,
                                               type);
                        }
                        olddata = data;

                        namecount2 = udaGetNodeStructureComponentCount(&temp); // Count of structure elements
                        namelist2 = udaGetNodeStructureComponentNames(&temp);  // List of structure element names
                        UDA_LOG(UDA_LOG_DEBUG, "Data Count %d   Type %s\n", namecount2, type);

                        for (int k = 0; k < namecount2; k++) {
                            udaPrintNodeStructureComponentData(&temp, logmalloclist, userdefinedtypelist, namelist2[k]);
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
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return void.
 */
void udaPrintNodeStructure(LOGMALLOCLIST* logmalloclist, NTREE* ntree)
{
    int count, acount, scount, kstart = 1;
    char **anamelist, **snamelist;
    NTREE *node, *node2;
    void* data;

    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }

    acount = udaGetNodeAtomicCount(ntree); // Count of the Tree Node Structure atomic type components
    anamelist = udaGetNodeAtomicNames(logmalloclist, ntree);
    scount = udaGetNodeStructureCount(ntree); // Count of the Tree Node Structure structure type components
    snamelist = udaGetNodeStructureNames(logmalloclist, ntree);
    count = udaGetNodeStructureDataCount(logmalloclist, ntree); // Count of the Tree Node Structure Array elements

    node = ntree; // Start at the base node: all other structure array elements are sibling nodes

    for (int j = 0; j < count; j++) {

        UDA_LOG(UDA_LOG_DEBUG, "%s contents:\n", ntree->userdefinedtype->name);

        data = udaGetNodeStructureArrayData(logmalloclist, ntree, j); // Loop over Structure Array Elements

        // Find the next structure array element node - it must be a sibling node
        // Nodes are ordered so start at the previous node
        // Nodes must be of the same type and heap address must match

        if (j > 0) {
            node = nullptr;
            for (int k = kstart; k < ntree->parent->branches; k++) {
                if (!strcmp(ntree->parent->children[k]->name, ntree->name) &&
                    (ntree->parent->children[k]->data == data)) {
                    node = ntree->parent->children[k];
                    kstart = k + 1; // Next Start from the next sibling node
                }
            }
            if (node == nullptr) {
                add_error(UDA_CODE_ERROR_TYPE, "udaPrintNodeStructure", 999, "Structure Array element Node not Found!");
                return;
            }
        }

        for (int i = 0; i < acount; i++) {
            udaPrintAtomicType(logmalloclist, node, anamelist[i]); // Print Atomic Components
        }

        for (int i = 0; i < scount; i++) { // Print Structured Components

            // Structured components must be children of this node.

            if ((node2 = udaFindNTreeStructure(logmalloclist, node, snamelist[i])) != nullptr) {
                udaPrintNodeStructure(logmalloclist, node2);
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "%40s: null\n", snamelist[i]);
            }
        }
    }
}

/** Return a pointer to a User Defined Structure Component's data cast to FLOAT.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @param target The name of a User Defined Structure Component.
 * @return the User Defined Structure Component's data cast to float.
 */
float* udaCastNodeStructureComponentDatatoFloat(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target)
{
    NTREE* node;
    int count;
    const char* type;
    const char* lastname;
    float* data;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }

    node = udaFindNTreeStructureComponent2(logmalloclist, ntree, target, &lastname);
    if (ntree == nullptr) {
        return nullptr;
    }

    count = udaGetNodeStructureComponentDataCount(logmalloclist, node, lastname);
    type = udaGetNodeStructureComponentDataDataType(logmalloclist, node, lastname);

    if (!strcmp(type, "float")) {
        return ((float*)udaGetNodeStructureComponentData(logmalloclist, node, lastname));
    }

    if (count == 0) {
        return nullptr;
    }

    data = (float*)malloc(count * sizeof(float));
    if (!strcmp(type, "double")) {
        double* s = (double*)udaGetNodeStructureComponentData(logmalloclist, node, lastname);
        for (int i = 0; i < count; i++) {
            data[i] = (float)s[i];
        }
        return data;
    }
    if (!strcmp(type, "int")) {
        int* s = (int*)udaGetNodeStructureComponentData(logmalloclist, node, lastname);
        for (int i = 0; i < count; i++) {
            data[i] = (float)s[i];
        }
        return data;
    }

    return nullptr;
}

/** Return a pointer to a User Defined Structure Component's data cast to DOUBLE.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @param target The name of a User Defined Structure Component.
 * @return the User Defined Structure Component's data cast to float.
 */
double* castNodeStructureComponentDatatoDouble(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target)
{
    NTREE* node;
    int count;
    const char* type;
    const char* lastname;
    double* data;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }

    node = udaFindNTreeStructureComponent2(logmalloclist, ntree, target, &lastname);
    if (ntree == nullptr) {
        return nullptr;
    }

    count = udaGetNodeStructureComponentDataCount(logmalloclist, node, lastname);
    type = udaGetNodeStructureComponentDataDataType(logmalloclist, node, lastname);

    if (!strcmp(type, "double")) {
        return (double*)udaGetNodeStructureComponentData(logmalloclist, node, lastname);
    }

    if (count == 0) {
        return nullptr;
    }

    data = (double*)malloc(count * sizeof(double));
    if (!strcmp(type, "float")) {
        float* s = (float*)udaGetNodeStructureComponentData(logmalloclist, node, lastname);
        for (int i = 0; i < count; i++) {
            data[i] = (double)s[i];
        }
        return data;
    }
    if (!strcmp(type, "int")) {
        int* s = (int*)udaGetNodeStructureComponentData(logmalloclist, node, lastname);
        for (int i = 0; i < count; i++) {
            data[i] = (double)s[i];
        }
        return data;
    }

    return nullptr;
}

//---------------------------------------------------------------------------------------------
// Tree Branch Family: Whole tree is in scope

/** Initialise a NTREE data structure.
 *
 * @param str A pointer to a NTREE data structure instance.
 * @return Void.
 */
void initNTree(NTREE* str)
{
    str->branches = 0;
    str->name[0] = '\0';
    str->userdefinedtype = nullptr;
    str->data = nullptr;
    str->parent = nullptr;
    str->children = nullptr;
}

/** Initialise the Global NTREE list structure.
 *
 * @return Void.
 */
void initNTreeList(NTREELIST* ntree_list)
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
void udaPrintNTree2(NTREE* tree)
{
    if (tree == nullptr) {
        tree = full_ntree;
    }
    UDA_LOG(UDA_LOG_DEBUG, "\nNTREE Node Contents\n");
    UDA_LOG(UDA_LOG_DEBUG, "Name    : %s\n", tree->name);
    UDA_LOG(UDA_LOG_DEBUG, "Type    : %s\n", tree->userdefinedtype->name);
#ifdef A64
    UDA_LOG(UDA_LOG_DEBUG, "Parent  : %llx\n", (UVOIDTYPE)tree->parent);
#else
    UDA_LOG(UDA_LOG_DEBUG, "Parent  : %x\n", (UVOIDTYPE)tree->parent);
#endif
    UDA_LOG(UDA_LOG_DEBUG, "Children: %d\n", tree->branches);
#ifdef A64
    for (int i = 0; i < tree->branches; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "[%2d]: %llx\n", i, (UVOIDTYPE)tree->children[i]);
    }
#else
    for (int i = 0; i < tree->branches; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "[%2d]: %x\n", i, (UVOIDTYPE)tree->children[i]);
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
void udaPrintNTree(NTREE* tree, USERDEFINEDTYPELIST* userdefinedtypelist)
{
    if (tree == nullptr) {
        tree = full_ntree;
    }
    UDA_LOG(UDA_LOG_DEBUG, "--------------------------------------------------------------------\n");
#ifdef A64
    UDA_LOG(UDA_LOG_DEBUG, "\nNTREE Node %llx (%lld) Contents\n", (UVOIDTYPE)tree, (UVOIDTYPE)tree);
#else
    UDA_LOG(UDA_LOG_DEBUG, "\nNTREE Node %x (%d) Contents\n", (UVOIDTYPE)tree, (UVOIDTYPE)tree);
#endif
    UDA_LOG(UDA_LOG_DEBUG, "Name: %s\n", tree->name);
    UDA_LOG(UDA_LOG_DEBUG, "Children: %d\n", tree->branches);
    udaPrintUserDefinedTypeTable(userdefinedtypelist, *tree->userdefinedtype);
    for (int i = 0; i < tree->branches; i++) {
        udaPrintNTree(tree->children[i], userdefinedtypelist);
    }
}

/** Print Details of the tree node List to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return Void
 */
void udaPrintNTreeList(NTREE* tree)
{
    if (tree == nullptr) {
        tree = full_ntree;
    }
#ifdef A64
    UDA_LOG(UDA_LOG_DEBUG, "%llx\t(%lld)\t%s\t%s\t%d\n", (UVOIDTYPE)tree, (UVOIDTYPE)tree, tree->name,
            tree->userdefinedtype->name, tree->branches);
#else
    UDA_LOG(UDA_LOG_DEBUG, "%x\t(%d)\t%s\t%s\t%d\n", (UVOIDTYPE)tree, (UVOIDTYPE)tree, tree->name,
            tree->userdefinedtype->name, tree->branches);
#endif
    for (int i = 0; i < tree->branches; i++) {
        udaPrintNTreeList(tree->children[i]);
    }
}

/** Return a Count of User Defined Type Tree Nodes from and including the passed tree node.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the Count of Tree Nodes.
 */
int udaGetNTreeStructureCount(NTREE* ntree)
{
    int count = 1;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    if (ntree->branches == 0) {
        return count;
    }
    for (int i = 0; i < ntree->branches; i++) {
        if (i == 0 ||
            strcmp(ntree->children[i]->userdefinedtype->name, ntree->children[i - 1]->userdefinedtype->name) != 0) {
            count = count + udaGetNTreeStructureCount(ntree->children[i]);
        }
    }
    return count;
}

/** Return a List of User Defined Type Structure Names attached to this tree branch.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of User Defined Type Structure names.
 */
char** udaGetNTreeStructureNames(LOGMALLOCLIST* logmalloclist, NTREE* ntree)
{
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }

    auto names = (char**)malloc(sizeof(char*));
    udaAddMalloc(logmalloclist, (void*)names, 1, sizeof(char*), "char *");
    names[0] = ntree->name;

    if (ntree->branches == 0) {
        return names;
    }

    int count = 1;
    for (int i = 0; i < ntree->branches; i++) {
        if (i == 0 ||
            strcmp(ntree->children[i]->userdefinedtype->name, ntree->children[i - 1]->userdefinedtype->name) != 0) {
            int childcount = udaGetNTreeStructureCount(ntree->children[i]);
            VOIDTYPE old = (VOIDTYPE)names;
            names = (char**)realloc((void*)names, (count + childcount) * sizeof(char*));
            udaChangeMalloc(logmalloclist, old, (void*)names, (count + childcount), sizeof(char*), "char *");
            char** childnames = udaGetNTreeStructureNames(logmalloclist, ntree->children[i]);
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
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of User Defined Type Structure Type names.
 */
char** udaGetNTreeStructureTypes(LOGMALLOCLIST* logmalloclist, NTREE* ntree)
{
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }

    auto names = (char**)malloc(sizeof(char*));
    udaAddMalloc(logmalloclist, (void*)names, 1, sizeof(char*), "char *");
    names[0] = ntree->userdefinedtype->name;
    if (ntree->branches == 0) {
        return names;
    }

    int count = 1;
    for (int i = 0; i < ntree->branches; i++) {
        if (i == 0 ||
            strcmp(ntree->children[i]->userdefinedtype->name, ntree->children[i - 1]->userdefinedtype->name) != 0) {
            int childcount = udaGetNTreeStructureCount(ntree->children[i]);
            VOIDTYPE old = (VOIDTYPE)names;
            names = (char**)realloc((void*)names, (count + childcount) * sizeof(char*));
            udaChangeMalloc(logmalloclist, old, (void*)names, (count + childcount), sizeof(char*), "char *");
            char** childnames = udaGetNTreeStructureTypes(logmalloclist, ntree->children[i]);
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
void udaPrintNTreeStructureNames(LOGMALLOCLIST* logmalloclist, NTREE* tree)
{
    int namecount;
    char **namelist, **typelist;
    if (tree == nullptr) {
        tree = full_ntree;
    }
#ifdef A64
    UDA_LOG(UDA_LOG_DEBUG, "\nData Tree %llx Structure Names and Types\n", (UVOIDTYPE)tree);
#else
    UDA_LOG(UDA_LOG_DEBUG, "\nData Tree %x Structure Names and Types\n", (UVOIDTYPE)tree);
#endif
    namecount = udaGetNTreeStructureCount(tree);               // Count of all Tree Nodes
    namelist = udaGetNTreeStructureNames(logmalloclist, tree); // Names of all user defined data structures
    typelist = udaGetNTreeStructureTypes(logmalloclist, tree); // Types of all user defined data structures
    UDA_LOG(UDA_LOG_DEBUG, "Total Structure Count %d\n", namecount);
    UDA_LOG(UDA_LOG_DEBUG, "  #\tName\tType\n");
    for (int i = 0; i < namecount; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "[%2d]\t%s\t%s\n", i, namelist[i], typelist[i]);
    }
}

/** Return the total number of User Defined Type Structure Definition Components attached to this tree branch.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the number of User Defined Type Structure Definition Components.
 */
int udaGetNTreeStructureComponentCount(NTREE* ntree)
{
    int count;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    count = ntree->userdefinedtype->fieldcount;

    for (int i = 0; i < ntree->branches; i++) {
        if (i == 0 ||
            strcmp(ntree->children[i]->userdefinedtype->name, ntree->children[i - 1]->userdefinedtype->name) != 0) {
            count = count + udaGetNTreeStructureComponentCount(ntree->children[i]);
        }
    }
    return count;
}

/** Return a List of User Defined Type Structure Definition Components Names attached to this tree branch.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of User Defined Type Structure Definition Component names.
 */
char** udaGetNTreeStructureComponentNames(LOGMALLOCLIST* logmalloclist, NTREE* ntree)
{
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }

    int count = ntree->userdefinedtype->fieldcount;
    if (count == 0) {
        return nullptr;
    }

    char** names = udaGetNodeStructureComponentNames(ntree);

    for (int i = 0; i < ntree->branches; i++) {
        if (i == 0 ||
            strcmp(ntree->children[i]->userdefinedtype->name, ntree->children[i - 1]->userdefinedtype->name) != 0) {
            int childcount = udaGetNTreeStructureComponentCount(ntree->children[i]);
            auto old = (VOIDTYPE)names;
            names = (char**)realloc((void*)names, (count + childcount) * sizeof(char*));
            udaChangeMalloc(logmalloclist, old, (void*)names, (count + childcount), sizeof(char*), "char *");
            char** childnames = udaGetNTreeStructureComponentNames(logmalloclist, ntree->children[i]);
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
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of User Defined Type Structure Definition Component Types.
 */
char** udaGetNTreeStructureComponentTypes(LOGMALLOCLIST* logmalloclist, NTREE* ntree)
{
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }

    int count = ntree->userdefinedtype->fieldcount;
    if (count == 0) {
        return nullptr;
    }

    char** names = udaGetNodeStructureComponentTypes(ntree);

    for (int i = 0; i < ntree->branches; i++) {
        if (i == 0 ||
            strcmp(ntree->children[i]->userdefinedtype->name, ntree->children[i - 1]->userdefinedtype->name) != 0) {
            int childcount = udaGetNTreeStructureComponentCount(ntree->children[i]);
            auto old = (VOIDTYPE)names;
            names = (char**)realloc((void*)names, (count + childcount) * sizeof(char*));
            udaChangeMalloc(logmalloclist, old, (void*)names, (count + childcount), sizeof(char*), "char *");
            char** childnames = udaGetNTreeStructureComponentTypes(logmalloclist, ntree->children[i]);
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
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the List of User Defined Type Structure Definition Component Descriptions.
 */
char** udaGetNTreeStructureComponentDescriptions(LOGMALLOCLIST* logmalloclist, NTREE* ntree)
{
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }

    int count = ntree->userdefinedtype->fieldcount;
    if (count == 0) {
        return nullptr;
    }

    char** names = udaGetNodeStructureComponentDescriptions(ntree);

    for (int i = 0; i < ntree->branches; i++) {
        if (i == 0 ||
            strcmp(ntree->children[i]->userdefinedtype->name, ntree->children[i - 1]->userdefinedtype->name) != 0) {
            int childcount = udaGetNTreeStructureComponentCount(ntree->children[i]);
            auto old = (VOIDTYPE)names;
            names = (char**)realloc((void*)names, (count + childcount) * sizeof(char*));
            udaChangeMalloc(logmalloclist, old, (void*)names, (count + childcount), sizeof(char*), "char *");
            char** childnames = udaGetNTreeStructureComponentDescriptions(logmalloclist, ntree->children[i]);
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
void udaPrintNTreeStructureComponentNames(LOGMALLOCLIST* logmalloclist, NTREE* tree)
{
    int namecount;
    char **namelist, **typelist, **desclist;
    if (tree == nullptr) {
        tree = full_ntree;
    }
    UDA_LOG(UDA_LOG_DEBUG, "\nData Tree Structure Component Names, Types and Descriptions\n");
    namecount = udaGetNTreeStructureComponentCount(tree);                      // Count of all Tree Nodes
    namelist = udaGetNTreeStructureComponentNames(logmalloclist, tree);        // Names of all structure elements
    typelist = udaGetNTreeStructureComponentTypes(logmalloclist, tree);        // Types of all structure elements
    desclist = udaGetNTreeStructureComponentDescriptions(logmalloclist, tree); // Descriptions of all structure elements
    UDA_LOG(UDA_LOG_DEBUG, "Total Structure Component Count %d\n", namecount);
    UDA_LOG(UDA_LOG_DEBUG, "  #\tName\tType\tDescription\n");
    for (int i = 0; i < namecount; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "[%2d]\t%s\t%s\t%s\n", i, namelist[i], typelist[i], desclist[i]);
    }
}

//=======================================================================================================
// Print utility functions: explicit output to stdout
void udaPrintNode_stdout(NTREE* tree)
{
    udaPrintNode(tree);
}

void udaPrintNodeNames_stdout(LOGMALLOCLIST* logmalloclist, NTREE* tree)
{
    udaPrintNodeNames(logmalloclist, tree);
}

void udaPrintNodeAtomic_stdout(LOGMALLOCLIST* logmalloclist, NTREE* tree)
{
    udaPrintNodeAtomic(logmalloclist, tree);
}

void udaPrintNTreeStructureNames_stdout(LOGMALLOCLIST* logmalloclist, NTREE* tree)
{
    udaPrintNTreeStructureNames(logmalloclist, tree);
}

void udaPrintNTreeStructureComponentNames_stdout(LOGMALLOCLIST* logmalloclist, NTREE* tree)
{
    udaPrintNTreeStructureComponentNames(logmalloclist, tree);
}

void udaPrintAtomicType_stdout(LOGMALLOCLIST* logmalloclist, NTREE* tree, const char* target)
{
    udaPrintAtomicType(logmalloclist, tree, target);
}

void udaGetNodeStructureComponentDataShape_f(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target,
                                             int* shape_f)
{
    int rank = udaGetNodeStructureComponentDataRank(logmalloclist, ntree, target);
    for (int i = 0; i < MAXRANK; i++) {
        shape_f[i] = 0;
    }
    if (rank > 1 && rank <= MAXRANK) {
        int* shape = udaGetNodeStructureComponentDataShape(logmalloclist, ntree, target);
        if (shape != nullptr) {
            for (int i = 0; i < rank; i++) {
                shape_f[i] = shape[i];
            }
        }
    } else {
        shape_f[0] = udaGetNodeStructureComponentDataCount(logmalloclist, ntree, target);
    }
}

void getNodeStructureComponentShortData_f(LOGMALLOCLIST* logmalloclist, NTREE* node, const char* target, short* data_f)
{
    auto data = (short*)udaGetNodeStructureComponentData(logmalloclist, node, target);
    int count = udaGetNodeStructureComponentDataCount(logmalloclist, node, target);
    for (int i = 0; i < count; i++) {
        data_f[i] = data[i];
    }
}

void getNodeStructureComponentFloatData_f(LOGMALLOCLIST* logmalloclist, NTREE* node, const char* target, float* data_f)
{
    float* data = (float*)udaGetNodeStructureComponentData(logmalloclist, node, target);
    int count = udaGetNodeStructureComponentDataCount(logmalloclist, node, target);
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

short* udaCastNodeStructureComponentDatatoShort(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target)
{
    NTREE* node;
    int count;
    const char* type;
    const char* lastname;
    short* data;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }

    node = udaFindNTreeStructureComponent2(logmalloclist, ntree, target, &lastname);
    if (ntree == nullptr) {
        return nullptr;
    }

    count = udaGetNodeStructureComponentDataCount(logmalloclist, node, lastname);
    type = udaGetNodeStructureComponentDataDataType(logmalloclist, node, lastname);

    if (!strcmp(type, "short")) {
        return (short*)udaGetNodeStructureComponentData(logmalloclist, node, lastname);
    }

    if (count == 0) {
        return nullptr;
    }

    data = (short*)malloc(count * sizeof(short));
    if (!strcmp(type, "double")) {
        double* s = (double*)udaGetNodeStructureComponentData(logmalloclist, node, lastname);
        for (int i = 0; i < count; i++) {
            data[i] = (short)s[i];
        }
        return data;
    }
    if (!strcmp(type, "float")) {
        float* s = (float*)udaGetNodeStructureComponentData(logmalloclist, node, lastname);
        for (int i = 0; i < count; i++) {
            data[i] = (short)s[i];
        }
        return data;
    }
    if (!strcmp(type, "int")) {
        int* s = (int*)udaGetNodeStructureComponentData(logmalloclist, node, lastname);
        for (int i = 0; i < count; i++) {
            data[i] = (short)s[i];
        }
        return data;
    }
    if (!strcmp(type, "unsigned int")) {
        unsigned int* s = (unsigned int*)udaGetNodeStructureComponentData(logmalloclist, node, lastname);
        for (int i = 0; i < count; i++) {
            data[i] = (short)s[i];
        }
        return data;
    }
    if (!strcmp(type, "unsigned short")) {
        unsigned short* s = (unsigned short*)udaGetNodeStructureComponentData(logmalloclist, node, lastname);
        for (int i = 0; i < count; i++) {
            data[i] = (short)s[i];
        }
        return data;
    }
    return nullptr;
}

void udaCastNodeStructureComponentDatatoShort_f(LOGMALLOCLIST* logmalloclist, NTREE* node, const char* target,
                                                short* data_f)
{
    short* data = udaCastNodeStructureComponentDatatoShort(logmalloclist, node, target);
    if (data != nullptr) {
        int count = udaGetNodeStructureComponentDataCount(logmalloclist, node, target);
        for (int i = 0; i < count; i++) {
            data_f[i] = data[i];
        }
        free(data);
    }
}

void udaCastNodeStructureComponentDatatoFloat_f(LOGMALLOCLIST* logmalloclist, NTREE* node, const char* target,
                                                float* data_f)
{
    float* data = udaCastNodeStructureComponentDatatoFloat(logmalloclist, node, target);
    if (data != nullptr) {
        int count = udaGetNodeStructureComponentDataCount(logmalloclist, node, target);
        for (int i = 0; i < count; i++) {
            data_f[i] = data[i];
        }
        free(data);
    }
}

void udaAddStructureField(USERDEFINEDTYPE* user_type, const char* name, const char* desc, UDA_TYPE data_type,
                          bool is_pointer, int rank, int* shape, size_t offset)
{
    COMPOUNDFIELD field;
    initCompoundField(&field);

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

    udaAddCompoundField(user_type, field);
}
