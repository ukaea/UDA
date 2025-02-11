#include "name_value_list.hpp"

#include <boost/algorithm/string.hpp>

namespace {

std::string do_strip(std::string input, const bool strip) {
    if (!strip) {
        return input;
    }
    const size_t len = input.length();
    if (len < 2) {
        return input;
    }
    if ((input[0] == '"' && input[len - 1] == '"') || (input[0] == '\'' && input[len - 1] == '\'')) {
        return input.substr(1, len - 2);
    }
    return input;
}

} // anon namespace

NameValueList::NameValue NameValueList::parse_name_value(std::string pair, const bool strip)
{
    boost::trim(pair);
    boost::to_lower(pair);

    auto pos = pair.find('=');
    if (pos != std::string::npos) {
        auto name = pair.substr(0, pos);
        auto value = pair.substr(pos + 1);

        boost::trim(name);
        boost::trim(value);

        return {pair, do_strip(name, strip), do_strip(value, strip)};
    }

    std::string name = do_strip(pair, strip);
    if (!name.empty() && name[0] == '/') {
        name = name.substr(1);
    }
    return {pair, name, "true"};
}

void NameValueList::add_value(const std::string& pair, bool strip) {
    auto item = parse_name_value(pair, strip);
    mapping_.emplace(item.name, item.value);
    items_.push_back(item);
}

namespace {

// Quote aware string splitting
std::vector<std::string_view> split(const std::string_view& input, const char delim) {
    std::vector<std::string_view> result;
    char quote = '\0';
    size_t pos = 0;
    size_t last_delim_pos = 0;
    const size_t len = input.length();
    while (pos < len) {
        if (quote == '\0') {
            if (input[pos] == '"' || input[pos] == '\'') {
                quote = input[pos];
            } else if (input[pos] == delim) {
                if (pos > last_delim_pos) {
                    if (input[last_delim_pos] == delim) {
                        result.emplace_back(&input[last_delim_pos + 1], pos - last_delim_pos - 1);
                    } else {
                        result.emplace_back(&input[last_delim_pos], pos - last_delim_pos);
                    }
                }
                last_delim_pos = pos;
            }
        } else if (input[pos] == quote) {
                quote = '\0';
        }
        ++pos;
    }
    if (pos > last_delim_pos + 1) {
        if (input[last_delim_pos] == delim) {
            result.emplace_back(&input[last_delim_pos + 1], pos - last_delim_pos - 1);
        } else {
            result.emplace_back(&input[last_delim_pos], pos - last_delim_pos);
        }
    }
    return result;
}

} // anon namespace

std::vector<NameValueList::NameValue> NameValueList::parse(std::string input, const bool strip) {
    boost::trim(input);
    const std::vector<std::string_view> tokens = split(input, ',');

    std::vector<NameValue> name_values;
    name_values.reserve(tokens.size());
    for (const auto& token : tokens) {
        name_values.push_back(parse_name_value(std::string{token}, strip));
    }

    return name_values;
}
