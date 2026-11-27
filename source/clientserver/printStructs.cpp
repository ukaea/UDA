// Print the Contents of the database Record Structures
//
//----------------------------------------------------------------------------------

#include "printStructs.h"

#include <logging/logging.h>
#include <clientserver/udaTypes.h>
#include <clientserver/errorLog.h>
#include <clientserver/udaDiagnostics.h>

void udaPrintRequestData(const UDA_DIAGNOSTICS* diag, REQUEST_DATA str)
{
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "request     : %d\n", str.request);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "exp_number  : %d\n", str.exp_number);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "pass        : %d\n", str.pass);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "tpass       : %s\n", str.tpass);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "path        : %s\n", str.path);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "file        : %s\n", str.file);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "format      : %s\n", str.format);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "archive     : %s\n", str.archive);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "device_name : %s\n", str.device_name);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "server      : %s\n", str.server);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "function    : %s\n", str.function);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "signal      : %s\n", str.signal);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "source      : %s\n", str.source);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "api_delim   : %s\n", str.api_delim);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "subset      : %s\n", str.subset);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "subsetCount : %d\n", str.datasubset.nbound);
    for (int i = 0; i < str.datasubset.nbound; i++) {
        UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "[%d] %d   %ld   %ld   %ld\n", i, str.datasubset.dimid[i],
                str.datasubset.lbindex[i].value,
                str.datasubset.ubindex[i].value,
                str.datasubset.stride[i].value);
    }
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "nameValueCount : %d\n", str.nameValueList.pairCount);
    for (int i = 0; i < str.nameValueList.pairCount; i++) {
        UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "[%d] %s,   %s,   %s\n", i, str.nameValueList.nameValue[i].pair,
                str.nameValueList.nameValue[i].name, str.nameValueList.nameValue[i].value);
    }
}

void udaPrintRequestBlock(const UDA_DIAGNOSTICS* diag, REQUEST_BLOCK str)
{
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Client Request Block\n");
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "# requests  : %d\n", str.num_requests);
    for (int i = 0; i < str.num_requests; ++i) {
        UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "number      : %d\n", i);
        udaPrintRequestData(diag, str.requests[0]);
    }
}

void udaPrintClientBlock(const UDA_DIAGNOSTICS* diag, CLIENT_BLOCK str)
{
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Client State Block\n");
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "version      : %d\n", str.version);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "pid          : %d\n", str.pid);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "uid          : %s\n", str.uid);

    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "timeout      : %d\n", str.timeout);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "compressDim  : %d\n", str.compressDim);

    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "clientFlags  : %d\n", str.clientFlags);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "altRank      : %d\n", str.altRank);

    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "get_nodimdata: %d\n", str.get_nodimdata);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "get_datadble : %d\n", str.get_datadble);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "get_timedble : %d\n", str.get_timedble);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "get_dimdble  : %d\n", str.get_dimdble);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "get_bad      : %d\n", str.get_bad);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "get_meta     : %d\n", str.get_meta);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "get_asis     : %d\n", str.get_asis);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "get_uncal    : %d\n", str.get_uncal);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "get_notoff   : %d\n", str.get_notoff);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "get_scalar   : %d\n", str.get_scalar);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "get_bytes    : %d\n", str.get_bytes);

    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "privateFlags : %d\n", str.privateFlags);

    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "OS Name      : %s\n", str.OSName);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Study DOI    : %s\n", str.DOI);
}


void udaPrintServerBlock(const UDA_DIAGNOSTICS* diag, SERVER_BLOCK str)
{
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Server State Block\n");
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "version          : %d\n", str.version);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "error            : %d\n", str.error);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "msg              : %s\n", str.msg);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Server PID       : %d\n", str.pid);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "OS Name          : %s\n", str.OSName);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Configuration DOI: %s\n", str.DOI);
    for (unsigned int i = 0; i < str.idamerrorstack.nerrors && i < UDA_MAX_ERRORS; ++i) {
        const UDA_ERROR* error = &str.idamerrorstack.idamerror[i];
        UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Error %u          : %d %d %s %s\n", i + 1, error->type, error->code,
                     error->location, error->msg);
    }
}

void udaPrintDataBlockList(const UDA_DIAGNOSTICS* diag, DATA_BLOCK_LIST str)
{
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Data Blocks\n");
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "count        : %d\n", str.count);
    for (int i = 0; i < str.count; ++i) {
        UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "block number : %d\n", i);
        udaPrintDataBlock(diag, str.data[i]);
    }
}

void udaPrintDataBlock(const UDA_DIAGNOSTICS* diag, DATA_BLOCK str)
{
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Data Block Contents\n");
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "handle       : %d\n", str.handle);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "error code   : %d\n", str.errcode);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "error msg    : %s\n", str.error_msg);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "source status: %d\n", str.source_status);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "signal status: %d\n", str.signal_status);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "data_number  : %d\n", str.data_n);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "rank         : %d\n", str.rank);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "order        : %d\n", str.order);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "data_type    : %d\n", str.data_type);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "error_type   : %d\n", str.error_type);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "errhi != nullptr: %d\n", str.errhi != nullptr);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "errlo != nullptr: %d\n", str.errlo != nullptr);

    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "opaque_type : %d\n", str.opaque_type);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "opaque_count: %d\n", str.opaque_count);

    switch (str.opaque_type) {
        case (UDA_OPAQUE_TYPE_XML_DOCUMENT):
            if (str.opaque_block != nullptr) {
                UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "XML: %s\n", (char*) str.opaque_block);
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
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "data[%d]: %f\n", j, *((float*)str.data + j));
        }
    }
    if (str.data_type == UDA_TYPE_DOUBLE) {
        for (int j = 0; j < k; j++) {
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "data[%d]: %f\n", j, *((double*)str.data + j));
        }
    }

    if (str.error_type == UDA_TYPE_FLOAT && str.errhi != nullptr) {
        for (int j = 0; j < k; j++) {
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "errhi[%d]: %f\n", j, *((float*)str.errhi + j));
        }
    }

    if (str.error_type == UDA_TYPE_FLOAT && str.errlo != nullptr && str.errasymmetry) {
        for (int j = 0; j < k; j++) {
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "errlo[%d]: %f\n", j, *((float*)str.errlo + j));
        }
    }

    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "error model : %d\n", str.error_model);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "asymmetry   : %d\n", str.errasymmetry);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "error model no. params : %d\n", str.error_param_n);

    for (int i = 0; i < str.error_param_n; i++) {
        UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "param[%d] = %f \n", i, str.errparams[i]);
    }
    
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "data_units  : %s\n", str.data_units);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "data_label  : %s\n", str.data_label);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "data_desc   : %s\n", str.data_desc);

    for (int i = 0; i < (int)str.rank; i++) {
        UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Dimension #%d Contents\n", i);
        UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "data_type    : %d\n", str.dims[i].data_type);
        UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "error_type   : %d\n", str.dims[i].error_type);
        UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "errhi != nullptr: %d\n", str.dims[i].errhi != nullptr);
        UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "errlo != nullptr: %d\n", str.dims[i].errlo != nullptr);
        UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "error model  : %d\n", str.dims[i].error_model);
        UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "asymmetry    : %d\n", str.dims[i].errasymmetry);
        UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "error model no. params : %d\n", str.dims[i].error_param_n);

        for (int j = 0; j < str.dims[i].error_param_n; j++) {
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "param[%d] = %f \n", j, str.dims[i].errparams[j]);
        }
        
        UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "data_number : %d\n", str.dims[i].dim_n);
        UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "compressed? : %d\n", str.dims[i].compressed);
        UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "method      : %d\n", str.dims[i].method);
        
        if (str.dims[i].method == 0) {
            if (str.dims[i].compressed) {
                UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "starting val: %f\n", str.dims[i].dim0);
                UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "stepping val: %f\n", str.dims[i].diff);
            } else {
                if (str.dims[i].data_type == UDA_TYPE_FLOAT) {
                    k = 10;
                    if (str.dims[i].dim_n < 10) k = str.dims[i].dim_n;
                    if (str.dims[i].dim != nullptr)
                        for (int j = 0; j < k; j++)
                            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "val[%d] = %f\n", j, *((float*) str.dims[i].dim + j));
                }
                if (str.dims[i].data_type == UDA_TYPE_DOUBLE) {
                    k = 10;
                    if (str.dims[i].dim_n < 10) k = str.dims[i].dim_n;
                    if (str.dims[i].dim != nullptr)
                        for (int j = 0; j < k; j++)
                            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "val[%d] = %f\n", j, *((double*) str.dims[i].dim + j));
                }
            }
        } else {
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "udoms: %d\n", str.dims[i].udoms);
            switch (str.dims[i].method) {
                case 1:
                    if (str.dims[i].data_type == UDA_TYPE_FLOAT) {
                        k = 10;
                        if (str.dims[i].udoms < 10) k = str.dims[i].udoms;
                        for (int j = 0; j < k; j++) {
                            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "sams[%d]: %d\n", j, (int) *(str.dims[i].sams + j));
                            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "offs[%d]: %f\n", j, *((float*) str.dims[i].offs + j));
                            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "ints[%d]: %f\n", j, *((float*) str.dims[i].ints + j));
                        }
                    }
                    if (str.dims[i].data_type == UDA_TYPE_DOUBLE) {
                        k = 10;
                        if (str.dims[i].udoms < 10) k = str.dims[i].udoms;
                        for (int j = 0; j < k; j++) {
                            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "sams[%d]: %d\n", j, (int) *(str.dims[i].sams + j));
                            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "offs[%d]: %f\n", j, *((double*) str.dims[i].offs + j));
                            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "ints[%d]: %f\n", j, *((double*) str.dims[i].ints + j));
                        }
                    }
                    break;
                case 2:
                    if (str.dims[i].data_type == UDA_TYPE_FLOAT) {
                        k = 10;
                        if (str.dims[i].udoms < 10) k = str.dims[i].udoms;
                        for (int j = 0; j < k; j++) UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "offs[%d]: %f\n", j, *((float*) str.dims[i].offs + j));
                    }
                    if (str.dims[i].data_type == UDA_TYPE_DOUBLE) {
                        k = 10;
                        if (str.dims[i].udoms < 10) k = str.dims[i].udoms;
                        for (int j = 0; j < k; j++) UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "offs[%d]: %f\n", j, *((double*) str.dims[i].offs + j));
                    }
                    break;
                case 3:
                    if (str.dims[i].data_type == UDA_TYPE_FLOAT) {
                        UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "offs[0] val: %f\n", *((float*) str.dims[i].offs));
                        UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "ints[0] val: %f\n", *((float*) str.dims[i].ints));
                    }
                    if (str.dims[i].data_type == UDA_TYPE_DOUBLE) {
                        UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "offs[0] val: %f\n", *((double*) str.dims[i].offs));
                        UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "ints[0] val: %f\n", *((double*) str.dims[i].ints));
                    }
                    break;
                default:
                    UDA_DIAG_LOG(diag, UDA_LOG_WARN, "unknown method (%d) for dim (%d)", str.dims[i].method, i);
            }
        }
        if (str.dims[i].error_type == UDA_TYPE_FLOAT) {
            k = 10;
            if (str.dims[i].dim_n < 10) k = str.dims[i].dim_n;
            if (str.dims[i].errhi != nullptr)
                for (int j = 0; j < k; j++)
                    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "errhi[%d] = %f\n", j, *((float*) str.dims[i].errhi + j));
            if (str.dims[i].errlo != nullptr && str.dims[i].errasymmetry)
                for (int j = 0; j < k; j++)
                    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "errlo[%d] = %f\n", j, *((float*) str.dims[i].errlo + j));
        }
        UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "data_units  : %s\n", str.dims[i].dim_units);
        UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "data_label  : %s\n", str.dims[i].dim_label);
    }
}


void udaPrintSystemConfig(const UDA_DIAGNOSTICS* diag, SYSTEM_CONFIG str)
{
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "System Configuration Record\n");
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "config_id   : %d\n", str.config_id);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "system_id   : %d\n", str.system_id);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "config_name : %s\n", str.config_name);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "config_desc : %s\n", str.config_desc);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "creation    : %s\n", str.creation);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "meta_id     : %d\n", str.meta_id);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "xml         : %s\n", str.xml);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "xml_creation: %s\n", str.xml_creation);
}


void udaPrintDataSystem(const UDA_DIAGNOSTICS* diag, DATA_SYSTEM str)
{
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Data System Record\n");
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "system_id   : %d\n", str.system_id);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "version     : %d\n", str.version);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "type        : %c\n", str.type);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "device_name : %s\n", str.device_name);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "system_name : %s\n", str.system_name);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "system_desc : %s\n", str.system_desc);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "creation    : %s\n", str.creation);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "meta_id     : %d\n", str.meta_id);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "xml         : %s\n", str.xml);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "xml_creation: %s\n", str.xml_creation);
}

void udaPrintDataSource(const UDA_DIAGNOSTICS* diag, DATA_SOURCE str)
{
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Data Source Record\n");
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "source_id     : %d\n", str.source_id);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "config_id     : %d\n", str.config_id);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "reason_id     : %d\n", str.reason_id);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "run_id        : %d\n", str.run_id);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "status_desc_id: %d\n", str.status_desc_id);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "exp_number    : %d\n", str.exp_number);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "pass          : %d\n", str.pass);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "status        : %d\n", str.status);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "status_reason_code: %d\n", str.status_reason_code);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "status_impact_code: %d\n", str.status_impact_code);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "access        : %c\n", str.access);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "reprocess     : %c\n", str.reprocess);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "type          : %c\n", str.type);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "source_alias  : %s\n", str.source_alias);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "pass_date     : %s\n", str.pass_date);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "archive       : %s\n", str.archive);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "device_name   : %s\n", str.device_name);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "format        : %s\n", str.format);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "path          : %s\n", str.path);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "filename      : %s\n", str.filename);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "server        : %s\n", str.server);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "userid        : %s\n", str.userid);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "reason_desc   : %s\n", str.reason_desc);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "run_desc      : %s\n", str.run_desc);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "status_desc   : %s\n", str.status_desc);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "creation      : %s\n", str.creation);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "modified      : %s\n", str.modified);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "meta_id       : %d\n", str.meta_id);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "xml           : %s\n", str.xml);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "xml_creation  : %s\n", str.xml_creation);
}

void udaPrintSignal(const UDA_DIAGNOSTICS* diag, SIGNAL str)
{
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Signal Record\n");
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "source_id         : %d\n", str.source_id);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "signal_desc_id    : %d\n", str.signal_desc_id);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "status_desc_id    : %d\n", str.status_desc_id);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "status            : %d\n", str.status);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "status_reason_code: %d\n", str.status_reason_code);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "status_impact_code: %d\n", str.status_impact_code);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "status_desc       : %s\n", str.status_desc);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "access            : %c\n", str.access);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "reprocess         : %c\n", str.reprocess);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "creation          : %s\n", str.creation);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "modified          : %s\n", str.modified);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "meta_id           : %d\n", str.meta_id);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "xml               : %s\n", str.xml);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "xml_creation      : %s\n", str.xml_creation);
}

void udaPrintSignalDesc(const UDA_DIAGNOSTICS* diag, SIGNAL_DESC str)
{
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Signal Description Record\n");
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "signal_desc_id: %d\n", str.signal_desc_id);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "signal_alias  : %s\n", str.signal_alias);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "signal_name   : %s\n", str.signal_name);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "generic_name  : %s\n", str.generic_name);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "rank          : %d\n", str.rank);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "range_start   : %d\n", str.range_start);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "range_stop    : %d\n", str.range_stop);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "signal_alias_type: %d\n", str.signal_alias_type);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "signal_map_id : %d\n", str.signal_map_id);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "type          : %c\n", str.type);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "source_alias  : %s\n", str.source_alias);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "description   : %s\n", str.description);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "signal_class  : %s\n", str.signal_class);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "signal_owner  : %s\n", str.signal_owner);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "modified      : %s\n", str.modified);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "creation      : %s\n", str.creation);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "meta_id       : %d\n", str.meta_id);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "xml           : %s\n", str.xml);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "xml_creation  : %s\n", str.xml_creation);
}

#ifndef UDA_COMMON_BUILD

// Legacy forms: print through this library's own logger

void printRequestData(REQUEST_DATA str)
{
    UDA_DIAGNOSTICS diag = udaGlobalDiagnostics();
    udaPrintRequestData(&diag, str);
}

void printRequestBlock(REQUEST_BLOCK str)
{
    UDA_DIAGNOSTICS diag = udaGlobalDiagnostics();
    udaPrintRequestBlock(&diag, str);
}

void printClientBlock(CLIENT_BLOCK str)
{
    UDA_DIAGNOSTICS diag = udaGlobalDiagnostics();
    udaPrintClientBlock(&diag, str);
}

void printServerBlock(SERVER_BLOCK str)
{
    UDA_DIAGNOSTICS diag = udaGlobalDiagnostics();
    udaPrintServerBlock(&diag, str);
    printIdamErrorStack();
}

void printDataBlockList(DATA_BLOCK_LIST str)
{
    UDA_DIAGNOSTICS diag = udaGlobalDiagnostics();
    udaPrintDataBlockList(&diag, str);
}

void printDataBlock(DATA_BLOCK str)
{
    UDA_DIAGNOSTICS diag = udaGlobalDiagnostics();
    udaPrintDataBlock(&diag, str);
}

void printSystemConfig(SYSTEM_CONFIG str)
{
    UDA_DIAGNOSTICS diag = udaGlobalDiagnostics();
    udaPrintSystemConfig(&diag, str);
}

void printDataSystem(DATA_SYSTEM str)
{
    UDA_DIAGNOSTICS diag = udaGlobalDiagnostics();
    udaPrintDataSystem(&diag, str);
}

void printDataSource(DATA_SOURCE str)
{
    UDA_DIAGNOSTICS diag = udaGlobalDiagnostics();
    udaPrintDataSource(&diag, str);
}

void printSignal(SIGNAL str)
{
    UDA_DIAGNOSTICS diag = udaGlobalDiagnostics();
    udaPrintSignal(&diag, str);
}

void printSignalDesc(SIGNAL_DESC str)
{
    UDA_DIAGNOSTICS diag = udaGlobalDiagnostics();
    udaPrintSignalDesc(&diag, str);
}

#endif // UDA_COMMON_BUILD
