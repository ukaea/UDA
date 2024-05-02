// Print the Contents of the database Record Structures
//
//----------------------------------------------------------------------------------

#include "printStructs.h"

#include "clientserver/errorLog.h"
#include "logging/logging.h"

using namespace uda::client_server;
using namespace uda::logging;

void uda::client_server::print_request_data(RequestData str)
{
    UDA_LOG(UDA_LOG_DEBUG, "request     : {}", str.request);
    UDA_LOG(UDA_LOG_DEBUG, "exp_number  : {}", str.exp_number);
    UDA_LOG(UDA_LOG_DEBUG, "pass        : {}", str.pass);
    UDA_LOG(UDA_LOG_DEBUG, "tpass       : {}", str.tpass);
    UDA_LOG(UDA_LOG_DEBUG, "path        : {}", str.path);
    UDA_LOG(UDA_LOG_DEBUG, "file        : {}", str.file);
    UDA_LOG(UDA_LOG_DEBUG, "format      : {}", str.format);
    UDA_LOG(UDA_LOG_DEBUG, "archive     : {}", str.archive);
    UDA_LOG(UDA_LOG_DEBUG, "device_name : {}", str.device_name);
    UDA_LOG(UDA_LOG_DEBUG, "server      : {}", str.server);
    UDA_LOG(UDA_LOG_DEBUG, "function    : {}", str.function);
    UDA_LOG(UDA_LOG_DEBUG, "signal      : {}", str.signal);
    UDA_LOG(UDA_LOG_DEBUG, "source      : {}", str.source);
    UDA_LOG(UDA_LOG_DEBUG, "api_delim   : {}", str.api_delim);
    UDA_LOG(UDA_LOG_DEBUG, "subset      : {}", str.subset);
    UDA_LOG(UDA_LOG_DEBUG, "subsetCount : {}", str.datasubset.nbound);
    for (int i = 0; i < str.datasubset.nbound; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "[{}] {}   {}   {}   {}", i, str.datasubset.dimid[i], str.datasubset.lbindex[i].value,
                str.datasubset.ubindex[i].value, str.datasubset.stride[i].value);
    }
    UDA_LOG(UDA_LOG_DEBUG, "nameValueCount : {}", str.nameValueList.pairCount);
    for (int i = 0; i < str.nameValueList.pairCount; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "[{}] {},   {},   {}", i, str.nameValueList.nameValue[i].pair,
                str.nameValueList.nameValue[i].name, str.nameValueList.nameValue[i].value);
    }
}

void uda::client_server::print_request_block(RequestBlock str)
{
    UDA_LOG(UDA_LOG_DEBUG, "Client Request Block");
    UDA_LOG(UDA_LOG_DEBUG, "# requests  : {}", str.num_requests);
    for (int i = 0; i < str.num_requests; ++i) {
        UDA_LOG(UDA_LOG_DEBUG, "number      : {}", i);
        print_request_data(str.requests[0]);
    }
}

void uda::client_server::print_client_block(ClientBlock str)
{
    UDA_LOG(UDA_LOG_DEBUG, "Client State Block");
    UDA_LOG(UDA_LOG_DEBUG, "version      : {}", str.version);
    UDA_LOG(UDA_LOG_DEBUG, "pid          : {}", str.pid);
    UDA_LOG(UDA_LOG_DEBUG, "uid          : {}", str.uid);

    UDA_LOG(UDA_LOG_DEBUG, "timeout      : {}", str.timeout);
    UDA_LOG(UDA_LOG_DEBUG, "compress_dim  : {}", str.compressDim);

    UDA_LOG(UDA_LOG_DEBUG, "clientFlags  : {}", str.clientFlags);
    UDA_LOG(UDA_LOG_DEBUG, "altRank      : {}", str.altRank);

    UDA_LOG(UDA_LOG_DEBUG, "get_nodimdata: {}", str.get_nodimdata);
    UDA_LOG(UDA_LOG_DEBUG, "get_datadble : {}", str.get_datadble);
    UDA_LOG(UDA_LOG_DEBUG, "get_timedble : {}", str.get_timedble);
    UDA_LOG(UDA_LOG_DEBUG, "get_dimdble  : {}", str.get_dimdble);
    UDA_LOG(UDA_LOG_DEBUG, "get_bad      : {}", str.get_bad);
    UDA_LOG(UDA_LOG_DEBUG, "get_meta     : {}", str.get_meta);
    UDA_LOG(UDA_LOG_DEBUG, "get_asis     : {}", str.get_asis);
    UDA_LOG(UDA_LOG_DEBUG, "get_uncal    : {}", str.get_uncal);
    UDA_LOG(UDA_LOG_DEBUG, "get_notoff   : {}", str.get_notoff);
    UDA_LOG(UDA_LOG_DEBUG, "get_scalar   : {}", str.get_scalar);
    UDA_LOG(UDA_LOG_DEBUG, "get_bytes    : {}", str.get_bytes);

    UDA_LOG(UDA_LOG_DEBUG, "privateFlags : {}", str.privateFlags);

    UDA_LOG(UDA_LOG_DEBUG, "OS Name      : {}", str.OSName);
    UDA_LOG(UDA_LOG_DEBUG, "Study DOI    : {}", str.DOI);
}

void uda::client_server::print_server_block(ServerBlock str)
{
    UDA_LOG(UDA_LOG_DEBUG, "Server State Block");
    UDA_LOG(UDA_LOG_DEBUG, "version          : {}", str.version);
    UDA_LOG(UDA_LOG_DEBUG, "error            : {}", str.error);
    UDA_LOG(UDA_LOG_DEBUG, "msg              : {}", str.msg);
    UDA_LOG(UDA_LOG_DEBUG, "Server PID       : {}", str.pid);
    UDA_LOG(UDA_LOG_DEBUG, "OS Name          : {}", str.OSName);
    UDA_LOG(UDA_LOG_DEBUG, "Configuration DOI: {}", str.DOI);
    print_error_stack();
}

void uda::client_server::print_data_block_list(DataBlockList str)
{
    UDA_LOG(UDA_LOG_DEBUG, "Data Blocks");
    UDA_LOG(UDA_LOG_DEBUG, "count        : {}", str.count);
    for (int i = 0; i < str.count; ++i) {
        UDA_LOG(UDA_LOG_DEBUG, "block number : {}", i);
        print_data_block(str.data[i]);
    }
}

void uda::client_server::print_data_block(DataBlock str)
{
    UDA_LOG(UDA_LOG_DEBUG, "Data Block Contents");
    UDA_LOG(UDA_LOG_DEBUG, "handle       : {}", str.handle);
    UDA_LOG(UDA_LOG_DEBUG, "error code   : {}", str.errcode);
    UDA_LOG(UDA_LOG_DEBUG, "error msg    : {}", str.error_msg);
    UDA_LOG(UDA_LOG_DEBUG, "source status: {}", str.source_status);
    UDA_LOG(UDA_LOG_DEBUG, "signal status: {}", str.signal_status);
    UDA_LOG(UDA_LOG_DEBUG, "data_number  : {}", str.data_n);
    UDA_LOG(UDA_LOG_DEBUG, "rank         : {}", str.rank);
    UDA_LOG(UDA_LOG_DEBUG, "order        : {}", str.order);
    UDA_LOG(UDA_LOG_DEBUG, "data_type    : {}", str.data_type);
    UDA_LOG(UDA_LOG_DEBUG, "error_type   : {}", str.error_type);
    UDA_LOG(UDA_LOG_DEBUG, "errhi != nullptr: {}", str.errhi != nullptr);
    UDA_LOG(UDA_LOG_DEBUG, "errlo != nullptr: {}", str.errlo != nullptr);

    UDA_LOG(UDA_LOG_DEBUG, "opaque_type : {}", str.opaque_type);
    UDA_LOG(UDA_LOG_DEBUG, "opaque_count: {}", str.opaque_count);

    switch (str.opaque_type) {
        case (UDA_OPAQUE_TYPE_XML_DOCUMENT):
            if (str.opaque_block != nullptr) {
                UDA_LOG(UDA_LOG_DEBUG, "XML: {}", (char*)str.opaque_block);
            }
            break;
        default:
            break;
    }

    int k = 10;
    if (str.data_n < 10) {
        k = str.data_n;
    }

    if (str.data_type == UDA_TYPE_FLOAT) {
        for (int j = 0; j < k; j++) {
            UDA_LOG(UDA_LOG_DEBUG, "data[{}]: {}", j, *((float*)str.data + j));
        }
    }
    if (str.data_type == UDA_TYPE_DOUBLE) {
        for (int j = 0; j < k; j++) {
            UDA_LOG(UDA_LOG_DEBUG, "data[{}]: {}", j, *((double*)str.data + j));
        }
    }

    if (str.error_type == UDA_TYPE_FLOAT && str.errhi != nullptr) {
        for (int j = 0; j < k; j++) {
            UDA_LOG(UDA_LOG_DEBUG, "errhi[{}]: {}", j, *((float*)str.errhi + j));
        }
    }

    if (str.error_type == UDA_TYPE_FLOAT && str.errlo != nullptr && str.errasymmetry) {
        for (int j = 0; j < k; j++) {
            UDA_LOG(UDA_LOG_DEBUG, "errlo[{}]: {}", j, *((float*)str.errlo + j));
        }
    }

    UDA_LOG(UDA_LOG_DEBUG, "error model : {}", str.error_model);
    UDA_LOG(UDA_LOG_DEBUG, "asymmetry   : {}", str.errasymmetry);
    UDA_LOG(UDA_LOG_DEBUG, "error model no. params : {}", str.error_param_n);

    for (int i = 0; i < str.error_param_n; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "param[{}] = {} ", i, str.errparams[i]);
    }

    UDA_LOG(UDA_LOG_DEBUG, "data_units  : {}", str.data_units);
    UDA_LOG(UDA_LOG_DEBUG, "data_label  : {}", str.data_label);
    UDA_LOG(UDA_LOG_DEBUG, "data_desc   : {}", str.data_desc);

    for (int i = 0; i < (int)str.rank; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "Dimension #{} Contents", i);
        UDA_LOG(UDA_LOG_DEBUG, "data_type    : {}", str.dims[i].data_type);
        UDA_LOG(UDA_LOG_DEBUG, "error_type   : {}", str.dims[i].error_type);
        UDA_LOG(UDA_LOG_DEBUG, "errhi != nullptr: {}", str.dims[i].errhi != nullptr);
        UDA_LOG(UDA_LOG_DEBUG, "errlo != nullptr: {}", str.dims[i].errlo != nullptr);
        UDA_LOG(UDA_LOG_DEBUG, "error model  : {}", str.dims[i].error_model);
        UDA_LOG(UDA_LOG_DEBUG, "asymmetry    : {}", str.dims[i].errasymmetry);
        UDA_LOG(UDA_LOG_DEBUG, "error model no. params : {}", str.dims[i].error_param_n);

        for (int j = 0; j < str.dims[i].error_param_n; j++) {
            UDA_LOG(UDA_LOG_DEBUG, "param[{}] = {} ", j, str.dims[i].errparams[j]);
        }

        UDA_LOG(UDA_LOG_DEBUG, "data_number : {}", str.dims[i].dim_n);
        UDA_LOG(UDA_LOG_DEBUG, "compressed? : {}", str.dims[i].compressed);
        UDA_LOG(UDA_LOG_DEBUG, "method      : {}", str.dims[i].method);

        if (str.dims[i].method == 0) {
            if (str.dims[i].compressed) {
                UDA_LOG(UDA_LOG_DEBUG, "starting val: {}", str.dims[i].dim0);
                UDA_LOG(UDA_LOG_DEBUG, "stepping val: {}", str.dims[i].diff);
            } else {
                if (str.dims[i].data_type == UDA_TYPE_FLOAT) {
                    k = 10;
                    if (str.dims[i].dim_n < 10) {
                        k = str.dims[i].dim_n;
                    }
                    if (str.dims[i].dim != nullptr) {
                        for (int j = 0; j < k; j++) {
                            UDA_LOG(UDA_LOG_DEBUG, "val[{}] = {}", j, *((float*)str.dims[i].dim + j));
                        }
                    }
                }
                if (str.dims[i].data_type == UDA_TYPE_DOUBLE) {
                    k = 10;
                    if (str.dims[i].dim_n < 10) {
                        k = str.dims[i].dim_n;
                    }
                    if (str.dims[i].dim != nullptr) {
                        for (int j = 0; j < k; j++) {
                            UDA_LOG(UDA_LOG_DEBUG, "val[{}] = {}", j, *((double*)str.dims[i].dim + j));
                        }
                    }
                }
            }
        } else {
            UDA_LOG(UDA_LOG_DEBUG, "udoms: {}", str.dims[i].udoms);
            switch (str.dims[i].method) {
                case 1:
                    if (str.dims[i].data_type == UDA_TYPE_FLOAT) {
                        k = 10;
                        if (str.dims[i].udoms < 10) {
                            k = str.dims[i].udoms;
                        }
                        for (int j = 0; j < k; j++) {
                            UDA_LOG(UDA_LOG_DEBUG, "sams[{}]: {}", j, (int)*(str.dims[i].sams + j));
                            UDA_LOG(UDA_LOG_DEBUG, "offs[{}]: {}", j, *((float*)str.dims[i].offs + j));
                            UDA_LOG(UDA_LOG_DEBUG, "ints[{}]: {}", j, *((float*)str.dims[i].ints + j));
                        }
                    }
                    if (str.dims[i].data_type == UDA_TYPE_DOUBLE) {
                        k = 10;
                        if (str.dims[i].udoms < 10) {
                            k = str.dims[i].udoms;
                        }
                        for (int j = 0; j < k; j++) {
                            UDA_LOG(UDA_LOG_DEBUG, "sams[{}]: {}", j, (int)*(str.dims[i].sams + j));
                            UDA_LOG(UDA_LOG_DEBUG, "offs[{}]: {}", j, *((double*)str.dims[i].offs + j));
                            UDA_LOG(UDA_LOG_DEBUG, "ints[{}]: {}", j, *((double*)str.dims[i].ints + j));
                        }
                    }
                    break;
                case 2:
                    if (str.dims[i].data_type == UDA_TYPE_FLOAT) {
                        k = 10;
                        if (str.dims[i].udoms < 10) {
                            k = str.dims[i].udoms;
                        }
                        for (int j = 0; j < k; j++) {
                            UDA_LOG(UDA_LOG_DEBUG, "offs[{}]: {}", j, *((float*)str.dims[i].offs + j));
                        }
                    }
                    if (str.dims[i].data_type == UDA_TYPE_DOUBLE) {
                        k = 10;
                        if (str.dims[i].udoms < 10) {
                            k = str.dims[i].udoms;
                        }
                        for (int j = 0; j < k; j++) {
                            UDA_LOG(UDA_LOG_DEBUG, "offs[{}]: {}", j, *((double*)str.dims[i].offs + j));
                        }
                    }
                    break;
                case 3:
                    if (str.dims[i].data_type == UDA_TYPE_FLOAT) {
                        UDA_LOG(UDA_LOG_DEBUG, "offs[0] val: {}", *((float*)str.dims[i].offs));
                        UDA_LOG(UDA_LOG_DEBUG, "ints[0] val: {}", *((float*)str.dims[i].ints));
                    }
                    if (str.dims[i].data_type == UDA_TYPE_DOUBLE) {
                        UDA_LOG(UDA_LOG_DEBUG, "offs[0] val: {}", *((double*)str.dims[i].offs));
                        UDA_LOG(UDA_LOG_DEBUG, "ints[0] val: {}", *((double*)str.dims[i].ints));
                    }
                    break;
                default:
                    UDA_LOG(UDA_LOG_WARN, "unknown method ({}) for dim ({})", str.dims[i].method, i);
            }
        }
        if (str.dims[i].error_type == UDA_TYPE_FLOAT) {
            k = 10;
            if (str.dims[i].dim_n < 10) {
                k = str.dims[i].dim_n;
            }
            if (str.dims[i].errhi != nullptr) {
                for (int j = 0; j < k; j++) {
                    UDA_LOG(UDA_LOG_DEBUG, "errhi[{}] = {}", j, *((float*)str.dims[i].errhi + j));
                }
            }
            if (str.dims[i].errlo != nullptr && str.dims[i].errasymmetry) {
                for (int j = 0; j < k; j++) {
                    UDA_LOG(UDA_LOG_DEBUG, "errlo[{}] = {}", j, *((float*)str.dims[i].errlo + j));
                }
            }
        }
        UDA_LOG(UDA_LOG_DEBUG, "data_units  : {}", str.dims[i].dim_units);
        UDA_LOG(UDA_LOG_DEBUG, "data_label  : {}", str.dims[i].dim_label);
    }
}

void uda::client_server::print_system_config(SystemConfig str)
{
    UDA_LOG(UDA_LOG_DEBUG, "System Configuration Record");
    UDA_LOG(UDA_LOG_DEBUG, "config_id   : {}", str.config_id);
    UDA_LOG(UDA_LOG_DEBUG, "system_id   : {}", str.system_id);
    UDA_LOG(UDA_LOG_DEBUG, "config_name : {}", str.config_name);
    UDA_LOG(UDA_LOG_DEBUG, "config_desc : {}", str.config_desc);
    UDA_LOG(UDA_LOG_DEBUG, "creation    : {}", str.creation);
    UDA_LOG(UDA_LOG_DEBUG, "meta_id     : {}", str.meta_id);
    UDA_LOG(UDA_LOG_DEBUG, "xml         : {}", str.xml);
    UDA_LOG(UDA_LOG_DEBUG, "xml_creation: {}", str.xml_creation);
}

void uda::client_server::print_data_system(DataSystem str)
{
    UDA_LOG(UDA_LOG_DEBUG, "Data System Record");
    UDA_LOG(UDA_LOG_DEBUG, "system_id   : {}", str.system_id);
    UDA_LOG(UDA_LOG_DEBUG, "version     : {}", str.version);
    UDA_LOG(UDA_LOG_DEBUG, "type        : {}", str.type);
    UDA_LOG(UDA_LOG_DEBUG, "device_name : {}", str.device_name);
    UDA_LOG(UDA_LOG_DEBUG, "system_name : {}", str.system_name);
    UDA_LOG(UDA_LOG_DEBUG, "system_desc : {}", str.system_desc);
    UDA_LOG(UDA_LOG_DEBUG, "creation    : {}", str.creation);
    UDA_LOG(UDA_LOG_DEBUG, "meta_id     : {}", str.meta_id);
    UDA_LOG(UDA_LOG_DEBUG, "xml         : {}", str.xml);
    UDA_LOG(UDA_LOG_DEBUG, "xml_creation: {}", str.xml_creation);
}

void uda::client_server::print_data_source(DataSource str)
{
    UDA_LOG(UDA_LOG_DEBUG, "Data Source Record");
    UDA_LOG(UDA_LOG_DEBUG, "source_id     : {}", str.source_id);
    UDA_LOG(UDA_LOG_DEBUG, "config_id     : {}", str.config_id);
    UDA_LOG(UDA_LOG_DEBUG, "reason_id     : {}", str.reason_id);
    UDA_LOG(UDA_LOG_DEBUG, "run_id        : {}", str.run_id);
    UDA_LOG(UDA_LOG_DEBUG, "status_desc_id: {}", str.status_desc_id);
    UDA_LOG(UDA_LOG_DEBUG, "exp_number    : {}", str.exp_number);
    UDA_LOG(UDA_LOG_DEBUG, "pass          : {}", str.pass);
    UDA_LOG(UDA_LOG_DEBUG, "status        : {}", str.status);
    UDA_LOG(UDA_LOG_DEBUG, "status_reason_code: {}", str.status_reason_code);
    UDA_LOG(UDA_LOG_DEBUG, "status_impact_code: {}", str.status_impact_code);
    UDA_LOG(UDA_LOG_DEBUG, "access        : {}", str.access);
    UDA_LOG(UDA_LOG_DEBUG, "reprocess     : {}", str.reprocess);
    UDA_LOG(UDA_LOG_DEBUG, "type          : {}", str.type);
    UDA_LOG(UDA_LOG_DEBUG, "source_alias  : {}", str.source_alias);
    UDA_LOG(UDA_LOG_DEBUG, "pass_date     : {}", str.pass_date);
    UDA_LOG(UDA_LOG_DEBUG, "archive       : {}", str.archive);
    UDA_LOG(UDA_LOG_DEBUG, "device_name   : {}", str.device_name);
    UDA_LOG(UDA_LOG_DEBUG, "format        : {}", str.format);
    UDA_LOG(UDA_LOG_DEBUG, "path          : {}", str.path);
    UDA_LOG(UDA_LOG_DEBUG, "filename      : {}", str.filename);
    UDA_LOG(UDA_LOG_DEBUG, "server        : {}", str.server);
    UDA_LOG(UDA_LOG_DEBUG, "userid        : {}", str.userid);
    UDA_LOG(UDA_LOG_DEBUG, "reason_desc   : {}", str.reason_desc);
    UDA_LOG(UDA_LOG_DEBUG, "run_desc      : {}", str.run_desc);
    UDA_LOG(UDA_LOG_DEBUG, "status_desc   : {}", str.status_desc);
    UDA_LOG(UDA_LOG_DEBUG, "creation      : {}", str.creation);
    UDA_LOG(UDA_LOG_DEBUG, "modified      : {}", str.modified);
    UDA_LOG(UDA_LOG_DEBUG, "meta_id       : {}", str.meta_id);
    UDA_LOG(UDA_LOG_DEBUG, "xml           : {}", str.xml);
    UDA_LOG(UDA_LOG_DEBUG, "xml_creation  : {}", str.xml_creation);
}

void uda::client_server::print_signal(Signal str)
{
    UDA_LOG(UDA_LOG_DEBUG, "Signal Record");
    UDA_LOG(UDA_LOG_DEBUG, "source_id         : {}", str.source_id);
    UDA_LOG(UDA_LOG_DEBUG, "signal_desc_id    : {}", str.signal_desc_id);
    UDA_LOG(UDA_LOG_DEBUG, "status_desc_id    : {}", str.status_desc_id);
    UDA_LOG(UDA_LOG_DEBUG, "status            : {}", str.status);
    UDA_LOG(UDA_LOG_DEBUG, "status_reason_code: {}", str.status_reason_code);
    UDA_LOG(UDA_LOG_DEBUG, "status_impact_code: {}", str.status_impact_code);
    UDA_LOG(UDA_LOG_DEBUG, "status_desc       : {}", str.status_desc);
    UDA_LOG(UDA_LOG_DEBUG, "access            : {}", str.access);
    UDA_LOG(UDA_LOG_DEBUG, "reprocess         : {}", str.reprocess);
    UDA_LOG(UDA_LOG_DEBUG, "creation          : {}", str.creation);
    UDA_LOG(UDA_LOG_DEBUG, "modified          : {}", str.modified);
    UDA_LOG(UDA_LOG_DEBUG, "meta_id           : {}", str.meta_id);
    UDA_LOG(UDA_LOG_DEBUG, "xml               : {}", str.xml);
    UDA_LOG(UDA_LOG_DEBUG, "xml_creation      : {}", str.xml_creation);
}

void uda::client_server::print_signal_desc(SignalDesc str)
{
    UDA_LOG(UDA_LOG_DEBUG, "Signal Description Record");
    UDA_LOG(UDA_LOG_DEBUG, "signal_desc_id: {}", str.signal_desc_id);
    UDA_LOG(UDA_LOG_DEBUG, "signal_alias  : {}", str.signal_alias);
    UDA_LOG(UDA_LOG_DEBUG, "signal_name   : {}", str.signal_name);
    UDA_LOG(UDA_LOG_DEBUG, "generic_name  : {}", str.generic_name);
    UDA_LOG(UDA_LOG_DEBUG, "rank          : {}", str.rank);
    UDA_LOG(UDA_LOG_DEBUG, "range_start   : {}", str.range_start);
    UDA_LOG(UDA_LOG_DEBUG, "range_stop    : {}", str.range_stop);
    UDA_LOG(UDA_LOG_DEBUG, "signal_alias_type: {}", str.signal_alias_type);
    UDA_LOG(UDA_LOG_DEBUG, "signal_map_id : {}", str.signal_map_id);
    UDA_LOG(UDA_LOG_DEBUG, "type          : {}", str.type);
    UDA_LOG(UDA_LOG_DEBUG, "source_alias  : {}", str.source_alias);
    UDA_LOG(UDA_LOG_DEBUG, "description   : {}", str.description);
    UDA_LOG(UDA_LOG_DEBUG, "signal_class  : {}", str.signal_class);
    UDA_LOG(UDA_LOG_DEBUG, "signal_owner  : {}", str.signal_owner);
    UDA_LOG(UDA_LOG_DEBUG, "modified      : {}", str.modified);
    UDA_LOG(UDA_LOG_DEBUG, "creation      : {}", str.creation);
    UDA_LOG(UDA_LOG_DEBUG, "meta_id       : {}", str.meta_id);
    UDA_LOG(UDA_LOG_DEBUG, "xml           : {}", str.xml);
    UDA_LOG(UDA_LOG_DEBUG, "xml_creation  : {}", str.xml_creation);
}
