
// Initialise Data Structures
//
//----------------------------------------------------------------------------------

#include "initStructs.h"

#ifdef __GNUC__
#  include <unistd.h>
#elif defined(_WIN32)
#  include <process.h>
#endif

#include <cstring>
#include <uda/types.h>

void uda::client_server::init_name_value_list(NameValueList* nameValueList)
{
    nameValueList->pairCount = 0;
    nameValueList->listSize = 0;
    nameValueList->nameValue = nullptr;
}

void uda::client_server::init_request_data(RequestData* str)
{
    str->request = 0;
    str->exp_number = 0;
    str->pass = -1;
    str->tpass[0] = '\0';
    str->path[0] = '\0';
    str->file[0] = '\0';
    str->format[0] = '\0';
    str->archive[0] = '\0';
    str->device_name[0] = '\0';
    str->server[0] = '\0';
    str->function[0] = '\0';

    str->signal[0] = '\0';
    str->source[0] = '\0';
    str->subset[0] = '\0';
    str->datasubset.nbound = 0;
    init_name_value_list(&str->nameValueList);

    str->put = 0;
    init_put_data_block_list(&str->putDataBlockList);
}

void uda::client_server::init_request_block(RequestBlock* str)
{
    str->num_requests = 0;
    str->requests = nullptr;
}

#ifdef _WIN32
#  define getpid _getpid
#endif

void uda::client_server::init_client_block(ClientBlock* str, int version, const char* clientname)
{
    str->version = version;
    str->timeout = TIMEOUT;
    if (getenv("UDA_TIMEOUT")) {
        str->timeout = (int)strtol(getenv("UDA_TIMEOUT"), nullptr, 10);
    }
    str->pid = (int)getpid();
    strcpy(str->uid, clientname); // Global userid
    str->compressDim = COMPRESS_DIM;

    str->clientFlags = 0;
    str->altRank = 0;
    str->get_nodimdata = 0;
    str->get_datadble = 0;
    str->get_dimdble = 0;
    str->get_timedble = 0;
    str->get_bad = 0;
    str->get_meta = 0;
    str->get_asis = 0;
    str->get_uncal = 0;
    str->get_notoff = 0;
    str->get_scalar = 0;
    str->get_bytes = 0;
    str->privateFlags = 0;

    str->OSName[0] = '\0'; // Operating System Name
    str->DOI[0] = '\0';    // Digital Object Identifier (client study reference)

#ifdef SECURITYENABLED
    initSecurityBlock(&(str->securityBlock));
#endif
}

void uda::client_server::init_server_block(ServerBlock* str, int version)
{
    str->version = version;
    str->error = 0;
    str->msg[0] = '\0';
    str->pid = (int)getpid();
    str->idamerrorstack.nerrors = 0;
    str->idamerrorstack.idamerror = nullptr;
    str->OSName[0] = '\0'; // Operating System Name
    str->DOI[0] = '\0';    // Digital Object Identifier (server configuration)

#ifdef SECURITYENABLED
    initSecurityBlock(&(str->securityBlock));
#endif
}

void uda::client_server::init_data_block_list(DataBlockList* str)
{
    str->count = 0;
    str->data = nullptr;
}

void uda::client_server::init_data_block(DataBlock* str)
{
    str->handle = 0;
    str->errcode = 0;
    str->source_status = 1;
    str->signal_status = 1;
    str->rank = 0;
    str->order = -1;
    str->data_n = 0;
    str->data_type = UDA_TYPE_UNKNOWN;
    str->error_type = UDA_TYPE_UNKNOWN;
    str->error_model = 0;
    str->errasymmetry = 0;
    str->error_param_n = 0;
    str->opaque_type = UDA_OPAQUE_TYPE_UNKNOWN;
    str->opaque_count = 0;
    str->opaque_block = nullptr;
    str->data = nullptr;
    str->synthetic = nullptr;
    str->errhi = nullptr;
    str->errlo = nullptr;
    memset(str->errparams, '\0', sizeof(str->errparams[0]) * MAXERRPARAMS);
    str->dims = nullptr;
    str->data_system = nullptr;
    str->system_config = nullptr;
    str->data_source = nullptr;
    str->signal_rec = nullptr;
    str->signal_desc = nullptr;
    memset(str->data_units, '\0', STRING_LENGTH);
    memset(str->data_label, '\0', STRING_LENGTH);
    memset(str->data_desc, '\0', STRING_LENGTH);
    memset(str->error_msg, '\0', STRING_LENGTH);
    init_client_block(&(str->client_block), 0, "");
}

void uda::client_server::init_dim_block(Dims* str)
{
    str->dim = nullptr;
    str->synthetic = nullptr;
    str->dim_n = 0;
    str->data_type = UDA_TYPE_FLOAT;
    str->error_type = UDA_TYPE_UNKNOWN;
    str->error_model = 0;
    str->errasymmetry = 0;
    str->error_param_n = 0;
    str->compressed = 0;
    str->method = 0;
    str->dim0 = 0.0E0;
    str->diff = 0.0E0;
    str->udoms = 0;
    str->sams = nullptr;
    str->offs = nullptr;
    str->ints = nullptr;
    str->errhi = nullptr;
    str->errlo = nullptr;
    for (int i = 0; i < MAXERRPARAMS; i++) {
        str->errparams[i] = 0.0;
    }
    str->dim_units[0] = '\0';
    str->dim_label[0] = '\0';
}

void uda::client_server::init_data_system(DataSystem* str)
{
    str->system_id = 0;
    str->version = 0;
    str->meta_id = 0;
    str->type = ' ';
    str->device_name[0] = '\0';
    str->system_name[0] = '\0';
    str->system_desc[0] = '\0';
    str->creation[0] = '\0';
    str->xml[0] = '\0';
    str->xml_creation[0] = '\0';
}

void uda::client_server::init_system_config(SystemConfig* str)
{
    str->config_id = 0;
    str->system_id = 0;
    str->meta_id = 0;
    str->config_name[0] = '\0';
    str->config_desc[0] = '\0';
    str->creation[0] = '\0';
    str->xml[0] = '\0';
    str->xml_creation[0] = '\0';
}

void uda::client_server::init_data_source(DataSource* str)
{
    str->source_id = 0;
    str->config_id = 0;
    str->reason_id = 0;
    str->run_id = 0;
    str->meta_id = 0;
    str->status_desc_id = 0;
    str->exp_number = 0;
    str->pass = 0;
    str->access = ' ';
    str->reprocess = ' ';
    str->status = 1;
    str->status_reason_code = 0;
    str->status_impact_code = 0;
    str->type = ' ';
    str->source_alias[0] = '\0';
    str->pass_date[0] = '\0';
    str->archive[0] = '\0';
    str->device_name[0] = '\0';
    str->format[0] = '\0';
    str->path[0] = '\0';
    str->filename[0] = '\0';
    str->server[0] = '\0';
    str->userid[0] = '\0';
    str->reason_desc[0] = '\0';
    str->status_desc[0] = '\0';
    str->run_desc[0] = '\0';
    str->creation[0] = '\0';
    str->modified[0] = '\0';
    str->xml[0] = '\0';
    str->xml_creation[0] = '\0';
}

void uda::client_server::init_signal(Signal* str)
{
    str->source_id = 0;
    str->signal_desc_id = 0;
    str->meta_id = 0;
    str->status_desc_id = 0;
    str->access = ' ';
    str->reprocess = ' ';
    str->status = 1;
    str->status_reason_code = 0;
    str->status_impact_code = 0;
    str->status_desc[0] = '\0';
    str->creation[0] = '\0';
    str->modified[0] = '\0';
    str->xml[0] = '\0';
    str->xml_creation[0] = '\0';
}

void uda::client_server::init_signal_desc(SignalDesc* str)
{
    str->signal_desc_id = 0;
    str->meta_id = 0;
    str->rank = 0;
    str->range_start = 0;
    str->range_stop = 0;
    str->signal_alias_type = 0;
    str->signal_map_id = 0;
    str->type = ' ';
    str->source_alias[0] = '\0';
    str->signal_alias[0] = '\0';
    str->signal_name[0] = '\0';
    str->generic_name[0] = '\0';
    str->description[0] = '\0';
    str->signal_class[0] = '\0';
    str->signal_owner[0] = '\0';
    str->creation[0] = '\0';
    str->modified[0] = '\0';
    str->xml[0] = '\0';
    str->xml_creation[0] = '\0';
}

void uda::client_server::init_put_data_block(PutDataBlock* str)
{
    str->data_type = UDA_TYPE_UNKNOWN;
    str->rank = 0;
    str->count = 0;
    str->shape = nullptr;
    str->data = nullptr;
    str->opaque_type = UDA_OPAQUE_TYPE_UNKNOWN;
    str->opaque_count = 0;
    str->opaque_block = nullptr;
    str->blockNameLength = 0;
    str->blockName = nullptr;
}

void uda::client_server::init_put_data_block_list(PutDataBlockList* putDataBlockList)
{
    putDataBlockList->putDataBlock = nullptr;
    putDataBlockList->blockCount = 0;
    putDataBlockList->blockListSize = 0;
}
