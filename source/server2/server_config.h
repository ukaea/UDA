#pragma once

#include <boost/any.hpp>
#include <string_view>
#include <fmt/format.h>

namespace uda::server
{

class ConfigError : std::runtime_error
{
  public:
    ConfigError(std::string_view description) : std::runtime_error(description.data()) {}
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
        if (is<T>()) {
            return boost::any_cast<T>(_value);
        }
        throw ConfigError{ fmt::format("invalid cast for option {}", _name) };
    }

    template<>
    char as<char>() const
    {
        if (is<std::string>()) {
            auto string = boost::any_cast<std::string>(_value);
            if (string.size() != 1) {
                throw ConfigError{ fmt::format("invalid char value {} for option {}", string, _name) };
            }
        }
        throw ConfigError{ fmt::format("invalid cast for option {}", _name) };
    }

    template <class T> T as_or_default(T default_value) const
    {
        if (is<T>()) {
            return boost::any_cast<T>(_value);
        } else if (_value.empty()) {
            return default_value;
        }
        throw ConfigError{ fmt::format("invalid cast for option {}", _name) };
    }

    template <>
    char as_or_default(char default_value) const
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

  private:
    std::string _name;
    boost::any _value;
};

class ConfigImpl;

class Config
{
  public:
    Config();
    ~Config();
    void load(std::string_view file_name);
    Option get(std::string_view name) const;

  private:
    std::unique_ptr<ConfigImpl> _impl;
};

} // namespace uda::server
