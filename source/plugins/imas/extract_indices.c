#include "extract_indices.h"

#include <regex.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int extract_array_indices(const char* input, char** output, int** indices)
{
    regex_t preg;
    int rc;
    const char* pattern = "(/[0-9]+/)";

    rc = regcomp(&preg, pattern, REG_EXTENDED);
    if (rc != 0) {
        fprintf(stderr, "regcomp() failed\n");
        return rc;
    }

    char* work = strdup(input);
    *indices = NULL;
    size_t num_indices = 0;

    size_t nmatch = 1;
    regmatch_t pmatch[1];
    while (regexec(&preg, work, nmatch, pmatch, 0) == 0) {
        size_t len = pmatch[0].rm_so + (strlen(input) - pmatch[0].rm_eo) + 4;
        
        char *temp = calloc(len, sizeof(char));
        strncpy(temp, work, pmatch[0].rm_so);
        strcpy(temp + pmatch[0].rm_so, "/#/");
        strcpy(temp + pmatch[0].rm_so + 3, &work[pmatch[0].rm_eo]);

        char *index = calloc(pmatch[0].rm_eo - pmatch[0].rm_so - 1, sizeof(char));
        strncpy(index, &work[pmatch[0].rm_so + 1], pmatch[0].rm_eo - pmatch[0].rm_so - 2);
        *indices = realloc(*indices, (num_indices + 1) * sizeof(int));
        (*indices)[num_indices] = atoi(index);
        ++num_indices;

        free(work);
        work = temp;
    }

    *output = work;

    return num_indices;
}

char* indices_to_string(const int* indices, int num_indices)
{
    if (num_indices == 0) return strdup("-1");

    char* string = strdup("");

    int i;
    for (i = 0; i < num_indices; ++i) {
       size_t len = snprintf(NULL, 0, "%s%d;", string, indices[i]);
       char* temp = malloc(len+1);
       snprintf(temp, len+1, "%s%d;", string, indices[i]);
       free(string);
       string = temp;
    }

    string[strlen(string)-1] = '\0'; // remove last ';'
    return string;
}

