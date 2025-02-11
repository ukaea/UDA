#pragma once

#include <boost/any.hpp>
#include <string_view>
#include <fmt/format.h>

#include "common/string_utils.h"

namespace uda::config
{

class ConfigError : public std::runtime_error
{
  public:
    ConfigError(std::string_view description) : std::runtime_error(description.data()) {}

    template <typename T>
    static ConfigError cast_error(const std::string& name, const boost::any& value) {
        auto arg_type_name = common::demangle(typeid(T).name());
        auto value_type_name = common::demangle(value.type().name());
        return ConfigError{ fmt::format("invalid cast for option '{}' - requested '{}' but actual type is '{}'", name, arg_type_name, value_type_name) };
    }
};

class Option
{
  public:
    Option(std::string name) : _name{name}, _value{} {}
    Option(std::string name, boost::any value) : _name{name}, _value{value} {}

    operator bool() const { return !_value.empty(); }

    template <class T> bool is() const { return _value.type() == typeid(T); }

    template <class T> T as() const
    {
        if constexpr (std::is_same_v<T, const char*>) {
            if (is<std::string>()) {
                auto& string = boost::any_cast<std::string&>(_value);
                return string.c_str();
            }
            throw ConfigError::cast_error<std::string>(_name, _value);
        } else if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>) {
            if (is<long long>()) {
                auto value = boost::any_cast<long long>(_value);
                return static_cast<T>(value);
            }
            throw ConfigError::cast_error<long long>(_name, _value);
        } else {
            if (is<T>()) {
                return boost::any_cast<T>(_value);
            }
            throw ConfigError::cast_error<T>(_name, _value);
        }
    }

    template <class T> T as_or_default(T default_value) const
    {
        if constexpr (std::is_same_v<T, const char*>) {
            if (is<std::string>()) {
                const auto& string = boost::any_cast<const std::string&>(_value);
                return string.c_str();
            } else if (_value.empty()) {
                return default_value;
            }
            throw ConfigError::cast_error<std::string>(_name, _value);
        } else if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>) {
            if (is<long long>()) {
                auto value = boost::any_cast<long long>(_value);
                return static_cast<T>(value);
            } else if (_value.empty()) {
                return default_value;
            }
            throw ConfigError::cast_error<long long>(_name, _value);
        } else {
            if (is<T>()) {
                return boost::any_cast<T>(_value);
            } else if (_value.empty()) {
                return default_value;
            }
            throw ConfigError::cast_error<T>(_name, _value);
        }
    }

  private:
    std::string _name;
    boost::any _value;
};

class ConfigImpl;

class Config
{
  public:
    Config();
    Config(Config&& other);
    ~Config();
    void load(std::string_view file_name);
    Option get(std::string_view name) const;
    void set(std::string_view name, const std::string& value);
    void set(std::string_view name, const char* value);
    void set(std::string_view name, bool value);
    void set(std::string_view name, int value);
    void print() const;

  private:
    std::unique_ptr<ConfigImpl> _impl;
};

} // namespace uda::config
