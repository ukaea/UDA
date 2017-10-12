#include <IdamAPI.h>

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include <client/accAPI.h>

int strdiff(const char* s1, const char* s2)
{
    int i = 0;
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
        i++;
    }
    return (*s1 == *s2) ? -1 : i;
}

int main(int argc, char* argv[])
{
    if (argc != 6) {
        fprintf(stderr, "wrong number of arguments to test function\n");
        return 1;
    }

    putIdamServerHost("idam0");
    putIdamServerPort(56565);

    int test_no = atoi(argv[1]);
    char* compare_dir = argv[2];
    char* signal = argv[3];
    char* source = argv[4];
    int should_fail = STR_IEQUALS(argv[5], "true");

    fprintf(stdout, "running: idamGetAPI(%s, %s)\n", signal, source);

    int handle = idamGetAPI(signal, source);
    if (handle < 0 && !should_fail) {
        fprintf(stderr, "error %d: %s\n", getIdamErrorCode(handle), getIdamErrorMsg(handle));
        return 1;
    }

    const char* fmt;
    size_t size;

    if (should_fail) {
        fmt = "Test #%d: expected error %d: %s";
        size = (size_t) snprintf(NULL, 0, fmt, test_no, getIdamErrorCode(handle), getIdamErrorMsg(handle));
    } else {
        fmt = "Test #%d: %d [%d] -- checksums: [%d] [%d]";
        size = (size_t) snprintf(NULL, 0, fmt, test_no, handle, getIdamDataNum(handle),
                                 getIdamDataCheckSum(handle), getIdamDimDataCheckSum(handle, 0));
    }

    char str[size + 1];

    if (should_fail) {
        snprintf(str, size + 1, fmt, test_no, getIdamErrorCode(handle), getIdamErrorMsg(handle));
        str[size + 1] = '\0';
    } else {
        snprintf(str, size + 1, fmt, test_no, handle, getIdamDataNum(handle),
                 getIdamDataCheckSum(handle), getIdamDimDataCheckSum(handle, 0));
        str[size + 1] = '\0';
    }

    fprintf(stdout, "%s\n", str);

    fmt = "%s/test_out%d";
    size = (size_t) snprintf(NULL, 0, fmt, compare_dir, test_no);
    char filename[size + 1];

    snprintf(filename, size + 1, fmt, compare_dir, test_no);
    filename[size + 1] = '\0';

    FILE* fid = fopen(filename, "r");
    if (!fid) {
        fprintf(stderr, "failed to open comparison file: %s\n", filename);
        return 1;
    }

    fseek(fid, 0, SEEK_END);
    size_t fsize = (size_t) ftell(fid);
    fseek(fid, 0, SEEK_SET);

    char contents[fsize + 1];
    fread(contents, fsize, 1, fid);
    fclose(fid);
    contents[fsize] = '\0';

    int pos;
    if ((pos = strdiff(str, contents)) > 0) {
        fprintf(stderr, "result does not match expected (differs at %d): %s\n", pos, contents);
        return 1;
    }

    return 0;
}