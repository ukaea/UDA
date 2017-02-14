#include "readCDFMeta.h"

#include <netcdf.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

#include <clientserver/idamErrorLog.h>
#include <clientserver/idamErrors.h>

void allocMetaXML(METAXML* str)
{
    str->lheap = str->lheap + 10 * STRING_LENGTH + 1;
    str->xml = (char*) realloc((void*) str->xml, str->lheap * sizeof(char));
}

void addMetaXML(METAXML* str, char* tag)
{
    int ltag;
    char* cp = NULL;
    if (tag[0] == '\0') return;
    ltag = (int) strlen(tag);
    if (str->nxml + ltag + 1 > str->lheap) allocMetaXML(str);    // Allocate additional heap for the Meta Data
    cp = &str->xml[str->nxml];
    strcpy(cp, tag);
    str->nxml = str->nxml + ltag;
}

int addTextMetaXML(int fd, int grpid, METAXML* str, char* tag)
{
    size_t attlen;    // dgm 64 bit bug 16Dec2011
    int err = 0;
    char* cp, * ftag;
    if (tag[0] == '\0') return NC_NOERR;
    if ((err = nc_inq_attlen(fd, grpid, tag, &attlen)) == NC_NOERR) {
        cp = (char*) malloc((attlen + 1) * sizeof(char));
        if ((err = nc_get_att_text(fd, grpid, tag, cp)) != NC_NOERR) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*) nc_strerror(err));
            free((void*) cp);
            return err;
        }
        cp[attlen] = '\0';        // Ensure Null terminated

        ftag = (char*) malloc((strlen(tag) + 6) * sizeof(char));
        sprintf(ftag, "<%s>\"", tag);

        addMetaXML(str, ftag);
        addMetaXML(str, cp);

        sprintf(ftag, "\"</%s>\n", tag);
        addMetaXML(str, ftag);

        free((void*) cp);
        free((void*) ftag);
    }
    return NC_NOERR;
}

int addIntMetaXML(int fd, int grpid, METAXML* str, char* tag)
{
    int err = 0, data;
    char sdata[56];
    char* ftag;
    if (tag[0] == '\0') return NC_NOERR;
    if ((err = nc_get_att_int(fd, grpid, tag, &data)) == NC_NOERR) {
        ftag = (char*) malloc((strlen(tag) + 6) * sizeof(char));
        sprintf(ftag, "<%s>", tag);
        sprintf(sdata, "%d", data);
        addMetaXML(str, ftag);
        addMetaXML(str, sdata);
        sprintf(ftag, "</%s>\n", tag);
        addMetaXML(str, ftag);
        free((void*) ftag);
    }
    return NC_NOERR;    // Never report an error - the attribute may not exist
}

