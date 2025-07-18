
// Initialise Data Structures
//
//----------------------------------------------------------------------------------

#include "initStructs.h"

#ifdef __GNUC__
#  include <unistd.h>
#elif defined(_WIN32)
#  include <process.h>
#endif

#include <clientserver/udaTypes.h>
#include <cstring>

void initNameValueList(NAMEVALUELIST* nameValueList)
{
    nameValueList->pairCount = 0;
    nameValueList->listSize = 0;
    nameValueList->nameValue = nullptr;
}

void initRequestData(REQUEST_DATA* str)
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
    initNameValueList(&str->nameValueList);

    str->put = 0;
    initPutDataBlockList(&str->putDataBlockList);
}

void initRequestBlock(REQUEST_BLOCK* str)
{
    str->num_requests = 0;
    str->requests = nullptr;
}

#ifdef _WIN32
#  define getpid _getpid
#endif

void initClientBlock(CLIENT_BLOCK* str, int version, const char* clientname)
{
    str->version = version;
    str->timeout = TIMEOUT;
    if (getenv("UDA_TIMEOUT")) {
        str->timeout = (int)strtol(getenv("UDA_TIMEOUT"), nullptr, 10);
    }
    str->pid = (int)getpid();
    strcpy(str->uid, clientname);        // Global userid
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

    str->OSName[0] = '\0';    // Operating System Name
    str->DOI[0] = '\0';    // Digital Object Identifier (client study reference)

#ifdef SECURITYENABLED
    initSecurityBlock(&(str->securityBlock));
#endif
}

void initServerBlock(SERVER_BLOCK* str, int version)
{
    str->version = version;
    str->error = 0;
    str->msg[0] = '\0';
    str->pid = (int)getpid();
    str->idamerrorstack.nerrors = 0;
    //RC str->idamerrorstack.idamerror = nullptr;
    str->OSName[0] = '\0';    // Operating System Name
    str->DOI[0] = '\0';    // Digital Object Identifier (server configuration)

#ifdef SECURITYENABLED
    initSecurityBlock(&(str->securityBlock));
#endif
}

void initDataBlockList(DATA_BLOCK_LIST* str)
{
    str->count = 0;
    str->data = nullptr;
}

void initDataBlock(DATA_BLOCK* str)
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
    initClientBlock(&(str->client_block), 0, "");
}

void initDimBlock(DIMS* str)
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

void initDataSystem(DATA_SYSTEM* str)
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

void initSystemConfig(SYSTEM_CONFIG* str)
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

void initDataSource(DATA_SOURCE* str)
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

void initSignal(SIGNAL* str)
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

void initSignalDesc(SIGNAL_DESC* str)
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

void initIdamPutDataBlock(PUTDATA_BLOCK* str)
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

void initPutDataBlockList(PUTDATA_BLOCK_LIST* putDataBlockList)
{
    putDataBlockList->putDataBlock = nullptr;
    putDataBlockList->blockCount = 0;
    putDataBlockList->blockListSize = 0;
}
