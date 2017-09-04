// Copy Data Structures
//
//----------------------------------------------------------------------------------

#include "copyStructs.h"

void copyServerBlock(SERVER_BLOCK *out, SERVER_BLOCK in) {
    *out = in;
    strcpy(out->msg, in.msg);
}

void copyRequestBlock(REQUEST_BLOCK *out, REQUEST_BLOCK in) {
    *out = in;
    strcpy(out->tpass,   in.tpass);
    strcpy(out->path,    in.path);
    strcpy(out->file,    in.file);
    strcpy(out->format,  in.format);
    strcpy(out->archive, in.archive);
    strcpy(out->device_name, in.device_name);
    strcpy(out->server,  in.server);

    strcpy(out->signal,   in.signal);
    strcpy(out->source,   in.source);
    strcpy(out->api_delim,in.api_delim);
}

void copyDataBlock(DATA_BLOCK *out, DATA_BLOCK in) {
    *out = in;
    strcpy(out->data_units, in.data_units);
    strcpy(out->data_label, in.data_label);
    strcpy(out->data_desc,  in.data_desc);
    strcpy(out->error_msg,  in.error_msg);
    out->data_system    = in.data_system;
    out->system_config  = in.system_config;
    out->data_source    = in.data_source;
    out->signal_rec     = in.signal_rec;
    out->signal_desc    = in.signal_desc;
}

void copyDataSystem(DATA_SYSTEM *out, DATA_SYSTEM in) {
    *out = in;
    strcpy(out->device_name, in.device_name);
    strcpy(out->system_name, in.system_name);
    strcpy(out->system_desc, in.system_desc);
    strcpy(out->creation,    in.creation);
    strcpy(out->xml,         in.xml);
    strcpy(out->xml_creation,in.xml_creation);
}

void copySystemConfig(SYSTEM_CONFIG *out, SYSTEM_CONFIG in) {
    *out = in;
    strcpy(out->config_name, in.config_name);
    strcpy(out->config_desc, in.config_desc);
    strcpy(out->creation,    in.creation);
    strcpy(out->xml,         in.xml);
    strcpy(out->xml_creation,in.xml_creation);
}

void copyDataSource(DATA_SOURCE *out, DATA_SOURCE in) {
    *out = in;
    strcpy(out->source_alias,in.source_alias);
    strcpy(out->pass_date,   in.pass_date);
    strcpy(out->archive,     in.archive);
    strcpy(out->device_name, in.device_name);
    strcpy(out->format,      in.format);
    strcpy(out->path,        in.path);
    strcpy(out->filename,    in.filename);
    strcpy(out->server,      in.server);
    strcpy(out->userid,      in.userid);
    strcpy(out->reason_desc, in.reason_desc);
    strcpy(out->status_desc, in.status_desc);
    strcpy(out->run_desc,    in.run_desc);
    strcpy(out->modified,    in.modified);
    strcpy(out->creation,    in.creation);
    strcpy(out->xml,         in.xml);
    strcpy(out->xml_creation,in.xml_creation);
}

void copySignal(SIGNAL *out, SIGNAL in) {
    *out = in;
    strcpy(out->status_desc, in.status_desc);
    strcpy(out->modified,    in.modified);
    strcpy(out->creation,    in.creation);
    strcpy(out->xml,         in.xml);
    strcpy(out->xml_creation,in.xml_creation);
}

void copySignalDesc(SIGNAL_DESC *out, SIGNAL_DESC in) {
    *out = in;
    strcpy(out->source_alias, in.source_alias);
    strcpy(out->signal_alias, in.signal_alias);
    strcpy(out->signal_name,  in.signal_name);
    strcpy(out->generic_name, in.generic_name);
    strcpy(out->description,  in.description);
    strcpy(out->signal_class, in.signal_class);
    strcpy(out->signal_owner, in.signal_owner);
    strcpy(out->modified,     in.modified);
    strcpy(out->creation,     in.creation);
    strcpy(out->xml,          in.xml);
    strcpy(out->xml_creation, in.xml_creation);
}

void copyPluginInterface(IDAM_PLUGIN_INTERFACE* out, IDAM_PLUGIN_INTERFACE* in)
{
    out->interfaceVersion       = in->interfaceVersion;
    out->pluginVersion          = in->pluginVersion;
    out->sqlConnectionType      = in->sqlConnectionType;
    out->verbose                = in->verbose;
    out->housekeeping           = in->housekeeping;
    out->changePlugin           = in->changePlugin;
    out->dbgout                 = in->dbgout;
    out->errout                 = in->errout;
    out->data_block             = in->data_block;
    out->request_block          = in->request_block;
    out->client_block           = in->client_block;
    out->data_source            = in->data_source;
    out->signal_desc            = in->signal_desc;
    out->environment            = in->environment;
    out->sqlConnection          = in->sqlConnection;
    out->pluginList             = in->pluginList;
    out->userdefinedtypelist    = in->userdefinedtypelist;
}