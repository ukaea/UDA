#include "keyvaluePlugin.h"

#include <leveldb/c.h>

#include <clientserver/stringUtils.h>
#include <clientserver/initStructs.h>
#include <clientserver/udaTypes.h>
#include <plugins/udaPlugin.h>

namespace uda {
namespace keyvalue {

class Plugin {
public:
    Plugin() = default;

    int help(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int version(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int build_date(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int default_method(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int max_interface_version(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int write(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int read(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int del(IDAM_PLUGIN_INTERFACE* plugin_interface);

    int init(IDAM_PLUGIN_INTERFACE* plugin_interface);
    void reset(IDAM_PLUGIN_INTERFACE* plugin_interface);

private:
    bool initialised_ = false;
    leveldb_readoptions_t* roptions_ = nullptr;
    leveldb_writeoptions_t* woptions_ = nullptr;
    leveldb_options_t* options_ = nullptr;
    leveldb_t* db_ = nullptr;
};

}
}

int keyValue(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    if (plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        RAISE_PLUGIN_ERROR("Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
    }

    static uda::keyvalue::Plugin plugin{};

    if (plugin.init(plugin_interface) != 0) {
        RAISE_PLUGIN_ERROR("Plugin initialisation failed.");
    }

    plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    REQUEST_DATA* request_data = plugin_interface->request_data;

    int rc = 0;

    if (STR_IEQUALS(request_data->function, "help")) {
        rc = plugin.help(plugin_interface);
    } else if (STR_IEQUALS(request_data->function, "version")) {
        rc = plugin.version(plugin_interface);
    } else if (STR_IEQUALS(request_data->function, "builddate")) {
        rc = plugin.build_date(plugin_interface);
    } else if (STR_IEQUALS(request_data->function, "defaultmethod")) {
        rc = plugin.default_method(plugin_interface);
    } else if (STR_IEQUALS(request_data->function, "maxinterfaceversion")) {
        rc = plugin.max_interface_version(plugin_interface);
    } else if (STR_IEQUALS(request_data->function, "write")) {
        rc = plugin.write(plugin_interface);
    } else if (STR_IEQUALS(request_data->function, "read")) {
        rc = plugin.read(plugin_interface);
    } else if (STR_IEQUALS(request_data->function, "delete")) {
        rc = plugin.del(plugin_interface);
    } else {
        RAISE_PLUGIN_ERROR("Unknown function requested!");
    }

    return rc;
}

int uda::keyvalue::Plugin::init(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
#if defined(UDA_VERSION) && UDA_VERSION_MAJOR > 3
    initPlugin(plugin_interface);
#endif

    REQUEST_DATA* request_data = plugin_interface->request_data;

    if (!initialised_
        || !strcasecmp(request_data->function, "init") || !strcasecmp(request_data->function, "initialise")) {

        options_ = leveldb_options_create();
        leveldb_options_set_create_if_missing(options_, 1);

        char* err = nullptr;
        db_ = leveldb_open(options_, "idam_ks", &err);
        if (err != nullptr) {
            RAISE_PLUGIN_ERROR(err);
        }

        woptions_ = leveldb_writeoptions_create();
        roptions_ = leveldb_readoptions_create();

        initialised_ = true;
    }

    return 0;
}

void uda::keyvalue::Plugin::reset(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    REQUEST_DATA* request_data = plugin_interface->request_data;

    if (plugin_interface->housekeeping || !strcasecmp(request_data->function, "reset")) {
        if (!initialised_) return;

        leveldb_close(db_);
        db_ = nullptr;

        leveldb_writeoptions_destroy(woptions_);
        woptions_ = nullptr;

        leveldb_readoptions_destroy(roptions_);
        roptions_ = nullptr;

        leveldb_options_destroy(options_);
        options_ = nullptr;

        initialised_ = false;
    }
}

// Help: A Description of library functionality
int uda::keyvalue::Plugin::help(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    char* p = (char*)malloc(sizeof(char) * 2 * 1024);

    strcpy(p, "\ntemplatePlugin: Add Functions Names, Syntax, and Descriptions\n\n");

    initDataBlock(data_block);

    data_block->rank = 1;
    data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));

    for (unsigned int i = 0; i < data_block->rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

    data_block->data_type = UDA_TYPE_STRING;
    strcpy(data_block->data_desc, "templatePlugin: help = description of this plugin");

    data_block->data = (char*)p;

    data_block->dims[0].data_type = UDA_TYPE_UNSIGNED_INT;
    data_block->dims[0].dim_n = strlen(p) + 1;
    data_block->dims[0].compressed = 1;
    data_block->dims[0].dim0 = 0.0;
    data_block->dims[0].diff = 1.0;
    data_block->dims[0].method = 0;

    data_block->data_n = data_block->dims[0].dim_n;

    strcpy(data_block->data_label, "");
    strcpy(data_block->data_units, "");

    return 0;
}

int uda::keyvalue::Plugin::version(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    initDataBlock(data_block);
    data_block->data_type = UDA_TYPE_INT;
    data_block->rank = 0;
    data_block->data_n = 1;
    int* data = (int*)malloc(sizeof(int));
    data[0] = THISPLUGIN_VERSION;
    data_block->data = (char*)data;
    strcpy(data_block->data_desc, "Plugin version number");
    strcpy(data_block->data_label, "version");
    strcpy(data_block->data_units, "");

    return 0;
}

// Plugin Build Date
int uda::keyvalue::Plugin::build_date(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    initDataBlock(data_block);
    data_block->data_type = UDA_TYPE_STRING;
    data_block->rank = 0;
    data_block->data_n = strlen(__DATE__) + 1;
    char* data = (char*)malloc(data_block->data_n * sizeof(char));
    strcpy(data, __DATE__);
    data_block->data = (char*)data;
    strcpy(data_block->data_desc, "Plugin build date");
    strcpy(data_block->data_label, "date");
    strcpy(data_block->data_units, "");

    return 0;
}

// Plugin Default Method
int uda::keyvalue::Plugin::default_method(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    initDataBlock(data_block);
    data_block->data_type = UDA_TYPE_STRING;
    data_block->rank = 0;
    data_block->data_n = strlen(THISPLUGIN_DEFAULT_METHOD) + 1;
    char* data = (char*)malloc(data_block->data_n * sizeof(char));
    strcpy(data, THISPLUGIN_DEFAULT_METHOD);
    data_block->data = (char*)data;
    strcpy(data_block->data_desc, "Plugin default method");
    strcpy(data_block->data_label, "method");
    strcpy(data_block->data_units, "");

    return 0;
}

// Plugin Maximum Interface Version
int uda::keyvalue::Plugin::max_interface_version(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    initDataBlock(data_block);
    data_block->data_type = UDA_TYPE_INT;
    data_block->rank = 0;
    data_block->data_n = 1;
    int* data = (int*)malloc(sizeof(int));
    data[0] = THISPLUGIN_MAX_INTERFACE_VERSION;
    data_block->data = (char*)data;
    strcpy(data_block->data_desc, "Maximum Interface Version");
    strcpy(data_block->data_label, "version");
    strcpy(data_block->data_units, "");

    return 0;
}

int uda::keyvalue::Plugin::write(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    const char* key = nullptr;
    const char* value = nullptr;

    FIND_REQUIRED_STRING_VALUE(plugin_interface->request_data->nameValueList, key);
    FIND_REQUIRED_STRING_VALUE(plugin_interface->request_data->nameValueList, value);

    char* env = getenv("UDA_PLUGIN_KEYVALUE_STORE");
    if (env == nullptr) {
        RAISE_PLUGIN_ERROR("Environmental variable IDAM_PLUGIN_KEYVALUE_STORE not found");
    }

    if (STR_EQUALS(env, "LEVELDB")) {
        UDA_LOG(UDA_LOG_DEBUG, "Writing key %s to LevelDB keystore", key);
    } else {
        RAISE_PLUGIN_ERROR("Unknown keyvalue store requested");
    }

    char* err = nullptr;
    leveldb_put(db_, woptions_, key, strlen(key), value, strlen(value), &err);

    if (err != nullptr) {
        RAISE_PLUGIN_ERROR_EX(err, { leveldb_free(err); });
    }

    return 0;
}

int uda::keyvalue::Plugin::read(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    const char* key = nullptr;

    FIND_REQUIRED_STRING_VALUE(plugin_interface->request_data->nameValueList, key);

    char* env = getenv("UDA_PLUGIN_KEYVALUE_STORE");
    if (env == nullptr) {
        RAISE_PLUGIN_ERROR("Environmental variable IDAM_PLUGIN_KEYVALUE_STORE not found");
    }

    if (STR_EQUALS(env, "LEVELDB")) {
        UDA_LOG(UDA_LOG_DEBUG, "Writing key %s to LevelDB keystore", key);
    } else {
        RAISE_PLUGIN_ERROR("Unknown keyvalue store requested");
    }

    char* err = nullptr;
    size_t value_len;

    char* value = leveldb_get(db_, roptions_, key, strlen(key), &value_len, &err);

    if (err != nullptr) {
        RAISE_PLUGIN_ERROR_EX(err, { leveldb_free(err); });
    }

    plugin_interface->data_block->data = value;
    plugin_interface->data_block->data_n = value_len;

    return 0;
}

int uda::keyvalue::Plugin::del(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    const char* key = nullptr;

    FIND_REQUIRED_STRING_VALUE(plugin_interface->request_data->nameValueList, key);

    char* env = getenv("UDA_PLUGIN_KEYVALUE_STORE");
    if (env == nullptr) {
        RAISE_PLUGIN_ERROR("Environmental variable IDAM_PLUGIN_KEYVALUE_STORE not found");
    }

    if (STR_EQUALS(env, "LEVELDB")) {
        UDA_LOG(UDA_LOG_DEBUG, "Writing key %s to LevelDB keystore", key);
    } else {
        RAISE_PLUGIN_ERROR("Unknown keyvalue store requested");
    }

    char* err = nullptr;

    leveldb_delete(db_, woptions_, key, strlen(key), &err);

    if (err != nullptr) {
        RAISE_PLUGIN_ERROR_EX(err, { leveldb_free(err); });
    }

    return 0;
}
