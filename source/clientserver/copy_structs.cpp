#include "copy_structs.h"

#include <cstdlib>
#include <cstring>

void uda::client_server::copy_request_data(RequestData* out, RequestData in)
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
