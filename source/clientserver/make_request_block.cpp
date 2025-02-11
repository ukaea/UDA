#include "logging/logging.h"
#include "udaStructs.h"
#include "config/config.h"
#include "plugins.h"

#include <boost/algorithm/string.hpp>
#include <boost/range/adaptors.hpp>
#include <regex>
#include <fmt/format.h>
#include <string>

using namespace uda::client_server;
using namespace uda::logging;
using namespace uda::config;

using namespace std::string_literals;

static const std::regex SourceRegex(
    R"(^((?:[a-z]+::)*)((([0-9]+)(\/([0-9a-z]+))?)|(\/[a-z0-9\/\.]+)|(([a-z]+)\((.*)\)))$)",
    std::regex::icase);

// std::regex doesn't support named capture groups
constexpr int SourceDeviceCaptureIdx = 1;
constexpr int SourcePulseCaptureIdx = 2;
constexpr int SourcePassCaptureIdx = 3;
constexpr int SourcePathCaptureIdx = 4;
constexpr int SourceFunctionCaptureIdx = 5;
// constexpr int SourceArgsCaptureIdx = 6;

static const std::regex SignalRegex(
    R"(^([a-z]+::)?($|(([a-z]+)|(([a-z]+)\((.*)\)))((\[([^\[\]]*)\])*)$))",
    std::regex::icase);

// std::regex doesn't support named capture groups
constexpr int SignalArchiveCaptureIdx = 1;
// constexpr int SignalSignalCaptureIdx = 2;
constexpr int SignalFunctionCaptureIdx = 3;
constexpr int SignalArgsCaptureIdx = 4;
constexpr int SignalSubsetsCaptureIdx = 5;

namespace uda
{

void parse_signal(RequestData& result, const std::string& signal, const std::vector<PluginData>& plugin_list);

void parse_source(RequestData& result, const std::string& source);

void write_string(char* out, std::string in, size_t len);

static int find_plugin_id_by_format(std::string_view format, const std::vector<PluginData>& plugin_list)
{
    size_t id = 0;
    for (const auto& plugin : plugin_list) {
        if (plugin.name == format) {
            return id;
        }
    }
    return -1;
}

std::string udaExpandEnvironmentalVariables(const std::string& path);

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

void write_name_values(NameValueList& name_value_list, const std::vector<NameValueList::NameValue>& name_values)
{
    for (const auto& name_value : name_values) {
        name_value_list.append(name_value);
    }
}

OptionalLong parse_integer(const std::string& value)
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

std::string udaExpandEnvironmentalVariables(const std::string& path)
{
    std::string new_path = path;

    if (path.find('$') == std::string::npos) {
        return new_path;
    }

    char old_cwd[StringLength];
    size_t old_cwd_sz = StringLength - 1;
    if (getcwd(old_cwd, old_cwd_sz) == nullptr) { // Current Working Directory
        UDA_LOG(UDA_LOG_DEBUG, "Unable to identify PWD!");
        return new_path;
    }

    if (chdir(path.c_str()) == 0) {
        // Change to path directory
        // The Current Working Directory is now the resolved directory name
        char cwd[StringLength];
        size_t cwd_sz = StringLength - 1;
        char* p_cwd = getcwd(cwd, cwd_sz);

        UDA_LOG(UDA_LOG_DEBUG, "Expanding embedded environment variable:");
        UDA_LOG(UDA_LOG_DEBUG, "from: {}", path.c_str());
        UDA_LOG(UDA_LOG_DEBUG, "to: {}", cwd);

        if (p_cwd != nullptr) {
            new_path = cwd; // The expanded path
        }
        if (chdir(old_cwd) != 0) {
            // Return to the Original WD
            UDA_LOG(UDA_LOG_ERROR, "failed to reset working directory");
        }
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "Direct substitution!");

        std::regex env_re{R"(\$\{([^}]+)\}|\$([^{}\/]+))"};
        std::sregex_iterator begin{new_path.begin(), new_path.end(), env_re};
        std::sregex_iterator end;

        std::for_each(begin, end, [&](std::sregex_iterator::reference match) {
            const auto& from = match[0];
            const std::string name = !match[1].str().empty() ? match[1].str() : match[2].str();
            const char* to = std::getenv(name.c_str());
            if (to != nullptr) {
                new_path.replace(from.first, from.second, to);
            }
        });

        UDA_LOG(UDA_LOG_DEBUG, "Expanding to: {}", new_path.c_str());
    }

    return new_path;
}

} // namespace uda

void uda::parse_signal(RequestData& result, const std::string& signal, const std::vector<PluginData>& plugin_list)
{
    std::smatch signal_match;
    if (!std::regex_search(signal, signal_match, SignalRegex)) {
        throw std::runtime_error{"invalid signal"};
    }

    // device 1
    // pulse 2
    // pass 3
    // path 4
    // function 5
    // args 6
    std::string function = signal_match[SignalFunctionCaptureIdx];
    bool is_function = !function.empty();

    write_string(result.function, function, StringLength);

    std::string archive = signal_match[SignalArchiveCaptureIdx];
    boost::trim_right_if(archive, boost::is_any_of(":"));

    std::string plugin;
    if (is_function) {
        plugin = archive;
        archive = "";

        result.request = find_plugin_id_by_format(plugin, plugin_list);
    } else {
        result.request = static_cast<int>(Request::ReadGeneric);
    }

    write_string(result.archive, archive, StringLength);

    std::string args = signal_match[SignalArgsCaptureIdx];
    auto name_values = NameValueList::parse(args, true);

    write_name_values(result.name_value_list, name_values);

    std::string subsets = signal_match[SignalSubsetsCaptureIdx];
    write_string(result.subset, subsets, StringLength);

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

    std::smatch source_match;
    if (!std::regex_search(source, source_match, SourceRegex) && !no_source) {
        throw std::runtime_error{"invalid source"};
    }

    std::string function = source_match[SourceFunctionCaptureIdx];
    bool is_function = !function.empty();

    write_string(result.function, function, StringLength);

    std::string path = source_match[SourcePathCaptureIdx];
    bool is_file = !path.empty();

    path = udaExpandEnvironmentalVariables(path);
    write_string(result.path, path, StringLength);

    std::string s_pulse = source_match[SourcePulseCaptureIdx];
    long pulse = std::stol(s_pulse);

    write_int(&result.exp_number, pulse);

    std::string s_pass = source_match[SourcePassCaptureIdx];
    char* end = nullptr;
    long pass = std::strtol(s_pass.c_str(), &end, 10);

    write_int(&result.pass, pass);
    if (*end == '\0') {
        write_string(result.tpass, s_pass, StringLength);
    }

    std::string device = source_match[SourceDeviceCaptureIdx];
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

    write_string(result.function, function, StringLength);
    write_string(result.format, format, StringLength);
}

void uda::write_string(char* out, std::string in, size_t len)
{
    if (in.size() > len) {
        throw std::runtime_error{"string is too long"};
    }
    std::strcpy(out, in.c_str());
}

RequestData make_request_data(const Config& config, const RequestData& request_data, const std::vector<PluginData>& plugin_list)
{
    RequestData result = {0};

    std::string delim;

    if (request_data.api_delim[0] == '\0') {
        delim = config.get("request.delim").as_or_default("::"s);
    } else {
        delim = request_data.api_delim;
    }

    std::string api_archive = config.get("request.default_archive").as_or_default(""s);
    std::string api_device = config.get("request.default_device").as_or_default(""s);

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

    bool is_proxy = config.get("server.is_proxy").as_or_default(false);

    if (is_proxy) {
        result.request = (int)Request::ReadUDA;
    }

    uda::write_string(result.api_delim, delim, MaxName);
    uda::write_string(result.signal, signal, MaxMeta);
    uda::write_string(result.source, source, StringLength);

    uda::parse_source(result, source);
    uda::parse_signal(result, signal, plugin_list);

    return result;
}