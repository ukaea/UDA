#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <tuple>
#include <boost/range/adaptor/map.hpp>

class NameValueList {
public:
    struct NameValue {
        std::string pair;
        std::string name;
        std::string value;
    };

    NameValueList() {}
    NameValueList(const std::string_view input, const bool strip) {
        const auto values = parse(input, strip);
        for (const auto& value : values) {
            append(value);
        }
    }

    static std::vector<NameValue> parse(std::string_view input, bool strip);

    void add_value(const std::string& pair, bool strip);

    const char* find(const std::string& name) const {
        const auto it = _mapping.find(name);
        if (it == _mapping.end()) {
            return nullptr;
        }
        return it->second.c_str();
    }

    bool contains(const std::string& name) const {
        return _mapping.find(name) != _mapping.end();
    }

    auto begin() const { return _items.begin(); }
    auto end() const { return _items.end(); }

    size_t size() const { return _items.size(); }
    bool empty() const { return _items.empty(); }

    const std::string& operator[](const std::string& name) const { return _mapping.at(name); }

    const std::string& name(const int idx) const { return _items.at(idx).name; }
    const std::string& value(const int idx) const { return _items.at(idx).value; }

    void set_value(const int idx, const std::string& value) {
        _items.at(idx).value = value;
        _mapping.at(_items.at(idx).name) = value;
    }

    void append(const std::string& pair, const std::string& name, const std::string& value) {
        _items.push_back({pair, name, value});
        _mapping.insert({name, value});
    }

    void append(const NameValue& name_vale) {
        _mapping.emplace(name_vale.name, name_vale.value);
        _items.push_back(name_vale);
    }

    auto names() const { return NameList(*this); }

private:

    class NameIterator {
    public:
        explicit NameIterator(const std::vector<NameValue>::const_iterator iter) : _iter(iter) {}
        NameIterator& operator++() { ++_iter; return *this; }
        NameIterator operator++(int) { const NameIterator retval = *this; ++_iter; return retval; }
        bool operator==(const NameIterator& other) const { return _iter == other._iter; }
        bool operator!=(const NameIterator& other) const { return _iter != other._iter; }
        const std::string& operator*() const { return _iter->name; }

    private:
        std::vector<NameValue>::const_iterator _iter;
    };

    class NameList {
    public:
        explicit NameList(const NameValueList& list) : _list(list) {}
        NameIterator begin() const { return NameIterator(_list.begin()); }
        NameIterator end() const { return NameIterator(_list.end()); }
    private:
        const NameValueList& _list;
    };

    std::vector<NameValue> _items;
    std::unordered_map<std::string, std::string> _mapping;

    static NameValue parse_name_value(std::string pair, bool strip);
};
