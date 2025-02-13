#include <boost/algorithm/string.hpp>
#include <sstream>
#include <string>
#include <vector>
#include <uda/plugins.h>
#include <uda/structured.h>
#include <uda/types.h>

#include "clientserver/error_log.h"
#include "clientserver/init_structs.h"
#include "clientserver/make_request_block.h"
#include "clientserver/parse_xml.h"
#include "clientserver/type_convertor.hpp"
#include "common/string_utils.h"
#include "config/config.h"
#include "logging/logging.h"
#include "server2/plugins.hpp"
#include "server2/server_subset_data.h"
#include "structures/accessors.h"
#include "structures/struct.h"

using namespace uda::client_server;
using namespace uda::server;
using namespace uda::logging;
using namespace uda::structures;
using namespace uda::config;
using namespace uda::common;

static int find_plugin_id_by_format(std::string_view format, const std::vector<PluginData>& plugin_list)
{
    size_t id = 0;
    for (const auto& plugin : plugin_list) {
        if (plugin.name == format) {
            return id;
        }
        ++id;
    }
    return -1;
}

UDA_PLUGIN_INTERFACE* udaCreatePluginInterface(UDA_PLUGIN_INTERFACE* plugin_interface, const char* request)
{
    auto old_plugin_interface = (UdaPluginInterface*)plugin_interface;
    auto new_plugin_interface = (UdaPluginInterface*)calloc(1, sizeof(UdaPluginInterface));

    auto config = old_plugin_interface->config;
    auto plugin_list = old_plugin_interface->pluginList;

    auto request_data = (RequestData*)calloc(1, sizeof(RequestData));
    copy_string(request, request_data->signal, MaxMeta);
    make_request_data(*config, request_data, *plugin_list);

    auto user_defined_type_list = (UserDefinedTypeList*)calloc(1, sizeof(UserDefinedTypeList));
    auto log_malloc_list = (LogMallocList*)calloc(1, sizeof(LogMallocList));

    new_plugin_interface->request_data = request_data;
    new_plugin_interface->pluginList = plugin_list;
    new_plugin_interface->config = config;
    new_plugin_interface->user_defined_type_list = user_defined_type_list;
    new_plugin_interface->log_malloc_list = log_malloc_list;

    new_plugin_interface->interface_version = 1;
    new_plugin_interface->house_keeping = false;
    new_plugin_interface->change_plugin = false;
    new_plugin_interface->error_stack = {};

    return new_plugin_interface;
}

void udaFreePluginInterface(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    const auto* interface = static_cast<UdaPluginInterface*>(plugin_interface);
    free(interface->request_data);
    free_user_defined_type_list(interface->user_defined_type_list);
    free(interface->user_defined_type_list);
    free_malloc_log_list(interface->log_malloc_list);
    free(interface->log_malloc_list);
    free(plugin_interface);
}

template <typename T> int setReturnDataScalar(UDA_PLUGIN_INTERFACE* plugin_interface, T value, const char* description)
{
    const auto* interface = static_cast<UdaPluginInterface*>(plugin_interface);
    DataBlock* data_block = interface->data_block;
    init_data_block(data_block);

    auto data = (T*)malloc(sizeof(T));
    data[0] = value;

    if (description != nullptr) {
        strncpy(data_block->data_desc, description, StringLength);
        data_block->data_desc[StringLength - 1] = '\0';
    }

    data_block->rank = 0;
    data_block->data_type = TypeConvertor<T>::type;
    data_block->data = (char*)data;
    data_block->data_n = 1;

    return 0;
}

template <typename T>
int setReturnDataArray(UDA_PLUGIN_INTERFACE* plugin_interface, const T* values, size_t rank, const size_t* shape,
                       const char* description)
{
    const auto* interface = static_cast<UdaPluginInterface*>(plugin_interface);
    DataBlock* data_block = interface->data_block;
    init_data_block(data_block);

    if (description != nullptr) {
        strncpy(data_block->data_desc, description, StringLength);
        data_block->data_desc[StringLength - 1] = '\0';
    }

    data_block->rank = (int)rank;
    data_block->dims = (Dims*)malloc(rank * sizeof(Dims));

    size_t len = 1;

    for (size_t i = 0; i < rank; ++i) {
        init_dim_block(&data_block->dims[i]);

        data_block->dims[i].data_type = UDA_TYPE_UNSIGNED_INT;
        data_block->dims[i].dim_n = (int)shape[i];
        data_block->dims[i].compressed = 1;
        data_block->dims[i].dim0 = 0.0;
        data_block->dims[i].diff = 1.0;
        data_block->dims[i].method = 0;

        len *= shape[i];
    }

    auto data = (T*)malloc(len * sizeof(T));
    memcpy(data, values, len * sizeof(T));

    data_block->data_type = TypeConvertor<T>::type;
    data_block->data = (char*)data;
    data_block->data_n = (int)len;

    return 0;
}

#define UDA_IMPL_SET_RETURN_FUNCS(NAME, TYPE)                                                                          \
    int udaPluginReturnData##NAME##Scalar(UDA_PLUGIN_INTERFACE* plugin_interface, const TYPE value,                    \
                                          const char* description)                                                     \
    {                                                                                                                  \
        return setReturnDataScalar<TYPE>(plugin_interface, value, description);                                        \
    }                                                                                                                  \
    int udaPluginReturnData##NAME##Array(UDA_PLUGIN_INTERFACE* plugin_interface, const TYPE* values, size_t rank,      \
                                         const size_t* shape, const char* description)                                 \
    {                                                                                                                  \
        return setReturnDataArray<TYPE>(plugin_interface, values, rank, shape, description);                           \
    }

UDA_IMPL_SET_RETURN_FUNCS(Float, float)
UDA_IMPL_SET_RETURN_FUNCS(Double, double)
UDA_IMPL_SET_RETURN_FUNCS(Char, char)
UDA_IMPL_SET_RETURN_FUNCS(UChar, unsigned char)
UDA_IMPL_SET_RETURN_FUNCS(Short, short)
UDA_IMPL_SET_RETURN_FUNCS(UShort, unsigned short)
UDA_IMPL_SET_RETURN_FUNCS(Int, int)
UDA_IMPL_SET_RETURN_FUNCS(UInt, unsigned int)
UDA_IMPL_SET_RETURN_FUNCS(Long, long)
UDA_IMPL_SET_RETURN_FUNCS(ULong, unsigned long)

#undef UDA_IMPL_SET_RETURN_FUNCS

int udaPluginReturnDataStringScalar(UDA_PLUGIN_INTERFACE* plugin_interface, const char* value, const char* description)
{
    const auto* interface = static_cast<UdaPluginInterface*>(plugin_interface);
    DataBlock* data_block = interface->data_block;
    init_data_block(data_block);

    data_block->data_type = UDA_TYPE_STRING;
    data_block->data = strdup(value);

    data_block->rank = 1;
    data_block->dims = (Dims*)malloc(data_block->rank * sizeof(Dims));

    for (unsigned int i = 0; i < data_block->rank; i++) {
        init_dim_block(&data_block->dims[i]);
    }

    if (description != nullptr) {
        strncpy(data_block->data_desc, description, StringLength);
        data_block->data_desc[StringLength - 1] = '\0';
    }

    data_block->dims[0].data_type = UDA_TYPE_UNSIGNED_INT;
    data_block->dims[0].dim_n = (int)strlen(value) + 1;
    data_block->dims[0].compressed = 1;
    data_block->dims[0].dim0 = 0.0;
    data_block->dims[0].diff = 1.0;
    data_block->dims[0].method = 0;

    data_block->data_n = data_block->dims[0].dim_n;

    return 0;
}

int udaPluginReturnDataStringArray(UDA_PLUGIN_INTERFACE* plugin_interface, const char** values, size_t rank,
                                               const size_t* shape, const char* description)
{
    const auto* interface = static_cast<UdaPluginInterface*>(plugin_interface);
    DataBlock* data_block = interface->data_block;
    init_data_block(data_block);

    // TODO: only rank = 1 handled
    if (rank != 1) {
        return 0;
    }

    size_t count = shape[0];

    size_t max_len = 0;
    for (size_t i = 0; i < count; ++i) {
        max_len = std::max(max_len, strlen(values[i]) + 1);
    }

    char* data = (char*)malloc(max_len * count * sizeof(char));
    for (size_t i = 0; i < count; ++i) {
        strcpy(&data[i * max_len], values[i]);
    }

    data_block->data_type = UDA_TYPE_STRING;
    data_block->data = data;

    data_block->rank = 2;
    data_block->dims = (Dims*)malloc(data_block->rank * sizeof(Dims));

    for (unsigned int i = 0; i < data_block->rank; i++) {
        init_dim_block(&data_block->dims[i]);
    }

    if (description != nullptr) {
        strncpy(data_block->data_desc, description, StringLength);
        data_block->data_desc[StringLength - 1] = '\0';
    }

    data_block->dims[0].data_type = UDA_TYPE_UNSIGNED_INT;
    data_block->dims[0].dim_n = max_len;
    data_block->dims[0].compressed = 1;
    data_block->dims[0].dim0 = 0.0;
    data_block->dims[0].diff = 1.0;
    data_block->dims[0].method = 0;

    data_block->dims[1].data_type = UDA_TYPE_UNSIGNED_INT;
    data_block->dims[1].dim_n = count;
    data_block->dims[1].compressed = 1;
    data_block->dims[1].dim0 = 0.0;
    data_block->dims[1].diff = 1.0;
    data_block->dims[1].method = 0;

    data_block->data_n = max_len * count;

    return 0;
}

int udaPluginReturnData(UDA_PLUGIN_INTERFACE* plugin_interface, void* value, size_t size, UDA_TYPE type, int rank,
                        const int* shape, const char* description)
{
    const auto* interface = static_cast<UdaPluginInterface*>(plugin_interface);
    DataBlock* data_block = interface->data_block;
    init_data_block(data_block);

    data_block->data_type = type;
    data_block->data = (char*)malloc(size);
    data_block->data_n = size;

    memcpy(data_block->data, value, size);

    data_block->rank = rank;
    data_block->dims = (Dims*)malloc(data_block->rank * sizeof(Dims));

    for (unsigned int i = 0; i < data_block->rank; i++) {
        init_dim_block(&data_block->dims[i]);
    }

    if (description != nullptr) {
        strncpy(data_block->data_desc, description, StringLength);
        data_block->data_desc[StringLength - 1] = '\0';
    }

    for (unsigned int i = 0; i < data_block->rank; i++) {
        data_block->dims[i].data_type = UDA_TYPE_UNSIGNED_INT;
        data_block->dims[i].dim_n = shape[i];
        data_block->dims[i].compressed = 1;
        data_block->dims[i].dim0 = 0.0;
        data_block->dims[i].diff = 1.0;
        data_block->dims[i].method = 0;
    }

    return 0;
}

int udaPluginArgumentCount(const UDA_PLUGIN_INTERFACE* plugin_interface)
{
    const auto* interface = static_cast<const UdaPluginInterface*>(plugin_interface);
    return interface->request_data->name_value_list.size();
}

const char* udaPluginArgument(const UDA_PLUGIN_INTERFACE* plugin_interface, int num)
{
    size_t s_num = num;
    const auto* interface = static_cast<const UdaPluginInterface*>(plugin_interface);
    if (s_num > 0 && s_num < interface->request_data->name_value_list.size()) {
        return interface->request_data->name_value_list.name(s_num).c_str();
    }
    return nullptr;
}

/**
 * Look for an argument with the given name in the provided name_value_list and return it's associated value.
 *
 * If the argument is found the value associated with the argument is provided via the value parameter and the function
 * returns 1. Otherwise value is set to nullptr and the function returns 0.
 * @param name_value_list
 * @param value
 * @param name
 * @return
 */
bool udaPluginFindStringArg(const UDA_PLUGIN_INTERFACE* plugin_interface, const char** value, const char* name)
{
    const auto interface = static_cast<const UdaPluginInterface*>(plugin_interface);
    const auto name_value_list = &interface->request_data->name_value_list;

    std::vector<std::string> names;
    boost::split(names, name, boost::is_any_of("|"));

    bool found = false;
    for (const auto& el : names) {
        *value = name_value_list->find(el);
        if (*value != nullptr) {
            found = true;
            break;
        }
    }

    return found;
}

template <typename T>
bool findArg(const UDA_PLUGIN_INTERFACE* plugin_interface, T* value, const char* name)
{
    const char* str;
    const bool found = udaPluginFindStringArg(plugin_interface, &str, name);
    if (found) {
        std::stringstream ss(str);
        ss >> *value;
    }
    return found;
}

template <typename T>
bool findArrayArg(const UDA_PLUGIN_INTERFACE* plugin_interface, T** values, size_t* nvalues, const char* name)
{
    const char* str;
    const bool found = udaPluginFindStringArg(plugin_interface, &str, name);
    if (found) {
        std::vector<std::string> tokens;
        boost::split(tokens, str, boost::is_any_of(";"), boost::token_compress_on);
        *values = static_cast<T*>(calloc(tokens.size(), sizeof(T)));
        size_t n = 0;
        for (const auto& token : tokens) {
            std::stringstream ss(token);
            ss >> (*values)[n];
            ++n;
        }
        *nvalues = tokens.size();
    }
    return found;
}

#define UDA_IMPL_FIND_FUNCS(NAME, TYPE)                                                                                \
    bool udaPluginFind##NAME##Arg(const UDA_PLUGIN_INTERFACE* plugin_interface, TYPE* value, const char* name)         \
    {                                                                                                                  \
        return findArg<TYPE>(plugin_interface, value, name);                                                           \
    }                                                                                                                  \
    bool udaPluginFind##NAME##ArrayArg(const UDA_PLUGIN_INTERFACE* plugin_interface, TYPE** value, size_t* nvalues,    \
                                       const char* name)                                                               \
    {                                                                                                                  \
        return findArrayArg<TYPE>(plugin_interface, value, nvalues, name);                                             \
    }

UDA_IMPL_FIND_FUNCS(Float, float)
UDA_IMPL_FIND_FUNCS(Double, double)
UDA_IMPL_FIND_FUNCS(Char, char)
UDA_IMPL_FIND_FUNCS(UChar, unsigned char)
UDA_IMPL_FIND_FUNCS(Short, short)
UDA_IMPL_FIND_FUNCS(UShort, unsigned short)
UDA_IMPL_FIND_FUNCS(Int, int)
UDA_IMPL_FIND_FUNCS(UInt, unsigned int)
UDA_IMPL_FIND_FUNCS(Long, long)
UDA_IMPL_FIND_FUNCS(ULong, unsigned long)

bool udaPluginFindArg(const UDA_PLUGIN_INTERFACE* plugin_interface, const char* name)
{
    const auto interface = static_cast<const UdaPluginInterface*>(plugin_interface);
    const auto name_value_list = &interface->request_data->name_value_list;

    std::vector<std::string> names;
    boost::split(names, name, boost::is_any_of("|"));

    bool found = false;
    for (const auto& el : names) {
        if (name_value_list->contains(el)) {
            found = true;
            break;
        }
    }

    return found;
}

void udaExpandEnvironmentalVariables(char* path)
{
    uda::client_server::expand_environmental_variables(path);
}

int udaCallPlugin(UDA_PLUGIN_INTERFACE* plugin_interface, const char* request)
{
    return udaCallPlugin2(plugin_interface, request, "");
}

int udaCallPlugin2(UDA_PLUGIN_INTERFACE* plugin_interface, const char* request, const char* source)
{
    const auto* interface = static_cast<UdaPluginInterface*>(plugin_interface);
    UdaPluginInterface new_plugin_interface = *interface;
    RequestData request_data = *interface->request_data;
    new_plugin_interface.request_data = &request_data;

    strcpy(request_data.signal, request);
    strcpy(request_data.source, source);

    make_request_data(*interface->config, &request_data, *interface->pluginList);

    request_data.request = find_plugin_id_by_format(request_data.format, *interface->pluginList);
    if (request_data.request == -1) {
        UDA_RAISE_PLUGIN_ERROR(plugin_interface, "Plugin not found!")
    }

    int err = 0;
    int id = request_data.request;
    if (id >= 0) {
        auto& plugin = (*interface->pluginList)[request_data.request];
        if (plugin.handle && plugin.entry_func)
        err = plugin.entry_func(&new_plugin_interface); // Call the data reader
    } else {
        UDA_RAISE_PLUGIN_ERROR(plugin_interface, "Data Access is not available for this data request!")
    }

    // Apply sub-setting
    if (request_data.datasubset.nbound > 0) {
        Action action = {};
        init_action(&action);
        action.actionType = (int)ActionType::Subset;
        action.subset = request_data.datasubset;
        err = server_subset_data(new_plugin_interface.data_block, action, new_plugin_interface.log_malloc_list);
    }

    return err;
}

int udaPluginReturnCompoundData(UDA_PLUGIN_INTERFACE* plugin_interface, char* data, const char* user_type,
                                const char* description)
{
    const auto* interface = static_cast<UdaPluginInterface*>(plugin_interface);
    DataBlock* data_block = interface->data_block;
    init_data_block(data_block);

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = data;

    strcpy(data_block->data_desc, "Local UDA server time");
    strcpy(data_block->data_label, "servertime");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)find_user_defined_type(interface->user_defined_type_list, user_type, 0);

    if (description != nullptr) {
        strncpy(data_block->data_desc, description, StringLength);
        data_block->data_desc[StringLength - 1] = '\0';
    }

    return 0;
}

int udaPluginReturnCompoundArrayData(UDA_PLUGIN_INTERFACE* plugin_interface, char* data, const char* user_type,
                                     const char* description, int rank, int* shape)
{
    const auto* interface = static_cast<UdaPluginInterface*>(plugin_interface);
    DataBlock* data_block = interface->data_block;
    init_data_block(data_block);

    int count = 1;
    for (int i = 0; i < rank; ++i) {
        count *= shape[i];
    }

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = rank;
    data_block->data_n = count;
    data_block->data = data;
    data_block->dims = (Dims*)malloc(rank * sizeof(Dims));

    for (int i = 0; i < rank; ++i) {
        init_dim_block(&data_block->dims[i]);

        data_block->dims[i].data_type = UDA_TYPE_UNSIGNED_INT;
        data_block->dims[i].dim_n = (int)shape[i];
        data_block->dims[i].compressed = 1;
        data_block->dims[i].dim0 = 0.0;
        data_block->dims[i].diff = 1.0;
        data_block->dims[i].method = 0;
    }

    strcpy(data_block->data_desc, "Local UDA server time");
    strcpy(data_block->data_label, "servertime");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = find_user_defined_type(interface->user_defined_type_list, user_type, 0);

    if (description != nullptr) {
        strncpy(data_block->data_desc, description, StringLength);
        data_block->data_desc[StringLength - 1] = '\0';
    }

    return 0;
}

COMPOUNDFIELD* udaNewCompoundField(const char* name, const char* description, int* offset, int type, int rank, int* shape)
{
    CompoundField* field = (CompoundField*)malloc(sizeof(CompoundField));
    defineField(field, name, description, offset, type, rank, shape, false, rank == 0);
    return field;
}

COMPOUNDFIELD* udaNewCompoundPointerField(const char* name, const char* description, int* offset, int type, bool is_scalar)
{
    CompoundField* field = (CompoundField*)malloc(sizeof(CompoundField));
    defineField(field, name, description, offset, type, 0, nullptr, true, is_scalar);
    return field;
}

COMPOUNDFIELD* udaNewCompoundFixedStringField(const char* name, const char* description, int* offset, int rank, int* shape)
{
    CompoundField* field = (CompoundField*)malloc(sizeof(CompoundField));

    int count = 1;
    for (int i = 0; i < rank; ++i) {
        count *= shape[i];
    }

    copy_string(name, field->name, MAXELEMENTNAME);
    field->atomictype = UDA_TYPE_STRING;
    copy_string("STRING", field->type, MAXELEMENTNAME);
    copy_string(description, field->desc, MAXELEMENTNAME);
    field->pointer = 0;
    field->count = count;
    field->rank = rank;
    field->shape = (int*)malloc(field->rank * sizeof(int));
    for (int i = 0; i < rank; ++i) {
        field->shape[i] = shape[i];
    }
    field->size = field->count * sizeof(char);
    field->offset = new_offset((size_t)*offset, field->type);
    field->offpad = padding((size_t)*offset, field->type);
    field->alignment = get_alignment_of(field->type);
    *offset = field->offset + field->size; // Next Offset

    return field;
}

COMPOUNDFIELD* udaNewCompoundVarStringField(const char* name, const char* description, int* offset)
{
    CompoundField* field = (CompoundField*)malloc(sizeof(CompoundField));

    copy_string(name, field->name, MAXELEMENTNAME);
    field->atomictype = UDA_TYPE_STRING;
    copy_string("STRING", field->type, MAXELEMENTNAME);
    copy_string(description, field->desc, MAXELEMENTNAME);
    field->pointer = 1;
    field->count = 1;
    field->rank = 0;
    field->shape = nullptr;
    field->size = field->count * sizeof(char*);
    field->offset = new_offset((size_t)*offset, field->type);
    field->offpad = padding((size_t)*offset, field->type);
    field->alignment = get_alignment_of(field->type);
    *offset = field->offset + field->size; // Next Offset

    return field;
}

COMPOUNDFIELD* udaNewCompoundVarStringArrayField(const char* name, const char* description, int* offset, int rank, int* shape)
{
    CompoundField* field = (CompoundField*)malloc(sizeof(CompoundField));

    int count = 1;
    bool is_pointer = rank == 0;

    for (int i = 0; i < rank; ++i) {
        count *= shape[i];
    }

    size_t size = is_pointer ? sizeof(char**) : sizeof(char*);

    copy_string(name, field->name, MAXELEMENTNAME);
    field->atomictype = UDA_TYPE_STRING;
    copy_string("STRING *", field->type, MAXELEMENTNAME);
    copy_string(description, field->desc, MAXELEMENTNAME);
    field->pointer = is_pointer;
    field->count = count;
    field->rank = rank;
    field->shape = shape;
    field->size = field->count * size;
    field->offset = new_offset((size_t)*offset, field->type);
    field->offpad = padding((size_t)*offset, field->type);
    field->alignment = get_alignment_of(field->type);
    *offset = field->offset + field->size; // Next Offset

    return field;
}

COMPOUNDFIELD* udaNewCompoundUserTypeField(const char* name, const char* description, int* offset,
                                           USERDEFINEDTYPE* user_type)
{
    CompoundField* field = (CompoundField*)malloc(sizeof(CompoundField));
    defineUserTypeField(field, name, description, offset, 0, nullptr, static_cast<UserDefinedType*>(user_type), false);
    return field;
}

COMPOUNDFIELD* udaNewCompoundUserTypePointerField(const char* name, const char* description, int* offset,
                                                  USERDEFINEDTYPE* user_type)
{
    CompoundField* field = (CompoundField*)malloc(sizeof(CompoundField));
    defineUserTypeField(field, name, description, offset, 0, nullptr, static_cast<UserDefinedType*>(user_type), true);
    return field;
}

COMPOUNDFIELD* udaNewCompoundUserTypeArrayField(const char* name, const char* description, int* offset,
                                                USERDEFINEDTYPE* user_type, int rank, int* shape)
{
    CompoundField* field = (CompoundField*)malloc(sizeof(CompoundField));
    defineUserTypeField(field, name, description, offset, rank, shape, static_cast<UserDefinedType*>(user_type), false);
    return field;
}

USERDEFINEDTYPE* udaNewUserType(const char* name, const char* source, int ref_id, int image_count, char* image,
                                size_t size, size_t num_fields, COMPOUNDFIELD** fields)
{
    UserDefinedType* user_type = (UserDefinedType*)malloc(sizeof(UserDefinedType));

    strcpy(user_type->name, name);
    strcpy(user_type->source, source);
    user_type->ref_id = ref_id;
    user_type->imagecount = image_count; // No Structure Image data
    user_type->image = image;
    user_type->size = size; // Structure size
    user_type->idamclass = UDA_TYPE_COMPOUND;
    user_type->fieldcount = 0;
    user_type->compoundfield = nullptr;

    for (size_t i = 0; i < num_fields; ++i) {
        CompoundField* field = static_cast<CompoundField*>(fields[i]);
        add_compound_field(user_type, *field);
    }

    return user_type;
}

int udaAddUserType(UDA_PLUGIN_INTERFACE* plugin_interface, USERDEFINEDTYPE* user_type)
{
    const auto* interface = static_cast<UdaPluginInterface*>(plugin_interface);
    UserDefinedTypeList* userdefinedtypelist = interface->user_defined_type_list;
    add_user_defined_type(userdefinedtypelist, *static_cast<UserDefinedType*>(user_type));

    return 0;
}

int udaRegisterMalloc(UDA_PLUGIN_INTERFACE* plugin_interface, void* data, int count, size_t size, const char* type)
{
    const auto* interface = static_cast<UdaPluginInterface*>(plugin_interface);
    add_malloc(interface->log_malloc_list, data, count, size, type);

    return 0;
}

int udaRegisterMallocArray(UDA_PLUGIN_INTERFACE* plugin_interface, void* data, int count, size_t size, const char* type,
                           int rank, int* shape)
{
    const auto* interface = static_cast<UdaPluginInterface*>(plugin_interface);
    add_malloc2(interface->log_malloc_list, data, count, size, type, rank, shape);

    return 0;
}

int udaPluginPluginsCount(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    const auto* interface = static_cast<UdaPluginInterface*>(plugin_interface);
    return interface->pluginList->size();
}

namespace
{
int check_plugin_class(UDA_PLUGIN_INTERFACE* plugin_interface, int plugin_num, int plugin_class)
{
    const auto* interface = static_cast<UdaPluginInterface*>(plugin_interface);
    auto& plugin_list = *interface->pluginList;
    auto config = interface->config;

    bool external_user = config->get("server.external_user").as_or_default(false);

    bool is_valid = plugin_list[plugin_num].type == plugin_class &&
                    (plugin_list[plugin_num].is_private == UDA_PLUGIN_PUBLIC ||
                     (plugin_list[plugin_num].is_private == UDA_PLUGIN_PRIVATE && !external_user));

    if (plugin_class == UDA_PLUGIN_CLASS_FILE) {
        is_valid |=
            (!plugin_list[plugin_num].default_method.empty() && !plugin_list[plugin_num].extension.empty());
    }

    return is_valid;
}

} // namespace

int udaPluginCheckPluginClass(UDA_PLUGIN_INTERFACE* plugin_interface, int plugin_num, const char* plugin_class)
{
    std::string plugin_class_string = plugin_class;

    if (plugin_class_string == "file") {
        return check_plugin_class(plugin_interface, plugin_num, UDA_PLUGIN_CLASS_FILE);
    } else if (plugin_class_string == "function") {
        return check_plugin_class(plugin_interface, plugin_num, UDA_PLUGIN_CLASS_FUNCTION);
    } else if (plugin_class_string == "server") {
        return check_plugin_class(plugin_interface, plugin_num, UDA_PLUGIN_CLASS_SERVER);
    } else if (plugin_class_string == "device") {
        return check_plugin_class(plugin_interface, plugin_num, UDA_PLUGIN_CLASS_DEVICE);
    } else if (plugin_class_string == "other") {
        return check_plugin_class(plugin_interface, plugin_num, UDA_PLUGIN_CLASS_OTHER);
    }

    return 0;
}

const char* udaPluginPluginFormat(UDA_PLUGIN_INTERFACE* plugin_interface, int plugin_num)
{
    const auto* interface = static_cast<UdaPluginInterface*>(plugin_interface);
    return interface->pluginList->at(plugin_num).name.c_str();
}

const char* udaPluginPluginExtension(UDA_PLUGIN_INTERFACE* plugin_interface, int plugin_num)
{
    const auto* interface = static_cast<UdaPluginInterface*>(plugin_interface);
    return interface->pluginList->at(plugin_num).extension.c_str();
}

const char* udaPluginPluginDescription(UDA_PLUGIN_INTERFACE* plugin_interface, int plugin_num)
{
    const auto* interface = static_cast<UdaPluginInterface*>(plugin_interface);
    return interface->pluginList->at(plugin_num).description.c_str();
}

const char* udaPluginPluginExample(UDA_PLUGIN_INTERFACE* plugin_interface, int plugin_num)
{
    const auto* interface = static_cast<UdaPluginInterface*>(plugin_interface);
    return interface->pluginList->at(plugin_num).example.c_str();
}

void udaPluginLog(UDA_PLUGIN_INTERFACE* plugin_interface, const char* file, int line, const char* msg)
{
    // TODO: pass logger on plugin_interface and call logger->log(...)
    log(UDA_LOG_DEBUG, file, line, msg);
}

void udaPluginLog_s(UDA_PLUGIN_INTERFACE* plugin_interface, const char* file, int line, const char* fmt, const char* arg)
{
    auto msg = fmt::format(fmt, arg);
    udaPluginLog(plugin_interface, file, line, msg.c_str());
}

void udaPluginLog_i(UDA_PLUGIN_INTERFACE* plugin_interface, const char* file, int line, const char* fmt, int arg)
{
    auto msg = fmt::format(fmt, arg);
    udaPluginLog(plugin_interface, file, line, msg.c_str());
}

void udaAddPluginError(UDA_PLUGIN_INTERFACE* plugin_interface, const char* location, int code, const char* msg)
{
    const auto interface = static_cast<UdaPluginInterface*>(plugin_interface);
    UDA_LOG(UDA_LOG_ERROR, msg)
    const auto error = create_error(ErrorType::Code, location, code, msg);
    interface->error_stack.push_back(error);
}

int udaPluginIsExternal(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    const auto* interface = static_cast<UdaPluginInterface*>(plugin_interface);
    auto config = interface->config;
    bool external_user = config->get("server.external_user").as_or_default(false);
    return external_user;
}

int udaPluginCheckInterfaceVersion(UDA_PLUGIN_INTERFACE* plugin_interface, int interface_version)
{
    const auto* interface = static_cast<UdaPluginInterface*>(plugin_interface);
    return interface->interface_version > interface_version;
}

int udaPluginGetVersion(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    const auto* interface = static_cast<UdaPluginInterface*>(plugin_interface);
    return interface->plugin_version;
}

void udaPluginSetVersion(UDA_PLUGIN_INTERFACE* plugin_interface, int plugin_version)
{
    auto* interface = static_cast<UdaPluginInterface*>(plugin_interface);
    interface->plugin_version = plugin_version;
}

const char* udaPluginFunction(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    const auto* interface = reinterpret_cast<UdaPluginInterface*>(plugin_interface);
    return interface->request_data->function;
}

int udaPluginReturnDataLabel(UDA_PLUGIN_INTERFACE* plugin_interface, const char* label)
{
    const auto* interface = static_cast<UdaPluginInterface*>(plugin_interface);
    DataBlock* data_block = interface->data_block;
    strcpy(data_block->data_label, label);
    return 0;
}

int udaPluginReturnDataUnits(UDA_PLUGIN_INTERFACE* plugin_interface, const char* units)
{
    const auto* interface = static_cast<UdaPluginInterface*>(plugin_interface);
    DataBlock* data_block = interface->data_block;
    strcpy(data_block->data_units, units);
    return 0;
}

int udaPluginReturnErrorAsymmetry(UDA_PLUGIN_INTERFACE* plugin_interface, bool flag)
{
    const auto* interface = static_cast<UdaPluginInterface*>(plugin_interface);
    DataBlock* data_block = interface->data_block;
    data_block->errasymmetry = flag;
    return 0;
}

char* memdup(const void* mem, size_t size)
{
    char* out = (char*)malloc(size);

    if (out != nullptr) {
        memcpy(out, mem, size);
    }

    return out;
}

int udaPluginReturnErrorLow(UDA_PLUGIN_INTERFACE* plugin_interface, float* data, size_t size)
{
    const auto* interface = static_cast<UdaPluginInterface*>(plugin_interface);
    DataBlock* data_block = interface->data_block;
    data_block->errlo = memdup(data, size);
    return 0;
}

int udaPluginReturnErrorHigh(UDA_PLUGIN_INTERFACE* plugin_interface, float* data, size_t size)
{
    const auto* interface = static_cast<UdaPluginInterface*>(plugin_interface);
    DataBlock* data_block = interface->data_block;
    data_block->errhi = memdup(data, size);
    return 0;
}

int udaPluginReturnDataOrder(UDA_PLUGIN_INTERFACE* plugin_interface, int order)
{
    const auto* interface = static_cast<UdaPluginInterface*>(plugin_interface);
    DataBlock* data_block = interface->data_block;
    data_block->order = order;
    return 0;
}

int udaPluginReturnDimensionFloatArray(UDA_PLUGIN_INTERFACE* plugin_interface, int dim_n, float* data, size_t size,
                                       const char* label, const char* units)
{
    const auto* interface = static_cast<UdaPluginInterface*>(plugin_interface);
    DataBlock* data_block = interface->data_block;

    data_block->dims[0].data_type = UDA_TYPE_FLOAT;
    data_block->dims[0].dim_n = dim_n;
    data_block->dims[0].compressed = 0;
    strcpy(data_block->dims[0].dim_label, label);
    strcpy(data_block->dims[0].dim_units, units);

    data_block->data = memdup(data, size);
    return 0;
}

void udaSetMetadata(UDA_PLUGIN_INTERFACE* plugin_interface, const char* key, const char* value) {
    const auto interface = static_cast<UdaPluginInterface*>(plugin_interface);
    interface->meta_data->set(key, value);
}

const char* udaGetMetadata(UDA_PLUGIN_INTERFACE* plugin_interface, const char* key) {
    const auto interface = static_cast<UdaPluginInterface*>(plugin_interface);
    return interface->meta_data->find(key).data();
}

bool udaHasMetadata(UDA_PLUGIN_INTERFACE* plugin_interface, const char* key) {
    const auto interface = static_cast<UdaPluginInterface*>(plugin_interface);
    return interface->meta_data->contains(key);
}
