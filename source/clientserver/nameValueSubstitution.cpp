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

static void embedded_value_substitution(NameValueList* name_value_list);

// Deconstruct the text pass parameter (tpass) for name value placeholder substitution values
// Identify name value placeholders in the signal argument and replace with substitution values
// Add additional name-value pairs and keywords to the plugin input

// shot/tpass data source pattern: "12345/a,b,c, name=value, name=value, d, e, delimiter=',', placeholder='$'"
//

int uda::client_server::name_value_substitution(NameValueList* name_value_list, const char* tpass)
{
    int err = 0;

    if (!tpass && strlen(tpass) == 0) {
        return 0; // nothing to substitute and no new NV pairs
    }

    UDA_LOG(UDA_LOG_DEBUG, "nameValueSubstitution - Initial set of {} NV pairs", name_value_list->pairCount);
    for (int i = 0; i < name_value_list->pairCount; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "Pair[{}] = {}, Name = {}, value = {}", i, name_value_list->nameValue[i].pair,
                name_value_list->nameValue[i].name, name_value_list->nameValue[i].value);
    }

    NameValueList new_name_value_list;
    init_name_value_list(&new_name_value_list);

    unsigned short strip = 0; // Do Not Remove enclosing quotes from name value pairs
    if (name_value_pairs(tpass, &new_name_value_list, strip) == -1) {
        err = 999;
        add_error(ErrorType::Code, "nameValueSubstitution", err, "Name Value pair syntax is incorrect!");
        return err;
    }

    if (new_name_value_list.pairCount == 0) { // No passed substitution values or additional name value pairs
        if (new_name_value_list.nameValue != nullptr) {
            free_name_value_list(&new_name_value_list);
        }
        return 0;
    }

    UDA_LOG(UDA_LOG_DEBUG, "nameValueSubstitution - Additional set of {} NV pairs", new_name_value_list.pairCount);
    for (int i = 0; i < new_name_value_list.pairCount; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "Pair[{}] = {}, Name = {}, value = {}", i, new_name_value_list.nameValue[i].pair,
                new_name_value_list.nameValue[i].name, new_name_value_list.nameValue[i].value);
    }

    // How many placeholders? [beginning with a '$' character. Placeholders may be numbered, named or simply a single
    // naked $ character]

    int placeholder_count = 0;
    int* placeholder_index = nullptr;
    int* tpass_index = nullptr;
    int tpass_position = 0;

    if (name_value_list->pairCount > 0) { // Identify and Count Placeholders
        do {
            placeholder_index = (int*)malloc(name_value_list->pairCount * sizeof(int));
            tpass_index = (int*)malloc(name_value_list->pairCount * sizeof(int));

            for (int i = 0; i < name_value_list->pairCount; i++) {
                // Is it a placeholder? (Not a keyword and begining '$')
                if (name_value_list->nameValue[i].value != nullptr && name_value_list->nameValue[i].value[0] == '$') {
                    placeholder_index[placeholder_count] = i; // Identify which pair
                    tpass_index[placeholder_count] =
                        tpass_position++; // Ordering: Default substitution value to use - list position
                    if (name_value_list->nameValue[i].value[1] != '\0') {
                        if (is_number(&name_value_list->nameValue[i]
                                           .value[1])) { // Is the placeholder numbered? - Use the specific substitution
                                                         // value identified by list order
                            tpass_index[placeholder_count] = atoi(&name_value_list->nameValue[i].value[1]) -
                                                           1; // Position labelling begins '1' not '0'
                        } else {
                            // scan the list of substitute pairs and match by name (case sensitive)
                            for (int j = 0; j < new_name_value_list.pairCount; j++) {
                                if (strcmp(&name_value_list->nameValue[i].value[1], new_name_value_list.nameValue[j].name) ==
                                    0) {
                                    tpass_index[placeholder_count] = j;
                                    break;
                                }
                            }
                        }
                    }
                    placeholder_count++;
                    if (placeholder_count <= new_name_value_list.pairCount) {
                        UDA_LOG(UDA_LOG_DEBUG, "[{}] name: {}, substitution position {}, substitution value {}",
                                placeholder_count, name_value_list->nameValue[i].name, tpass_index[placeholder_count - 1],
                                new_name_value_list.nameValue[tpass_index[placeholder_count - 1]].value);
                    }
                }
            }

            if (placeholder_count == 0) {
                break; // There are no placeholders to process
            }

            UDA_LOG(UDA_LOG_DEBUG, "Name Value Pair Count = {}", name_value_list->pairCount);
            UDA_LOG(UDA_LOG_DEBUG, "Placeholder Count     = {}", placeholder_count);
            UDA_LOG(UDA_LOG_DEBUG, "Additional Name Value Pairs and Substitutions = {}", tpass);
            UDA_LOG(UDA_LOG_DEBUG, "Additional Name Value Pairs and Substitutions Count = {}",
                    new_name_value_list.pairCount);

            // Placeholder substitutions (must be the specified first in tpass)

            if (placeholder_count > new_name_value_list.pairCount) {
                // Too many placeholders for the available substitutions
                UDA_LOG(UDA_LOG_DEBUG, "Inconsistent count of placeholders and available substitutions!");
                err = 999;
                add_error(ErrorType::Code, "nameValueSubstitution", err,
                          "Inconsistent count of placeholders and available substitutions!");
                break;
            }

            // Replace placeholders with identifed values
            // Replacement values can be used multiple times

            for (int i = 0; i < placeholder_count; i++) {
                if (tpass_index[i] < 0 || tpass_index[i] > new_name_value_list.pairCount) {
                    UDA_LOG(UDA_LOG_DEBUG, "Placeholder numbering is Inconsistent with Placeholder Count!");
                    UDA_LOG(UDA_LOG_DEBUG, "tpass_index[{}] = {}  ({})", i, tpass_index[i], placeholder_count);
                    err = 999;
                    add_error(ErrorType::Code, "nameValueSubstitution", err,
                              "Placeholder numbering is Inconsistent with Placeholder Count!");
                    break;
                }

                if (name_value_list->nameValue[placeholder_index[i]].value != nullptr) {
                    free(name_value_list->nameValue[placeholder_index[i]].value);
                }
                name_value_list->nameValue[placeholder_index[i]].value = new_name_value_list.nameValue[tpass_index[i]].value;
                new_name_value_list.nameValue[tpass_index[i]].value = nullptr;

                UDA_LOG(UDA_LOG_DEBUG, "Placeholder: [{}][{}] {}, Substitution Value [{}] {}", i, placeholder_index[i],
                        name_value_list->nameValue[placeholder_index[i]].name, tpass_index[i],
                        name_value_list->nameValue[placeholder_index[i]].value);
            }

            // Remove all used NV pairs used in placeholder substitution
            for (int i = 0; i < placeholder_count; i++) {
                if (new_name_value_list.nameValue[tpass_index[i]].pair != nullptr) {
                    free(new_name_value_list.nameValue[tpass_index[i]].pair);
                }
                if (new_name_value_list.nameValue[tpass_index[i]].name != nullptr) {
                    free(new_name_value_list.nameValue[tpass_index[i]].name);
                }
                if (new_name_value_list.nameValue[tpass_index[i]].value != nullptr) {
                    free(new_name_value_list.nameValue[tpass_index[i]].value);
                }
                new_name_value_list.nameValue[tpass_index[i]].pair = nullptr;
                new_name_value_list.nameValue[tpass_index[i]].name = nullptr;
                new_name_value_list.nameValue[tpass_index[i]].value = nullptr;
            }

        } while (0);
    }

    free(tpass_index);
    free(placeholder_index);
    if (err != 0) {
        free_name_value_list(&new_name_value_list);
        return err;
    }

    // Add unused new name value pairs and keywords

    if (name_value_list->pairCount + new_name_value_list.pairCount > name_value_list->listSize) {
        name_value_list->nameValue =
            (NameValue*)realloc((void*)name_value_list->nameValue,
                                (name_value_list->listSize + new_name_value_list.pairCount) * sizeof(NameValue));
        name_value_list->listSize = name_value_list->listSize + new_name_value_list.pairCount;
    }

    for (int i = 0; i < new_name_value_list.pairCount; i++) {
        if (new_name_value_list.nameValue[i].name != nullptr) {
            name_value_list->nameValue[name_value_list->pairCount].pair = new_name_value_list.nameValue[i].pair;
            if (name_value_list->nameValue[name_value_list->pairCount].pair == nullptr) {
                name_value_list->nameValue[name_value_list->pairCount].pair = strdup("");
            }
            name_value_list->nameValue[name_value_list->pairCount].name = new_name_value_list.nameValue[i].name;
            name_value_list->nameValue[name_value_list->pairCount++].value = new_name_value_list.nameValue[i].value;
            UDA_LOG(UDA_LOG_DEBUG, "[{}] Name = {}, Value = {}", i, new_name_value_list.nameValue[i].name,
                    new_name_value_list.nameValue[i].value);
            new_name_value_list.nameValue[i].pair = nullptr;
            new_name_value_list.nameValue[i].name = nullptr;
            new_name_value_list.nameValue[i].value = nullptr;
        }
    }

    free_name_value_list(&new_name_value_list);

    // Scan all values for embedded placeholders and substitute

    embedded_value_substitution(name_value_list);

    return 0;
}

// Substitute named placeholders within value strings

// patterns with name values: string="UDA::getdata(variable='/a/b/c', shot=$shot, tstart=$tstart, tend=$tend)"
// substitution for named string elements: $shot, $tstart, $tend

void embedded_value_substitution(NameValueList* name_value_list)
{
    int m;
    NameValueList new_name_value_list;

    if (name_value_list->pairCount == 0) {
        return;
    }

    UDA_LOG(UDA_LOG_DEBUG, "embeddedValueSubstitution");
    for (int i = 0; i < name_value_list->pairCount; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "Pair[{}] = {}, Name = {}, value = {}", i, name_value_list->nameValue[i].pair,
                name_value_list->nameValue[i].name, name_value_list->nameValue[i].value);
    }

    // Search all NV pairs for values with placeholders

    for (int i = 0; i < name_value_list->pairCount; i++) {

        char* p = strchr(name_value_list->nameValue[i].value, '$'); // Is there a placeholder?
        if (!p) {
            continue;
        }

        init_name_value_list(&new_name_value_list);

        UDA_LOG(UDA_LOG_DEBUG, "Extracting Name Value Pairs from [{}]: {}", i, name_value_list->nameValue[i].value);

        unsigned short strip = 1; // Remove enclosing quotes from name value pairs
        char* work = strdup(name_value_list->nameValue[i].value);
        UDA_LOG(UDA_LOG_DEBUG, "[{}] work = {}", i, work);

        // Extract NV pairs and keywords from the input placeholder value

        int rc = name_value_pairs(work, &new_name_value_list, strip);
        free(work);
        if (rc == -1) {
            continue;
        }

        UDA_LOG(UDA_LOG_DEBUG, "Embedded Pair Count = {}", new_name_value_list.pairCount);
        for (int j = 0; j < new_name_value_list.pairCount; j++) {
            UDA_LOG(UDA_LOG_DEBUG, "Pair[{}] {}, Name = {}, value = {}", j, new_name_value_list.nameValue[j].pair,
                    new_name_value_list.nameValue[j].name, new_name_value_list.nameValue[j].value);
        }

        for (int j = 0; j < new_name_value_list.pairCount; j++) {

            if (new_name_value_list.nameValue[j].value[0] == '$') {
                // Match with substitution value
                for (int k = 0; k < name_value_list->pairCount; k++) {
                    if (k == i) {
                        continue;
                    }

                    // Match by name (case sensitive) only
                    if (!strcmp(&new_name_value_list.nameValue[j].value[1], name_value_list->nameValue[k].name)) {

                        UDA_LOG(UDA_LOG_DEBUG, "Substitution: embedded[{}] {}={} with [{}] {}={}", j,
                                new_name_value_list.nameValue[j].name, new_name_value_list.nameValue[j].value, k,
                                name_value_list->nameValue[k].name, name_value_list->nameValue[k].value);

                        // Substitute into original string and replace original

                        // Substitute here with name_value_list->nameValue[k].value
                        char* original = strdup(name_value_list->nameValue[i].value);
                        UDA_LOG(UDA_LOG_DEBUG, "Original: {}", original);

                        char* pp = strstr(original, new_name_value_list.nameValue[j].value);
                        int lstr = strlen(new_name_value_list.nameValue[j].value); // Target this
                        int ok = 1;

                        UDA_LOG(UDA_LOG_DEBUG, "targeting {} [{}] from {} to {}", pp, lstr,
                                new_name_value_list.nameValue[j].value, name_value_list->nameValue[k].value);

                        for (m = 0; m < lstr; m++) {
                            ok = ok && pp[m] == new_name_value_list.nameValue[j].value[m]; // Test the name is correct
                        }

                        if (ok) {
                            UDA_LOG(UDA_LOG_DEBUG, "Substituting {} with {}", new_name_value_list.nameValue[j].value,
                                    name_value_list->nameValue[k].value);

                            pp[0] = '\0';
                            std::string replace =
                                fmt::format("{}{}{}", original, name_value_list->nameValue[k].value, &pp[lstr]);

                            UDA_LOG(UDA_LOG_DEBUG, "original {}", original);
                            UDA_LOG(UDA_LOG_DEBUG, "value    {}", name_value_list->nameValue[k].value);
                            UDA_LOG(UDA_LOG_DEBUG, "residual {}", &pp[lstr]);
                            UDA_LOG(UDA_LOG_DEBUG, "Modified Original {}", replace.c_str());

                            free(original);
                            free(name_value_list->nameValue[i].value);
                            name_value_list->nameValue[i].value = strdup(replace.c_str());

                            break;
                        }
                        free(original);
                    }
                }
            }
        }
    }
    free_name_value_list(&new_name_value_list);
}
