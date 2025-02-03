#include "nameValueSubstitution.h"

#include <cerrno>
#include <cstdlib>

#if defined(__GNUC__)
#  include <strings.h>
#  include <unistd.h>
#endif

#include "clientserver/errorLog.h"
#include "clientserver/makeRequestBlock.h"
#include "common/stringUtils.h"
#include "initStructs.h"
#include "logging/logging.h"
#include "udaErrors.h"
#include <fmt/format.h>

using namespace uda::client_server;
using namespace uda::logging;
using namespace uda::common;

static void embedded_value_substitution(NameValueList& name_value_list);

// Deconstruct the text pass parameter (tpass) for name value placeholder substitution values
// Identify name value placeholders in the signal argument and replace with substitution values
// Add additional name-value pairs and keywords to the plugin input

// shot/tpass data source pattern: "12345/a,b,c, name=value, name=value, d, e, delimiter=',', placeholder='$'"
//

int identify_placeholders(NameValueList& name_value_list, NameValueList& new_name_value_list) {

    size_t placeholder_count = 0;
    size_t tpass_position = 0;

    std::vector<int> placeholder_indices(name_value_list.size());
    std::vector<size_t> tpass_index(name_value_list.size());

    int i = 0;
    for (auto& [_1, name, value] : name_value_list) {

        // Is it a placeholder? (Not a keyword and beginning '$')
        if (!value.empty() && value[0] == '$') {
            placeholder_indices[placeholder_count] = i; // Identify which pair
            tpass_index[placeholder_count] = tpass_position++; // Ordering: Default substitution value to use - list position

            if (value.size() > 1 && value[1] != '\0') {
                if (is_number(&value[1])) { // Is the placeholder numbered? - Use the specific substitution
                                                 // value identified by list order
                    tpass_index[placeholder_count] = atoi(&value[1]) - 1; // Position labelling begins '1' not '0'
                } else {
                    // scan the list of substitute pairs and match by name (case-sensitive)
                    int j = 0;
                    for (auto& comp_name : new_name_value_list.names()) {
                        if (comp_name == &value[1]) {
                            tpass_index[placeholder_count] = j;
                            break;
                        }
                        ++j;
                    }
                }
            }
            placeholder_count++;
            if (placeholder_count <= new_name_value_list.size()) {
                UDA_LOG(UDA_LOG_DEBUG, "[{}] name: {}, substitution position {}, substitution value {}",
                        placeholder_count, name, tpass_index[placeholder_count - 1],
                        new_name_value_list.value(tpass_index[placeholder_count - 1]));
            }
        }
        ++i;
    }

    if (placeholder_count == 0) {
        return 0; // There are no placeholders to process
    }

    UDA_LOG(UDA_LOG_DEBUG, "Name Value Pair Count = {}", name_value_list.size());
    UDA_LOG(UDA_LOG_DEBUG, "Placeholder Count     = {}", placeholder_count);
    // UDA_LOG(UDA_LOG_DEBUG, "Additional Name Value Pairs and Substitutions = {}", tpass);
    UDA_LOG(UDA_LOG_DEBUG, "Additional Name Value Pairs and Substitutions Count = {}", new_name_value_list.size());

    // Placeholder substitutions (must be the specified first in tpass)

    if (placeholder_count > new_name_value_list.size()) {
        // Too many placeholders for the available substitutions
        UDA_LOG(UDA_LOG_DEBUG, "Inconsistent count of placeholders and available substitutions!");
        constexpr int err = 999;
        add_error(ErrorType::Code, "nameValueSubstitution", err,
                  "Inconsistent count of placeholders and available substitutions!");
        return err;
    }

    // Replace placeholders with identified values
    // Replacement values can be used multiple times

    for (size_t i = 0; i < placeholder_count; i++) {
        if (tpass_index[i] < 0 || tpass_index[i] > new_name_value_list.size()) {
            UDA_LOG(UDA_LOG_DEBUG, "Placeholder numbering is Inconsistent with Placeholder Count!");
            UDA_LOG(UDA_LOG_DEBUG, "tpass_index[{}] = {}  ({})", i, tpass_index[i], placeholder_count);
            constexpr int err = 999;
            add_error(ErrorType::Code, "nameValueSubstitution", err,
                      "Placeholder numbering is Inconsistent with Placeholder Count!");
            return err;
        }

        int placeholder_index = placeholder_indices[i];
        name_value_list.set_value(placeholder_index, new_name_value_list.value(tpass_index[i]));

        UDA_LOG(UDA_LOG_DEBUG, "Placeholder: [{}][{}] {}, Substitution Value [{}] {}", i, placeholder_index,
                name_value_list.name(placeholder_index), tpass_index[i],
                name_value_list.value(placeholder_index));
    }

    return 0;
}

int uda::client_server::name_value_substitution(NameValueList& name_value_list, const char* tpass)
{
    if (!tpass && strlen(tpass) == 0) {
        return 0; // nothing to substitute and no new NV pairs
    }

    UDA_LOG(UDA_LOG_DEBUG, "nameValueSubstitution - Initial set of {} NV pairs", name_value_list.size());
    size_t i = 0;
    for (auto& [pair, name, value] : name_value_list) {
        UDA_LOG(UDA_LOG_DEBUG, "Pair[{}] = {}, Name = {}, value = {}", i, pair, name, value);
        ++i;
    }

    NameValueList new_name_value_list{tpass, false};

    if (new_name_value_list.empty()) {
        // No passed substitution values or additional name value pairs
        return 0;
    }

    UDA_LOG(UDA_LOG_DEBUG, "nameValueSubstitution - Additional set of {} NV pairs", new_name_value_list.size());
    i = 0;
    for (auto& [pair, name, value] : name_value_list) {
        UDA_LOG(UDA_LOG_DEBUG, "Pair[{}] = {}, Name = {}, value = {}", i, pair, name, value);
        ++i;
    }

    // How many placeholders? [beginning with a '$' character. Placeholders may be numbered, named or simply a single
    // naked $ character]

    if (!name_value_list.empty()) {
        // Identify and Count Placeholders
        int err = identify_placeholders(name_value_list, new_name_value_list);
        if (err) {
            return err;
        }
    }

    // Add unused new name value pairs and keywords

    for (auto& [pair, name, value] : new_name_value_list) {
        name_value_list.append(pair, name, value);
    }

    // Scan all values for embedded placeholders and substitute

    embedded_value_substitution(name_value_list);

    return 0;
}

// Substitute named placeholders within value strings

// patterns with name values: string="UDA::getdata(variable='/a/b/c', shot=$shot, tstart=$tstart, tend=$tend)"
// substitution for named string elements: $shot, $tstart, $tend

void embedded_value_substitution(NameValueList& name_value_list)
{
    if (name_value_list.empty()) {
        return;
    }

    UDA_LOG(UDA_LOG_DEBUG, "embeddedValueSubstitution");
    int i = 0;
    for (auto& [pair, name, value] : name_value_list) {
        UDA_LOG(UDA_LOG_DEBUG, "Pair[{}] = {}, Name = {}, value = {}", i, pair, name, value);
        ++i;
    }

    // Search all NV pairs for values with placeholders

    i = 0;
    for (auto& [pair, name, value] : name_value_list) {

        auto pos = value.find('$');
        if (pos == std::string::npos) {
            continue;
        }

        UDA_LOG(UDA_LOG_DEBUG, "Extracting Name Value Pairs from [{}]: {}", i, value);

        // Extract NV pairs and keywords from the input placeholder value

        NameValueList new_name_value_list{value, true};

        UDA_LOG(UDA_LOG_DEBUG, "Embedded Pair Count = {}", new_name_value_list.size());
        int j = 0;
        for (auto& [pair, name, value] : new_name_value_list) {
            UDA_LOG(UDA_LOG_DEBUG, "Pair[{}] {}, Name = {}, value = {}", j, pair, name, value);
            ++j;
        }

        j = 0;
        for (auto& [pair, name, value] : new_name_value_list) {
            if (!value.empty() && value[0] == '$') {
                // Match with substitution value

                int k = 0;
                for (auto& [comp_pair, comp_name, comp_value] : new_name_value_list) {
                    if (k == i) {
                        continue;
                    }

                    // Match by name (case-sensitive) only
                    if (comp_name == &value[0]) {

                        UDA_LOG(UDA_LOG_DEBUG, "Substitution: embedded[{}] {}={} with [{}] {}={}", j,
                                name, value, k, comp_name, comp_value);

                        // Substitute into original string and replace original

                        // Substitute here with name_value_list->nameValue[k].value
                        std::string original = comp_value;
                        UDA_LOG(UDA_LOG_DEBUG, "Original: {}", original);

                        char* pp = strstr(original.data(), value.c_str());
                        int lstr = value.size(); // Target this
                        int ok = 1;

                        UDA_LOG(UDA_LOG_DEBUG, "targeting {} [{}] from {} to {}", pp, lstr, value, comp_value);

                        for (int m = 0; m < lstr; m++) {
                            ok = ok && pp[m] == comp_value[m]; // Test the name is correct
                        }

                        if (ok) {
                            UDA_LOG(UDA_LOG_DEBUG, "Substituting {} with {}", value, comp_value);

                            pp[0] = '\0';
                            std::string replace =
                                fmt::format("{}{}{}", original, comp_value, &pp[lstr]);

                            UDA_LOG(UDA_LOG_DEBUG, "original {}", original);
                            UDA_LOG(UDA_LOG_DEBUG, "value    {}", comp_value);
                            UDA_LOG(UDA_LOG_DEBUG, "residual {}", &pp[lstr]);
                            UDA_LOG(UDA_LOG_DEBUG, "Modified Original {}", replace.c_str());

                            name_value_list.set_value(i, replace);

                            break;
                        }
                    }
                    ++k;
                }
            }
            ++j;
        }
        ++i;
    }
}
