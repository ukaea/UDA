#include "config.h"
#include "logging/logging.h"

#include <boost/algorithm/string.hpp>
#include <fmt/format.h>
#include <toml++/toml.hpp>
#include <type_traits>
#include <vector>
#include <unordered_map>

#ifdef __GNUC__ // GCC 4.8+, Clang, Intel and other compilers compatible with GCC (-std=c++0x or above)
[[noreturn]] inline __attribute__((always_inline)) void unreachable() { __builtin_unreachable(); }
#elif defined(_MSC_VER) // MSVC
[[noreturn]] __forceinline void unreachable() { __assume(false); }
#else // ???
inline void unreachable() {}
#endif

namespace
{

enum class ValueType {
    String,
    Integer,
    Float,
    Boolean,
    Char,
};

template<typename T>
ValueType get_value_type(T value)
{
    if constexpr(std::is_same_v<T, std::string>)
    {
        if (value.size() == 1)
        {
            return ValueType::Char;
        }
        return ValueType::String;
    }
    else if constexpr(std::is_same_v<T, bool>)
    {
        return ValueType::Boolean;
    }
    else if constexpr(std::is_integral_v<T>)
    {
        return ValueType::Integer;
    }
    else if constexpr(std::is_floating_point_v<T>)
    {
        return ValueType::Float;
    }
    throw uda::config::ConfigError{"Unrecognised value type"};
}

std::string to_string(ValueType type)
{
    switch (type) {
        case ValueType::String:
            return "string";
        case ValueType::Integer:
            return "integer";
        case ValueType::Float:
            return "floating_point";
        case ValueType::Boolean:
            return "boolean";
        case ValueType::Char:
            return "char";
    }
    unreachable();
}

toml::node_type as_toml_type(ValueType type)
{
    switch (type) {
        case ValueType::String:
            return toml::node_type::string;
        case ValueType::Integer:
            return toml::node_type::integer;
        case ValueType::Float:
            return toml::node_type::floating_point;
        case ValueType::Boolean:
            return toml::node_type::boolean;
        case ValueType::Char:
            return toml::node_type::string;
    }
    unreachable();
}

std::string to_string(toml::node_type type)
{
    using toml::node_type;
    switch (type) {
        case node_type::none: return "none";
        case node_type::table: return "table";
        case node_type::array: return "array";
        case node_type::string: return "string";
        case node_type::integer: return "integer";
        case node_type::floating_point: return "floating_point";
        case node_type::boolean: return "boolean";
        case node_type::date: return "date";
        case node_type::time: return "time";
        case node_type::date_time: return "date_time";
    }
    unreachable();
}

class ValueValidator
{
  public:
    ValueValidator(std::string name, ValueType type) : _name{name}, _type{type} {}

    std::string_view name() const { return _name; }

    void validate(const toml::node& value) const
    {
        if (!value.is_value()) {
            throw uda::config::ConfigError{fmt::format("option {} must a value", _name)};
        }
        if (value.type() != as_toml_type(_type)) {
            throw uda::config::ConfigError{
                fmt::format("invalid type for option {}, should be {}", _name, to_string(_type))};
        }
        if (_type == ValueType::Char) {
            auto string = value.as<std::string>()->get();
            if (string.size() != 1) {
                throw uda::config::ConfigError{fmt::format("invalid char value '{}' for option {}", string, _name)};
            }
        }
    }

    void validate(ValueType type) const
    {
        if (type != _type) 
        {
            throw uda::config::ConfigError{
                fmt::format("invalid type for option {}, should be {}", _name, to_string(_type))};
        }
    }

  private:
    std::string _name;
    ValueType _type;
};

class SectionValidator
{
  public:
    SectionValidator(std::string name, std::vector<ValueValidator> value_validators)
        : _name{name}, _value_validators{value_validators}
    {
    }

    void validate(std::string_view field_name, ValueType type) const
    {
        bool found = false;
        for (const auto& validator : _value_validators) 
        {
            if (validator.name() == field_name) 
            {
                found = true;
                validator.validate(type);
            }
        }
        if (!found) 
        {
            throw uda::config::ConfigError{fmt::format("invalid option {} in section {}", field_name, _name)};
        }
    }

    void validate(const toml::table& table) const
    {
        for (const auto& entry : table) 
        {
            const auto& name = entry.first;
            bool found = false;
            for (const auto& validator : _value_validators) 
            {
                if (validator.name() == name) 
                {
                    found = true;
                    validator.validate(entry.second);
                }
            }
            if (!found) 
            {
                throw uda::config::ConfigError{fmt::format("invalid option {} in section {}", name.data(), _name)};
            }
        }
    }

    std::string_view name() const { return _name; }

  private:
    std::string _name;
    std::vector<ValueValidator> _value_validators;
};

const std::vector<SectionValidator> Validators = {
    SectionValidator{"logging",
        {
            {"path", ValueType::String},
            {"mode", ValueType::Char},
            {"level", ValueType::Integer},
        }},
    SectionValidator{"plugins",
        {
            {"debug_single_file", ValueType::Boolean},
            {"directories", ValueType::String},
            {"metadata_plugin", ValueType::String},
            {"provenance_plugin", ValueType::String},
            {"proxy_plugin", ValueType::String},
        }},
    SectionValidator{"request",
        {
            {"delim", ValueType::String},
            {"default_device", ValueType::String},
            {"default_archive", ValueType::String},
        }},
    SectionValidator{"server",
        {
            {"is_proxy", ValueType::Boolean},
            {"proxy_target", ValueType::String},
            {"address", ValueType::String},
            {"port", ValueType::Integer},
            {"startup_sleep", ValueType::Integer},
            {"private_path_target", ValueType::String},
            {"private_path_substitute", ValueType::String},
            {"default_format", ValueType::String},
        }},
    SectionValidator{"connection",
        {
            {"host", ValueType::String},
            {"port", ValueType::Integer},
            {"host_list", ValueType::String},
            {"max_socket_delay", ValueType::Integer},
            {"max_socket_attempts", ValueType::Integer},
            {"failover_host", ValueType::String},
            {"failover_port", ValueType::Integer},
        }},
    SectionValidator{"client_flags",
        {
            {"get_data_double", ValueType::Boolean},
            {"get_dim_double", ValueType::Boolean},
            {"get_time_double", ValueType::Boolean},
            {"get_bytes", ValueType::Boolean},
            {"get_bad", ValueType::Boolean},
            {"get_as_is", ValueType::Boolean},
            {"get_uncalibrated", ValueType::Boolean},
            {"get_not_offset", ValueType::Boolean},
            {"get_synthetic", ValueType::Boolean},
            {"get_scalar", ValueType::Boolean},
            {"get_no_dim_data", ValueType::Boolean},
            {"get_meta", ValueType::Boolean},
            {"timeout", ValueType::Integer},
            {"verbose", ValueType::Boolean},
            {"debug", ValueType::Boolean},
            {"alt_data", ValueType::Boolean},
            {"alt_rank", ValueType::Boolean},
            {"reuse_last_handle", ValueType::Boolean},
            {"free_and_reuse_last_handle", ValueType::Boolean},
            {"file_cache", ValueType::Boolean},
            {"full_reset", ValueType::Boolean},
            {"xdr_file", ValueType::Boolean},
            {"closedown", ValueType::Boolean},
            {"xdr_object", ValueType::Boolean},
            {"cache", ValueType::Boolean},
        }},
    SectionValidator{"client",
        {
            {"DOI", ValueType::String},
        }},
    SectionValidator{"private_flags",
        {
            {"full_reset", ValueType::Boolean},
            {"xdr_file", ValueType::Boolean},
            {"external", ValueType::Boolean},
            {"cache", ValueType::Boolean},
            {"xdr_object", ValueType::Boolean},
        }},
    SectionValidator{"test",
        {
            {"boolean", ValueType::Boolean},
            {"integer", ValueType::Integer},
            {"float", ValueType::Float},
            {"string", ValueType::String},
            {"char", ValueType::Char},
        }},
};

void validate(toml::table& table)
{
    for (const auto& entry : table) {
        const auto& name = entry.first;
        const auto& value = entry.second;
        if (!value.is_table()) {
            throw uda::config::ConfigError{"top level entries must be sections"};
        }
        bool found = false;
        for (const auto& validator : Validators) {
            if (validator.name() == name) {
                found = true;
                validator.validate(*value.as_table());
            }
        }
        if (!found) {
            throw uda::config::ConfigError{fmt::format("invalid section name {}", name.data())};
        }
    }
}

void validate(std::string_view section_name, std::string_view field_name, ValueType type)
{
    bool found = false;
            for (const auto& validator : Validators) 
            {
            if (validator.name() == section_name) 
            {
                found = true;
                validator.validate(field_name, type);
            }
        }
        if (!found) 
        {
            throw uda::config::ConfigError{fmt::format("invalid section name {}", section_name)};
        }
}

} // namespace

namespace uda::config
{

template<>
char Option::as<char>() const
{
    if (is<std::string>()) {
        auto string = boost::any_cast<std::string>(_value);
        if (string.size() != 1) {
            throw ConfigError{ fmt::format("invalid char value {} for option {}", string, _name) };
        }
    }
    throw ConfigError{ fmt::format("invalid cast for option {}", _name) };
}

template <>
char Option::as_or_default<char>(char default_value) const
{
    if (is<std::string>()) {
        auto string = boost::any_cast<std::string>(_value);
        if (string.size() != 1) {
            throw ConfigError{ fmt::format("invalid char value {} for option {}", string, _name) };
        }
    } else if (_value.empty()) {
        return default_value;
    }
    throw ConfigError{ fmt::format("invalid cast for option {}", _name) };
}

class ConfigImpl
{
  public:
    ConfigImpl() : _table{} {};
    ConfigImpl(std::string_view file_name) : _table{} { load(file_name); };

    Option get(std::string_view name) const
    {
        std::vector<std::string> tokens;
        boost::split(tokens, name, boost::is_any_of("."), boost::token_compress_on);
        if (tokens.size() != 2) {
            throw ConfigError{fmt::format("invalid config option name {}", name.data())};
        }
        auto& section_name = tokens[0];
        auto& option_name = tokens[1];
        toml::node_view option = _table[section_name][option_name];
        if (!option) {
            return {name.data()};
        }
        switch (option.type()) {
            case toml::node_type::string:
                return {name.data(), option.ref<std::string>()};
            case toml::node_type::integer:
                return {name.data(), option.ref<int64_t>()};
            case toml::node_type::floating_point:
                return {name.data(), option.ref<double>()};
            case toml::node_type::boolean:
                return {name.data(), option.ref<bool>()};
            default:
                throw ConfigError{fmt::format("invalid option type {}", to_string(option.type()))};
        }
    }

    template<typename ValueType>
    void set(std::string_view name, ValueType value)
    {
        std::vector<std::string> tokens;
        boost::split(tokens, name, boost::is_any_of("."), boost::token_compress_on);
        if (tokens.size() != 2) {
            throw ConfigError{fmt::format("invalid config option name {}", name.data())};
        }
        auto& section_name = tokens[0];
        auto& option_name = tokens[1];
        validate(section_name, option_name, get_value_type(value));
        auto section = _table[section_name];
        if (section) {
            auto sec = section.as_table();
            sec->insert(option_name, value);
        } else {
            auto new_sec = toml::table{};
            new_sec.insert(option_name, value);
            _table.insert(section_name, new_sec);
        }
    }

    void print_section(std::string_view name, const toml::table* table) const
    {
        for (auto& row : *table) {
            auto key = row.first.str();
            auto& value = row.second;
            if (value.is_string()) {
                auto val = value.as_string()->get();
                UDA_LOG(logging::UDA_LOG_DEBUG, ">> {}.{} = {}", name, key, val)
            } else if (value.is_integer()) {
                auto val = value.as_integer()->get();
                UDA_LOG(logging::UDA_LOG_DEBUG, ">> {}.{} = {}", name, key, val)
            } else if (value.is_boolean()) {
                auto val = value.as_boolean()->get();
                UDA_LOG(logging::UDA_LOG_DEBUG, ">> {}.{} = {}", name, key, val)
            }
        }
    }

    void print() const
    {
        UDA_LOG(logging::UDA_LOG_DEBUG, "Config:")
        for (auto& row : _table) {
            auto key = row.first.str();
            auto& value = row.second;
            if (value.is_table()) {
                auto table = value.as_table();
                print_section(key, table);
            }
        }
    }

    std::unordered_map<std::string, Option>
    get_section_as_map(std::string_view section_name) const
    {
        std::unordered_map<std::string, Option> result {};

        toml::node_view section_option = _table[section_name];
        if (!section_option or !section_option.is_table()) {
            return result;
        }
        auto section_table = section_option.as_table();
        for (const auto& [key, value] : *section_table)
        {
            switch (value.type()) 
            {
                case toml::node_type::string:
                    result.insert({key.data(), Option{key.data(), value.ref<std::string>()}});
                    break;
                case toml::node_type::integer:
                    result.insert({key.data(), Option{key.data(), value.ref<int64_t>()}});
                    break;
                case toml::node_type::floating_point:
                    result.insert({key.data(), Option{key.data(), value.ref<double>()}});
                    break;
                case toml::node_type::boolean:
                    result.insert({key.data(), Option{key.data(), value.ref<bool>()}});
                    break;
                default:
                    throw ConfigError{fmt::format("invalid option type {}", to_string(value.type()))};
            }
        }
        return result;
    }

  private:
    toml::table _table;

    void load(std::string_view file_name)
    {
        try {
            _table = toml::parse_file(file_name);
            validate(_table);
        } catch (const toml::parse_error& err) {
            throw ConfigError{err.description()};
        }
    }
};

void Config::load(std::string_view file_name)
{
    _impl = std::make_unique<ConfigImpl>(file_name);
}

void Config::load_in_memory()
{
    _impl = std::make_unique<ConfigImpl>();
}

Option Config::get(std::string_view name) const
{
    if (!_impl) {
        // throw ConfigError{"config has not been loaded"};
        return Option(std::string(name));
    }
    return _impl->get(name);
}

std::unordered_map<std::string, Option> Config::get_section_as_map(std::string_view section_name) const
{
    if (!_impl) {
        return {};
    }
    return _impl->get_section_as_map(section_name);
}

void Config::set(std::string_view name, bool value)
{
    if (!_impl) {
        throw ConfigError{"config has not been loaded"};
    }
    return _impl->set(name, value);
}

void Config::set(std::string_view name, const char* value)
{
    if (!_impl) {
        throw ConfigError{"config has not been loaded"};
    }
    return _impl->set(name, value);
}

void Config::set(std::string_view name, const std::string& value)
{
    if (!_impl) {
        throw ConfigError{"config has not been loaded"};
    }
    return _impl->set(name, value);
}

void Config::set(std::string_view name, int64_t value)
{
    if (!_impl) {
        throw ConfigError{"config has not been loaded"};
    }
    return _impl->set(name, value);
}

void Config::set(std::string_view name, double value)
{
    if (!_impl) {
        throw ConfigError{"config has not been loaded"};
    }
    return _impl->set(name, value);
}

void Config::print() const
{
    if (!_impl) {
        throw ConfigError{"config has not been loaded"};
    }
    return _impl->print();
}

Config::Config() = default;

Config::Config(Config&& other) = default;

Config::~Config() = default;

} // namespace uda::config
