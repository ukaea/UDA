#include "name_value_list.hpp"

#include <boost/algorithm/string.hpp>

NameValueList::NameValue NameValueList::parse_name_value(std::string pair, bool strip)
{
    boost::trim(pair);
    boost::to_lower(pair);

    auto pos = pair.find('=');
    if (pos != std::string::npos) {
        auto name = pair.substr(0, pos);
        auto value = pair.substr(pos + 1);

        boost::trim(name);
        boost::trim(value);

        if ((boost::starts_with(name, "\"") && boost::ends_with(name, "\""))
            || (boost::starts_with(name, "'") && boost::ends_with(name, "'"))) {
            value = value.substr(1, value.size() - 2);
        }

        return {name, value};
    }

    if (boost::starts_with(pair, "/")) {
        pair = pair.substr(1);
    }
    return {pair, "true"};
}

void NameValueList::add_value(const std::string& pair, bool strip) {
    auto item = parse_name_value(pair, strip);
    _mapping.emplace(item.name, item.value);
    _items.push_back(item);
}

std::vector<NameValueList::NameValue> NameValueList::parse(std::string_view input, const bool strip) {
    std::vector<NameValue> name_values;

    std::vector<std::string> tokens;
    boost::split(tokens, input, boost::is_any_of(","), boost::token_compress_on);

    name_values.reserve(tokens.size());
    for (const auto& token : tokens) {
        name_values.push_back(parse_name_value(token, strip));
    }

    return name_values;
}

// const std::string& NameValueList::get_value(const std::string& name) const {
//     return _mapping.at(name);
// }
//
// const std::string& NameValueList::get_name(const int index) const {
//     return _items[index].name;
// }

// const std::vector<std::string>& NameValueList::get_values() const {
//
// }
