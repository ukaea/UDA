#include "makeServerRequestBlock.h"

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <strings.h>

#include <clientserver/udaErrors.h>
#include <clientserver/stringUtils.h>
#include <clientserver/initStructs.h>
#include <clientserver/makeRequestBlock.h>

// Deconstruct the text pass parameter for name valaue placeholder substitution 
// and for additional name-value pairs and keywords

// patterns	a, b, c					Name value substitution or keywords
//		name=a, name=b, name=c 			Additional Name Value Pairs
//		a,b,c, name=value, name=value, d, e	Name value pairs and keywords MUST be included after all substitution values

int nameValueSubstitution(NAMEVALUELIST* nameValueList, char* tpass)
{
    int i, err = 0;

    // Identify placeholder name-value pairs and additional name-values

    if ((!tpass && strlen(tpass) == 0) || nameValueList->pairCount == 0) {
        return 0;
    }    // nothing to substitute or no name value pairs

    NAMEVALUELIST newNameValueList;
    initNameValueList(&newNameValueList);

    unsigned short strip = 1;        // Remove enclosing quotes from name value pairs
    if (nameValuePairs(tpass, &newNameValueList, strip) == -1) {
        err = 999;
        addIdamError(CODEERRORTYPE, "nameValueSubstitution", err, "Name Value pair syntax is incorrect!");
        return err;
    }

    // How many placeholders?

    int placeholderCount = 0;
    int* placeholderIndex = NULL;
    int* tpassIndex = NULL;

    if (nameValueList->pairCount > 0) {    // Count how many Placeholders
        do {
            placeholderIndex = (int*)malloc(nameValueList->pairCount * sizeof(int));
            tpassIndex = (int*)malloc(nameValueList->pairCount * sizeof(int));

            for (i = 0; i < nameValueList->pairCount; i++) {
                if (nameValueList->nameValue[i].value != NULL && nameValueList->nameValue[i].value[0] ==
                                                                 '$') {        // Is it a placeholder? (Not a keyword and begining '$')
                    placeholderIndex[placeholderCount] = i;                                    // Identify which pair
                    tpassIndex[placeholderCount] = placeholderCount;                                // Ordering: Default substitution value to use
                    if (nameValueList->nameValue[i].value[1] != '\0' &&
                        IsNumber(&nameValueList->nameValue[i].value[1]))    // Is the placeholder numbered?
                        // Use a specific substitution value
                        tpassIndex[placeholderCount] = atoi(&nameValueList->nameValue[i].value[1]) - 1;
                    placeholderCount++;
                    UDA_LOG(UDA_LOG_DEBUG, "[%d] value: %s, placeholderCount %d\n", i,
                            nameValueList->nameValue[i].value, placeholderCount);
                }
            }

            if (placeholderCount == 0)
                break;        // There are no placeholders to process - add new name-value pairs and keywords

            UDA_LOG(UDA_LOG_DEBUG, "Name Value Pair Count = %d\n", nameValueList->pairCount);
            UDA_LOG(UDA_LOG_DEBUG, "Placeholder Count     = %d\n", placeholderCount);
            UDA_LOG(UDA_LOG_DEBUG, "Additional Name Value Pairs and Substitutions = %s\n", tpass);
            UDA_LOG(UDA_LOG_DEBUG, "Additional Name Value Pairs and Substitutions Count = %d\n",
                    newNameValueList.pairCount);

            // Placeholder substitutions (must be the specified first in tpass)

            if (placeholderCount > newNameValueList.pairCount) {
                // Too many placeholders for the available substitutions
                err = 999;
                addIdamError(CODEERRORTYPE, "nameValueSubstitution", err,
                             "Inconsistent count of placeholders and available substitutions!\n");
                break;
            }

            for (i = 0; i < placeholderCount; i++) {
                if (tpassIndex[i] < 0 || tpassIndex[i] > placeholderCount) {
                    err = 999;
                    addIdamError(CODEERRORTYPE, "nameValueSubstitution", err,
                                 "Placeholder numbering is Inconsistent with Placeholder Count!\n");
                    break;
                }

                nameValueList->nameValue[placeholderIndex[i]].value = newNameValueList.nameValue[tpassIndex[i]].value;

                UDA_LOG(UDA_LOG_DEBUG, "Placeholder: [%d][%d] %s, Substitution Value [%d] %s\n", i, placeholderIndex[i],
                        nameValueList->nameValue[placeholderIndex[i]].name,
                        tpassIndex[i], nameValueList->nameValue[placeholderIndex[i]].value);
            }
        } while (0);
    }

    free(tpassIndex);
    free(placeholderIndex);

    if (err != 0) return err;

    // How many new name value pairs and keywords?

    int count = newNameValueList.pairCount - placeholderCount;
    if (count == 0) return 0;

    // Add new name value pairs and keywords

    if (nameValueList->pairCount + count > nameValueList->listSize) {
        nameValueList->nameValue = (NAMEVALUE*)realloc((void*)nameValueList->nameValue,
                                                       (nameValueList->listSize + count) * sizeof(NAMEVALUE));
        nameValueList->listSize = nameValueList->listSize + count;
    }

    UDA_LOG(UDA_LOG_DEBUG, "Additional Name Value PairCount = %d\n", count);

    for (i = 0; i < count; i++) {
        nameValueList->nameValue[nameValueList->pairCount].name = newNameValueList.nameValue[placeholderCount + i].name;
        nameValueList->nameValue[nameValueList->pairCount++].value = newNameValueList.nameValue[placeholderCount +
                                                                                                i].value;
        UDA_LOG(UDA_LOG_DEBUG, "[%d] Name = %s, Value = %s\n", i, newNameValueList.nameValue[placeholderCount + i].name,
                newNameValueList.nameValue[placeholderCount + i].value);
    }

    // Scan all values for embedded placeholders and substitute

    embeddedValueSubstitution(nameValueList);

    return 0;
}


// Substitute named placeholders within name value strings

// patterns with name values: string="UDA::getdata(variable=/a/b/c, shot=$shot, tstart=$tstart, tend=$tend)"	substitution for named string elements: $shot, $tstart, $tend

void embeddedValueSubstitution(NAMEVALUELIST* nameValueList)
{
    int i, j, k, m;
    NAMEVALUELIST newNameValueList;

    if (nameValueList->pairCount == 0) return;

    for (i = 0; i < nameValueList->pairCount; i++) {

        char* p = strchr(nameValueList->nameValue[i].value, '$');
        if (!p) continue;

        initNameValueList(&newNameValueList);

        UDA_LOG(UDA_LOG_DEBUG, "Extracting Name Value Pairs from [%d]: %s\n", i, nameValueList->nameValue[i].value);

        unsigned short strip = 1;        // Remove enclosing quotes from name value pairs
        char* work = strdup(nameValueList->nameValue[i].value);
        if (nameValuePairs(work, &newNameValueList, strip) == -1) continue;

        for (j = 0; j < newNameValueList.pairCount; j++)
            UDA_LOG(UDA_LOG_DEBUG, "Pair[%d], Name = %s, value = %s\n", j, newNameValueList.nameValue[j].name,
                    newNameValueList.nameValue[j].value);

        for (j = 0; j < newNameValueList.pairCount; j++) {
            if (newNameValueList.nameValue[j].value[0] == '$') {
                for (k = 0; k < nameValueList->pairCount; k++) {
                    if (k == i) continue;
                    if (!strcasecmp(newNameValueList.nameValue[j].name, nameValueList->nameValue[k].name)) {
                        UDA_LOG(UDA_LOG_DEBUG, "Substitution: embedded[%d] %s=%s with [%d] %s=%s\n",
                                j, newNameValueList.nameValue[j].name, newNameValueList.nameValue[j].value,
                                k, nameValueList->nameValue[k].name, nameValueList->nameValue[k].value);

                        // Substitute into original string and replace original

                        char* original = strdup(
                                nameValueList->nameValue[i].value);    // Substitute here with nameValueList->nameValue[k].value
                        UDA_LOG(UDA_LOG_DEBUG, "Original: %s\n", original);

                        char* p = strchr(original, '$');                // Should be in order
                        int lstr = strlen(newNameValueList.nameValue[j].value);    // Target this
                        int ok = 1;

                        UDA_LOG(UDA_LOG_DEBUG, "targeting %s [%d] from %s to %s\n", p, lstr,
                                newNameValueList.nameValue[j].value, nameValueList->nameValue[k].value);

                        for (m = 0; m < lstr; m++)
                            ok = ok && p[m] == newNameValueList.nameValue[j].value[m];    // Test the name is correct

                        if (ok) {
                            UDA_LOG(UDA_LOG_DEBUG, "Substituting %s with %s\n", newNameValueList.nameValue[j].value,
                                    nameValueList->nameValue[k].value);

                            char* replace = (char*)malloc(
                                    (strlen(original) + strlen(nameValueList->nameValue[k].value) + 2) * sizeof(char));
                            replace[0] = '\0';
                            p[0] = '\0';
                            sprintf(replace, "%s%s%s", original, nameValueList->nameValue[k].value, &p[lstr]);

                            UDA_LOG(UDA_LOG_DEBUG, "Modified Original %s\n", replace);

                            free(original);
                            free(nameValueList->nameValue[i].value);
                            nameValueList->nameValue[i].value = replace;
                        }
                    }
                }
            }
        }
    }
}   

