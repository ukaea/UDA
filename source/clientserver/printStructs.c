// Print the Contents of the database Record Structures
//
//----------------------------------------------------------------------------------

#include "printStructs.h"

#include <logging/logging.h>
#include <clientserver/udaTypes.h>
#include "errorLog.h"

void printRequestBlock(REQUEST_BLOCK str)
{
    int i;
    IDAM_LOG(LOG_DEBUG, "Client Request Block\n");
    IDAM_LOG(LOG_DEBUG, "--------------------------------------------------------------------------------\n");
    IDAM_LOGF(LOG_DEBUG, "request     : %d\n", str.request);
    IDAM_LOGF(LOG_DEBUG, "exp_number  : %d\n", str.exp_number);
    IDAM_LOGF(LOG_DEBUG, "pass        : %d\n", str.pass);
    IDAM_LOGF(LOG_DEBUG, "tpass       : %s\n", str.tpass);
    IDAM_LOGF(LOG_DEBUG, "path        : %s\n", str.path);
    IDAM_LOGF(LOG_DEBUG, "file        : %s\n", str.file);
    IDAM_LOGF(LOG_DEBUG, "format      : %s\n", str.format);
    IDAM_LOGF(LOG_DEBUG, "archive     : %s\n", str.archive);
    IDAM_LOGF(LOG_DEBUG, "device_name : %s\n", str.device_name);
    IDAM_LOGF(LOG_DEBUG, "server      : %s\n", str.server);
    IDAM_LOGF(LOG_DEBUG, "function    : %s\n", str.function);
    IDAM_LOGF(LOG_DEBUG, "signal      : %s\n", str.signal);
    IDAM_LOGF(LOG_DEBUG, "source      : %s\n", str.source);
    IDAM_LOGF(LOG_DEBUG, "api_delim   : %s\n", str.api_delim);
    IDAM_LOGF(LOG_DEBUG, "subset      : %s\n", str.subset);
    IDAM_LOGF(LOG_DEBUG, "subsetCount : %d\n", str.datasubset.subsetCount);
    for (i = 0; i < str.datasubset.subsetCount; i++) {
        IDAM_LOGF(LOG_DEBUG, "[%d] %d   %d   %d   %d   %d\n", i, str.datasubset.subset[i], str.datasubset.start[i],
                str.datasubset.stop[i], str.datasubset.count[i], str.datasubset.stride[i]);
    }
    IDAM_LOGF(LOG_DEBUG, "nameValueCount : %d\n", str.nameValueList.pairCount);
    for (i = 0; i < str.nameValueList.pairCount; i++) {
        IDAM_LOGF(LOG_DEBUG, "[%d] %s,   %s,   %s\n", i, str.nameValueList.nameValue[i].pair,
                str.nameValueList.nameValue[i].name, str.nameValueList.nameValue[i].value);
    }
    IDAM_LOG(LOG_DEBUG, "--------------------------------------------------------------------------------\n");
}

#ifdef SECURITYENABLED
static void printUCharArray(const char* name, unsigned char* arr, size_t arr_len, size_t max_len)
{
    IDAM_LOGF(LOG_DEBUG, "%s :  [ ", name);
    size_t max = max_len < arr_len ? max_len : arr_len;
    int i;
    for (i = 0; i < max; ++i) {
        if (i == 0) {
            idamLog(LOG_DEBUG, "%x", arr[i]);
        } else {
            idamLog(LOG_DEBUG, ", %x", arr[i]);
        }
    }
    if (arr_len > 0 && max_len < arr_len) {
        idamLog(LOG_DEBUG, ", ...");
    }
    idamLog(LOG_DEBUG, " ]\n");
}
#endif

void printClientBlock(CLIENT_BLOCK str)
{
    IDAM_LOG(LOG_DEBUG, "Client State Block\n");
    IDAM_LOG(LOG_DEBUG, "--------------------------------------------------------------------------------\n");
    IDAM_LOGF(LOG_DEBUG, "version      : %d\n", str.version);
    IDAM_LOGF(LOG_DEBUG, "pid          : %d\n", str.pid);
    IDAM_LOGF(LOG_DEBUG, "uid          : %s\n", str.uid);

    IDAM_LOGF(LOG_DEBUG, "timeout      : %d\n", str.timeout);
    IDAM_LOGF(LOG_DEBUG, "compressDim  : %d\n", str.compressDim);

    IDAM_LOGF(LOG_DEBUG, "clientFlags  : %d\n", str.clientFlags);
    IDAM_LOGF(LOG_DEBUG, "altRank      : %d\n", str.altRank);

    IDAM_LOGF(LOG_DEBUG, "get_nodimdata: %d\n", str.get_nodimdata);
    IDAM_LOGF(LOG_DEBUG, "get_datadble : %d\n", str.get_datadble);
    IDAM_LOGF(LOG_DEBUG, "get_timedble : %d\n", str.get_timedble);
    IDAM_LOGF(LOG_DEBUG, "get_dimdble  : %d\n", str.get_dimdble);
    IDAM_LOGF(LOG_DEBUG, "get_bad      : %d\n", str.get_bad);
    IDAM_LOGF(LOG_DEBUG, "get_meta     : %d\n", str.get_meta);
    IDAM_LOGF(LOG_DEBUG, "get_asis     : %d\n", str.get_asis);
    IDAM_LOGF(LOG_DEBUG, "get_uncal    : %d\n", str.get_uncal);
    IDAM_LOGF(LOG_DEBUG, "get_notoff   : %d\n", str.get_notoff);
    IDAM_LOGF(LOG_DEBUG, "get_scalar   : %d\n", str.get_scalar);
    IDAM_LOGF(LOG_DEBUG, "get_bytes    : %d\n", str.get_bytes);

    IDAM_LOGF(LOG_DEBUG, "privateFlags : %d\n", str.privateFlags);

    IDAM_LOGF(LOG_DEBUG, "OS Name      : %s\n", str.OSName);
    IDAM_LOGF(LOG_DEBUG, "Study DOI    : %s\n", str.DOI);
#ifdef SECURITYENABLED
    IDAM_LOGF(LOG_DEBUG, "uid2         : %s\n", str.uid2);
    IDAM_LOG(LOG_DEBUG, "Client State Security Block\n");
    IDAM_LOG(LOG_DEBUG, "--------------------------------------------------------------------------------\n");
    IDAM_LOGF(LOG_DEBUG, "encryptionMethod          : %d\n", str.securityBlock.encryptionMethod);
    IDAM_LOGF(LOG_DEBUG, "authenticationStep        : %d\n", str.securityBlock.authenticationStep);
    IDAM_LOGF(LOG_DEBUG, "client_ciphertextLength   : %d\n", str.securityBlock.client_ciphertextLength);
    printUCharArray("client_ciphertext", str.securityBlock.client_ciphertext, str.securityBlock.client_ciphertextLength, 10);
    IDAM_LOGF(LOG_DEBUG, "server_ciphertextLength   : %d\n", str.securityBlock.server_ciphertextLength);
    printUCharArray("server_ciphertext", str.securityBlock.server_ciphertext, str.securityBlock.server_ciphertextLength, 10);
    IDAM_LOGF(LOG_DEBUG, "client_X509Length         : %d\n", str.securityBlock.client_X509Length);
    printUCharArray("client_X509", str.securityBlock.client_X509, str.securityBlock.client_X509Length, 10);
#endif
    IDAM_LOG(LOG_DEBUG, "--------------------------------------------------------------------------------\n");
}


void printServerBlock(SERVER_BLOCK str)
{
    IDAM_LOG(LOG_DEBUG, "Server State Block\n");
    IDAM_LOG(LOG_DEBUG, "--------------------------------------------------------------------------------\n");
    IDAM_LOGF(LOG_DEBUG, "version          : %d\n", str.version);
    IDAM_LOGF(LOG_DEBUG, "error            : %d\n", str.error);
    IDAM_LOGF(LOG_DEBUG, "msg              : %s\n", str.msg);
    IDAM_LOGF(LOG_DEBUG, "Server PID       : %d\n", str.pid);
    IDAM_LOGF(LOG_DEBUG, "OS Name          : %s\n", str.OSName);
    IDAM_LOGF(LOG_DEBUG, "Configuration DOI: %s\n", str.DOI);
#ifdef SECURITYENABLED
    IDAM_LOG(LOG_DEBUG, "Server State Security Block\n");
    IDAM_LOG(LOG_DEBUG, "--------------------------------------------------------------------------------\n");
    IDAM_LOGF(LOG_DEBUG, "encryptionMethod          : %d\n", str.securityBlock.encryptionMethod);
    IDAM_LOGF(LOG_DEBUG, "authenticationStep        : %d\n", str.securityBlock.authenticationStep);
    IDAM_LOGF(LOG_DEBUG, "client_ciphertextLength   : %d\n", str.securityBlock.client_ciphertextLength);
    printUCharArray("client_ciphertext", str.securityBlock.client_ciphertext, str.securityBlock.client_ciphertextLength, 10);
    IDAM_LOGF(LOG_DEBUG, "server_ciphertextLength   : %d\n", str.securityBlock.server_ciphertextLength);
    printUCharArray("server_ciphertext", str.securityBlock.server_ciphertext, str.securityBlock.server_ciphertextLength, 10);
    IDAM_LOGF(LOG_DEBUG, "client_X509Length         : %d\n", str.securityBlock.client_X509Length);
    printUCharArray("client_X509", str.securityBlock.client_X509, str.securityBlock.client_X509Length, 10);
#endif
    IDAM_LOG(LOG_DEBUG, "Server State Error Stack\n");
    IDAM_LOG(LOG_DEBUG, "--------------------------------------------------------------------------------\n");
    printIdamErrorStack(str.idamerrorstack);
    IDAM_LOG(LOG_DEBUG, "--------------------------------------------------------------------------------\n");
}


void printDataBlock(DATA_BLOCK str)
{
    int i, j, k;
    IDAM_LOG(LOG_DEBUG, "Data Block Contents\n");
    IDAM_LOG(LOG_DEBUG, "--------------------------------------------------------------------------------\n");
    IDAM_LOGF(LOG_DEBUG, "handle       : %d\n", str.handle);
    IDAM_LOGF(LOG_DEBUG, "error code   : %d\n", str.errcode);
    IDAM_LOGF(LOG_DEBUG, "error msg    : %s\n", str.error_msg);
    IDAM_LOGF(LOG_DEBUG, "source status: %d\n", str.source_status);
    IDAM_LOGF(LOG_DEBUG, "signal status: %d\n", str.signal_status);
    IDAM_LOGF(LOG_DEBUG, "data_number  : %d\n", str.data_n);
    IDAM_LOGF(LOG_DEBUG, "rank         : %d\n", str.rank);
    IDAM_LOGF(LOG_DEBUG, "order        : %d\n", str.order);
    IDAM_LOGF(LOG_DEBUG, "data_type    : %d\n", str.data_type);
    IDAM_LOGF(LOG_DEBUG, "error_type   : %d\n", str.error_type);
    IDAM_LOGF(LOG_DEBUG, "errhi != NULL: %d\n", str.errhi != NULL);
    IDAM_LOGF(LOG_DEBUG, "errlo != NULL: %d\n", str.errlo != NULL);

    IDAM_LOGF(LOG_DEBUG, "opaque_type : %d\n", str.opaque_type);
    IDAM_LOGF(LOG_DEBUG, "opaque_count: %d\n", str.opaque_count);

    switch (str.opaque_type) {
        case (OPAQUE_TYPE_XML_DOCUMENT):
            if (str.opaque_block != NULL) IDAM_LOGF(LOG_DEBUG, "\nXML: %s\n\n", (char*) str.opaque_block);
            break;
        default:
            break;
    }

    k = 10;
    if (str.data_n < 10) k = str.data_n;

    if (str.data_type == TYPE_FLOAT) {
        for (j = 0; j < k; j++)IDAM_LOGF(LOG_DEBUG, "data[%d]: %f\n", j, *((float*) str.data + j));
    }
    if (str.data_type == TYPE_DOUBLE) {
        for (j = 0; j < k; j++)IDAM_LOGF(LOG_DEBUG, "data[%d]: %f\n", j, *((double*) str.data + j));
    }

    if (str.error_type == TYPE_FLOAT && str.errhi != NULL) {
        for (j = 0; j < k; j++)IDAM_LOGF(LOG_DEBUG, "errhi[%d]: %f\n", j, *((float*) str.errhi + j));
    }

    if (str.error_type == TYPE_FLOAT && str.errlo != NULL && str.errasymmetry) {
        for (j = 0; j < k; j++)IDAM_LOGF(LOG_DEBUG, "errlo[%d]: %f\n", j, *((float*) str.errlo + j));
    }

    IDAM_LOGF(LOG_DEBUG, "error model : %d\n", str.error_model);
    IDAM_LOGF(LOG_DEBUG, "asymmetry   : %d\n", str.errasymmetry);
    IDAM_LOGF(LOG_DEBUG, "error model no. params : %d\n", str.error_param_n);
    for (i = 0; i < str.error_param_n; i++)IDAM_LOGF(LOG_DEBUG, "param[%d] = %f \n", i, str.errparams[i]);
    IDAM_LOGF(LOG_DEBUG, "data_units  : %s\n", str.data_units);
    IDAM_LOGF(LOG_DEBUG, "data_label  : %s\n", str.data_label);
    IDAM_LOGF(LOG_DEBUG, "data_desc   : %s\n", str.data_desc);
    for (i = 0; i < str.rank; i++) {
        IDAM_LOGF(LOG_DEBUG, "Dimension #%d Contents\n", i);
        IDAM_LOGF(LOG_DEBUG, "  data_type    : %d\n", str.dims[i].data_type);
        IDAM_LOGF(LOG_DEBUG, "  error_type   : %d\n", str.dims[i].error_type);
        IDAM_LOGF(LOG_DEBUG, "  errhi != NULL: %d\n", str.dims[i].errhi != NULL);
        IDAM_LOGF(LOG_DEBUG, "  errlo != NULL: %d\n", str.dims[i].errlo != NULL);
        IDAM_LOGF(LOG_DEBUG, "  error model  : %d\n", str.dims[i].error_model);
        IDAM_LOGF(LOG_DEBUG, "  asymmetry    : %d\n", str.dims[i].errasymmetry);
        IDAM_LOGF(LOG_DEBUG, "  error model no. params : %d\n", str.dims[i].error_param_n);
        for (j = 0; j < str.dims[i].error_param_n; j++)IDAM_LOGF(LOG_DEBUG, "param[%d] = %f \n", j, str.dims[i].errparams[j]);
        IDAM_LOGF(LOG_DEBUG, "  data_number : %d\n", str.dims[i].dim_n);
        IDAM_LOGF(LOG_DEBUG, "  compressed? : %d\n", str.dims[i].compressed);
        IDAM_LOGF(LOG_DEBUG, "  method      : %d\n", str.dims[i].method);
        if (str.dims[i].method == 0) {
            if (str.dims[i].compressed) {
                IDAM_LOGF(LOG_DEBUG, "  starting val: %f\n", str.dims[i].dim0);
                IDAM_LOGF(LOG_DEBUG, "  stepping val: %f\n", str.dims[i].diff);
            } else {
                if (str.dims[i].data_type == TYPE_FLOAT) {
                    k = 10;
                    if (str.dims[i].dim_n < 10) k = str.dims[i].dim_n;
                    if (str.dims[i].dim != NULL)
                        for (j = 0; j < k; j++)
                            IDAM_LOGF(LOG_DEBUG, "  val[%d] = %f\n", j, *((float*) str.dims[i].dim + j));
                }
                if (str.dims[i].data_type == TYPE_DOUBLE) {
                    k = 10;
                    if (str.dims[i].dim_n < 10) k = str.dims[i].dim_n;
                    if (str.dims[i].dim != NULL)
                        for (j = 0; j < k; j++)
                            IDAM_LOGF(LOG_DEBUG, "v  al[%d] = %f\n", j, *((double*) str.dims[i].dim + j));
                }
            }
        } else {
            IDAM_LOGF(LOG_DEBUG, "  udoms: %d\n", str.dims[i].udoms);
            switch (str.dims[i].method) {
                case 1:
                    if (str.dims[i].data_type == TYPE_FLOAT) {
                        k = 10;
                        if (str.dims[i].udoms < 10) k = str.dims[i].udoms;
                        for (j = 0; j < k; j++) {
                            IDAM_LOGF(LOG_DEBUG, "  sams[%d]: %d\n", j, (int) *(str.dims[i].sams + j));
                            IDAM_LOGF(LOG_DEBUG, "  offs[%d]: %f\n", j, *((float*) str.dims[i].offs + j));
                            IDAM_LOGF(LOG_DEBUG, "  ints[%d]: %f\n", j, *((float*) str.dims[i].ints + j));
                        }
                    }
                    if (str.dims[i].data_type == TYPE_DOUBLE) {
                        k = 10;
                        if (str.dims[i].udoms < 10) k = str.dims[i].udoms;
                        for (j = 0; j < k; j++) {
                            IDAM_LOGF(LOG_DEBUG, "  sams[%d]: %d\n", j, (int) *(str.dims[i].sams + j));
                            IDAM_LOGF(LOG_DEBUG, "  offs[%d]: %f\n", j, *((double*) str.dims[i].offs + j));
                            IDAM_LOGF(LOG_DEBUG, "  ints[%d]: %f\n", j, *((double*) str.dims[i].ints + j));
                        }
                    }
                    break;
                case 2:
                    if (str.dims[i].data_type == TYPE_FLOAT) {
                        k = 10;
                        if (str.dims[i].udoms < 10) k = str.dims[i].udoms;
                        for (j = 0; j < k; j++) IDAM_LOGF(LOG_DEBUG, "offs[%d]: %f\n", j, *((float*) str.dims[i].offs + j));
                    }
                    if (str.dims[i].data_type == TYPE_DOUBLE) {
                        k = 10;
                        if (str.dims[i].udoms < 10) k = str.dims[i].udoms;
                        for (j = 0; j < k; j++) IDAM_LOGF(LOG_DEBUG, "offs[%d]: %f\n", j, *((double*) str.dims[i].offs + j));
                    }
                    break;
                case 3:
                    if (str.dims[i].data_type == TYPE_FLOAT) {
                        IDAM_LOGF(LOG_DEBUG, "  offs[0] val: %f\n", *((float*) str.dims[i].offs));
                        IDAM_LOGF(LOG_DEBUG, "  ints[0] val: %f\n", *((float*) str.dims[i].ints));
                    }
                    if (str.dims[i].data_type == TYPE_DOUBLE) {
                        IDAM_LOGF(LOG_DEBUG, "  offs[0] val: %f\n", *((double*) str.dims[i].offs));
                        IDAM_LOGF(LOG_DEBUG, "  ints[0] val: %f\n", *((double*) str.dims[i].ints));
                    }
                    break;
                default:
                    IDAM_LOGF(LOG_WARN, "  unknown method (%d) for dim (%d)", str.dims[i].method, i);
            }
        }
        if (str.dims[i].error_type == TYPE_FLOAT) {
            k = 10;
            if (str.dims[i].dim_n < 10) k = str.dims[i].dim_n;
            if (str.dims[i].errhi != NULL)
                for (j = 0; j < k; j++)
                    IDAM_LOGF(LOG_DEBUG, "  errhi[%d] = %f\n", j, *((float*) str.dims[i].errhi + j));
            if (str.dims[i].errlo != NULL && str.dims[i].errasymmetry)
                for (j = 0; j < k; j++)
                    IDAM_LOGF(LOG_DEBUG, "  errlo[%d] = %f\n", j, *((float*) str.dims[i].errlo + j));
        }
        IDAM_LOGF(LOG_DEBUG, "  data_units  : %s\n", str.dims[i].dim_units);
        IDAM_LOGF(LOG_DEBUG, "  data_label  : %s\n", str.dims[i].dim_label);
    }
    IDAM_LOG(LOG_DEBUG, "--------------------------------------------------------------------------------\n");
}


void printSystemConfig(SYSTEM_CONFIG str)
{
    IDAM_LOG(LOG_DEBUG, "System Configuration Record\n");
    IDAM_LOG(LOG_DEBUG, "--------------------------------------------------------------------------------\n");
    IDAM_LOGF(LOG_DEBUG, "config_id   : %d\n", str.config_id);
    IDAM_LOGF(LOG_DEBUG, "system_id   : %d\n", str.system_id);
    IDAM_LOGF(LOG_DEBUG, "config_name : %s\n", str.config_name);
    IDAM_LOGF(LOG_DEBUG, "config_desc : %s\n", str.config_desc);
    IDAM_LOGF(LOG_DEBUG, "creation    : %s\n", str.creation);
    IDAM_LOGF(LOG_DEBUG, "meta_id     : %d\n", str.meta_id);
    IDAM_LOGF(LOG_DEBUG, "xml         : %s\n", str.xml);
    IDAM_LOGF(LOG_DEBUG, "xml_creation: %s\n", str.xml_creation);
    IDAM_LOG(LOG_DEBUG, "--------------------------------------------------------------------------------\n");
}


void printDataSystem(DATA_SYSTEM str)
{
    IDAM_LOG(LOG_DEBUG, "Data System Record\n");
    IDAM_LOG(LOG_DEBUG, "--------------------------------------------------------------------------------\n");
    IDAM_LOGF(LOG_DEBUG, "system_id   : %d\n", str.system_id);
    IDAM_LOGF(LOG_DEBUG, "version     : %d\n", str.version);
    IDAM_LOGF(LOG_DEBUG, "type        : %c\n", str.type);
    IDAM_LOGF(LOG_DEBUG, "device_name : %s\n", str.device_name);
    IDAM_LOGF(LOG_DEBUG, "system_name : %s\n", str.system_name);
    IDAM_LOGF(LOG_DEBUG, "system_desc : %s\n", str.system_desc);
    IDAM_LOGF(LOG_DEBUG, "creation    : %s\n", str.creation);
    IDAM_LOGF(LOG_DEBUG, "meta_id     : %d\n", str.meta_id);
    IDAM_LOGF(LOG_DEBUG, "xml         : %s\n", str.xml);
    IDAM_LOGF(LOG_DEBUG, "xml_creation: %s\n", str.xml_creation);
    IDAM_LOG(LOG_DEBUG, "--------------------------------------------------------------------------------\n");
}

void printDataSource(DATA_SOURCE str)
{
    IDAM_LOG(LOG_DEBUG, "Data Source Record\n");
    IDAM_LOG(LOG_DEBUG, "--------------------------------------------------------------------------------\n");
    IDAM_LOGF(LOG_DEBUG, "source_id     : %d\n", str.source_id);
    IDAM_LOGF(LOG_DEBUG, "config_id     : %d\n", str.config_id);
    IDAM_LOGF(LOG_DEBUG, "reason_id     : %d\n", str.reason_id);
    IDAM_LOGF(LOG_DEBUG, "run_id        : %d\n", str.run_id);
    IDAM_LOGF(LOG_DEBUG, "status_desc_id: %d\n", str.status_desc_id);
    IDAM_LOGF(LOG_DEBUG, "exp_number    : %d\n", str.exp_number);
    IDAM_LOGF(LOG_DEBUG, "pass          : %d\n", str.pass);
    IDAM_LOGF(LOG_DEBUG, "status        : %d\n", str.status);
    IDAM_LOGF(LOG_DEBUG, "status_reason_code: %d\n", str.status_reason_code);
    IDAM_LOGF(LOG_DEBUG, "status_impact_code: %d\n", str.status_impact_code);
    IDAM_LOGF(LOG_DEBUG, "access        : %c\n", str.access);
    IDAM_LOGF(LOG_DEBUG, "reprocess     : %c\n", str.reprocess);
    IDAM_LOGF(LOG_DEBUG, "type          : %c\n", str.type);
    IDAM_LOGF(LOG_DEBUG, "source_alias  : %s\n", str.source_alias);
    IDAM_LOGF(LOG_DEBUG, "pass_date     : %s\n", str.pass_date);
    IDAM_LOGF(LOG_DEBUG, "archive       : %s\n", str.archive);
    IDAM_LOGF(LOG_DEBUG, "device_name   : %s\n", str.device_name);
    IDAM_LOGF(LOG_DEBUG, "format        : %s\n", str.format);
    IDAM_LOGF(LOG_DEBUG, "path          : %s\n", str.path);
    IDAM_LOGF(LOG_DEBUG, "filename      : %s\n", str.filename);
    IDAM_LOGF(LOG_DEBUG, "server        : %s\n", str.server);
    IDAM_LOGF(LOG_DEBUG, "userid        : %s\n", str.userid);
    IDAM_LOGF(LOG_DEBUG, "reason_desc   : %s\n", str.reason_desc);
    IDAM_LOGF(LOG_DEBUG, "run_desc      : %s\n", str.run_desc);
    IDAM_LOGF(LOG_DEBUG, "status_desc   : %s\n", str.status_desc);
    IDAM_LOGF(LOG_DEBUG, "creation      : %s\n", str.creation);
    IDAM_LOGF(LOG_DEBUG, "modified      : %s\n", str.modified);
    IDAM_LOGF(LOG_DEBUG, "meta_id       : %d\n", str.meta_id);
    IDAM_LOGF(LOG_DEBUG, "xml           : %s\n", str.xml);
    IDAM_LOGF(LOG_DEBUG, "xml_creation  : %s\n", str.xml_creation);
    IDAM_LOG(LOG_DEBUG, "--------------------------------------------------------------------------------\n");
}

void printSignal(SIGNAL str)
{
    IDAM_LOG(LOG_DEBUG, "Signal Record\n");
    IDAM_LOG(LOG_DEBUG, "--------------------------------------------------------------------------------\n");
    IDAM_LOGF(LOG_DEBUG, "source_id         : %d\n", str.source_id);
    IDAM_LOGF(LOG_DEBUG, "signal_desc_id    : %d\n", str.signal_desc_id);
    IDAM_LOGF(LOG_DEBUG, "status_desc_id    : %d\n", str.status_desc_id);
    IDAM_LOGF(LOG_DEBUG, "status            : %d\n", str.status);
    IDAM_LOGF(LOG_DEBUG, "status_reason_code: %d\n", str.status_reason_code);
    IDAM_LOGF(LOG_DEBUG, "status_impact_code: %d\n", str.status_impact_code);
    IDAM_LOGF(LOG_DEBUG, "status_desc       : %s\n", str.status_desc);
    IDAM_LOGF(LOG_DEBUG, "access            : %c\n", str.access);
    IDAM_LOGF(LOG_DEBUG, "reprocess         : %c\n", str.reprocess);
    IDAM_LOGF(LOG_DEBUG, "creation          : %s\n", str.creation);
    IDAM_LOGF(LOG_DEBUG, "modified          : %s\n", str.modified);
    IDAM_LOGF(LOG_DEBUG, "meta_id           : %d\n", str.meta_id);
    IDAM_LOGF(LOG_DEBUG, "xml               : %s\n", str.xml);
    IDAM_LOGF(LOG_DEBUG, "xml_creation      : %s\n", str.xml_creation);
    IDAM_LOG(LOG_DEBUG, "--------------------------------------------------------------------------------\n");
}

void printSignalDesc(SIGNAL_DESC str)
{
    IDAM_LOG(LOG_DEBUG, "Signal Description Record\n");
    IDAM_LOG(LOG_DEBUG, "--------------------------------------------------------------------------------\n");
    IDAM_LOGF(LOG_DEBUG, "signal_desc_id: %d\n", str.signal_desc_id);
    IDAM_LOGF(LOG_DEBUG, "signal_alias  : %s\n", str.signal_alias);
    IDAM_LOGF(LOG_DEBUG, "signal_name   : %s\n", str.signal_name);
    IDAM_LOGF(LOG_DEBUG, "generic_name  : %s\n", str.generic_name);
    IDAM_LOGF(LOG_DEBUG, "rank          : %d\n", str.rank);
    IDAM_LOGF(LOG_DEBUG, "range_start   : %d\n", str.range_start);
    IDAM_LOGF(LOG_DEBUG, "range_stop    : %d\n", str.range_stop);
    IDAM_LOGF(LOG_DEBUG, "signal_alias_type: %d\n", str.signal_alias_type);
    IDAM_LOGF(LOG_DEBUG, "signal_map_id : %d\n", str.signal_map_id);
    IDAM_LOGF(LOG_DEBUG, "type          : %c\n", str.type);
    IDAM_LOGF(LOG_DEBUG, "source_alias  : %s\n", str.source_alias);
    IDAM_LOGF(LOG_DEBUG, "description   : %s\n", str.description);
    IDAM_LOGF(LOG_DEBUG, "signal_class  : %s\n", str.signal_class);
    IDAM_LOGF(LOG_DEBUG, "signal_owner  : %s\n", str.signal_owner);
    IDAM_LOGF(LOG_DEBUG, "modified      : %s\n", str.modified);
    IDAM_LOGF(LOG_DEBUG, "creation      : %s\n", str.creation);
    IDAM_LOGF(LOG_DEBUG, "meta_id       : %d\n", str.meta_id);
    IDAM_LOGF(LOG_DEBUG, "xml           : %s\n", str.xml);
    IDAM_LOGF(LOG_DEBUG, "xml_creation  : %s\n", str.xml_creation);
    IDAM_LOG(LOG_DEBUG, "--------------------------------------------------------------------------------\n");
}

void printPerformance(PERFORMANCE str)
{
    int i;
    double testtime;
    IDAM_LOG(LOG_DEBUG, "\n==================== Performance Report =================\n");
    for (i = 0; i < str.npoints; i++) {
        testtime = (float) (str.tv_end[i].tv_sec - str.tv_start[i].tv_sec) * 1.0E6 +
                   (float) (str.tv_end[i].tv_usec - str.tv_start[i].tv_usec);
        IDAM_LOGF(LOG_DEBUG, "%s %.2f (micro-secs)\n", str.label[i], (float) testtime);
    }
    IDAM_LOG(LOG_DEBUG, "=========================================================\n\n");
}

//-------------------------------------------------------------------------------------------------------------------

#ifdef IdamClientPublicInclude                // Client Side only

extern void printIdamRequestBlock(FILE *fh, REQUEST_BLOCK str) {
    printRequestBlock(fh, str);
}
extern void printIdamClientBlock(FILE *fh, CLIENT_BLOCK str) {
    printClientBlock(fh, str);
}
extern void printIdamServerBlock(FILE *fh, SERVER_BLOCK str) {
    printServerBlock(fh, str);
}
extern void printIdamDataBlock(FILE *fh, DATA_BLOCK str) {
    printDataBlock(fh, str);
}
extern void printIdamSystemConfig(FILE *fh, SYSTEM_CONFIG str) {
    printSystemConfig(fh, str);
}
extern void printIdamDataSystem(FILE *fh, DATA_SYSTEM str) {
    printDataSystem(fh, str);
}
extern void printIdamDataSource(FILE *fh, DATA_SOURCE str) {
    printDataSource(fh, str);
}
extern void printIdamSignal(FILE *fh, SIGNAL str) {
    printSignal(fh, str);
}
extern void printIdamSignalDesc(FILE *fh, SIGNAL_DESC str) {
    printSignalDesc(fh, str);
}
extern void printIdamPerformance(FILE *fh, PERFORMANCE str) {
    printPerformance(fh, str);
}

#endif

