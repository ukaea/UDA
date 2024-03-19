#include "server_config.h"

#include <boost/algorithm/string.hpp>
#include <fmt/format.h>
#include <toml++/toml.hpp>
#include <vector>

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
            throw uda::server::ConfigError{fmt::format("option {} must a value", _name)};
        }
        if (value.type() != as_toml_type(_type)) {
            throw uda::server::ConfigError{
                fmt::format("invalid type for option {}, should be {}", _name, to_string(_type))};
        }
        if (_type == ValueType::Char) {
            auto string = value.as<std::string>()->get();
            if (string.size() != 1) {
                throw uda::server::ConfigError{fmt::format("invalid char value '{}' for option {}", string, _name)};
            }
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
    void validate(const toml::table& table) const
    {
        for (const auto& entry : table) {
            const auto& name = entry.first;
            bool found = false;
            for (const auto& validator : _value_validators) {
                if (validator.name() == name) {
                    found = true;
                    validator.validate(entry.second);
                }
            }
            if (!found) {
                throw uda::server::ConfigError{fmt::format("invalid option {} in section {}", name.data(), _name)};
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
                         {"mode", ValueType::String},
                         {"level", ValueType::String},
                     }},
    SectionValidator{"plugins",
                     {
                         {"config", ValueType::String},
                         {"debug_single_file", ValueType::Boolean},
                     }},
};

void validate(toml::table& table)
{
    for (const auto& entry : table) {
        const auto& name = entry.first;
        const auto& value = entry.second;
        if (!value.is_table()) {
            throw uda::server::ConfigError{"top level entries must be sections"};
        }
        bool found = false;
        for (const auto& validator : Validators) {
            if (validator.name() == name) {
                found = true;
                validator.validate(*value.as_table());
            }
        }
        if (!found) {
            throw uda::server::ConfigError{fmt::format("invalid section name {}", name.data())};
        }
    }
}

} // namespace

namespace uda::server
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
    ConfigImpl(std::string_view file_name) : _table{} { load(file_name); };
    Option get(std::string_view name)
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
            //            throw ConfigError{ fmt::format("config option not found {}", name.data()) };
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

Option Config::get(std::string_view name) const
{
    if (!_impl) {
        throw ConfigError{"config has not been loaded"};
    }
    return _impl->get(name);
}

Config::Config() : _impl{nullptr} {}

Config::~Config() {}

} // namespace uda::server
