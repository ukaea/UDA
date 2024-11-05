/*---------------------------------------------------------------------------------------------
 * Apply Actionable XML based Corrections to Data
 *
 * Returns:
 *
 * Notes: Actions defined within the Signal record have a Higher priority than
 *     Actions defined within the SignalDesc record.
 *
 *        Priorities are subject to exp_number and pass range checks
 *
 *     A specific time_offset correction occurs befor any time dimension rescaling.
 *
 *-----------------------------------------------------------------------------------------------*/

#include "apply_XML.hpp"

#include <cmath>
#include <memory.h>

#include "clientserver/udaDefines.h"
#include "common/stringUtils.h"
#include "logging/logging.h"

#include <uda/types.h>

using namespace uda::logging;
using namespace uda::client_server;

int uda::server_parse_signal_xml(uda::client_server::DataSource data_source, uda::client_server::Signal signal,
                                 uda::client_server::SignalDesc signal_desc, uda::client_server::Actions* actions_desc,
                                 uda::client_server::Actions* actions_sig)
{

    // return -1 if No Qualifying Actionable XML otherwise return 0

    int ndesc, rc = 0;

    UDA_LOG(UDA_LOG_DEBUG, "Parsing XML");

    //----------------------------------------------------------------------
    // Anything to Parse?

    if (strlen(signal.xml) == 0 && strlen(signal_desc.xml) == 0) {
        return -1; // No XML to parse or switched off!
    }

    //----------------------------------------------------------------------
    // Initialise

    init_actions(actions_desc); // Array of actions from the Signal_Desc record
    init_actions(actions_sig);  // Array of actions from the Signal Record

    //----------------------------------------------------------------------
    // Parse Signal XML

    if (strlen(signal.xml) > 0) { // If this Signal level XML exists then populated components takes priority.
        if ((rc = parse_doc(signal.xml, actions_sig)) != 0) {
            return 1;
        }
        UDA_LOG(UDA_LOG_DEBUG, "XML from the Signal Record parsed");
        print_actions(*actions_sig);
    }

    //----------------------------------------------------------------------
    // Parse Signal_Desc XML

    if (strlen(signal_desc.xml) > 0) {
        if ((rc = parse_doc(signal_desc.xml, actions_desc)) != 0) {
            return 1;
        }

        UDA_LOG(UDA_LOG_DEBUG, "XML from the Signal_Desc Record parsed");
        print_actions(*actions_desc);
    }

    //----------------------------------------------------------------------------------------------
    // Test Range Groups (Only Applies to XML from Signal_desc records)

    for (int i = 0; i < actions_sig->nactions; i++) {
        actions_sig->action[i].inRange = 1; // Always in Range
    }

    ndesc = 0;
    for (int i = 0; i < actions_desc->nactions; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "Range Test on Record {}", i);

        UDA_LOG(UDA_LOG_DEBUG, "#1 {}",
                (actions_desc->action[i].exp_range[0] == 0 ||
                 (actions_desc->action[i].exp_range[0] > 0 &&
                  actions_desc->action[i].exp_range[0] <= data_source.exp_number)));

        UDA_LOG(UDA_LOG_DEBUG, "#2 {}",
                (actions_desc->action[i].exp_range[1] == 0 ||
                 (actions_desc->action[i].exp_range[1] > 0 &&
                  actions_desc->action[i].exp_range[1] >= data_source.exp_number)));

        UDA_LOG(UDA_LOG_DEBUG, "#3 {}",
                (data_source.pass = -1 || ((actions_desc->action[i].pass_range[0] == -1 ||
                                            (actions_desc->action[i].pass_range[0] > -1 &&
                                             actions_desc->action[i].pass_range[0] <= data_source.pass)) &&
                                           (actions_desc->action[i].pass_range[1] == -1 ||
                                            (actions_desc->action[i].pass_range[1] > -1 &&
                                             actions_desc->action[i].pass_range[1] >= data_source.pass)))));

        if ((actions_desc->action[i].exp_range[0] == 0 ||
             (actions_desc->action[i].exp_range[0] > 0 &&
              actions_desc->action[i].exp_range[0] <= data_source.exp_number)) &&
            (actions_desc->action[i].exp_range[1] == 0 ||
             (actions_desc->action[i].exp_range[1] > 0 &&
              actions_desc->action[i].exp_range[1] >= data_source.exp_number)) &&
            (data_source.pass == -1 || ((actions_desc->action[i].pass_range[0] == -1 ||
                                         (actions_desc->action[i].pass_range[0] > -1 &&
                                          actions_desc->action[i].pass_range[0] <= data_source.pass)) &&
                                        (actions_desc->action[i].pass_range[1] == -1 ||
                                         (actions_desc->action[i].pass_range[1] > -1 &&
                                          actions_desc->action[i].pass_range[1] >= data_source.pass))))) {
            ndesc++;
            actions_desc->action[i].inRange = 1;
        } else {
            actions_desc->action[i].inRange = 0;
        }
    }

    print_actions(*actions_desc);

    if (actions_sig->nactions == 0 && ndesc == 0) { // No qualifying XML from either source
        UDA_LOG(UDA_LOG_DEBUG, "No Applicable Actionable XML Found");
        return -1;
    }

    return 0;
}

namespace
{

void applyCalibration(int type, int ndata, double factor, double offset, int invert, char* array)
{
    float* fp;
    double* dp;
    char* cp;
    short* sp;
    int* ip;
    long* lp;
    long long* llp;
    unsigned char* uc;
    unsigned short* us;
    unsigned int* up;
    unsigned long* ul;
    unsigned long long* ull;

    if (array == nullptr) {
        return; // No Data
    }
    if (factor == (double)1.0E0 && offset == (double)0.0E0 && !invert) {
        return; // Nothing to be applied
    }

    if (factor != (double)1.0E0 || offset != (double)0.0E0) {
        switch (type) {

            case UDA_TYPE_FLOAT:
                fp = (float*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (float)factor * fp[i] + (float)offset;
                }
                break;

            case UDA_TYPE_DOUBLE:
                dp = (double*)array;
                for (int i = 0; i < ndata; i++) {
                    dp[i] = factor * dp[i] + offset;
                }
                break;

            case UDA_TYPE_CHAR:
                cp = (char*)array;
                for (int i = 0; i < ndata; i++) {
                    cp[i] = (char)factor * cp[i] + (char)offset;
                }
                break;

            case UDA_TYPE_SHORT:
                sp = (short*)array;
                for (int i = 0; i < ndata; i++) {
                    sp[i] = (short)factor * sp[i] + (short)offset;
                }
                break;

            case UDA_TYPE_INT:
                ip = (int*)array;
                for (int i = 0; i < ndata; i++) {
                    ip[i] = (int)factor * ip[i] + (int)offset;
                }
                break;

            case UDA_TYPE_LONG:
                lp = (long*)array;
                for (int i = 0; i < ndata; i++) {
                    lp[i] = (long)factor * lp[i] + (long)offset;
                }
                break;

                // cause a bug: unresolved __fixunsdfdi  - need gcc_s library
            case UDA_TYPE_LONG64:
                llp = (long long*)array;
                for (int i = 0; i < ndata; i++) {
                    llp[i] = (long long)factor * llp[i] + (long long)offset;
                }
                break;

            case UDA_TYPE_UNSIGNED_CHAR:
                uc = (unsigned char*)array;
                for (int i = 0; i < ndata; i++) {
                    uc[i] = (unsigned char)factor * uc[i] + (unsigned char)offset;
                }
                break;

            case UDA_TYPE_UNSIGNED_SHORT:
                us = (unsigned short*)array;
                for (int i = 0; i < ndata; i++) {
                    us[i] = (unsigned short)factor * us[i] + (unsigned short)offset;
                }
                break;

            case UDA_TYPE_UNSIGNED_INT:
                up = (unsigned int*)array;
                for (int i = 0; i < ndata; i++) {
                    up[i] = (unsigned int)factor * up[i] + (unsigned int)offset;
                }
                break;

            case UDA_TYPE_UNSIGNED_LONG:
                ul = (unsigned long*)array;
                for (int i = 0; i < ndata; i++) {
                    ul[i] = (unsigned long)factor * ul[i] + (unsigned long)offset;
                }
                break;

            case UDA_TYPE_UNSIGNED_LONG64:
                ull = (unsigned long long*)array;
                for (int i = 0; i < ndata; i++) {
                    ull[i] = (unsigned long long)factor * ull[i] + (unsigned long long)offset;
                }
                break;

            default:
                break;
        }
    }

    // Invert floating point number: Integers would all be zero!

    if (invert) {
        switch (type) {

            case UDA_TYPE_FLOAT:
                fp = (float*)array;
                for (int i = 0; i < ndata; i++) {
                    if (fp[i] != 0.0) {
                        fp[i] = 1.0f / fp[i];
                    } else {
                        fp[i] = NAN;
                    }
                }
                break;

            case UDA_TYPE_DOUBLE:
                dp = (double*)array;
                for (int i = 0; i < ndata; i++) {
                    if (dp[i] != 0.0) {
                        dp[i] = 1.0 / dp[i];
                    } else {
                        dp[i] = NAN;
                    }
                }
                break;

            default:
                break;
        }
    }
}

} // namespace

void uda::server_apply_signal_xml(uda::client_server::ClientBlock client_block,
                                  uda::client_server::DataSource* data_source, uda::client_server::Signal* signal,
                                  uda::client_server::SignalDesc* signal_desc,
                                  uda::client_server::DataBlock* data_block, uda::client_server::Actions actions)
{

    int ndata, dimid;

    float* fp;
    float* fp0;
    double* dp;
    char* cp;
    short* sp;
    int* ip;
    long* lp;
    unsigned char* uc;
    unsigned short* us;
    unsigned int* up;
    unsigned long* ul;

    UDA_LOG(UDA_LOG_DEBUG, "Applying XML");

    if (client_block.get_asis) {
        return; // User specifies No Actions to be Applied
    }

    //----------------------------------------------------------------------------------------------
    // Loop over all actions

    for (int i = 0; i < actions.nactions; i++) {

        if (!actions.action[i].inRange) {
            continue; // Out of Pulse/Pass Range
        }

        switch ((ActionType)actions.action[i].actionType) {

                //----------------------------------------------------------------------------------------------
                // Documentation Changes: Label/Units Corrections

            case ActionType::Documentation:
                if (strlen(actions.action[i].documentation.label) > 0) {
                    strcpy(data_block->data_label, actions.action[i].documentation.label);
                }

                if (strlen(actions.action[i].documentation.units) > 0) {
                    strcpy(data_block->data_units, actions.action[i].documentation.units);
                }

                if (strlen(actions.action[i].documentation.description) > 0) {
                    strcpy(data_block->data_desc, actions.action[i].documentation.description);
                }

                for (int j = 0; j < actions.action[i].documentation.ndimensions; j++) {
                    if (actions.action[i].documentation.dimensions[j].dimType == (int)ActionDimType::Documentation) {
                        if (actions.action[i].documentation.dimensions[j].dimid > -1 &&
                            (unsigned int)actions.action[i].documentation.dimensions[j].dimid < data_block->rank) {
                            if (strlen(actions.action[i].documentation.dimensions[j].dimdocumentation.label) > 0) {
                                strcpy(data_block->dims[actions.action[i].documentation.dimensions[j].dimid].dim_label,
                                       actions.action[i].documentation.dimensions[j].dimdocumentation.label);
                            }
                            if (strlen(actions.action[i].documentation.dimensions[j].dimdocumentation.units) > 0) {
                                strcpy(data_block->dims[actions.action[i].documentation.dimensions[j].dimid].dim_units,
                                       actions.action[i].documentation.dimensions[j].dimdocumentation.units);
                            }
                        }
                    }
                }
                break;

                //----------------------------------------------------------------------------------------------
                // Error Models (Asymmetry is decided by the model's properties, not by the XML)

            case ActionType::ErrorModel:
                data_block->error_model = actions.action[i].errormodel.model;
                data_block->error_param_n = actions.action[i].errormodel.param_n;
                for (int j = 0; j < data_block->error_param_n; j++) {
                    data_block->errparams[j] = actions.action[i].errormodel.params[j];
                }

                for (int j = 0; j < actions.action[i].errormodel.ndimensions; j++) {
                    if (actions.action[i].errormodel.dimensions[j].dimType == (int)ActionDimType::ErrorModel) {
                        if (actions.action[i].errormodel.dimensions[j].dimid > -1 &&
                            (unsigned int)actions.action[i].errormodel.dimensions[j].dimid < data_block->rank) {
                            data_block->dims[actions.action[i].errormodel.dimensions[j].dimid].error_model =
                                actions.action[i].errormodel.dimensions[j].dimerrormodel.model;
                            data_block->dims[actions.action[i].errormodel.dimensions[j].dimid].error_param_n =
                                actions.action[i].errormodel.dimensions[j].dimerrormodel.param_n;
                            int jj;
                            for (jj = 0; jj < actions.action[i].errormodel.dimensions[j].dimerrormodel.param_n; jj++) {
                                data_block->dims[actions.action[i].errormodel.dimensions[j].dimid].errparams[jj] =
                                    actions.action[i].errormodel.dimensions[j].dimerrormodel.params[jj];
                            }
                        }
                    }
                }
                break;

                //----------------------------------------------------------------------------------------------
                // Timing Offset error corrections

                // Default compression method 0 = start value + multiple offsets

                // timeoffset.method == 0 => there must be a Non_Zero offset value
                // timeoffset.method == 1 => Create a New Time Vector using both Offset and Interval values.
                // timeoffset.method == 2 => Create a New Time Vector using an interval value only. Use the Original
                // Starting value if possible.

            case ActionType::Offset:

                if (client_block.get_notoff || data_block->order < 0) {
                    break;
                }

                // Create a New Time Vector

                if ((actions.action[i].timeoffset.method == 1 || actions.action[i].timeoffset.method == 2) &&
                    actions.action[i].timeoffset.interval != (double)0.0E0) {

                    // Correct the Interval Only not the Starting value: Change to the standard simple data compression
                    // model

                    if (actions.action[i].timeoffset.method == 2) {

                        if (data_block->dims[data_block->order].method == 1 &&
                            data_block->dims[data_block->order].udoms == 1 &&
                            data_block->dims[data_block->order].data_type == UDA_TYPE_FLOAT) {
                            float* offs = (float*)data_block->dims[data_block->order].offs;
                            data_block->dims[data_block->order].method = 0;
                            data_block->dims[data_block->order].dim0 = (double)*offs;
                            data_block->dims[data_block->order].diff = actions.action[i].timeoffset.interval;

                            if (data_block->dims[data_block->order].compressed) { // Free Heap
                                void* cptr = nullptr;
                                if ((cptr = (void*)data_block->dims[data_block->order].sams) != nullptr) {
                                    free(cptr);
                                }
                                if ((cptr = (void*)data_block->dims[data_block->order].offs) != nullptr) {
                                    free(cptr);
                                }
                                if ((cptr = (void*)data_block->dims[data_block->order].ints) != nullptr) {
                                    free(cptr);
                                }
                                data_block->dims[data_block->order].sams = nullptr;
                                data_block->dims[data_block->order].offs = nullptr;
                                data_block->dims[data_block->order].ints = nullptr;
                            } else {
                                if (data_block->dims[data_block->order].dim != nullptr) {
                                    free(data_block->dims[data_block->order].dim);
                                }
                                data_block->dims[data_block->order].dim = nullptr;
                            }

                            data_block->dims[data_block->order].compressed = 1;
                        }

                        break;
                    }

                    // Correct both the Starting and Interval values: Change to the standard simple data compression
                    // model

                    if (actions.action[i].timeoffset.method == 1) {

                        if (data_block->dims[data_block->order].compressed) { // Free Heap with erroneous data
                            void* cptr = nullptr;
                            if ((cptr = (void*)data_block->dims[data_block->order].sams) != nullptr) {
                                free(cptr);
                            }
                            if ((cptr = (void*)data_block->dims[data_block->order].offs) != nullptr) {
                                free(cptr);
                            }
                            if ((cptr = (void*)data_block->dims[data_block->order].ints) != nullptr) {
                                free(cptr);
                            }
                            data_block->dims[data_block->order].sams = nullptr;
                            data_block->dims[data_block->order].offs = nullptr;
                            data_block->dims[data_block->order].ints = nullptr;
                        } else {
                            if (data_block->dims[data_block->order].dim != nullptr) {
                                free(data_block->dims[data_block->order].dim);
                            }
                            data_block->dims[data_block->order].dim = nullptr;
                        }

                        data_block->dims[data_block->order].method = 0;
                        data_block->dims[data_block->order].compressed = 1;
                        data_block->dims[data_block->order].dim0 = actions.action[i].timeoffset.offset;
                        data_block->dims[data_block->order].diff = actions.action[i].timeoffset.interval;

                        break;
                    }

                    break;
                }

                // Standard Offset Correction Only

                if (actions.action[i].timeoffset.method == 0 && actions.action[i].timeoffset.offset != (double)0.0E0) {

                    if (data_block->dims[data_block->order].compressed) {

                        UDA_LOG(UDA_LOG_DEBUG, "Time Dimension Compressed");
                        UDA_LOG(UDA_LOG_DEBUG, "Order           = {}", data_block->order);
                        UDA_LOG(UDA_LOG_DEBUG, "Timing Offset   = {}", (float)actions.action[i].timeoffset.offset);
                        UDA_LOG(UDA_LOG_DEBUG, "Method          = {}", data_block->dims[data_block->order].method);

                        switch (data_block->dims[data_block->order].method) {

                            case 0:
                                // ***
                                data_block->dims[data_block->order].dim0 =
                                    data_block->dims[data_block->order].dim0 + actions.action[i].timeoffset.offset;
                                break;

                            case 1:
                                switch (data_block->dims[data_block->order].data_type) {
                                    unsigned int ii;
                                    case UDA_TYPE_FLOAT:
                                        fp = (float*)data_block->dims[data_block->order].offs;
                                        for (ii = 0; ii < data_block->dims[data_block->order].udoms; ii++) {
                                            fp[ii] = fp[ii] + (float)actions.action[i].timeoffset.offset;
                                        }
                                        break;
                                    case UDA_TYPE_DOUBLE:
                                        dp = (double*)data_block->dims[data_block->order].offs;
                                        for (ii = 0; ii < data_block->dims[data_block->order].udoms; ii++) {
                                            dp[ii] = dp[ii] + actions.action[i].timeoffset.offset;
                                        }
                                        break;
                                    case UDA_TYPE_CHAR:
                                        cp = data_block->dims[data_block->order].offs;
                                        for (ii = 0; ii < data_block->dims[data_block->order].udoms; ii++) {
                                            cp[ii] = cp[ii] + (char)actions.action[i].timeoffset.offset;
                                        }
                                        break;
                                    case UDA_TYPE_SHORT:
                                        sp = (short*)data_block->dims[data_block->order].offs;
                                        for (ii = 0; ii < data_block->dims[data_block->order].udoms; ii++) {
                                            sp[ii] = sp[ii] + (short)actions.action[i].timeoffset.offset;
                                        }
                                        break;
                                    case UDA_TYPE_INT:
                                        ip = (int*)data_block->dims[data_block->order].offs;
                                        for (ii = 0; ii < data_block->dims[data_block->order].udoms; ii++) {
                                            ip[ii] = ip[ii] + (int)actions.action[i].timeoffset.offset;
                                        }
                                        break;
                                    case UDA_TYPE_LONG:
                                        lp = (long*)data_block->dims[data_block->order].offs;
                                        for (ii = 0; ii < data_block->dims[data_block->order].udoms; ii++) {
                                            lp[ii] = lp[ii] + (long)actions.action[i].timeoffset.offset;
                                        }
                                        break;
                                    case UDA_TYPE_UNSIGNED_CHAR:
                                        uc = (unsigned char*)data_block->dims[data_block->order].offs;
                                        for (ii = 0; ii < data_block->dims[data_block->order].udoms; ii++) {
                                            uc[ii] = uc[ii] + (unsigned char)actions.action[i].timeoffset.offset;
                                        }
                                        break;
                                    case UDA_TYPE_UNSIGNED_SHORT:
                                        us = (unsigned short*)data_block->dims[data_block->order].offs;
                                        for (ii = 0; ii < data_block->dims[data_block->order].udoms; ii++) {
                                            us[ii] = us[ii] + (unsigned short)actions.action[i].timeoffset.offset;
                                        }
                                        break;
                                    case UDA_TYPE_UNSIGNED_INT:
                                        up = (unsigned int*)data_block->dims[data_block->order].offs;
                                        for (ii = 0; ii < data_block->dims[data_block->order].udoms; ii++) {
                                            up[ii] = up[ii] + (unsigned int)actions.action[i].timeoffset.offset;
                                        }
                                        break;
                                    case UDA_TYPE_UNSIGNED_LONG:
                                        ul = (unsigned long*)data_block->dims[data_block->order].offs;
                                        for (ii = 0; ii < data_block->dims[data_block->order].udoms; ii++) {
                                            ul[ii] = ul[ii] + (unsigned long)actions.action[i].timeoffset.offset;
                                        }
                                        break;
                                    default:
                                        break;
                                }
                                break;

                            case 2:
                                switch (data_block->dims[data_block->order].data_type) {
                                    unsigned int ii;
                                    case UDA_TYPE_FLOAT:
                                        fp = (float*)data_block->dims[data_block->order].offs;
                                        for (ii = 0; ii < data_block->dims[data_block->order].udoms; ii++) {
                                            fp[ii] = fp[ii] + (float)actions.action[i].timeoffset.offset;
                                        }
                                        break;
                                    case UDA_TYPE_DOUBLE:
                                        dp = (double*)data_block->dims[data_block->order].offs;
                                        for (ii = 0; ii < data_block->dims[data_block->order].udoms; ii++) {
                                            dp[ii] = dp[ii] + actions.action[i].timeoffset.offset;
                                        }
                                        break;
                                    case UDA_TYPE_CHAR:
                                        cp = (char*)data_block->dims[data_block->order].offs;
                                        for (ii = 0; ii < data_block->dims[data_block->order].udoms; ii++) {
                                            cp[ii] = cp[ii] + (char)actions.action[i].timeoffset.offset;
                                        }
                                        break;
                                    case UDA_TYPE_SHORT:
                                        sp = (short*)data_block->dims[data_block->order].offs;
                                        for (ii = 0; ii < data_block->dims[data_block->order].udoms; ii++) {
                                            sp[ii] = sp[ii] + (short)actions.action[i].timeoffset.offset;
                                        }
                                        break;
                                    case UDA_TYPE_INT:
                                        ip = (int*)data_block->dims[data_block->order].offs;
                                        for (ii = 0; ii < data_block->dims[data_block->order].udoms; ii++) {
                                            ip[ii] = ip[ii] + (int)actions.action[i].timeoffset.offset;
                                        }
                                        break;
                                    case UDA_TYPE_LONG:
                                        lp = (long*)data_block->dims[data_block->order].offs;
                                        for (ii = 0; ii < data_block->dims[data_block->order].udoms; ii++) {
                                            lp[ii] = lp[ii] + (long)actions.action[i].timeoffset.offset;
                                        }
                                        break;
                                    case UDA_TYPE_UNSIGNED_CHAR:
                                        uc = (unsigned char*)data_block->dims[data_block->order].offs;
                                        for (ii = 0; ii < data_block->dims[data_block->order].udoms; ii++) {
                                            uc[ii] = uc[ii] + (unsigned char)actions.action[i].timeoffset.offset;
                                        }
                                        break;
                                    case UDA_TYPE_UNSIGNED_SHORT:
                                        us = (unsigned short*)data_block->dims[data_block->order].offs;
                                        for (ii = 0; ii < data_block->dims[data_block->order].udoms; ii++) {
                                            us[ii] = us[ii] + (unsigned short)actions.action[i].timeoffset.offset;
                                        }
                                        break;
                                    case UDA_TYPE_UNSIGNED_INT:
                                        up = (unsigned int*)data_block->dims[data_block->order].offs;
                                        for (ii = 0; ii < data_block->dims[data_block->order].udoms; ii++) {
                                            up[ii] = up[ii] + (unsigned int)actions.action[i].timeoffset.offset;
                                        }
                                        break;
                                    case UDA_TYPE_UNSIGNED_LONG:
                                        ul = (unsigned long*)data_block->dims[data_block->order].offs;
                                        for (ii = 0; ii < data_block->dims[data_block->order].udoms; ii++) {
                                            ul[ii] = ul[ii] + (unsigned long)actions.action[i].timeoffset.offset;
                                        }
                                        break;
                                    default:
                                        break;
                                }
                                break;

                            case 3:
                                switch (data_block->dims[data_block->order].data_type) {
                                    case UDA_TYPE_FLOAT:
                                        fp = (float*)data_block->dims[data_block->order].offs;
                                        fp[0] = fp[0] + (float)actions.action[i].timeoffset.offset;
                                        break;
                                    case UDA_TYPE_DOUBLE:
                                        dp = (double*)data_block->dims[data_block->order].offs;
                                        dp[0] = dp[0] + actions.action[i].timeoffset.offset;
                                        break;
                                    case UDA_TYPE_CHAR:
                                        cp = (char*)data_block->dims[data_block->order].offs;
                                        cp[0] = cp[0] + (char)actions.action[i].timeoffset.offset;
                                        break;
                                    case UDA_TYPE_SHORT:
                                        sp = (short*)data_block->dims[data_block->order].offs;
                                        sp[0] = sp[0] + (short)actions.action[i].timeoffset.offset;
                                        break;
                                    case UDA_TYPE_INT:
                                        ip = (int*)data_block->dims[data_block->order].offs;
                                        ip[0] = ip[0] + (int)actions.action[i].timeoffset.offset;
                                        break;
                                    case UDA_TYPE_LONG:
                                        lp = (long*)data_block->dims[data_block->order].offs;
                                        lp[0] = lp[0] + (long)actions.action[i].timeoffset.offset;
                                        break;
                                    case UDA_TYPE_UNSIGNED_CHAR:
                                        uc = (unsigned char*)data_block->dims[data_block->order].offs;
                                        uc[0] = uc[0] + (unsigned char)actions.action[i].timeoffset.offset;
                                        break;
                                    case UDA_TYPE_UNSIGNED_SHORT:
                                        us = (unsigned short*)data_block->dims[data_block->order].offs;
                                        us[0] = us[0] + (unsigned short)actions.action[i].timeoffset.offset;
                                        break;
                                    case UDA_TYPE_UNSIGNED_INT:
                                        up = (unsigned int*)data_block->dims[data_block->order].offs;
                                        up[0] = up[0] + (unsigned int)actions.action[i].timeoffset.offset;
                                        break;
                                    case UDA_TYPE_UNSIGNED_LONG:
                                        ul = (unsigned long*)data_block->dims[data_block->order].offs;
                                        ul[0] = ul[0] + (unsigned long)actions.action[i].timeoffset.offset;
                                        break;
                                    default:
                                        break;
                                }
                                break;

                            default:
                                break;
                        }

                    } else {

                        ndata = data_block->dims[data_block->order].dim_n;

                        UDA_LOG(UDA_LOG_DEBUG, "Dimension Not Compressed");
                        UDA_LOG(UDA_LOG_DEBUG, "No. Time Points = {}", ndata);
                        UDA_LOG(UDA_LOG_DEBUG, "Order           = {}", data_block->order);
                        UDA_LOG(UDA_LOG_DEBUG, "Timing Offset   = {}", (float)actions.action[i].timeoffset.offset);

                        switch (data_block->dims[data_block->order].data_type) {
                            int ii;
                            case UDA_TYPE_FLOAT:
                                UDA_LOG(UDA_LOG_DEBUG, "Correcting Time Dimension");
                                UDA_LOG(UDA_LOG_DEBUG, "Offset ? : {}", (float)actions.action[i].timeoffset.offset);
                                fp = (float*)data_block->dims[data_block->order].dim;
                                for (ii = 0; ii < ndata; ii++) {
                                    fp[ii] = (float)actions.action[i].timeoffset.offset + fp[ii];
                                }
                                break;
                            case UDA_TYPE_DOUBLE:
                                dp = (double*)data_block->dims[data_block->order].dim;
                                for (ii = 0; ii < ndata; ii++) {
                                    dp[ii] = actions.action[i].timeoffset.offset + dp[ii];
                                }
                                break;
                            case UDA_TYPE_CHAR:
                                cp = (char*)data_block->dims[data_block->order].dim;
                                for (ii = 0; ii < ndata; ii++) {
                                    cp[ii] = (char)actions.action[i].timeoffset.offset + cp[ii];
                                }
                                break;
                            case UDA_TYPE_SHORT:
                                sp = (short*)data_block->dims[data_block->order].dim;
                                for (ii = 0; ii < ndata; ii++) {
                                    sp[ii] = (short)actions.action[i].timeoffset.offset + sp[ii];
                                }
                                break;
                            case UDA_TYPE_INT:
                                ip = (int*)data_block->dims[data_block->order].dim;
                                for (ii = 0; ii < ndata; ii++) {
                                    ip[ii] = (int)actions.action[i].timeoffset.offset + ip[ii];
                                }
                                break;
                            case UDA_TYPE_LONG:
                                lp = (long*)data_block->dims[data_block->order].dim;
                                for (ii = 0; ii < ndata; ii++) {
                                    lp[ii] = (long)actions.action[i].timeoffset.offset + lp[ii];
                                }
                                break;
                            case UDA_TYPE_UNSIGNED_CHAR:
                                uc = (unsigned char*)data_block->dims[data_block->order].dim;
                                for (ii = 0; ii < ndata; ii++) {
                                    uc[ii] = (unsigned char)actions.action[i].timeoffset.offset + uc[ii];
                                }
                                break;
                            case UDA_TYPE_UNSIGNED_SHORT:
                                us = (unsigned short*)data_block->dims[data_block->order].dim;
                                for (ii = 0; ii < ndata; ii++) {
                                    us[ii] = (unsigned short)actions.action[i].timeoffset.offset + us[ii];
                                }
                                break;
                            case UDA_TYPE_UNSIGNED_INT:
                                up = (unsigned int*)data_block->dims[data_block->order].dim;
                                for (ii = 0; ii < ndata; ii++) {
                                    up[ii] = (unsigned int)actions.action[i].timeoffset.offset + up[ii];
                                }
                                break;
                            case UDA_TYPE_UNSIGNED_LONG:
                                ul = (unsigned long*)data_block->dims[data_block->order].dim;
                                for (ii = 0; ii < ndata; ii++) {
                                    ul[ii] = (unsigned long)actions.action[i].timeoffset.offset + ul[ii];
                                }
                                break;

                            default:
                                break;
                        }
                    }
                }
                break;

                //----------------------------------------------------------------------------------------------
                // Calibration Corrections

            case ActionType::Calibration:

                if (!client_block.get_uncal) {

                    // Data Corrections

                    if (STR_EQUALS("data", actions.action[i].calibration.target) ||
                        STR_EQUALS("all", actions.action[i].calibration.target)) {

                        if (strlen(actions.action[i].calibration.units) > 0) {
                            strcpy(data_block->data_units, actions.action[i].calibration.units);
                        }

                        applyCalibration(data_block->data_type, data_block->data_n,
                                         actions.action[i].calibration.factor, actions.action[i].calibration.offset,
                                         actions.action[i].calibration.invert, data_block->data);
                    }

                    if (STR_EQUALS("error", actions.action[i].calibration.target) ||
                        STR_EQUALS("all", actions.action[i].calibration.target)) {
                        applyCalibration(data_block->error_type, data_block->data_n,
                                         actions.action[i].calibration.factor, actions.action[i].calibration.offset,
                                         actions.action[i].calibration.invert, data_block->errhi);
                    }

                    if (STR_EQUALS("aserror", actions.action[i].calibration.target) ||
                        STR_EQUALS("all", actions.action[i].calibration.target)) {
                        applyCalibration(data_block->error_type, data_block->data_n,
                                         actions.action[i].calibration.factor, actions.action[i].calibration.offset,
                                         actions.action[i].calibration.invert, data_block->errlo);
                    }

                    // Dimension Corrections

                    for (int j = 0; j < actions.action[i].calibration.ndimensions; j++) {

                        dimid = actions.action[i].calibration.dimensions[j].dimid;

                        if (actions.action[i].calibration.dimensions[j].dimType == (int)ActionDimType::Calibration &&
                            actions.action[i].actionId != 0 && dimid > -1 && (unsigned int)dimid < data_block->rank) {

                            if (strlen(actions.action[i].calibration.dimensions[j].dimcalibration.units) > 0) {
                                strcpy(data_block->dims[dimid].dim_units,
                                       actions.action[i].calibration.dimensions[j].dimcalibration.units);
                            }

                            if (actions.action[i].calibration.dimensions[j].dimcalibration.factor != (double)1.0E0 ||
                                actions.action[i].calibration.dimensions[j].dimcalibration.offset != (double)0.0E0) {

                                if (STR_EQUALS("data", actions.action[i].calibration.target) ||
                                    STR_EQUALS("all", actions.action[i].calibration.target)) {
                                    if (data_block->dims[dimid].compressed) {
                                        UDA_LOG(UDA_LOG_DEBUG, "Dimension {} Compressed", i);
                                        UDA_LOG(UDA_LOG_DEBUG, "Method = {}", data_block->dims[dimid].method);

                                        if (data_block->dims[dimid].method == 0) {
                                            if (actions.action[i].calibration.dimensions[j].dimcalibration.factor !=
                                                (double)1.0E0) {
                                                data_block->dims[dimid].diff =
                                                    data_block->dims[dimid].diff *
                                                    actions.action[i].calibration.dimensions[j].dimcalibration.factor;
                                            }

                                            if (actions.action[i].calibration.dimensions[j].dimcalibration.offset !=
                                                (double)0.0E0) {
                                                data_block->dims[dimid].dim0 =
                                                    data_block->dims[dimid].dim0 +
                                                    actions.action[i].calibration.dimensions[j].dimcalibration.offset;
                                            }

                                        } else {

                                            switch (data_block->dims[dimid].data_type) {
                                                unsigned int jj;

                                                case UDA_TYPE_FLOAT:
                                                    switch (data_block->dims[dimid].method) {
                                                        case 1:
                                                            fp0 = (float*)data_block->dims[dimid].offs;
                                                            fp = (float*)data_block->dims[dimid].ints;
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    fp[jj] = fp[jj] * (float)actions.action[i]
                                                                                          .calibration.dimensions[j]
                                                                                          .dimcalibration.factor;
                                                                }
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.offset != (double)0.0E0) {
                                                                    fp0[jj] = fp0[jj] + (float)actions.action[i]
                                                                                            .calibration.dimensions[j]
                                                                                            .dimcalibration.offset;
                                                                }
                                                            }
                                                            break;
                                                        case 2:
                                                            fp0 = (float*)data_block->dims[dimid].offs;
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    fp0[jj] = fp0[jj] * (float)actions.action[i]
                                                                                            .calibration.dimensions[j]
                                                                                            .dimcalibration.factor;
                                                                }
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.offset != (double)0.0E0) {
                                                                    fp0[jj] = fp0[jj] + (float)actions.action[i]
                                                                                            .calibration.dimensions[j]
                                                                                            .dimcalibration.offset;
                                                                }
                                                            }
                                                            break;
                                                        case 3:
                                                            fp0 = (float*)data_block->dims[dimid].offs;
                                                            fp = (float*)data_block->dims[dimid].ints;
                                                            if (actions.action[i]
                                                                    .calibration.dimensions[j]
                                                                    .dimcalibration.offset != (double)0.0E0) {
                                                                fp0[0] = fp0[0] + (float)actions.action[i]
                                                                                      .calibration.dimensions[j]
                                                                                      .dimcalibration.offset;
                                                            }
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    fp[jj] = fp[jj] * (float)actions.action[i]
                                                                                          .calibration.dimensions[j]
                                                                                          .dimcalibration.factor;
                                                                }
                                                            }
                                                            break;
                                                        default:
                                                            break;
                                                    }
                                                    break;

                                                case UDA_TYPE_DOUBLE: {
                                                    double* ar0 = (double*)data_block->dims[dimid].offs;
                                                    double* ar = (double*)data_block->dims[dimid].ints;
                                                    switch (data_block->dims[dimid].method) {
                                                        case 1:
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    ar[jj] = ar[jj] * (double)actions.action[i]
                                                                                          .calibration.dimensions[j]
                                                                                          .dimcalibration.factor;
                                                                }
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.offset != (double)0.0E0) {
                                                                    ar0[jj] = ar0[jj] + (double)actions.action[i]
                                                                                            .calibration.dimensions[j]
                                                                                            .dimcalibration.offset;
                                                                }
                                                            }
                                                            break;
                                                        case 2:
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    ar0[jj] = ar0[jj] * (double)actions.action[i]
                                                                                            .calibration.dimensions[j]
                                                                                            .dimcalibration.factor;
                                                                }
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.offset != (double)0.0E0) {
                                                                    ar0[jj] = ar0[jj] + (double)actions.action[i]
                                                                                            .calibration.dimensions[j]
                                                                                            .dimcalibration.offset;
                                                                }
                                                            }
                                                            break;
                                                        case 3:
                                                            if (actions.action[i]
                                                                    .calibration.dimensions[j]
                                                                    .dimcalibration.offset != (double)0.0E0) {
                                                                ar0[0] = ar0[0] + (double)actions.action[i]
                                                                                      .calibration.dimensions[j]
                                                                                      .dimcalibration.offset;
                                                            }
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    ar[jj] = ar[jj] * (double)actions.action[i]
                                                                                          .calibration.dimensions[j]
                                                                                          .dimcalibration.factor;
                                                                }
                                                            }
                                                            break;
                                                        default:
                                                            break;
                                                    }
                                                } break;

                                                case UDA_TYPE_CHAR: {
                                                    char* ar0 = (char*)data_block->dims[dimid].offs;
                                                    char* ar = (char*)data_block->dims[dimid].ints;
                                                    switch (data_block->dims[dimid].method) {
                                                        case 1:
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    ar[jj] = ar[jj] * (char)actions.action[i]
                                                                                          .calibration.dimensions[j]
                                                                                          .dimcalibration.factor;
                                                                }
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.offset != (double)0.0E0) {
                                                                    ar0[jj] = ar0[jj] + (char)actions.action[i]
                                                                                            .calibration.dimensions[j]
                                                                                            .dimcalibration.offset;
                                                                }
                                                            }
                                                            break;
                                                        case 2:
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    ar0[jj] = ar0[jj] * (char)actions.action[i]
                                                                                            .calibration.dimensions[j]
                                                                                            .dimcalibration.factor;
                                                                }
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.offset != (double)0.0E0) {
                                                                    ar0[jj] = ar0[jj] + (char)actions.action[i]
                                                                                            .calibration.dimensions[j]
                                                                                            .dimcalibration.offset;
                                                                }
                                                            }
                                                            break;
                                                        case 3:
                                                            if (actions.action[i]
                                                                    .calibration.dimensions[j]
                                                                    .dimcalibration.offset != (double)0.0E0) {
                                                                ar0[0] = ar0[0] + (char)actions.action[i]
                                                                                      .calibration.dimensions[j]
                                                                                      .dimcalibration.offset;
                                                            }
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    ar[jj] = ar[jj] * (char)actions.action[i]
                                                                                          .calibration.dimensions[j]
                                                                                          .dimcalibration.factor;
                                                                }
                                                            }
                                                            break;
                                                        default:
                                                            break;
                                                    }
                                                } break;

                                                case UDA_TYPE_SHORT: {
                                                    short* ar = (short*)data_block->dims[dimid].ints;
                                                    short* ar0 = (short*)data_block->dims[dimid].offs;
                                                    switch (data_block->dims[dimid].method) {
                                                        case 1:
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    ar[jj] = ar[jj] * (short)actions.action[i]
                                                                                          .calibration.dimensions[j]
                                                                                          .dimcalibration.factor;
                                                                }
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.offset != (double)0.0E0) {
                                                                    ar0[jj] = ar0[jj] + (short)actions.action[i]
                                                                                            .calibration.dimensions[j]
                                                                                            .dimcalibration.offset;
                                                                }
                                                            }
                                                            break;
                                                        case 2:
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    ar0[jj] = ar0[jj] * (short)actions.action[i]
                                                                                            .calibration.dimensions[j]
                                                                                            .dimcalibration.factor;
                                                                }
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.offset != (double)0.0E0) {
                                                                    ar0[jj] = ar0[jj] + (short)actions.action[i]
                                                                                            .calibration.dimensions[j]
                                                                                            .dimcalibration.offset;
                                                                }
                                                            }
                                                            break;
                                                        case 3:
                                                            if (actions.action[i]
                                                                    .calibration.dimensions[j]
                                                                    .dimcalibration.offset != (double)0.0E0) {
                                                                ar0[0] = ar0[0] + (short)actions.action[i]
                                                                                      .calibration.dimensions[j]
                                                                                      .dimcalibration.offset;
                                                            }
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    ar[jj] = ar[jj] * (short)actions.action[i]
                                                                                          .calibration.dimensions[j]
                                                                                          .dimcalibration.factor;
                                                                }
                                                            }
                                                            break;
                                                        default:
                                                            break;
                                                    }
                                                } break;

                                                case UDA_TYPE_INT: {
                                                    int* ar = (int*)data_block->dims[dimid].ints;
                                                    int* ar0 = (int*)data_block->dims[dimid].offs;
                                                    switch (data_block->dims[dimid].method) {
                                                        case 1:
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    ar[jj] = ar[jj] * (int)actions.action[i]
                                                                                          .calibration.dimensions[j]
                                                                                          .dimcalibration.factor;
                                                                }
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.offset != (double)0.0E0) {
                                                                    ar0[jj] = ar0[jj] + (int)actions.action[i]
                                                                                            .calibration.dimensions[j]
                                                                                            .dimcalibration.offset;
                                                                }
                                                            }
                                                            break;
                                                        case 2:
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    ar0[jj] = ar0[jj] * (int)actions.action[i]
                                                                                            .calibration.dimensions[j]
                                                                                            .dimcalibration.factor;
                                                                }
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.offset != (double)0.0E0) {
                                                                    ar0[jj] = ar0[jj] + (int)actions.action[i]
                                                                                            .calibration.dimensions[j]
                                                                                            .dimcalibration.offset;
                                                                }
                                                            }
                                                            break;
                                                        case 3:
                                                            if (actions.action[i]
                                                                    .calibration.dimensions[j]
                                                                    .dimcalibration.offset != (double)0.0E0) {
                                                                ar0[0] = ar0[0] + (int)actions.action[i]
                                                                                      .calibration.dimensions[j]
                                                                                      .dimcalibration.offset;
                                                            }
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    ar[jj] = ar[jj] * (int)actions.action[i]
                                                                                          .calibration.dimensions[j]
                                                                                          .dimcalibration.factor;
                                                                }
                                                            }
                                                            break;
                                                        default:
                                                            break;
                                                    }
                                                } break;

                                                case UDA_TYPE_LONG: {
                                                    long* ar = (long*)data_block->dims[dimid].ints;
                                                    long* ar0 = (long*)data_block->dims[dimid].offs;
                                                    switch (data_block->dims[dimid].method) {
                                                        case 1:
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    ar[jj] = ar[jj] * (long)actions.action[i]
                                                                                          .calibration.dimensions[j]
                                                                                          .dimcalibration.factor;
                                                                }
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.offset != (double)0.0E0) {
                                                                    ar0[jj] = ar0[jj] + (long)actions.action[i]
                                                                                            .calibration.dimensions[j]
                                                                                            .dimcalibration.offset;
                                                                }
                                                            }
                                                            break;
                                                        case 2:
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    ar0[jj] = ar0[jj] * (long)actions.action[i]
                                                                                            .calibration.dimensions[j]
                                                                                            .dimcalibration.factor;
                                                                }
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.offset != (double)0.0E0) {
                                                                    ar0[jj] = ar0[jj] + (long)actions.action[i]
                                                                                            .calibration.dimensions[j]
                                                                                            .dimcalibration.offset;
                                                                }
                                                            }
                                                            break;
                                                        case 3:
                                                            if (actions.action[i]
                                                                    .calibration.dimensions[j]
                                                                    .dimcalibration.offset != (double)0.0E0) {
                                                                ar0[0] = ar0[0] + (long)actions.action[i]
                                                                                      .calibration.dimensions[j]
                                                                                      .dimcalibration.offset;
                                                            }
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    ar[jj] = ar[jj] * (long)actions.action[i]
                                                                                          .calibration.dimensions[j]
                                                                                          .dimcalibration.factor;
                                                                }
                                                            }
                                                            break;
                                                        default:
                                                            break;
                                                    }
                                                } break;

                                                case UDA_TYPE_UNSIGNED_CHAR: {
                                                    unsigned char* ar = (unsigned char*)data_block->dims[dimid].ints;
                                                    unsigned char* ar0 = (unsigned char*)data_block->dims[dimid].offs;
                                                    switch (data_block->dims[dimid].method) {
                                                        case 1:
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    ar[jj] = ar[jj] * (unsigned char)actions.action[i]
                                                                                          .calibration.dimensions[j]
                                                                                          .dimcalibration.factor;
                                                                }
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.offset != (double)0.0E0) {
                                                                    ar0[jj] = ar0[jj] + (unsigned char)actions.action[i]
                                                                                            .calibration.dimensions[j]
                                                                                            .dimcalibration.offset;
                                                                }
                                                            }
                                                            break;
                                                        case 2:
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    ar0[jj] = ar0[jj] * (unsigned char)actions.action[i]
                                                                                            .calibration.dimensions[j]
                                                                                            .dimcalibration.factor;
                                                                }
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.offset != (double)0.0E0) {
                                                                    ar0[jj] = ar0[jj] + (unsigned char)actions.action[i]
                                                                                            .calibration.dimensions[j]
                                                                                            .dimcalibration.offset;
                                                                }
                                                            }
                                                            break;
                                                        case 3:
                                                            if (actions.action[i]
                                                                    .calibration.dimensions[j]
                                                                    .dimcalibration.offset != (double)0.0E0) {
                                                                ar0[0] = ar0[0] + (unsigned char)actions.action[i]
                                                                                      .calibration.dimensions[j]
                                                                                      .dimcalibration.offset;
                                                            }
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    ar[jj] = ar[jj] * (unsigned char)actions.action[i]
                                                                                          .calibration.dimensions[j]
                                                                                          .dimcalibration.factor;
                                                                }
                                                            }
                                                            break;
                                                        default:
                                                            break;
                                                    }
                                                } break;

                                                case UDA_TYPE_UNSIGNED_SHORT: {
                                                    unsigned short* ar = (unsigned short*)data_block->dims[dimid].ints;
                                                    unsigned short* ar0 = (unsigned short*)data_block->dims[dimid].offs;
                                                    switch (data_block->dims[dimid].method) {
                                                        case 1:
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    ar[jj] = ar[jj] * (unsigned short)actions.action[i]
                                                                                          .calibration.dimensions[j]
                                                                                          .dimcalibration.factor;
                                                                }
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.offset != (double)0.0E0) {
                                                                    ar0[jj] =
                                                                        ar0[jj] + (unsigned short)actions.action[i]
                                                                                      .calibration.dimensions[j]
                                                                                      .dimcalibration.offset;
                                                                }
                                                            }
                                                            break;
                                                        case 2:
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    ar0[jj] =
                                                                        ar0[jj] * (unsigned short)actions.action[i]
                                                                                      .calibration.dimensions[j]
                                                                                      .dimcalibration.factor;
                                                                }
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.offset != (double)0.0E0) {
                                                                    ar0[jj] =
                                                                        ar0[jj] + (unsigned short)actions.action[i]
                                                                                      .calibration.dimensions[j]
                                                                                      .dimcalibration.offset;
                                                                }
                                                            }
                                                            break;
                                                        case 3:
                                                            if (actions.action[i]
                                                                    .calibration.dimensions[j]
                                                                    .dimcalibration.offset != (double)0.0E0) {
                                                                ar0[0] = ar0[0] + (unsigned short)actions.action[i]
                                                                                      .calibration.dimensions[j]
                                                                                      .dimcalibration.offset;
                                                            }
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    ar[jj] = ar[jj] * (unsigned short)actions.action[i]
                                                                                          .calibration.dimensions[j]
                                                                                          .dimcalibration.factor;
                                                                }
                                                            }
                                                            break;
                                                        default:
                                                            break;
                                                    }
                                                } break;

                                                case UDA_TYPE_UNSIGNED_INT: {
                                                    unsigned int* ar = (unsigned int*)data_block->dims[dimid].ints;
                                                    unsigned int* ar0 = (unsigned int*)data_block->dims[dimid].offs;
                                                    switch (data_block->dims[dimid].method) {
                                                        case 1:
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    ar[jj] = ar[jj] * (unsigned int)actions.action[i]
                                                                                          .calibration.dimensions[j]
                                                                                          .dimcalibration.factor;
                                                                }
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.offset != (double)0.0E0) {
                                                                    ar0[jj] = ar0[jj] + (unsigned int)actions.action[i]
                                                                                            .calibration.dimensions[j]
                                                                                            .dimcalibration.offset;
                                                                }
                                                            }
                                                            break;
                                                        case 2:
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    ar0[jj] = ar0[jj] * (unsigned int)actions.action[i]
                                                                                            .calibration.dimensions[j]
                                                                                            .dimcalibration.factor;
                                                                }
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.offset != (double)0.0E0) {
                                                                    ar0[jj] = ar0[jj] + (unsigned int)actions.action[i]
                                                                                            .calibration.dimensions[j]
                                                                                            .dimcalibration.offset;
                                                                }
                                                            }
                                                            break;
                                                        case 3:
                                                            if (actions.action[i]
                                                                    .calibration.dimensions[j]
                                                                    .dimcalibration.offset != (double)0.0E0) {
                                                                ar0[0] = ar0[0] + (unsigned int)actions.action[i]
                                                                                      .calibration.dimensions[j]
                                                                                      .dimcalibration.offset;
                                                            }
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    ar[jj] = ar[jj] * (unsigned int)actions.action[i]
                                                                                          .calibration.dimensions[j]
                                                                                          .dimcalibration.factor;
                                                                }
                                                            }
                                                            break;
                                                        default:
                                                            break;
                                                    }
                                                } break;

                                                case UDA_TYPE_UNSIGNED_LONG: {
                                                    unsigned long* ar = (unsigned long*)data_block->dims[dimid].ints;
                                                    unsigned long* ar0 = (unsigned long*)data_block->dims[dimid].offs;
                                                    switch (data_block->dims[dimid].method) {
                                                        case 1:
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    ar[jj] = ar[jj] * (unsigned long)actions.action[i]
                                                                                          .calibration.dimensions[j]
                                                                                          .dimcalibration.factor;
                                                                }
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.offset != (double)0.0E0) {
                                                                    ar0[jj] = ar0[jj] + (unsigned long)actions.action[i]
                                                                                            .calibration.dimensions[j]
                                                                                            .dimcalibration.offset;
                                                                }
                                                            }
                                                            break;
                                                        case 2:
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    ar0[jj] = ar0[jj] * (unsigned long)actions.action[i]
                                                                                            .calibration.dimensions[j]
                                                                                            .dimcalibration.factor;
                                                                }
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.offset != (double)0.0E0) {
                                                                    ar0[jj] = ar0[jj] + (unsigned long)actions.action[i]
                                                                                            .calibration.dimensions[j]
                                                                                            .dimcalibration.offset;
                                                                }
                                                            }
                                                            break;
                                                        case 3:
                                                            if (actions.action[i]
                                                                    .calibration.dimensions[j]
                                                                    .dimcalibration.offset != (double)0.0E0) {
                                                                ar0[0] = ar0[0] + (unsigned long)actions.action[i]
                                                                                      .calibration.dimensions[j]
                                                                                      .dimcalibration.offset;
                                                            }
                                                            for (jj = 0; jj < data_block->dims[dimid].udoms; jj++) {
                                                                if (actions.action[i]
                                                                        .calibration.dimensions[j]
                                                                        .dimcalibration.factor != (double)1.0E0) {
                                                                    ar[jj] = ar[jj] * (unsigned long)actions.action[i]
                                                                                          .calibration.dimensions[j]
                                                                                          .dimcalibration.factor;
                                                                }
                                                            }
                                                            break;
                                                        default:
                                                            break;
                                                    }
                                                } break;

                                                default:
                                                    break; // Need to Add for all data types
                                            }
                                        }

                                    } else {

                                        applyCalibration(
                                            data_block->dims[dimid].data_type, data_block->dims[dimid].dim_n,
                                            actions.action[i].calibration.dimensions[j].dimcalibration.factor,
                                            actions.action[i].calibration.dimensions[j].dimcalibration.offset,
                                            actions.action[i].calibration.dimensions[j].dimcalibration.invert,
                                            data_block->dims[dimid].dim);

                                        UDA_LOG(UDA_LOG_DEBUG, "Rescaling Dimension : {}", dimid);
                                        UDA_LOG(UDA_LOG_DEBUG, "Time Dimension ?    : {}", data_block->order);
                                        UDA_LOG(
                                            UDA_LOG_DEBUG, "Scale ?             : {}\n",
                                            (float)actions.action[i].calibration.dimensions[j].dimcalibration.factor);
                                        UDA_LOG(
                                            UDA_LOG_DEBUG, "Offset ?            : {}\n",
                                            (float)actions.action[i].calibration.dimensions[j].dimcalibration.offset);
                                        UDA_LOG(UDA_LOG_DEBUG, "Invert ?            : {}",
                                                (int)actions.action[i].calibration.dimensions[j].dimcalibration.invert);
                                    }
                                }

                                if (STR_EQUALS("error", actions.action[i].calibration.target) ||
                                    STR_EQUALS("all", actions.action[i].calibration.target)) {
                                    applyCalibration(data_block->dims[dimid].error_type, data_block->dims[dimid].dim_n,
                                                     actions.action[i].calibration.dimensions[j].dimcalibration.factor,
                                                     actions.action[i].calibration.dimensions[j].dimcalibration.offset,
                                                     actions.action[i].calibration.dimensions[j].dimcalibration.invert,
                                                     data_block->dims[dimid].errhi);
                                }

                                if (STR_EQUALS("aserror", actions.action[i].calibration.target) ||
                                    STR_EQUALS("all", actions.action[i].calibration.target)) {
                                    applyCalibration(data_block->dims[dimid].error_type, data_block->dims[dimid].dim_n,
                                                     actions.action[i].calibration.dimensions[j].dimcalibration.factor,
                                                     actions.action[i].calibration.dimensions[j].dimcalibration.offset,
                                                     actions.action[i].calibration.dimensions[j].dimcalibration.invert,
                                                     data_block->dims[dimid].errlo);
                                }
                            }
                        }
                    }
                }
                break;

            default:
                break;
        }
    }
}

// Combine the set of Actions from both sources with Signal XML having Priority of Signal_Desc XML

void uda::server_deselect_signal_xml(uda::client_server::Actions* actions_desc,
                                     uda::client_server::Actions* actions_sig)
{

    int type;

    UDA_LOG(UDA_LOG_DEBUG, "Deselecting Conflicting XML");

    //----------------------------------------------------------------------------------------------
    // Loop over all Signal actions

    for (int i = 0; i < actions_sig->nactions; i++) {

        type = actions_sig->action[i].actionType; // Target (This has Priority)

        // Loop over all Signal_Desc actions

        for (int j = 0; j < actions_desc->nactions; j++) {
            if (!actions_desc->action[j].inRange) {
                continue; // Out of Pulse/Pass Range
            }
            if (actions_desc->action[j].actionType == type) {
                actions_desc->action[j].inRange = 0; // Disable
            }
        }
    }
}
