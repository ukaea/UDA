#include "init_structs.h"

#ifdef __GNUC__
#  include <unistd.h>
#elif defined(_WIN32)
#  include <process.h>
#endif

#include <cstring>
#include <uda/types.h>

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
    str->name_value_list = {};

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
    str->timeout = TimeOut;
    if (getenv("UDA_TIMEOUT")) {
        str->timeout = (int)strtol(getenv("UDA_TIMEOUT"), nullptr, 10);
    }
    str->pid = (int)getpid();
    strcpy(str->uid, clientname); // Global userid
    str->compressDim = CompressDim;

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
    memset(str->errparams, '\0', sizeof(str->errparams[0]) * MaxErrParams);
    str->dims = nullptr;
    str->meta_data = {};
    memset(str->data_units, '\0', StringLength);
    memset(str->data_label, '\0', StringLength);
    memset(str->data_desc, '\0', StringLength);
    memset(str->error_msg, '\0', StringLength);
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
    for (int i = 0; i < MaxErrParams; i++) {
        str->errparams[i] = 0.0;
    }
    str->dim_units[0] = '\0';
    str->dim_label[0] = '\0';
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
