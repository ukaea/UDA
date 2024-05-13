#include "keyvaluePlugin.h"

#include <leveldb/c.h>

#include <boost/filesystem.hpp>

//#include "common/stringUtils.h"
#include <uda/types.h>
#include <uda/uda_plugin_base.hpp>

UDA_PLUGIN_INFO UDA_PLUGIN_INFO_FUNCTION_NAME()
{
    UDA_PLUGIN_INFO info;
    info.name = "KEYVALUE";
    info.version = "1.0";
    info.entry_function = "keyValue";
    info.type = UDA_PLUGIN_CLASS_FUNCTION;
    info.extension = "";
    info.default_method = "read";
    info.description = "Plugin for saving and loading from a key value store";
    info.cache_mode = UDA_PLUGIN_CACHE_MODE_OK;
    info.is_private = false;
    info.interface_version = 1;
    return info;
}

namespace uda
{
namespace keyvalue
{

class Plugin : public UDAPluginBase
{
  public:
    Plugin();
    int write(UDA_PLUGIN_INTERFACE* plugin_interface);
    int read(UDA_PLUGIN_INTERFACE* plugin_interface);
    int del(UDA_PLUGIN_INTERFACE* plugin_interface);
    void init(UDA_PLUGIN_INTERFACE* plugin_interface) override;
    void reset() override;

  private:
    leveldb_readoptions_t* roptions_ = nullptr;
    leveldb_writeoptions_t* woptions_ = nullptr;
    leveldb_options_t* options_ = nullptr;
    leveldb_t* db_ = nullptr;
};

Plugin::Plugin()
    : UDAPluginBase("KEYVALUE", 1, "read", boost::filesystem::path(__FILE__).parent_path().append("help.txt").string())
{
    register_method("write", static_cast<UDAPluginBase::plugin_member_type>(&Plugin::write));
    register_method("read", static_cast<UDAPluginBase::plugin_member_type>(&Plugin::read));
    register_method("delete", static_cast<UDAPluginBase::plugin_member_type>(&Plugin::del));
}

} // namespace keyvalue
} // namespace uda

extern "C" int keyValue(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    static uda::keyvalue::Plugin plugin = {};
    return plugin.call(plugin_interface);
}

void uda::keyvalue::Plugin::init(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    options_ = leveldb_options_create();
    leveldb_options_set_create_if_missing(options_, 1);

    char* err = nullptr;
    db_ = leveldb_open(options_, "idam_ks", &err);
    if (err != nullptr) {
        error(plugin_interface, err);
    }

    woptions_ = leveldb_writeoptions_create();
    roptions_ = leveldb_readoptions_create();
}

void uda::keyvalue::Plugin::reset()
{
    leveldb_close(db_);
    db_ = nullptr;

    leveldb_writeoptions_destroy(woptions_);
    woptions_ = nullptr;

    leveldb_readoptions_destroy(roptions_);
    roptions_ = nullptr;

    leveldb_options_destroy(options_);
    options_ = nullptr;
}

int uda::keyvalue::Plugin::write(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    auto key = find_required_arg<std::string>(plugin_interface, "key");
    auto value = find_required_arg<std::string>(plugin_interface, "value");

    char* env = getenv("UDA_PLUGIN_KEYVALUE_STORE");
    if (env == nullptr) {
        error(plugin_interface, "Environmental variable IDAM_PLUGIN_KEYVALUE_STORE not found");
    }

    std::string store = env;
    if (store == "LEVELDB") {
        debug(plugin_interface, "Writing key {} to LevelDB keystore", key);
    } else {
        error(plugin_interface, "Unknown key-value store requested");
    }

    char* err = nullptr;
    leveldb_put(db_, woptions_, key.c_str(), key.size(), value.c_str(), value.size(), &err);

    if (err != nullptr) {
        debug(plugin_interface, err);
        leveldb_free(err);
        throw std::runtime_error{err};
    }

    return 0;
}

int uda::keyvalue::Plugin::read(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    auto key = find_required_arg<std::string>(plugin_interface, "key");

    char* env = getenv("UDA_PLUGIN_KEYVALUE_STORE");
    if (env == nullptr) {
        error(plugin_interface, "Environmental variable IDAM_PLUGIN_KEYVALUE_STORE not found");
    }

    std::string store = env;
    if (store == "LEVELDB") {
        debug(plugin_interface, "Writing key {} to LevelDB keystore", key);
    } else {
        error(plugin_interface, "Unknown key-value store requested");
    }

    char* err = nullptr;
    size_t value_len;

    char* value = leveldb_get(db_, roptions_, key.c_str(), key.size(), &value_len, &err);

    if (err != nullptr) {
        debug(plugin_interface, err);
        throw std::runtime_error{err};
    }

    int shape[] = {(int)value_len};
    udaPluginReturnData(plugin_interface, value, value_len, UDA_TYPE_CHAR, 1, shape, nullptr);

    return 0;
}

int uda::keyvalue::Plugin::del(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    auto key = find_required_arg<std::string>(plugin_interface, "key");

    char* env = getenv("UDA_PLUGIN_KEYVALUE_STORE");
    if (env == nullptr) {
        error(plugin_interface, "Environmental variable IDAM_PLUGIN_KEYVALUE_STORE not found");
    }

    std::string store = env;
    if (store == "LEVELDB") {
        debug(plugin_interface, "Writing key {} to LevelDB keystore", key);
    } else {
        error(plugin_interface, "Unknown keyvalue store requested");
    }

    char* err = nullptr;

    leveldb_delete(db_, woptions_, key.c_str(), key.size(), &err);

    if (err != nullptr) {
        std::string message = err;
        leveldb_free(err);
        error(plugin_interface, message);
    }

    return 0;
}
