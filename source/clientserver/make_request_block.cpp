#include "logging/logging.h"
#include "udaStructs.h"
#include "server/serverPlugin.h"

#include <boost/algorithm/string.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/regex.hpp>
#include <fmt/format.h>
#include <string>

static const boost::regex SOURCE_RE(
    R"(^(?<device>(?:[a-z]+::)*)(((?<pulse>[0-9]+)(\/(?<pass>[0-9a-z]+))?)|(?<path>\/[a-z0-9\/\.]+)|((?<function>[a-z]+)\((?<args>.*)\)))$)",
    boost::regex::icase);
static const boost::regex SIGNAL_RE(
    R"(^(?<archive>[a-z]+::)?($|((?<signal>[a-z]+)|((?<function>[a-z]+)\((?<args>.*)\)))(?<subsets>(\[([^\[\]]*)\])*)$))",
    boost::regex::icase);

namespace uda
{
struct NameValue {
    std::string pair;
    std::string name;
    std::string value;
};

std::vector<uda::NameValue> parse_args(std::string_view input, bool strip);

void parse_signal(RequestData& result, const std::string& signal, const PluginList& plugin_list);

void parse_source(RequestData& result, const std::string& source);

void write_string(char* out, std::string in, size_t len);

static int find_plugin_id_by_format(std::string_view format, const PluginList& plugin_list)
{
    for (int i = 0; i < plugin_list.count; i++) {
        if (format == plugin_list.plugin[i].format) {
            return i;
        }
    }
    return -1;
}

std::string expand_environment_variables(const std::string& path);

void write_int(int* out, long in)
{
    if (in > INT_MAX || in < INT_MIN) {
        throw std::runtime_error{"int is too large"};
    }
    *out = static_cast<int>(in);
}

void write_int(int* out, std::size_t in)
{
    if (in > INT_MAX) {
        throw std::runtime_error{"int is too large"};
    }
    *out = static_cast<int>(in);
}

void write_name_values(NameValueList& name_value_list, const std::vector<uda::NameValue>& name_values)
{
    write_int(&name_value_list.listSize, name_values.size());
    write_int(&name_value_list.pairCount, name_values.size());
    name_value_list.nameValue = static_cast<::NameValue*>(malloc(name_values.size() * sizeof(NameValue)));
    size_t i = 0;
    for (const auto& name_value : name_values) {
        name_value_list.nameValue[i].pair = strdup(name_value.pair.c_str());
        name_value_list.nameValue[i].name = strdup(name_value.name.c_str());
        name_value_list.nameValue[i].value = strdup(name_value.value.c_str());
        ++i;
    }
}

uda::NameValue parse_name_value(std::string_view argument, bool strip)
{
    std::vector<std::string> tokens;
    boost::split(tokens, argument, boost::is_any_of("="), boost::token_compress_on);

    for (auto& token : tokens) {
        boost::trim(token);
    }

    uda::NameValue name_value = {};
    name_value.pair = argument;

    if (tokens.size() == 2) {
        // argument is name=value
        name_value.name = tokens[0];
        name_value.value = tokens[1];
    } else if (tokens.size() == 1) {
        // argument is name or /name
        if (boost::starts_with(tokens[0], "/")) {
            name_value.name = tokens[0].substr(1);
        } else {
            name_value.name = tokens[0];
        }
    } else {
        throw std::runtime_error{"invalid token"};
    }

    if (strip) {
        boost::trim_if(name_value.value, boost::is_any_of("'\""));
    }

    return name_value;
}

OPTIONAL_LONG parse_integer(const std::string& value)
{
    if (value.empty()) {
        return {.init = false, .value = 0};
    }
    size_t idx;
    long num = std::stol(value, &idx, 10);
    if (idx != value.size()) {
        throw std::runtime_error("invalid integer");
    }
    return {.init = true, .value = num};
}

void parse_element(Subset& subset, const std::string& element)
{
    std::vector<std::string> tokens;
    boost::split(tokens, element, boost::is_any_of(":"), boost::token_compress_off);

    int index = subset.nbound;
    strcpy(subset.operation[index], ":");
    subset.dimid[index] = index;

    switch (tokens.size()) {
        case 0:
            subset.lbindex[index] = {.init = false, .value = 0};
            subset.ubindex[index] = {.init = false, .value = 0};
            subset.stride[index] = {.init = false, .value = 0};
            break;
        case 1:
            // TODO: handle non-slice operations? i.e. [>=4], etc.
            subset.lbindex[index] = parse_integer(tokens[0]);
            subset.ubindex[index] = {.init = true, .value = (subset.lbindex[index].value + 1)};
            subset.stride[index] = {.init = false, .value = 0};
            break;
        case 2:
            subset.lbindex[index] = parse_integer(tokens[0]);
            subset.ubindex[index] = parse_integer(tokens[1]);
            subset.stride[index] = {.init = false, .value = 0};
            break;
        case 3:
            subset.lbindex[index] = parse_integer(tokens[0]);
            subset.ubindex[index] = parse_integer(tokens[1]);
            subset.stride[index] = parse_integer(tokens[2]);
            break;
        default:
            throw std::runtime_error{"invalid number of elements in subset operation"};
    }

    subset.nbound += 1;
}

void parse_operation(Subset& subset, const std::string& operation)
{
    //----------------------------------------------------------------------------------------------------------------------------
    // Split instructions using syntax [a:b:c, d:e:f] where [startIndex:stopIndex:stride]
    //
    // Syntax   [a]     single items at index position a
    //          [*]     all items
    //          []      all items
    //
    //          [:]     all items starting at 0
    //          [a:]    all items starting at a
    //          [a:*]   all items starting at a
    //          [a:b]   all items starting at a and ending at b
    //
    //          [a::c]  all items starting at a with stride c
    //          [a:*:c] all items starting at a with stride c
    //          [a:b:c] all items starting at a, ending at b with stride c

    std::vector<std::string> tokens;
    boost::split(tokens, operation, boost::is_any_of(","), boost::token_compress_off);

    for (const auto& token : tokens) {
        parse_element(subset, token);
    }
}

Subset parse_subsets(const std::vector<std::string>& subsets)
{
    Subset result = {0};

    for (const auto& subset : subsets) {
        parse_operation(result, subset);
    }

    return result;
}

std::string expand_environment_variables(const std::string& path)
{
    std::string new_path = path;

    if (path.find('$') == std::string::npos) {
        return new_path;
    }

    char old_cwd[STRING_LENGTH];
    size_t old_cwd_sz = STRING_LENGTH - 1;
    if (getcwd(old_cwd, old_cwd_sz) == nullptr) { // Current Working Directory
        UDA_LOG(UDA_LOG_DEBUG, "Unable to identify PWD!\n");
        return new_path;
    }

    if (chdir(path.c_str()) == 0) {
        // Change to path directory
        // The Current Working Directory is now the resolved directory name
        char cwd[STRING_LENGTH];
        size_t cwd_sz = STRING_LENGTH - 1;
        char* p_cwd = getcwd(cwd, cwd_sz);

        UDA_LOG(UDA_LOG_DEBUG, "Expanding embedded environment variable:\n");
        UDA_LOG(UDA_LOG_DEBUG, "from: %s\n", path.c_str());
        UDA_LOG(UDA_LOG_DEBUG, "to: %s\n", cwd);

        if (p_cwd != nullptr) {
            new_path = cwd; // The expanded path
        }
        if (chdir(old_cwd) != 0) {
            // Return to the Original WD
            UDA_LOG(UDA_LOG_ERROR, "failed to reset working directory\n");
        }
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "Direct substitution! \n");

        boost::regex env_re{R"(\$\{([^}]+)\}|\$([^{}\/]+))"};
        boost::sregex_iterator begin{new_path.begin(), new_path.end(), env_re};
        boost::sregex_iterator end;

        std::for_each(begin, end, [&](boost::sregex_iterator::reference match) {
            const auto& from = match[0];
            std::string name = !match[1].str().empty() ? match[1].str() : match[2].str();
            char* to = std::getenv(name.c_str());
            if (to != nullptr) {
                new_path.replace(from.first, from.second, to);
            }
        });

        UDA_LOG(UDA_LOG_DEBUG, "Expanding to: %s\n", new_path.c_str());
    }

    return new_path;
}

} // namespace uda

std::vector<uda::NameValue> uda::parse_args(std::string_view input, bool strip)
{
    std::vector<uda::NameValue> name_values;

    std::vector<std::string> tokens;
    boost::split(tokens, input, boost::is_any_of(","), boost::token_compress_on);

    name_values.reserve(tokens.size());
    for (const auto& token : tokens) {
        name_values.push_back(parse_name_value(token, strip));
    }

    return name_values;
}

void uda::parse_signal(RequestData& result, const std::string& signal, const PluginList& plugin_list)
{
    boost::smatch signal_match;
    if (!boost::regex_search(signal, signal_match, SIGNAL_RE)) {
        throw std::runtime_error{"invalid signal"};
    }

    std::string function = signal_match["function"];
    bool is_function = !function.empty();

    write_string(result.function, function, STRING_LENGTH);

    std::string archive = signal_match["archive"];
    boost::trim_right_if(archive, boost::is_any_of(":"));

    std::string plugin;
    if (is_function) {
        plugin = archive;
        archive = "";

        result.request = find_plugin_id_by_format(plugin, plugin_list);
    } else {
        result.request = REQUEST_READ_GENERIC;
    }

    write_string(result.archive, archive, STRING_LENGTH);

    std::string args = signal_match["args"];
    auto name_values = uda::parse_args(args, true);

    write_name_values(result.nameValueList, name_values);

    std::string subsets = signal_match["subsets"];
    write_string(result.subset, subsets, STRING_LENGTH);

    std::vector<std::string> tokens;
    boost::split(tokens, subsets, boost::is_any_of("[]"), boost::algorithm::token_compress_on);

    auto x = tokens | boost::adaptors::filtered([](const std::string& x) { return !x.empty(); }) |
             boost::adaptors::transformed([](const std::string& x) { return "[" + x + "]"; });
    tokens = std::vector<std::string>{x.begin(), x.end()};

    result.datasubset = parse_subsets(tokens);
}

void uda::parse_source(RequestData& result, const std::string& source)
{
    bool no_source = !source.empty();

    boost::smatch source_match;
    if (!boost::regex_search(source, source_match, SOURCE_RE) && !no_source) {
        throw std::runtime_error{"invalid source"};
    }

    std::string function = source_match["function"];
    bool is_function = !function.empty();

    write_string(result.function, function, STRING_LENGTH);

    std::string path = source_match["path"];
    bool is_file = !path.empty();

    path = expand_environment_variables(path);
    write_string(result.path, path, STRING_LENGTH);

    std::string s_pulse = source_match["pulse"];
    long pulse = std::stol(s_pulse);

    write_int(&result.exp_number, pulse);

    std::string s_pass = source_match["pass"];
    char* end = nullptr;
    long pass = std::strtol(s_pass.c_str(), &end, 10);

    write_int(&result.pass, pass);
    if (*end == '\0') {
        write_string(result.tpass, s_pass, STRING_LENGTH);
    }

    std::string device = source_match["device"];
    std::vector<std::string> tokens;
    boost::split(tokens, device, boost::is_any_of("::"), boost::token_compress_on);

    std::string plugin;
    std::string format;
    if (tokens.size() == 2) {
        device = tokens[0];
        if (is_function) {
            plugin = tokens[1];
        } else if (is_file) {
            format = tokens[1];
        } else {
            throw std::runtime_error{"invalid source"};
        }
    } else if (tokens.size() == 1) {
        if (is_function) {
            plugin = tokens[0];
        } else if (is_file) {
            format = tokens[0];
        } else {
            device = tokens[0];
        }
    } else if (!tokens.empty()) {
        throw std::runtime_error{"invalid source - too many prefixes"};
    }

    write_string(result.function, function, STRING_LENGTH);
    write_string(result.format, format, STRING_LENGTH);
}

void uda::write_string(char* out, std::string in, size_t len)
{
    if (in.size() > len) {
        throw std::runtime_error{"string is too long"};
    }
    std::strcpy(out, in.c_str());
}

RequestData make_request_data(const RequestData& request_data, const PluginList& plugin_list,
                              const Environment& environment)
{
    std::string delim;

    RequestData result = {0};

    if (request_data.api_delim[0] == '\0') {
        delim = environment.api_delim;
    } else {
        delim = request_data.api_delim;
    }

    std::string api_archive = environment.api_archive;
    std::string api_device = environment.api_device;

    boost::to_lower(api_archive);
    boost::to_lower(api_device);

    auto default_archive = fmt::format("{}{}", api_archive, delim);
    auto default_device = fmt::format("{}{}", api_device, delim);

    std::string signal = request_data.signal;
    std::string source = request_data.source;

    boost::trim(signal);
    boost::trim(source);

    boost::to_lower(signal);
    boost::to_lower(source);

    bool no_source = source.empty() || source == api_device || source == default_device;

    if ((signal.empty() || signal == default_archive) && no_source) {
        throw std::runtime_error{"neither data object nor source specified"};
    }

    if (boost::starts_with(source, default_device)) {
        source = source.substr(default_device.size());
        boost::trim_left(source);
    }

    bool is_proxy = environment.server_proxy[0] == '\0';

    if (is_proxy) {
        result.request = REQUEST_READ_IDAM;
    }

    uda::write_string(result.api_delim, delim, MAXNAME);
    uda::write_string(result.signal, signal, MAXMETA);
    uda::write_string(result.source, source, STRING_LENGTH);

    uda::parse_source(result, source);
    uda::parse_signal(result, signal, plugin_list);

    return result;
}