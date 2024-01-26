#include "copyStructs.h"

#include <cstring>
#include <cstdlib>

void copyRequestData(REQUEST_DATA* out, REQUEST_DATA in)
{
    *out = in;
    strcpy(out->tpass, in.tpass);
    strcpy(out->path, in.path);
    strcpy(out->file, in.file);
    strcpy(out->format, in.format);
    strcpy(out->archive, in.archive);
    strcpy(out->device_name, in.device_name);
    strcpy(out->server, in.server);
    strcpy(out->signal, in.signal);
    strcpy(out->source, in.source);
    strcpy(out->api_delim, in.api_delim);
}

void copyRequestBlock(REQUEST_BLOCK* out, REQUEST_BLOCK in)
{
    *out = in;
    out->requests = (REQUEST_DATA*)malloc(out->num_requests * sizeof(REQUEST_DATA));
    for (int i = 0; i < out->num_requests; ++i) {
        copyRequestData(&out->requests[i], in.requests[i]);
    }
}

