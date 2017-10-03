#include "west_utilities.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <logging/logging.h>
#include <clientserver/initStructs.h>
#include <clientserver/errorLog.h>
#include <clientserver/stringUtils.h>
#include <clientserver/udaTypes.h>

void getFunName(const char* s, char** fun_name)
{
    const char delim[] = ";";
    char* s_copy = strdup(s);
    *fun_name = strdup(strtok(s_copy, delim));
    RemoveSpaces(*fun_name);
    free(s_copy);
}

void tokenizeFunParameters(const char* s, char** TOP_collections_parameters, char** attributes,
                           char** normalizationAttributes)
{
    const char delim[] = ";";
    char* s_copy = strdup(s);
    strdup(strtok(s_copy, delim)); //function name
    *TOP_collections_parameters = strdup(strtok(NULL, delim));
    RemoveSpaces(*TOP_collections_parameters);
    *attributes = strdup(strtok(NULL, delim));
    RemoveSpaces(*attributes);
    char* token = strtok(NULL, delim);
    if (token != NULL) {
        *normalizationAttributes = strdup(token);
        RemoveSpaces(*normalizationAttributes);
    }
    free(s_copy);
}

void tokenizeFunParametersWithChannels(const char* s, char** unvalid_channels, char** TOP_collections_parameters,
                                       char** attributes, char** normalizationAttributes)
{
    const char delim[] = ";";
    char* s_copy = strdup(s);
    char* s1 = strdup(strtok(s_copy, delim)); //function name
    UDA_LOG(UDA_LOG_DEBUG, "fun name : %s\n", s1);
    *unvalid_channels = strdup(strtok(NULL, delim));
    UDA_LOG(UDA_LOG_DEBUG, "unvalid channels : %s\n", *unvalid_channels);
    *TOP_collections_parameters = strdup(strtok(NULL, delim));
    RemoveSpaces(*TOP_collections_parameters);
    *attributes = strdup(strtok(NULL, delim));
    RemoveSpaces(*attributes);
    char* token = strtok(NULL, delim);
    if (token != NULL) {
        *normalizationAttributes = strdup(token);
        RemoveSpaces(*normalizationAttributes);
    }
    free(s_copy);
    free(s1);
}

void tokenize_set_channels_validity(const char* s, char** unvalid_channels, char** attributes)
{
    const char delim[] = ";";
    char* s_copy = strdup(s);
    strdup(strtok(s_copy, delim)); //function name
    *unvalid_channels = strdup(strtok(NULL, delim)); //example: unvalid_channels:2:1,2
    *attributes = strdup(strtok(NULL, delim));
    RemoveSpaces(*attributes);
    free(s_copy);
}

void getUnvalidChannelsSize(char* unvalid_channels, int* size)
{
    const char delim[] = ":";
    char* s_copy = strdup(unvalid_channels);
    char* token = strdup(strtok(s_copy, delim)); //should return "unvalid_channels" string
    token = strdup(strtok(NULL, delim)); //return the number of unvalid channels
    *size = atoi(token);
    free(s_copy);
    free(token);
}

void getUnvalidChannels(char* unvalid_channels, int* v)
{
    const char delim[] = ":";
    char* s_copy = strdup(unvalid_channels);
    char* token = strdup(strtok(s_copy, delim)); //should return "unvalid_channels" string

    token = strdup(strtok(NULL, delim)); //return the number of unvalid channels
    int n = atoi(token);

    UDA_LOG(UDA_LOG_DEBUG, "unvalid channels count : %d\n", n);

    token = strdup(strtok(NULL, delim)); //return for example "1,2" if channels 1 and 2 are not valid

    UDA_LOG(UDA_LOG_DEBUG, "unvalid channels list : %s\n", token);

    const char delim2[] = ",";
    char* s_copy2 = strdup(token);

    int i;
    for (i = 0; i < n; i++) {
        if (i == 0) {
            v[i] = atoi(strdup(strtok(s_copy2, delim2)));
        } else {
            token = strdup(strtok(NULL, delim2));
            v[i] = atoi(token);
        }
        UDA_LOG(UDA_LOG_DEBUG, "unvalid channel : %d\n", v[i]);
    }
    free(s_copy);
    free(s_copy2);
    free(token);
}

int isChannelValid(int channel_number, int* unvalid_channels_list, int unvalid_channels_list_size)
{
    int i;
    UDA_LOG(UDA_LOG_DEBUG, "unvalid_channels_list_size : %d\n", unvalid_channels_list_size);
    for (i = 0; i < unvalid_channels_list_size; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "unvalid_channels_list[i] : %d\n", unvalid_channels_list[i]);
        if (unvalid_channels_list[i] == channel_number) {
            return 0;
        }
    }
    return 1;
}

int getCommand(int i, char** command, const char* TOP_collections_parameters)
{
    //UDA_LOG(UDA_LOG_DEBUG, "In getCommand, i: %d\n", i);
    char* s_copy = strdup(TOP_collections_parameters);
    const char delim[] = ",";
    int j = 0;
    char* token = strdup(strtok(s_copy, delim));
    //UDA_LOG(UDA_LOG_DEBUG, "In getCommand, token: %s\n", token);
    if (token == NULL) {
        return -1;
    }
    while (token != NULL) {
        if (i == j) {
            *command = strdup(token);
            RemoveSpaces(*command);
            free(s_copy);
            return 0;
        }
        token = strtok(NULL, delim);
        j++;
    }
    free(s_copy);
    return -1;
}

void getTopCollectionsCount(const char* TOP_collections_parameters, int* collectionsCount)
{
    *collectionsCount = 0;
    char* s_copy = strdup(TOP_collections_parameters);
    const char delim[] = ",";
    char* token = strtok(s_copy, delim);
    while (token != NULL) {
        (*collectionsCount)++;
        token = strtok(NULL, delim);
    }
    free(s_copy);
    free(token);
}

void getNormalizationFactor(float* normalizationFactor, char* normalizationAttributes)
{
    char* s_copy = NULL;
    char* operation = NULL;
    char* funname = NULL;
    char* csteStr = NULL;

    if (normalizationAttributes != NULL) {
        UDA_LOG(UDA_LOG_DEBUG, "normalization attributes found\n");
        const char delim[] = ":";
        s_copy = strdup(normalizationAttributes);
        operation = strdup(strtok(s_copy, delim));
        if (STR_EQUALS("multiply", operation)) {
            UDA_LOG(UDA_LOG_DEBUG, "Multiply operation\n");
            funname = strdup(strtok(NULL, delim));
            if (STR_EQUALS("cste", funname)) {
                UDA_LOG(UDA_LOG_DEBUG, "multiply data by constant value\n");
                csteStr = strdup(strtok(NULL, delim));
                UDA_LOG(UDA_LOG_DEBUG, "%s\n", csteStr);
                *normalizationFactor = atof(csteStr);
            } else {
                int err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "Unsupported operand for 'multiply' operation\n");
                addIdamError(CODEERRORTYPE, "Unsupported operand for 'multiply' operation", err,
                             "");
            }

        } else {
            int err = 999;
            UDA_LOG(UDA_LOG_DEBUG, "Unsupported operation to apply\n");
            addIdamError(CODEERRORTYPE, "Unsupported operation to apply", err, "");
        }
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "no normalization attributes found\n");
    }
    free(s_copy);
    free(operation);
    free(funname);
    free(csteStr);
}

void multiplyFloat(float* p, float factor, int val_nb)
{
    UDA_LOG(UDA_LOG_DEBUG, "Entering multiplyFloat...\n");
    UDA_LOG(UDA_LOG_DEBUG, "val_nb : %d\n", val_nb);
    UDA_LOG(UDA_LOG_DEBUG, "factor : %f\n", factor);
    if (factor != 1) {
        int i;
        for (i = 0; i < val_nb; i++) {
            p[i] *= factor;
        }
    }
}

void multiplyInt(int* p, float factor, int val_nb)
{
    if (factor != 1) {
        int i;
        for (i = 0; i < val_nb; i++)
            p[i] *= (int)factor;
    }
}

int getNumIDAMIndex(char* attributes, int* nodeIndices)
{

    char* s_copy = strdup(attributes);
    const char delim[] = ":";
    char* charIDAMIndex = NULL;
    strtok(s_copy, delim); //rank
    strtok(NULL, delim); //type
    charIDAMIndex = strdup(strtok(NULL, delim)); //#0
    char firstChar;
    firstChar = charIDAMIndex[0];

    if (firstChar == '#') {
        UDA_LOG(UDA_LOG_DEBUG, "index specified in IDAM request\n");
        return nodeIndices[atoi(&charIDAMIndex[1])] - 1;
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "idam index hard coded in mapping file\n");
        return atoi(&firstChar) - 1;
    }
    free(s_copy);
    free(charIDAMIndex);
}

int getNumIDAMIndex2(char* s, int* nodeIndices)
{

    char firstChar;
    firstChar = s[0];

    if (firstChar == '#') {
        UDA_LOG(UDA_LOG_DEBUG, "index specified in IDAM request\n");
        return nodeIndices[atoi(&s[1])] - 1;
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "no index specified\n");
        return -1;
    }
}

void getReturnType(char* attributes, int* dataType)
{
    UDA_LOG(UDA_LOG_DEBUG, "attributes3: %s\n", attributes);
    char* s_copy = strdup(attributes);
    const char delim[] = ":";
    strtok(s_copy, delim); //the rank
    char* token = strtok(NULL, delim);
    UDA_LOG(UDA_LOG_DEBUG, " token: %s\n", token);

    int i = UDA_TYPE_UNKNOWN;

    int err = 0;

    if (STR_EQUALS(token, "vecstring_type") || STR_EQUALS(token, "string") || STR_EQUALS(token, "STR_0D")) {
        i = UDA_TYPE_STRING;
    } else if (STR_EQUALS(token, "vecflt_type") || STR_EQUALS(token, "float") || STR_EQUALS(token, "FLT_0D")) {
        i = UDA_TYPE_FLOAT;
    } else if (STR_EQUALS(token, "vecint_type") || STR_EQUALS(token, "integer") || STR_EQUALS(token, "int") ||
               STR_EQUALS(token, "INT_0D")) {
        i = UDA_TYPE_INT;
    } else {
        err = 999;
        addIdamError(CODEERRORTYPE, "west convertToInt() : Unsupported data type", err, "");
    }
    *dataType = i;
    free(s_copy);
}

void getRank(char* attributes, int* rank)
{
    char* s_copy = strdup(attributes);
    const char delim[] = ":";
    char* rankStr = strdup(strtok(s_copy, delim));
    *rank = atoi(&rankStr[0]);
    free(s_copy);
    free(rankStr);
}

void getValueCollect(char* command, char** value, int* nodeIndices)
{
    char* s_copy = strdup(command);
    const char delim[] = ":";
    strtok(s_copy, delim); //prod_name
    strtok(NULL, delim); //object name
    strtok(NULL, delim); //param name
    *value = strdup(strtok(NULL, delim)); //value for the setvalue_collect function
    char* token = strtok(NULL, delim);
    if (token != NULL) {
        int UDAIndex = getNumIDAMIndex2(token, nodeIndices);
        char str[2];
        sprintf(str, "%s", "_");
        sprintf(str, "%d", UDAIndex);
        *value = strcat(*value, str);
    }
    free(s_copy);
}

void tokenizeCommand(const char* s, char** prod_name, char** obj_name, char** param_name, char** flag)
{
    char* s_copy = strdup(s);
    const char delim[] = ":";
    UDA_LOG(UDA_LOG_DEBUG, "Tokenizing: %s\n", s);
    *prod_name = strdup(strtok(s_copy, delim));
    RemoveSpaces(*prod_name);
    UDA_LOG(UDA_LOG_DEBUG, "%s\n", *prod_name);
    *obj_name = strdup(strtok(NULL, delim));
    RemoveSpaces(*obj_name);
    UDA_LOG(UDA_LOG_DEBUG, "%s\n", *obj_name);
    *param_name = strdup(strtok(NULL, delim));
    RemoveSpaces(*param_name);
    char* token = strtok(NULL, delim);
    if (token != NULL) {
        *flag = strdup(token);
        RemoveSpaces(*flag);
    }
    free(s_copy);
}

void getObjectName(char** obj_name, char* command)
{
    char* s_copy = strdup(command);
    const char delim[] = ":";
    strdup(strtok(s_copy, delim));
    *obj_name = strdup(strtok(NULL, delim));
    RemoveSpaces(*obj_name);
    free(s_copy);
}

void searchIndices(int requestedIndex, int* l, int* searchedArray, int* searchedArrayIndex)
{
    *searchedArray = 0;
    *searchedArrayIndex = 0;

    if (requestedIndex < l[0]) {

        *searchedArrayIndex = requestedIndex; //we take the (requestedIndex) signal in the signals group
        *searchedArray = 0; //we take the first group of signals which appears in the tsmat_collect string

    } else if (requestedIndex == l[0]) {

        *searchedArrayIndex = 0;
        *searchedArray = 1;

    } else if (requestedIndex > l[0]) {

        int i = 0;

        int d = requestedIndex - l[0];

        while (d >= 0) {
            i++;
            d = d - l[i];
        }

        int k = i - 1;
        int j;
        int s = 0;
        for (j = 0; j <= k; j++) {
            s += l[j];
        }
        *searchedArrayIndex = requestedIndex - s;
        *searchedArray = k + 1;
    }
}

void printNum(const char* label, int i)
{
    UDA_LOG(UDA_LOG_DEBUG, "%s -> %d\n", label, i);
}

void addExtractionChars(char* result, char* signalName, int extractionIndex)
{
    strcpy(result, signalName);
    strcat(result, "!");
    char s[5];
    sprintf(s, "%d", extractionIndex);
    strcat(result, s);
}

void RemoveSpaces(char* source)
{
    char* i = source;
    char* j = source;
    while (*j != 0) {
        *i = *j++;
        if (*i != ' ') {
            i++;
        }
    }
    *i = 0;
}

