#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class NameValueList {
public:
    struct NameValue {
        std::string pair;
        std::string name;
        std::string value;
    };

    NameValueList() = default;
    NameValueList(const std::string_view input, const bool strip) : input_{input} {
        const auto values = parse(input_, strip);
        for (const auto& value : values) {
            append(value);
        }
    }

    static std::vector<NameValue> parse(std::string input, bool strip);

    void add_value(const std::string& pair, bool strip);

    [[nodiscard]] const char* find(const std::string& name) const {
        const auto iter = mapping_.find(name);
        if (iter == mapping_.end()) {
            return nullptr;
        }
        return iter->second.c_str();
    }

    [[nodiscard]] bool contains(const std::string& name) const {
        return mapping_.find(name) != mapping_.end();
    }

    [[nodiscard]] auto begin() const { return items_.begin(); }
    [[nodiscard]] auto end() const { return items_.end(); }

    [[nodiscard]] size_t size() const { return items_.size(); }
    [[nodiscard]] bool empty() const { return items_.empty(); }

    const std::string& operator[](const std::string& name) const { return mapping_.at(name); }

    [[nodiscard]] const std::string& name(const size_t idx) const { return items_.at(idx).name; }
    [[nodiscard]] const std::string& value(const size_t idx) const { return items_.at(idx).value; }

    void set_value(const size_t idx, const std::string& value) {
        items_.at(idx).value = value;
        mapping_.at(items_.at(idx).name) = value;
    }

    void append(const std::string& pair, const std::string& name, const std::string& value) {
        items_.push_back({pair, name, value});
        mapping_.insert({name, value});
    }

    void append(const NameValue& name_vale) {
        mapping_.emplace(name_vale.name, name_vale.value);
        items_.push_back(name_vale);
    }

    [[nodiscard]] auto names() const { return NameList(*this); }

private:

    class NameIterator {
    public:
        explicit NameIterator(const std::vector<NameValue>::const_iterator iter) : iter_(iter) {}
        NameIterator& operator++() { ++iter_; return *this; }
        NameIterator operator++(int) { const NameIterator retval = *this; ++iter_; return retval; }
        bool operator==(const NameIterator& other) const { return iter_ == other.iter_; }
        bool operator!=(const NameIterator& other) const { return iter_ != other.iter_; }
        const std::string& operator*() const { return iter_->name; }

    private:
        std::vector<NameValue>::const_iterator iter_;
    };

    class NameList {
    public:
        explicit NameList(const NameValueList& list) : list_(list) {}
        [[nodiscard]] NameIterator begin() const { return NameIterator(list_.begin()); }
        [[nodiscard]] NameIterator end() const { return NameIterator(list_.end()); }
    private:
        const NameValueList& list_;
    };

    std::string input_;
    std::vector<NameValue> items_;
    std::unordered_map<std::string, std::string> mapping_;

    static NameValue parse_name_value(std::string pair, bool strip);
};
