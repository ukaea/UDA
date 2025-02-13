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
#include "protocol/protocolXML2.h"
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

#include <boost/algorithm/string/case_conv.hpp>
#include <client2/thread_client.hpp>

#include "clientserver/error_log.h"
#include "common/string_utils.h"
#include "logging/logging.h"
#include "protocol/protocolXML2Put.h"
#include "protocol/xdrlib.h"

#include "xdrUserDefinedData.h"

#if defined(SERVERBUILD)
#  include "server/udaServer.h"
#endif

using namespace uda::client_server;
using namespace uda::logging;
using namespace uda::structures;
using namespace uda::common;
using namespace uda::protocol;

static unsigned int g_last_malloc_index = 0; // Malloc Log search index last value
static unsigned int* g_last_malloc_index_value = &g_last_malloc_index; // Preserve Malloc Log search index last value in GENERAL_STRUCT
static NTree* g_full_ntree = nullptr;

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

    print_image(str.image, str.imagecount);
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
            get_structure_size(user_defined_type_list, &str), str.fieldcount, str.ref_id);
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

/** Copy the Master User Defined Structure Definition List.
 *
 * @param anew The copy of the type definition list.
 * @return void.
 */
#if defined(SERVERBUILD)
void uda::structures::copy_user_defined_type_list(UserDefinedTypeList** anew,
                                                  const UserDefinedTypeList* parseduserdefinedtype_list)
{
    UserDefinedTypeList* list = (UserDefinedTypeList*)malloc(sizeof(UserDefinedTypeList));
    init_user_defined_type_list(list);
    list->listCount = parseduserdefinedtype_list->listCount; // Copy the standard set of structure definitions
    list->userdefinedtype = (UserDefinedType*)malloc(parseduserdefinedtype_list->listCount * sizeof(UserDefinedType));

    for (int i = 0; i < list->listCount; i++) {
        UserDefinedType usertypeOld = parseduserdefinedtype_list->userdefinedtype[i];
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
                                                  const UserDefinedTypeList* parseduserdefinedtype_list)
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
    field.offset = new_offset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = get_alignment_of(field.type);
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
    field.offpad = padding(offset, field.type);
    field.alignment = get_alignment_of(field.type);
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
    field.offpad = padding(offset, field.type);
    field.alignment = get_alignment_of(field.type);
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
    field.offpad = padding(offset, field.type);
    field.alignment = get_alignment_of(field.type);
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
    int type_id = get_type_of(type);
    char* d;
    if (xdrs->x_op == XDR_DECODE) {
        d = (char*)malloc(count * size);
        add_malloc(log_malloc_list, d, count, size, type);
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

int uda::protocol::xdr_user_defined_type_data(std::vector<UdaError>& error_stack, XDR* xdrs, LogMallocList* log_malloc_list,
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

        set_full_ntree(dataNTree); // Copy to Global
    } else {

        if (user_defined_type == nullptr) {
            add_error(error_stack, ErrorType::Code, "udaXDRUserDefinedTypeData", 999,
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

    free_log_struct_list(log_struct_list); // Free Linked List Structure Log heap

    return rc;
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

bool_t uda::protocol::xdr_user_defined_type(XDR* xdrs, UserDefinedTypeList* user_defined_type_list,
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
                size = get_size_of(user_defined_type_list, str->compoundfield[i].type);
            }
            alignment = get_alignment_of(str->compoundfield[i].type);
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
                alignment = get_alignment_of("*");
            } else {
                size = get_size_of(user_defined_type_list, str->compoundfield[i].type);
                alignment = get_alignment_of(str->compoundfield[i].type);
            }

            space = size * str->compoundfield[i].count;

            if (i != 0) {
                if (str->compoundfield[i].pointer) {
                    offpad = padding(offset + space0, "*");
                    offset = new_offset(offset + space0, "*");
                } else {
                    offpad = padding(offset + space0, str->compoundfield[i].type);
                    offset = new_offset(
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

bool_t uda::protocol::xdr_user_defined_type_list(XDR* xdrs, UserDefinedTypeList* str, bool xdr_stdio_flag)
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

    UDA_LOG(UDA_LOG_DEBUG, "xdr_userdefinedtype_list: rc = {}, listCount = {}", rc, str->listCount);

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

void uda::structures::add_malloc(LogMallocList* log_malloc_list, void* heap, int count, size_t size, const char* type)
{
    // Log all Heap allocations for Data from User Defined Structures
    // Grow the list when necessary

    // UserDefinedType *udt;

    if (heap == nullptr) {
        return;
    }

    if (log_malloc_list->listcount + 1 >= log_malloc_list->listsize) {
        log_malloc_list->logmalloc = (LogMalloc*)realloc(
            log_malloc_list->logmalloc, (log_malloc_list->listsize + GROWMALLOCLIST) * sizeof(LogMalloc));
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

void uda::structures::add_malloc2(LogMallocList* log_malloc_list, void* heap, int count, size_t size, const char* type, int rank,
                   int* shape)
{
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

size_t uda::structures::get_size_of(UserDefinedTypeList* user_defined_type_list, const char* type) {
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
    if ((c_udt = find_user_defined_type(user_defined_type_list, base, 0)) != nullptr) {
        auto udt = static_cast<UserDefinedType*>(c_udt);
        return (size_t)udt->size;
    }

    if (strchr(type, '*') != nullptr) {
        return sizeof(void*);
    }

    return 0;
}

USERDEFINEDTYPE* uda::structures::find_user_defined_type(UserDefinedTypeList* user_defined_type_list, const char* name, int ref_id) {
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

// Put a non malloc'd memory location on the malloc log flagging it as freed
void uda::structures::add_non_malloc(LogMallocList* log_malloc_list, void* stack, int count, size_t size, const char* type) {
    if (log_malloc_list->listcount + 1 >= log_malloc_list->listsize) {
        log_malloc_list->logmalloc = (LogMalloc*)realloc(
            log_malloc_list->logmalloc, (log_malloc_list->listsize + GROWMALLOCLIST) * sizeof(LogMalloc));
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

void uda::structures::find_malloc(LogMallocList* log_malloc_list, void* heap, int* count, int* size, const char** type)
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

    if (g_last_malloc_index >= (unsigned int)log_malloc_list->listcount) { // Defensive check
        g_last_malloc_index = 0;
        *g_last_malloc_index_value = g_last_malloc_index;
    }

    for (unsigned int i = g_last_malloc_index; i < (unsigned int)log_malloc_list->listcount; i++) {
        candidate = (VOIDTYPE)log_malloc_list->logmalloc[i].heap;
        if (target == candidate) {
            *count = log_malloc_list->logmalloc[i].count;
            *size = log_malloc_list->logmalloc[i].size;
            *type = log_malloc_list->logmalloc[i].type;
            g_last_malloc_index = i;
            *g_last_malloc_index_value = g_last_malloc_index;
            return;
        }
    }

    for (unsigned int i = 0; i < g_last_malloc_index; i++) {
        candidate = (VOIDTYPE)log_malloc_list->logmalloc[i].heap;
        if (target == candidate) {
            *count = log_malloc_list->logmalloc[i].count;
            *size = log_malloc_list->logmalloc[i].size;
            *type = log_malloc_list->logmalloc[i].type;
            g_last_malloc_index = i;
            *g_last_malloc_index_value = g_last_malloc_index;
            return;
        }
    }
}

void uda::structures::find_malloc2(LogMallocList* log_malloc_list, void* heap, int* count, int* size, const char** type, int* rank,
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

    if (g_last_malloc_index >= (unsigned int)log_malloc_list->listcount) { // Defensive check
        g_last_malloc_index = 0;
        *g_last_malloc_index_value = g_last_malloc_index;
    }

    for (unsigned int i = g_last_malloc_index; i < (unsigned int)log_malloc_list->listcount; i++) {
        candidate = (VOIDTYPE)log_malloc_list->logmalloc[i].heap;
        if (target == candidate) {
            *count = log_malloc_list->logmalloc[i].count;
            *size = log_malloc_list->logmalloc[i].size;
            *type = log_malloc_list->logmalloc[i].type;
            *rank = log_malloc_list->logmalloc[i].rank;
            if (*rank > 1) {
                *shape = log_malloc_list->logmalloc[i].shape;
            }
            g_last_malloc_index = i; // Start at the current log entry
            *g_last_malloc_index_value = g_last_malloc_index;
            return;
        }
    }

    for (unsigned int i = 0; i < g_last_malloc_index; i++) { // Start search at the first log entry
        candidate = (VOIDTYPE)log_malloc_list->logmalloc[i].heap;
        if (target == candidate) {
            *count = log_malloc_list->logmalloc[i].count;
            *size = log_malloc_list->logmalloc[i].size;
            *type = log_malloc_list->logmalloc[i].type;
            *rank = log_malloc_list->logmalloc[i].rank;
            if (*rank > 1) {
                *shape = log_malloc_list->logmalloc[i].shape;
            }
            g_last_malloc_index = i;
            *g_last_malloc_index_value = g_last_malloc_index;
            return;
        }
    }
}

void uda::structures::change_malloc(LogMallocList* log_malloc_list, VOIDTYPE old, void* anew, int count, size_t size,
                     const char* type)
{
    // Change a List Entry
    if (old == 0) {
        add_malloc(log_malloc_list, anew, count, size, type);
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

int uda::structures::find_struct_id(LogStructList* log_struct_list, void* heap, char** type) {
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

void* uda::structures::find_struct_heap(LogStructList* log_struct_list, int id, char** type) {

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

int uda::structures::get_type_of(std::string type) {
    boost::to_upper(type);
    if (type =="FLOAT") {
        return UDA_TYPE_FLOAT;
    }
    if (type =="DOUBLE") {
        return UDA_TYPE_DOUBLE;
    }
    if (type =="CHAR") {
        return UDA_TYPE_CHAR;
    }
    if (type =="SHORT") {
        return UDA_TYPE_SHORT;
    }
    if (type =="INT") {
        return UDA_TYPE_INT;
    }
    if (type =="LONG") {
        return UDA_TYPE_LONG;
    }
    if (type =="LONG64") {
        return UDA_TYPE_LONG64;
    }
    if (type =="LONG LONG") {
        return UDA_TYPE_LONG64;
    }
    if (type =="COMPLEX") {
        return UDA_TYPE_COMPLEX;
    }
    if (type =="DCOMPLEX") {
        return UDA_TYPE_DCOMPLEX;
    }
    if (type =="STRING") {
        return UDA_TYPE_STRING;
    }
    if (type =="VOID") {
        return UDA_TYPE_VOID;
    }

    if (type =="UCHAR") {
        return UDA_TYPE_UNSIGNED_CHAR;
    }
    if (type =="USHORT") {
        return UDA_TYPE_UNSIGNED_SHORT;
    }
    if (type =="UINT") {
        return UDA_TYPE_UNSIGNED_INT;
    }
    if (type =="ULONG") {
        return UDA_TYPE_UNSIGNED_LONG;
    }
#ifndef __APPLE__
    if (type =="ULONG64") {
        return UDA_TYPE_UNSIGNED_LONG64;
    }
#endif
    if (type =="UNSIGNED CHAR") {
        return UDA_TYPE_UNSIGNED_CHAR;
    }
    if (type =="UNSIGNED SHORT") {
        return UDA_TYPE_UNSIGNED_SHORT;
    }
    if (type =="UNSIGNED INT") {
        return UDA_TYPE_UNSIGNED_INT;
    }
    if (type =="UNSIGNED LONG") {
        return UDA_TYPE_UNSIGNED_LONG;
    }
#ifndef __APPLE__
    if (type =="UNSIGNED LONG64") {
        return UDA_TYPE_UNSIGNED_LONG64;
    }
    if (type =="UNSIGNED LONG LONG") {
        return UDA_TYPE_UNSIGNED_LONG64;
    }
#endif
    return UDA_TYPE_UNKNOWN; // Means Non Atomic => User defined structure type
}

void uda::structures::add_ntree(NTree* parent, NTree* child)
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

void uda::structures::free_malloc_log_list(LogMallocList* log_malloc_list)
{
    if (log_malloc_list == nullptr) {
        return;
    }
    free_malloc_log(log_malloc_list);
    if (log_malloc_list->logmalloc != nullptr) {
        free(log_malloc_list->logmalloc);
    }
    log_malloc_list->logmalloc = nullptr;
    init_log_malloc_list(log_malloc_list);
}

void uda::structures::free_malloc_log(LogMallocList* log_malloc_list)
{
    if (log_malloc_list == nullptr) {
        return;
    }
    for (int i = 0; i < log_malloc_list->listcount; i++) {
        if (log_malloc_list->logmalloc[i].freed == 0) {
            if (log_malloc_list->logmalloc[i].heap != nullptr && log_malloc_list->logmalloc[i].count > 0) {
                free(log_malloc_list->logmalloc[i].heap);
            }
            log_malloc_list->logmalloc[i].freed = 1;
            if (log_malloc_list->logmalloc[i].rank > 1 && log_malloc_list->logmalloc[i].shape != nullptr) {
                free(log_malloc_list->logmalloc[i].shape);
                log_malloc_list->logmalloc[i].shape = nullptr;
            }
        }
    }
}

void uda::structures::set_full_ntree(NTree* tree)
{
    g_full_ntree = tree;
}

void uda::structures::reset_last_malloc_index()
{
    g_last_malloc_index = 0;
    g_last_malloc_index_value = &g_last_malloc_index;
}

int uda::structures::get_alignment_of(const char* type)
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

size_t uda::structures::new_offset(size_t offset, const char* type)
{
    int alignment = get_alignment_of(type);
    return (offset + ((alignment - (offset % alignment)) % alignment));
}

size_t uda::structures::padding(size_t offset, const char* type)
{
    int alignment = get_alignment_of(type);
    return ((alignment - (offset % alignment)) % alignment);
}

size_t uda::structures::get_structure_size(UserDefinedTypeList* user_defined_type_list, UserDefinedType* user_defined_type)
{
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
            alignment = get_alignment_of("*");
        } else {
            size = get_size_of(user_defined_type_list, user_defined_type->compoundfield[i].type);
            alignment = get_alignment_of(user_defined_type->compoundfield[i].type);
        }

        size_t space = size * user_defined_type->compoundfield[i].count;

        if (i != 0) {
            if (user_defined_type->compoundfield[i].pointer) {
                offset = new_offset(offset + space0, "*");
            } else {
                offset = new_offset(offset + space0, user_defined_type->compoundfield[i].type);
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

void uda::structures::add_ntree_list(LogMallocList* log_malloc_list, NTree* tree, NTreeList* list) {
    VOIDTYPE old = (VOIDTYPE)list->forrest;
    list->forrest = (NTree*)realloc((void*)list->forrest, (++list->listCount) * sizeof(NTree*));
    change_malloc(log_malloc_list, old, list->forrest, list->listCount, sizeof(NTree*), "NTree *");
    list->forrest[list->listCount] = *tree;
}

void uda::structures::free_ntree_node(NTree* tree) {
    if (tree == nullptr) {
        return;
    }
    if (tree->branches > 0 && tree->children != nullptr) {
        for (int i = 0; i < tree->branches; i++) {
            free_ntree_node(tree->children[i]);
        }
        ::free(tree->children);
        tree->children = nullptr;
    }
}

void uda::structures::print_type_count(NTree* tree, const char* target) {
    UserDefinedType* user_defined_type = tree->userdefinedtype;
    int fieldcount = tree->userdefinedtype->fieldcount;
    for (int i = 0; i < fieldcount; i++) {
        if (!strcmp(user_defined_type->compoundfield[i].name, target)) {
            print_compound_field(user_defined_type->compoundfield[i]);
            UDA_LOG(UDA_LOG_DEBUG, "{}[ {} ]", target, user_defined_type->compoundfield[i].count);
        }
    }
}

NTree* uda::structures::get_full_ntree() {
    return g_full_ntree;
}

void uda::structures::set_last_malloc_index_value(unsigned int* last_malloc_index_value) {
    g_last_malloc_index_value = last_malloc_index_value;
    g_last_malloc_index = *last_malloc_index_value;
}

void uda::structures::print_node(NTree* tree) {
    if (tree == nullptr) {
        tree = g_full_ntree;
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

void uda::structures::print_node_atomic(LogMallocList* log_malloc_list, NTree* tree) {
    if (tree == nullptr) {
        tree = g_full_ntree;
    }
    int name_count = get_node_atomic_count(tree);                   // Count of all local atomic data
    char** name_list = get_node_atomic_names(log_malloc_list, tree); // Names
    for (int i = 0; i < name_count; i++) {
        print_atomic_type(log_malloc_list, tree, name_list[i]);
    }
}

void uda::structures::print_node_names(LogMallocList* log_malloc_list, NTree* tree) {
    if (tree == nullptr) {
        tree = g_full_ntree;
    }

    UDA_LOG(UDA_LOG_DEBUG, "\nData Node Structure Names and Types");
    int name_count = get_node_atomic_count(tree);                   // Count of all local data structures
    char** name_list = get_node_atomic_names(log_malloc_list, tree); // Names
    char** type_list = get_node_atomic_types(log_malloc_list, tree); // Types
    UDA_LOG(UDA_LOG_DEBUG, "Structure Count {}", name_count);
    if (name_count > 0) {
        UDA_LOG(UDA_LOG_DEBUG, "  #\tName\tType");
        for (int i = 0; i < name_count; i++) {
            UDA_LOG(UDA_LOG_DEBUG, "[{}]\t{}\t{}", i, name_list[i], type_list[i]);
        }
    }
    UDA_LOG(UDA_LOG_DEBUG, "\nData Node Atomic Names and Types");
    name_count = get_node_atomic_count(tree);                   // Count of all local atomic data
    name_list = get_node_atomic_names(log_malloc_list, tree); // Names
    type_list = get_node_atomic_types(log_malloc_list, tree); // Types
    UDA_LOG(UDA_LOG_DEBUG, "Atomic Count {}", name_count);
    if (name_count > 0) {
        UDA_LOG(UDA_LOG_DEBUG, "  #\tName\tType");
        for (int i = 0; i < name_count; i++) {
            UDA_LOG(UDA_LOG_DEBUG, "[{}]\t{}\t{}", i, name_list[i], type_list[i]);
        }
    }
}

void uda::structures::print_ntree2(NTree* tree) {
    if (tree == nullptr) {
        tree = g_full_ntree;
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
        print_ntree2(tree->children[i]);
    }
}

void uda::structures::print_ntree(UserDefinedTypeList* user_defined_type_list, NTree* tree) {
    if (tree == nullptr) {
        tree = g_full_ntree;
    }
    UDA_LOG(UDA_LOG_DEBUG, "--------------------------------------------------------------------");
#ifdef A64
    UDA_LOG(UDA_LOG_DEBUG, "\nNTree Node {} ({}) Contents", (UVOIDTYPE)tree, (UVOIDTYPE)tree);
#else
    UDA_LOG(UDA_LOG_DEBUG, "\nNTree Node {} ({}) Contents", (UVOIDTYPE)tree, (UVOIDTYPE)tree);
#endif
    UDA_LOG(UDA_LOG_DEBUG, "Name: {}", tree->name);
    UDA_LOG(UDA_LOG_DEBUG, "Children: {}", tree->branches);
    print_user_defined_type_table(user_defined_type_list, *tree->userdefinedtype);
    for (int i = 0; i < tree->branches; i++) {
        print_ntree(user_defined_type_list, tree->children[i]);
    }
}

void uda::structures::print_ntree_list(NTree* tree) {
    if (tree == nullptr) {
        tree = g_full_ntree;
    }
#ifdef A64
    UDA_LOG(UDA_LOG_DEBUG, "{}\t({})\t{}\t{}\t{}", (UVOIDTYPE)tree, (UVOIDTYPE)tree, tree->name,
            tree->userdefinedtype->name, tree->branches);
#else
    UDA_LOG(UDA_LOG_DEBUG, "{}\t({})\t{}\t{}\t{}", (UVOIDTYPE)tree, (UVOIDTYPE)tree, tree->name,
            tree->userdefinedtype->name, tree->branches);
#endif
    for (int i = 0; i < tree->branches; i++) {
        print_ntree_list(tree->children[i]);
    }
}

void uda::structures::print_ntree_structure_names(LogMallocList* log_malloc_list, NTree* tree) {
    if (tree == nullptr) {
        tree = g_full_ntree;
    }
#ifdef A64
    UDA_LOG(UDA_LOG_DEBUG, "\nData Tree {} Structure Names and Types", (UVOIDTYPE)tree);
#else
    UDA_LOG(UDA_LOG_DEBUG, "\nData Tree {} Structure Names and Types", (UVOIDTYPE)tree);
#endif
    int name_count = get_ntree_structure_count(tree);                   // Count of all Tree Nodes
    char** name_list = get_ntree_structure_names(log_malloc_list, tree); // Names of all user defined data structures
    char** type_list = get_ntree_structure_types(log_malloc_list, tree); // Types of all user defined data structures
    UDA_LOG(UDA_LOG_DEBUG, "Total Structure Count {}", name_count);
    UDA_LOG(UDA_LOG_DEBUG, "  #\tName\tType");
    for (int i = 0; i < name_count; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "[{}]\t{}\t{}", i, name_list[i], type_list[i]);
    }
}

void uda::structures::print_ntree_structure_component_names(LogMallocList* log_malloc_list, NTree* tree) {
    if (tree == nullptr) {
        tree = g_full_ntree;
    }
    UDA_LOG(UDA_LOG_DEBUG, "\nData Tree Structure Component Names, Types and Descriptions");
    const int name_count = get_ntree_structure_component_count(tree);                   // Count of all Tree Nodes
    char** name_list = get_ntree_structure_component_names(log_malloc_list, tree); // Names of all structure elements
    char** type_list = get_ntree_structure_component_types(log_malloc_list, tree); // Types of all structure elements
    char** desc_list = get_ntree_structure_component_descriptions(log_malloc_list, tree); // Descriptions of all structure elements
    UDA_LOG(UDA_LOG_DEBUG, "Total Structure Component Count {}", name_count);
    UDA_LOG(UDA_LOG_DEBUG, "  #\tName\tType\tDescription");
    for (int i = 0; i < name_count; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "[{}]\t{}\t{}\t{}", i, name_list[i], type_list[i], desc_list[i]);
    }
}

void uda::structures::add_image(char** image, int* imagecount, const char* line) {
    int len;
    void* old = *image;
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

void uda::structures::expand_image(char* buffer, char defnames[MAXELEMENTS][MAXELEMENTNAME], int* defvalues, int defCount,
                    char* expand) {
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

void uda::structures::free_log_struct_list(LogStructList* log_struct_list) {
    free(log_struct_list->logstruct);
    init_log_struct_list(log_struct_list);
}

int uda::structures::get_node_structure_count(NTree* tree) {
    int count = 0;
    if (tree == nullptr) {
        tree = g_full_ntree;
    }

    for (int i = 0; i < tree->userdefinedtype->fieldcount; i++) {
        if (tree->userdefinedtype->compoundfield[i].atomictype == UDA_TYPE_UNKNOWN) {
            count++;
        }
    }
    return count;
}

char** uda::structures::get_node_structure_names(LogMallocList* log_malloc_list, NTree* tree) {
    int count;
    char** names;
    if (tree == nullptr) {
        tree = g_full_ntree;
    }
    if ((count = get_node_structure_count(tree)) == 0) {
        return nullptr;
    }
    names = (char**)malloc(count * sizeof(char*));
    add_malloc(log_malloc_list, names, count, sizeof(char*), "char *");
    count = 0;

    for (int i = 0; i < tree->userdefinedtype->fieldcount; i++) {
        if (tree->userdefinedtype->compoundfield[i].atomictype == UDA_TYPE_UNKNOWN) {
            names[count++] = tree->userdefinedtype->compoundfield[i].name;
        }
    }
    return names;
}

char** uda::structures::get_node_structure_types(LogMallocList* log_malloc_list, NTree* tree) {
    int count;
    char** names;
    if (tree == nullptr) {
        tree = g_full_ntree;
    }
    if ((count = get_node_structure_count(tree)) == 0) {
        return nullptr;
    }
    names = (char**)malloc(count * sizeof(char*));
    add_malloc(log_malloc_list, names, count, sizeof(char*), "char *");

    count = 0;
    for (int i = 0; i < tree->userdefinedtype->fieldcount; i++) {
        if (tree->userdefinedtype->compoundfield[i].atomictype == UDA_TYPE_UNKNOWN) {
            names[count++] = tree->userdefinedtype->compoundfield[i].type;
        }
    }
    return names;
}

int uda::structures::get_node_atomic_count(NTree* tree) {
    int count = 0;
    if (tree == nullptr) {
        tree = g_full_ntree;
    }

    for (int i = 0; i < tree->userdefinedtype->fieldcount; i++) {
        if (tree->userdefinedtype->compoundfield[i].atomictype != UDA_TYPE_UNKNOWN) {
            count++;
        }
    }
    return count;
}

char** uda::structures::get_node_atomic_names(LogMallocList* log_malloc_list, NTree* tree) {
    int count;
    char** names;
    if (tree == nullptr) {
        tree = g_full_ntree;
    }
    if ((count = get_node_atomic_count(tree)) == 0) {
        return nullptr;
    }
    names = (char**)malloc(count * sizeof(char*));
    add_malloc(log_malloc_list, names, count, sizeof(char*), "char *");

    count = 0;
    for (int i = 0; i < tree->userdefinedtype->fieldcount; i++) {
        if (tree->userdefinedtype->compoundfield[i].atomictype != UDA_TYPE_UNKNOWN) {
            names[count++] = tree->userdefinedtype->compoundfield[i].name;
        }
    }
    return names;
}

char** uda::structures::get_node_atomic_types(LogMallocList* log_malloc_list, NTree* tree) {
    int count;
    char** names;
    if (tree == nullptr) {
        tree = g_full_ntree;
    }
    if ((count = get_node_atomic_count(tree)) == 0) {
        return nullptr;
    }
    names = (char**)malloc(count * sizeof(char*));
    add_malloc(log_malloc_list, names, count, sizeof(char*), "char *");

    count = 0;
    for (int i = 0; i < tree->userdefinedtype->fieldcount; i++) {
        if (tree->userdefinedtype->compoundfield[i].atomictype != UDA_TYPE_UNKNOWN) {
            names[count++] = tree->userdefinedtype->compoundfield[i].type;
        }
    }
    return names;
}

int uda::structures::get_ntree_structure_count(NTree* tree) {
    int count = 1;
    if (tree == nullptr) {
        tree = g_full_ntree;
    }
    if (tree->branches == 0) {
        return count;
    }
    for (int i = 0; i < tree->branches; i++) {
        if (i == 0 ||
            strcmp(tree->children[i]->userdefinedtype->name, tree->children[i - 1]->userdefinedtype->name) != 0) {
            count = count + get_ntree_structure_count(tree->children[i]);
        }
    }
    return count;
}

char** uda::structures::get_ntree_structure_names(LogMallocList* log_malloc_list, NTree* tree) {
    if (tree == nullptr) {
        tree = g_full_ntree;
    }

    auto names = (char**)malloc(sizeof(char*));
    add_malloc(log_malloc_list, names, 1, sizeof(char*), "char *");

    names[0] = tree->name;

    if (tree->branches == 0) {
        return names;
    }

    int count = 1;
    for (int i = 0; i < tree->branches; i++) {
        if (i == 0 ||
            strcmp(tree->children[i]->userdefinedtype->name, tree->children[i - 1]->userdefinedtype->name) != 0) {
            int childcount = get_ntree_structure_count(tree->children[i]);
            VOIDTYPE old = (VOIDTYPE)names;
            names = (char**)realloc(names, (count + childcount) * sizeof(char*));
            change_malloc(log_malloc_list, old, names, (count + childcount), sizeof(char*), "char *");
            char** childnames = get_ntree_structure_names(log_malloc_list, tree->children[i]);
            for (int j = 0; j < childcount; j++) {
                names[count + j] = childnames[j];
            }
            count += childcount;
            }
    }
    return names;
}

char** uda::structures::get_ntree_structure_types(LogMallocList* log_malloc_list, NTree* tree) {
    if (tree == nullptr) {
        tree = g_full_ntree;
    }

    auto names = (char**)malloc(sizeof(char*));
    add_malloc(log_malloc_list, names, 1, sizeof(char*), "char *");

    names[0] = tree->userdefinedtype->name;
    if (tree->branches == 0) {
        return names;
    }

    int count = 1;
    for (int i = 0; i < tree->branches; i++) {
        if (i == 0 ||
            strcmp(tree->children[i]->userdefinedtype->name, tree->children[i - 1]->userdefinedtype->name) != 0) {
            int childcount = get_ntree_structure_count(tree->children[i]);
            VOIDTYPE old = (VOIDTYPE)names;
            names = (char**)realloc(names, (count + childcount) * sizeof(char*));
            change_malloc(log_malloc_list, old, names, (count + childcount), sizeof(char*), "char *");
            char** childnames = get_ntree_structure_types(log_malloc_list, tree->children[i]);
            for (int j = 0; j < childcount; j++) {
                names[count + j] = childnames[j];
            }
            count = count + childcount;
            }
    }
    return names;
}

int uda::structures::get_ntree_structure_component_count(NTree* tree) {
    int count;
    if (tree == nullptr) {
        tree = g_full_ntree;
    }

    count = tree->userdefinedtype->fieldcount;

    for (int i = 0; i < tree->branches; i++) {
        if (i == 0 ||
            strcmp(tree->children[i]->userdefinedtype->name, tree->children[i - 1]->userdefinedtype->name) != 0) {
            count = count + get_ntree_structure_component_count(tree->children[i]);
        }
    }
    return count;
}

char** uda::structures::get_ntree_structure_component_names(LogMallocList* log_malloc_list, NTree* tree) {
    if (tree == nullptr) {
        tree = g_full_ntree;
    }

    int count = tree->userdefinedtype->fieldcount;
    if (count == 0) {
        return nullptr;
    }

    char** names = get_node_structure_component_names(tree);

    for (int i = 0; i < tree->branches; i++) {
        if (i == 0 ||
            strcmp(tree->children[i]->userdefinedtype->name, tree->children[i - 1]->userdefinedtype->name) != 0) {
            int childcount = get_ntree_structure_component_count(tree->children[i]);
            auto old = (VOIDTYPE)names;
            names = (char**)realloc(names, (count + childcount) * sizeof(char*));
            change_malloc(log_malloc_list, old, names, (count + childcount), sizeof(char*), "char *");
            char** childnames = get_ntree_structure_component_names(log_malloc_list, tree->children[i]);
            for (int j = 0; j < childcount; j++) {
                names[count + j] = childnames[j];
            }
            count += childcount;
            }
    }
    return names;
}

char** uda::structures::get_ntree_structure_component_types(LogMallocList* log_malloc_list, NTree* tree) {
    if (tree == nullptr) {
        tree = g_full_ntree;
    }

    int count = tree->userdefinedtype->fieldcount;
    if (count == 0) {
        return nullptr;
    }

    char** names = get_node_structure_component_types(tree);

    for (int i = 0; i < tree->branches; i++) {
        if (i == 0 ||
            strcmp(tree->children[i]->userdefinedtype->name, tree->children[i - 1]->userdefinedtype->name) != 0) {
            int childcount = get_ntree_structure_component_count(tree->children[i]);
            auto old = (VOIDTYPE)names;
            names = (char**)realloc(names, (count + childcount) * sizeof(char*));
            change_malloc(log_malloc_list, old, names, (count + childcount), sizeof(char*), "char *");
            char** childnames = get_ntree_structure_component_types(log_malloc_list, tree->children[i]);
            for (int j = 0; j < childcount; j++) {
                names[count + j] = childnames[j];
            }
            count += childcount;
            }
    }
    return names;
}

char** uda::structures::get_ntree_structure_component_descriptions(LogMallocList* log_malloc_list, NTree* tree) {
    if (tree == nullptr) {
        tree = g_full_ntree;
    }

    int count = tree->userdefinedtype->fieldcount;
    if (count == 0) {
        return nullptr;
    }

    char** names = get_node_structure_component_descriptions(tree);

    for (int i = 0; i < tree->branches; i++) {
        if (i == 0 ||
            strcmp(tree->children[i]->userdefinedtype->name, tree->children[i - 1]->userdefinedtype->name) != 0) {
            int childcount = get_ntree_structure_component_count(tree->children[i]);
            auto old = (VOIDTYPE)names;
            names = (char**)realloc(names, (count + childcount) * sizeof(char*));
            change_malloc(log_malloc_list, old, names, (count + childcount), sizeof(char*), "char *");
            char** childnames = get_ntree_structure_component_descriptions(log_malloc_list, tree->children[i]);
            for (int j = 0; j < childcount; j++) {
                names[count + j] = childnames[j];
            }
            count += childcount;
            }
    }
    return names;
}

void uda::structures::print_atomic_type(LogMallocList* log_malloc_list, NTree* tree, const char* target) {
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
                    find_malloc(log_malloc_list, &data, &count, &size, &type);
                    if (count > 0) {
                        print_atomic_data(data, get_type_of(type), count, target);
                    }
                    } else {
                        data = (void*)&p[user_defined_type->compoundfield[i].offset];
                        print_atomic_data(data, user_defined_type->compoundfield[i].atomictype,
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

void uda::structures::print_atomic_data(void* data, int atomictype, int count, const char* label) {
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

int uda::structures::get_node_structure_component_count(NTree* tree) {
    if (tree == nullptr) {
        tree = g_full_ntree;
    }

    return tree->userdefinedtype->fieldcount;
}

char** uda::structures::get_node_structure_component_names(NTree* tree) {
    int count;
    char** names;
    if (tree == nullptr) {
        tree = g_full_ntree;
    }

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

char** uda::structures::get_node_structure_component_types(NTree* tree) {
    int count;
    char** names;
    if (tree == nullptr) {
        tree = g_full_ntree;
    }

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

char** uda::structures::get_node_structure_component_descriptions(NTree* tree) {
    int count;
    char** names;
    if (tree == nullptr) {
        tree = g_full_ntree;
    }

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

/** Free heap from a Compound Field.
 *
 * @param str The Compound Field.
 * @return void.
 */
void free_compound_field(CompoundField* compound_field)
{
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
void free_user_defined_type(UserDefinedType* user_defined_type)
{
    if (user_defined_type == nullptr) {
        return;
    }
    for (int i = 0; i < user_defined_type->fieldcount; i++) {
        free_compound_field(&user_defined_type->compoundfield[i]);
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
void uda::structures::free_user_defined_type_list(UserDefinedTypeList* user_defined_type_list)
{
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
        free_user_defined_type(&user_defined_type_list->userdefinedtype[i]);
    }
    free(user_defined_type_list->userdefinedtype);
    init_user_defined_type_list(user_defined_type_list);
}

/** Free allocated heap memory and reinitialise a new log_malloc_list-> There are no arguments.
 *
 * @return void.
 */
void uda::structures::free_log_malloc_list(LogMallocList* log_malloc_list)
{
    free_malloc_log_list(log_malloc_list);
}
