#include "copyStructs.h"

#include <cstring>

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

void copyDataSource(DATA_SOURCE* out, DATA_SOURCE in)
{
    *out = in;
    strcpy(out->source_alias, in.source_alias);
    strcpy(out->pass_date, in.pass_date);
    strcpy(out->archive, in.archive);
    strcpy(out->device_name, in.device_name);
    strcpy(out->format, in.format);
    strcpy(out->path, in.path);
    strcpy(out->filename, in.filename);
    strcpy(out->server, in.server);
    strcpy(out->userid, in.userid);
    strcpy(out->reason_desc, in.reason_desc);
    strcpy(out->status_desc, in.status_desc);
    strcpy(out->run_desc, in.run_desc);
    strcpy(out->modified, in.modified);
    strcpy(out->creation, in.creation);
    strcpy(out->xml, in.xml);
    strcpy(out->xml_creation, in.xml_creation);
}

void copyPluginInterface(IDAM_PLUGIN_INTERFACE* out, IDAM_PLUGIN_INTERFACE* in)
{
    out->interfaceVersion = in->interfaceVersion;
    out->pluginVersion = in->pluginVersion;
    out->sqlConnectionType = in->sqlConnectionType;
    out->verbose = in->verbose;
    out->housekeeping = in->housekeeping;
    out->changePlugin = in->changePlugin;
    out->dbgout = in->dbgout;
    out->errout = in->errout;
    out->data_block = in->data_block;
    out->request_data = in->request_data;
    out->client_block = in->client_block;
    out->data_source = in->data_source;
    out->signal_desc = in->signal_desc;
    out->environment = in->environment;
    out->sqlConnection = in->sqlConnection;
    out->pluginList = in->pluginList;
    out->userdefinedtypelist = in->userdefinedtypelist;
}
