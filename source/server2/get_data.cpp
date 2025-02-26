#include "get_data.hpp"

#include "apply_XML.hpp"
#include "clientserver/error_log.h"
#include "clientserver/init_structs.h"
#include "clientserver/name_value_substitution.h"
#include "clientserver/print_structs.h"
#include "common/string_utils.h"
#include "logging/logging.h"
#include "make_server_request_block.hpp"
#include "plugins.hpp"
#include "server.hpp"
#include "server_plugin.h"
#include "server_subset_data.h"

#include <boost/algorithm/string.hpp>
#include <uda/types.h>
#include <uda/plugins.h>

using namespace uda::client_server;
using namespace uda::logging;
using namespace uda::common;

int uda::server::swap_signal_error(DataBlock* data_block, DataBlock* data_block2, int asymmetry)
{
    // Check Rank and Array Block Size are equal

    if (data_block->rank == data_block2->rank && data_block->data_n == data_block2->data_n) {

        if (!asymmetry) {
            if (data_block->errhi != nullptr) {
                free(data_block->errhi); // Free unwanted Error Data Heap
            }
            data_block->errhi = data_block2->data; // straight swap!
            data_block2->data = nullptr;           // Prevent Double Heap Free
            data_block->errasymmetry = 0;
        } else {
            if (data_block->errlo != nullptr) {
                free(data_block->errlo);
            }
            data_block->errlo = data_block2->data;
            data_block2->data = nullptr;
            data_block->errasymmetry = 1;
        }

        data_block->error_type = data_block2->data_type;

    } else {
        UDA_THROW(7777, "Error Data Substitution Not Possible - Incompatible Lengths");
    }

    return 0;
}

int uda::server::swap_signal_dim(DimComposite dimcomposite, DataBlock* data_block, DataBlock* data_block2)
{
    void* cptr = nullptr;

    // Possible Swaps: Replace Dimension with Signal Data or with a Dimension of the Swap Signal Data

    // Swap Signal Data

    if (dimcomposite.from_dim < 0 && dimcomposite.to_dim >= 0) {

        if (data_block->dims[dimcomposite.to_dim].dim_n == data_block2->data_n) {

            if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].dim) != nullptr) {
                free(cptr);
            } // Free unwanted dimension Heap
            if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].sams) != nullptr) {
                free(cptr);
            }
            if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].offs) != nullptr) {
                free(cptr);
            }
            if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].ints) != nullptr) {
                free(cptr);
            }
            if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].errhi) != nullptr) {
                free(cptr);
            }
            if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].errlo) != nullptr) {
                free(cptr);
            }

            data_block->dims[dimcomposite.to_dim].dim = nullptr; // Prevent Double Heap Free
            data_block->dims[dimcomposite.to_dim].sams = nullptr;
            data_block->dims[dimcomposite.to_dim].offs = nullptr;
            data_block->dims[dimcomposite.to_dim].ints = nullptr;
            data_block->dims[dimcomposite.to_dim].errhi = nullptr;
            data_block->dims[dimcomposite.to_dim].errlo = nullptr;

            data_block->dims[dimcomposite.to_dim].dim = data_block2->data; // straight swap!
            data_block->dims[dimcomposite.to_dim].errhi = data_block2->errhi;
            data_block->dims[dimcomposite.to_dim].errlo = data_block2->errlo;
            for (int i = 0; i < data_block2->error_param_n; i++) {
                data_block->dims[dimcomposite.to_dim].errparams[i] = data_block2->errparams[i];
            }
            data_block2->data = nullptr; // Prevent Double Heap Free
            data_block2->errhi = nullptr;
            data_block2->errlo = nullptr;

            data_block->dims[dimcomposite.to_dim].dim_n = data_block2->data_n;
            data_block->dims[dimcomposite.to_dim].data_type = data_block2->data_type;
            data_block->dims[dimcomposite.to_dim].error_type = data_block2->error_type;
            data_block->dims[dimcomposite.to_dim].errasymmetry = data_block2->errasymmetry;
            data_block->dims[dimcomposite.to_dim].compressed = 0; // Not Applicable to Signal Data
            data_block->dims[dimcomposite.to_dim].dim0 = 0.0E0;
            data_block->dims[dimcomposite.to_dim].diff = 0.0E0;
            data_block->dims[dimcomposite.to_dim].method = 0;
            data_block->dims[dimcomposite.to_dim].udoms = 0;
            data_block->dims[dimcomposite.to_dim].error_model = data_block2->error_model;
            data_block->dims[dimcomposite.to_dim].error_param_n = data_block2->error_param_n;

            strcpy(data_block->dims[dimcomposite.to_dim].dim_units, data_block2->data_units);
            strcpy(data_block->dims[dimcomposite.to_dim].dim_label, data_block2->data_label);

        } else {
            UDA_THROW(7777, "Dimension Data Substitution Not Possible - Incompatible Lengths");
        }

        // Swap Signal Dimension Data

    } else {

        if (dimcomposite.from_dim >= 0 && dimcomposite.to_dim >= 0) {
            if (data_block->dims[dimcomposite.to_dim].dim_n == data_block2->dims[dimcomposite.from_dim].dim_n) {

                if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].dim) != nullptr) {
                    free(cptr);
                } // Free unwanted dimension Heap
                if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].errhi) != nullptr) {
                    free(cptr);
                }
                if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].errlo) != nullptr) {
                    free(cptr);
                }
                if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].sams) != nullptr) {
                    free(cptr);
                }
                if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].offs) != nullptr) {
                    free(cptr);
                }
                if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].ints) != nullptr) {
                    free(cptr);
                }

                data_block->dims[dimcomposite.to_dim].dim =
                        data_block2->dims[dimcomposite.from_dim].dim; // straight swap!
                data_block->dims[dimcomposite.to_dim].errhi = data_block2->dims[dimcomposite.from_dim].errhi;
                data_block->dims[dimcomposite.to_dim].errlo = data_block2->dims[dimcomposite.from_dim].errlo;
                data_block->dims[dimcomposite.to_dim].sams = data_block2->dims[dimcomposite.from_dim].sams;
                data_block->dims[dimcomposite.to_dim].offs = data_block2->dims[dimcomposite.from_dim].offs;
                data_block->dims[dimcomposite.to_dim].ints = data_block2->dims[dimcomposite.from_dim].ints;
                for (int i = 0; i < data_block2->dims[dimcomposite.from_dim].error_param_n; i++) {
                    data_block->dims[dimcomposite.to_dim].errparams[i] =
                            data_block2->dims[dimcomposite.from_dim].errparams[i];
                }
                data_block2->dims[dimcomposite.from_dim].dim = nullptr; // Prevent Double Heap Free
                data_block2->dims[dimcomposite.from_dim].errhi = nullptr;
                data_block2->dims[dimcomposite.from_dim].errlo = nullptr;
                data_block2->dims[dimcomposite.from_dim].sams = nullptr;
                data_block2->dims[dimcomposite.from_dim].offs = nullptr;
                data_block2->dims[dimcomposite.from_dim].ints = nullptr;

                data_block->dims[dimcomposite.to_dim].dim_n = data_block2->dims[dimcomposite.from_dim].dim_n;
                data_block->dims[dimcomposite.to_dim].data_type = data_block2->dims[dimcomposite.from_dim].data_type;
                data_block->dims[dimcomposite.to_dim].compressed = data_block2->dims[dimcomposite.from_dim].compressed;
                data_block->dims[dimcomposite.to_dim].dim0 = data_block2->dims[dimcomposite.from_dim].dim0;
                data_block->dims[dimcomposite.to_dim].diff = data_block2->dims[dimcomposite.from_dim].diff;
                data_block->dims[dimcomposite.to_dim].method = data_block2->dims[dimcomposite.from_dim].method;
                data_block->dims[dimcomposite.to_dim].udoms = data_block2->dims[dimcomposite.from_dim].udoms;

                data_block->dims[dimcomposite.to_dim].error_model = data_block2->dims[dimcomposite.from_dim].error_type;
                data_block->dims[dimcomposite.to_dim].error_model =
                        data_block2->dims[dimcomposite.from_dim].errasymmetry;
                data_block->dims[dimcomposite.to_dim].error_model =
                        data_block2->dims[dimcomposite.from_dim].error_model;
                data_block->dims[dimcomposite.to_dim].error_param_n =
                        data_block2->dims[dimcomposite.from_dim].error_param_n;

                strcpy(data_block->dims[dimcomposite.to_dim].dim_units,
                       data_block2->dims[dimcomposite.from_dim].dim_units);
                strcpy(data_block->dims[dimcomposite.to_dim].dim_label,
                       data_block2->dims[dimcomposite.from_dim].dim_label);

            } else {
                UDA_THROW(7777, "Dimension Data Substitution Not Possible - Incompatible Lengths");
            }
        }
    }
    return 0;
}

int uda::server::swap_signal_dim_error(DimComposite dimcomposite, DataBlock* data_block, DataBlock* data_block2, int asymmetry)
{
    void* cptr = nullptr;

    // Replace Dimension Error Data with Signal Data

    if (dimcomposite.from_dim < 0 && dimcomposite.to_dim >= 0) {

        if (data_block->dims[dimcomposite.to_dim].dim_n == data_block2->data_n) {

            if (!asymmetry) {
                if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].errhi) != nullptr) {
                    free(cptr);
                }                                                                // Unwanted
                data_block->dims[dimcomposite.to_dim].errhi = data_block2->data; // straight swap!
                data_block2->data = nullptr;                                     // Prevent Double Heap Free
                data_block->dims[dimcomposite.to_dim].errasymmetry = 0;
            } else {
                if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].errlo) != nullptr) {
                    free(cptr);
                }
                data_block->dims[dimcomposite.to_dim].errlo = data_block2->data;
                data_block2->data = nullptr;
                data_block->dims[dimcomposite.to_dim].errasymmetry = 1;
            }
            data_block->dims[dimcomposite.to_dim].error_type = data_block2->data_type;

        } else {
            UDA_THROW(7777, "Dimension Error Data Substitution Not Possible - Incompatible Lengths");
        }
    }
    return 0;
}

int uda::server::Server::get_data(int* depth, RequestData* request_data, DataBlock* data_block, int protocol_version)
{
    int is_derived = 0;
    int comp_id = -1;
    int serverside = 0;

    RequestData request_block2;
    DataBlock data_block2;
    MetaData meta_data;
    Actions actions_serverside{};
    Actions actions_comp_desc{};
    Actions actions_comp_sig{};
    Actions actions_comp_desc2{};
    Actions actions_comp_sig2{};

    static int original_request = 0; // First entry value of the Plugin Request
    static int original_xml = 0;     // First entry flag that XML was passed in

    //--------------------------------------------------------------------------------------------------------------------------
    // Retain the original request (Needed to flag that signal/file details are in the Request or Action structures)

#ifndef PROXYSERVER
    if (original_request == 0 || *depth == 0) {
        original_request = request_data->request;
        if (request_data->request != (int)Request::ReadXML) {
            if (STR_STARTSWITH(request_data->signal, "<?xml")) {
                original_xml = 1;
            }
        }
    }

    if (original_xml == 1 && *depth == 1) {
        meta_data.set("xml", "");
    } // remove redirected XML after first recursive pass
#endif

    //--------------------------------------------------------------------------------------------------------------------------
    // Limit the Recursive Depth

    if (*depth == XmlMaxRecursive) {
        UDA_THROW(7777, "Recursive Depth (Derived or Substitute Data) Exceeds Internal Limit");
    }

    (*depth)++;

    UDA_LOG(UDA_LOG_DEBUG, "GetData Recursive Depth = {}", *depth);

    // Can't use Request::ReadServerside because data must be read first using a 'real' data reader or
    // Request::ReadGeneric

    if (protocol_version < 6) {
        if (STR_IEQUALS(request_data->archive, "SS") || STR_IEQUALS(request_data->archive, "SERVERSIDE")) {
            if (!strncasecmp(request_data->signal, "Subset(", 7)) {
                serverside = 1;
                init_actions(&actions_serverside);
                int rc;
                if ((rc = server_parse_server_side(config_, request_data, &actions_serverside)) != 0) {
                    return rc;
                }
                // Erase original Subset request
                meta_data.set("signal_name", request_data->signal);
            }
        }
    } else if (STR_IEQUALS(request_data->function, "subset")) {
        auto [_, maybe_plugin] = _plugins.find_by_name(request_data->archive);
        if (maybe_plugin) {
            if (maybe_plugin.get().entry_func_name == "serverside") {
                serverside = 1;
                init_actions(&actions_serverside);
                int rc;
                if ((rc = server_parse_server_side(config_, request_data, &actions_serverside)) != 0) {
                    return rc;
                }
                // Erase original Subset request
                meta_data.set("signal_name", request_data->signal);
            }
        }
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // Read the Data (Returns rc < 0 if the signal is a derived type or is defined in an XML document)

    int rc = read_data(request_data, data_block);

    UDA_LOG(UDA_LOG_DEBUG, "After read_data rc = {}", rc);
    UDA_LOG(UDA_LOG_DEBUG, "Is the Signal a Composite? {}", meta_data.find("type") == "C");

    if (rc > 0) {
        (*depth)--;
        return rc; // An Error Occurred
    }

    // Perform data subsetting if requested

    if (request_data->datasubset.nbound > 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Calling serverSubsetData (Subset)   {}", *depth);
        Action action = {};
        init_action(&action);
        action.actionType = (int)ActionType::Subset;
        action.subset = request_data->datasubset;
        if ((rc = server_subset_data(data_block, action, _log_malloc_list)) != 0) {
            (*depth)--;
            return rc;
        }
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // If the Request is Not for a Generic Signal then exit - No XML source to apply to data as it is just regular data.
    // Allow Composites (C) or Signal Switch (S) through regardless of request type

    if (meta_data.find("type") != "C" && !serverside && meta_data.find("type") != "S" &&
        (!(request_data->request == (int)Request::ReadGeneric || request_data->request == (int)Request::ReadXML))) {
        return 0;
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // Is the Signal a Derived or Signal Composite?

    if (meta_data.find("type") == "C") {
        // The Signal is a Derived/Composite Type so Parse the XML for the data signal identity and read the data

        UDA_LOG(UDA_LOG_DEBUG, "Derived/Composite Signal {}", request_data->signal);

        is_derived = 1; // is True

        // derived_signal_desc     = *signal_desc;                // Preserve details of Derived Signal Description
        // Record
        meta_data.set("exp_number", request_data->exp_number); // Needed for Pulse Number Range Check in XML Parser
        meta_data.set("pass", request_data->pass); // Needed for a Pass/Sequence Range Check in XML Parser

        // Allways Parse Signal XML to Identify the True Data Source for this Pulse Number - not subject to client
        // request: get_asis (First Valid Action Record found only - others ignored)

        init_actions(&actions_comp_desc);
        init_actions(&actions_comp_sig);

        UDA_LOG(UDA_LOG_DEBUG, "parsing XML for a COMPOSITE Signal");

        rc = server_parse_signal_xml(error_stack_, meta_data, &actions_comp_desc, &actions_comp_sig);

        UDA_LOG(UDA_LOG_DEBUG, "parsing XML RC? {}", rc);

        if (rc > 0) {
            free_actions(&actions_comp_desc);
            free_actions(&actions_comp_sig);
            (*depth)--;
            UDA_THROW(8881, "Unable to Parse XML");
        }

        // Identify which XML statements are in Range (Only signal_desc xml need be checked as signal xml is specific to
        // a single pulse/pass)

        comp_id = -1;
        if (rc == 0) {
            for (int i = 0; i < actions_comp_desc.nactions; i++) {
                if (actions_comp_desc.action[i].actionType == (int)ActionType::Composite &&
                    actions_comp_desc.action[i].inRange) {
                    comp_id = i;
                    break; // First Record found only!
                }
            }

            // Identify the data's signal

            if (comp_id >= 0) {
                if (strlen(actions_comp_desc.action[comp_id].composite.data_signal) > 0) {
                    // If we haven't a True Signal then can't identify the data required!

                    request_block2 =
                        *request_data; // Preserve details of the Original User Request (Do Not FREE Elements)

                    strcpy(request_block2.signal,
                           actions_comp_desc.action[comp_id].composite.data_signal); // True Signal Identity

                    // Does this Composite originate from a subsetting operation? If so then fill out any missing items
                    // in the composite record

                    if (actions_comp_desc.action[comp_id].composite.nsubsets > 0 ||
                        actions_comp_desc.action[comp_id].composite.nmaps > 0 ||
                        (strlen(actions_comp_desc.action[comp_id].composite.file) == 0 &&
                            !meta_data.find("path").empty())) {

                        // ******** If there is No subset then composite.file is missing!!!

                        if (strlen(actions_comp_desc.action[comp_id].composite.file) == 0 &&
                            !meta_data.find("path").empty()) {
                            strcpy(actions_comp_desc.action[comp_id].composite.file, meta_data.find("path").data());
                        }

                        if (strlen(actions_comp_desc.action[comp_id].composite.format) == 0 &&
                            !meta_data.find("format").empty()) {
                            strcpy(actions_comp_desc.action[comp_id].composite.format, meta_data.find("format").data());
                        }

                        if (strlen(actions_comp_desc.action[comp_id].composite.data_signal) > 0 &&
                            meta_data.find("signal_name").empty()) {
                            meta_data.set("signal_name", actions_comp_desc.action[comp_id].composite.data_signal);
                        }
                    }

                    //=======>>> Experimental ============================================
                    // Need to change formats from GENERIC if Composite and Signal Description record only exists and
                    // format Not Generic!

                    if (request_data->request == static_cast<int>(Request::ReadGeneric) && request_data->exp_number <= 0) {
                        request_data->request = static_cast<int>(Request::ReadXML);
                    }

                    //=======>>>==========================================================

                    if (request_data->request == static_cast<int>(Request::ReadXML) || request_data->exp_number <= 0) {
                        if ((strlen(actions_comp_desc.action[comp_id].composite.file) == 0 ||
                             strlen(actions_comp_desc.action[comp_id].composite.format) == 0) &&
                            request_block2.exp_number <= 0) {
                            free_actions(&actions_comp_desc);
                            free_actions(&actions_comp_sig);
                            (*depth)--;
                            UDA_THROW(8888,
                                            "User Specified Composite Data Signal Not Fully Defined: Format?, File?");
                        }
                        strcpy(request_block2.path, actions_comp_desc.action[comp_id].composite.file);

                        auto [id, maybe_plugin] = _plugins.find_by_name(actions_comp_desc.action[comp_id].composite.format);
                        if (maybe_plugin) {
                            request_block2.request = id;
                        } else {
                            request_block2.request = static_cast<int>(Request::ReadUnknown);
                        }

                        if (request_block2.request == static_cast<int>(Request::ReadUnknown)) {
                            if (actions_comp_desc.action[comp_id].composite.format[0] == '\0' &&
                                request_block2.exp_number > 0) {
                                request_block2.request = static_cast<int>(Request::ReadGeneric);
                            } else {
                                free_actions(&actions_comp_desc);
                                free_actions(&actions_comp_sig);
                                (*depth)--;
                                UDA_THROW(8889,
                                                "User Specified Composite Data Signal's File Format NOT Recognised");
                            }
                        }

                        if (request_block2.request == static_cast<int>(Request::ReadHDF5)) {
                            meta_data.set("path", request_block2.exp_number); // HDF5 File Location
                            meta_data.set("signal_name", request_block2.signal); // HDF5 Variable Name
                        }
                    }

                    // Does the request type need an SQL socket?
                    // This is not passed back via the argument as only a 'by value' pointer is specified.
                    // Assign to a global to pass back - poor design that needs correcting at a later date!

                    // If the Archive is XML and the signal contains a ServerSide Subset function then parse and replace

                    if (STR_IEQUALS(request_block2.archive, "XML") &&
                        (strstr(request_block2.signal, "SS::Subset") != nullptr ||
                         strstr(request_block2.signal, "SERVERSIDE::Subset") != nullptr)) {
                        strcpy(request_block2.archive, "SS");
                        char* p = strstr(request_block2.signal, "::Subset");
                        strcpy(request_block2.signal, &p[2]);
                    }

                    UDA_LOG(UDA_LOG_DEBUG, "Reading Composite Signal DATA");

                    // Recursive Call for True Data with XML Transformations Applied and Associated Meta Data

                    UDA_LOG(UDA_LOG_DEBUG, "Reading Composite Signal DATA");

                    rc = get_data(depth, &request_block2, data_block, 0);

                    free_actions(&_actions_desc); // Added 06Nov2008
                    free_actions(&_actions_sig);

                    if (rc != 0) { // Error
                        free_actions(&actions_comp_desc);
                        free_actions(&actions_comp_sig);
                        (*depth)--;
                        return rc;
                    }

                    // Has a Time Dimension been Identified?

                    if (actions_comp_desc.action[comp_id].composite.order > -1) {
                        data_block->order = actions_comp_desc.action[comp_id].composite.order;
                    }
                } else {
                    if (rc == -1 || rc == 1) {
                        free_actions(&actions_comp_desc);
                        free_actions(&actions_comp_sig);
                        (*depth)--;
                        UDA_THROW(7770, "Composite Data Signal Not Available - No XML Document to define it!");
                    }
                }
            }
        }

    } else {
        is_derived = 0;
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // Parse Qualifying Actionable XML

    if (is_derived) {
        // All Actions are applicable to the Derived/Composite Data Structure
        copy_actions(&_actions_desc, &actions_comp_desc);
        copy_actions(&_actions_sig, &actions_comp_sig);
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "parsing XML for a Regular Signal");

        if (!client_block_.get_asis) {

            // Regular Signal
            rc = server_parse_signal_xml(error_stack_, meta_data, &_actions_desc, &_actions_sig);

            if (rc == -1) {
                if (!serverside) {
                    (*depth)--;
                    return 0; // No XML to Apply so No More to be Done!
                }
            } else {
                if (rc == 1) {
                    (*depth)--;
                    UDA_THROW(7770, "Error Parsing Signal XML Document");
                }
            }
        } else {
            (*depth)--;
            return 0; // Ignore All XML so nothing to be done! Done!
        }
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // Swap Error Data if Required

    // ***************************   Need to Replicate the process used with Dimension Replacement?
    // ***************************

    if (is_derived && comp_id > -1) {

        if (strlen(_actions_desc.action[comp_id].composite.error_signal) > 0) {

            UDA_LOG(UDA_LOG_DEBUG, "Substituting Error Data: {}",
                    _actions_desc.action[comp_id].composite.error_signal);

            request_block2 = *request_data;
            strcpy(request_block2.signal, _actions_desc.action[comp_id].composite.error_signal);

            // Recursive Call for Error Data

            init_actions(&actions_comp_desc2);
            init_actions(&actions_comp_sig2);
            init_data_block(&data_block2);

            // Check if the source file was originally defined in the client API?

            if (original_xml) {
                meta_data.set("format", request_data->format);
                meta_data.set("path", request_data->path);
                meta_data.set("filename", request_data->file);
            }

            rc = get_data(depth, request_data, data_block, 0);

            free_actions(&actions_comp_desc2);
            free_actions(&actions_comp_sig2);

            if (rc != 0) {
                free_data_block(&data_block2);
                (*depth)--;
                return rc;
            }

            // Replace Error Data

            rc = swap_signal_error(data_block, &data_block2, 0);
            free_data_block(&data_block2);

            if (rc != 0) {
                (*depth)--;
                return rc;
            }
        }

        if (strlen(_actions_desc.action[comp_id].composite.aserror_signal) > 0) {

            UDA_LOG(UDA_LOG_DEBUG, "Substituting Asymmetric Error Data: {}",
                    _actions_desc.action[comp_id].composite.aserror_signal);

            request_block2 = *request_data;
            strcpy(request_block2.signal, _actions_desc.action[comp_id].composite.aserror_signal);

            // Recursive Call for Error Data

            init_actions(&actions_comp_desc2);
            init_actions(&actions_comp_sig2);
            init_data_block(&data_block2);

            // Check if the source file was originally defined in the client API?

            if (original_xml) {
                meta_data.set("format", request_data->format);
                meta_data.set("path", request_data->pass);
                meta_data.set("filename", request_data->file);
            }

            rc = get_data(depth, &request_block2, data_block, 0);

            free_actions(&actions_comp_desc2);
            free_actions(&actions_comp_sig2);

            if (rc != 0) {
                free_data_block(&data_block2);
                (*depth)--;
                return rc;
            }

            // Replace Error Data

            rc = swap_signal_error(data_block, &data_block2, 1);
            free_data_block(&data_block2);

            if (rc != 0) {
                (*depth)--;
                return rc;
            }
        }
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // Swap Dimension Data if Required

    if (is_derived && comp_id > -1) {
        for (int i = 0; i < _actions_desc.action[comp_id].composite.ndimensions; i++) {
            if (_actions_desc.action[comp_id].composite.dimensions[i].dimType == (int)ActionDimType::Composite) {
                if (strlen(_actions_desc.action[comp_id].composite.dimensions[i].dimcomposite.dim_signal) > 0) {

                    UDA_LOG(UDA_LOG_DEBUG, "Substituting Dimension Data");

                    strcpy(request_block2.format,
                           "GENERIC"); // Database Lookup if not specified in XML or by Client

                    // Replace signal name re-using the Local Working REQUEST Block

                    strcpy(request_block2.signal,
                           _actions_desc.action[comp_id].composite.dimensions[i].dimcomposite.dim_signal);

                    // Replace other properties if defined by the original client request or the XML DIMCOMPOSITE record

                    if (strlen(request_data->path) > 0) {
                        strcpy(request_block2.path, request_data->file);
                    }
                    if (strlen(request_data->format) > 0) {
                        strcpy(request_block2.format, request_data->format);
                    }

                    if (strlen(_actions_desc.action[comp_id].composite.file) > 0) {
                        strcpy(request_block2.path, _actions_desc.action[comp_id].composite.file);
                    }

                    if (strlen(_actions_desc.action[comp_id].composite.format) > 0) {
                        strcpy(request_block2.format, _actions_desc.action[comp_id].composite.format);
                    }

                    if (strlen(_actions_desc.action[comp_id].composite.dimensions[i].dimcomposite.file) > 0) {
                        strcpy(request_block2.path,
                               _actions_desc.action[comp_id].composite.dimensions[i].dimcomposite.file);
                    }

                    if (strlen(_actions_desc.action[comp_id].composite.dimensions[i].dimcomposite.format) > 0) {
                        strcpy(request_block2.format,
                               _actions_desc.action[comp_id].composite.dimensions[i].dimcomposite.format);
                    }

                    // Recursive Call for Data

                    init_actions(&actions_comp_desc2);
                    init_actions(&actions_comp_sig2);
                    init_data_block(&data_block2);

                    // Check if the source file was originally defined in the client API?

                    meta_data.set("format", request_block2.format);
                    meta_data.set("path", request_block2.path);
                    meta_data.set("signal_name", trim_string(request_block2.signal));

                    auto [id, maybe_plugin] = _plugins.find_by_name(request_block2.format);
                    if (maybe_plugin) {
                        request_block2.request = id;
                    } else {
                        request_block2.request = (int)Request::ReadUnknown;
                    }

                    if (request_block2.request == (int)Request::ReadUnknown) {
                        free_actions(&actions_comp_desc2);
                        free_actions(&actions_comp_sig2);
                        (*depth)--;
                        UDA_THROW(9999,
                                        "User Specified Composite Dimension Data Signal's File Format NOT Recognised");
                    }

                    // If the Archive is XML and the signal contains a ServerSide Subset function then parse and replace

                    if ((strstr(request_block2.signal, "SS::Subset") != nullptr ||
                         strstr(request_block2.signal, "SERVERSIDE::Subset") != nullptr)) {
                        strcpy(request_block2.archive, "SS");
                        char* p = strstr(request_block2.signal, "::Subset");
                        strcpy(request_block2.signal, &p[2]);
                    }

                    // Recursive call

                    rc = get_data(depth, &request_block2, data_block, 0);

                    free_actions(&actions_comp_desc2);
                    free_actions(&actions_comp_sig2);

                    if (rc != 0) {
                        free_data_block(&data_block2);
                        (*depth)--;
                        return rc;
                    }

                    // Replace Dimension Data

                    rc = swap_signal_dim(_actions_desc.action[comp_id].composite.dimensions[i].dimcomposite, data_block,
                                         &data_block2);

                    free_data_block(&data_block2);

                    if (rc != 0) {
                        (*depth)--;
                        return rc;
                    }
                }

                if (strlen(_actions_desc.action[comp_id].composite.dimensions[i].dimcomposite.dim_error) > 0) {

                    UDA_LOG(UDA_LOG_DEBUG, "Substituting Dimension Error Data");

                    request_block2 = *request_data;
                    strcpy(request_block2.signal,
                           _actions_desc.action[comp_id].composite.dimensions[i].dimcomposite.dim_error);

                    // Recursive Call for Data

                    init_actions(&actions_comp_desc2);
                    init_actions(&actions_comp_sig2);
                    init_data_block(&data_block2);

                    // Check if the source file was originally defined in the client API?

                    if (original_xml) {
                        meta_data.set("format", request_data->format);
                        meta_data.set("path", request_data->path);
                        meta_data.set("filename", request_data->file);
                    }

                    rc = get_data(depth, &request_block2, data_block, 0);

                    free_actions(&actions_comp_desc2);
                    free_actions(&actions_comp_sig2);

                    if (rc != 0) {
                        free_data_block(&data_block2);
                        (*depth)--;
                        return rc;
                    }

                    // Replace Dimension Error Data

                    rc = swap_signal_dim_error(_actions_desc.action[comp_id].composite.dimensions[i].dimcomposite,
                                               data_block, &data_block2, 0);

                    free_data_block(&data_block2);

                    if (rc != 0) {
                        (*depth)--;
                        return rc;
                    }
                }

                if (strlen(_actions_desc.action[comp_id].composite.dimensions[i].dimcomposite.dim_aserror) > 0) {

                    UDA_LOG(UDA_LOG_DEBUG, "Substituting Dimension Asymmetric Error Data");

                    request_block2 = *request_data;
                    strcpy(request_block2.signal,
                           _actions_desc.action[comp_id].composite.dimensions[i].dimcomposite.dim_aserror);

                    // Recursive Call for Data

                    init_actions(&actions_comp_desc2);
                    init_actions(&actions_comp_sig2);
                    init_data_block(&data_block2);

                    // Check if the source file was originally defined in the client API?

                    if (original_xml) {
                        meta_data.set("format", request_data->format);
                        meta_data.set("path", request_data->path);
                        meta_data.set("filename", request_data->file);
                    }

                    rc = get_data(depth, &request_block2, data_block, 0);

                    free_actions(&actions_comp_desc2);
                    free_actions(&actions_comp_sig2);

                    if (rc != 0) {
                        free_data_block(&data_block2);
                        (*depth)--;
                        return rc;
                    }

                    // Replace Dimension Asymmetric Error Data

                    rc = swap_signal_dim_error(_actions_desc.action[comp_id].composite.dimensions[i].dimcomposite,
                                               data_block, &data_block2, 1);
                    free_data_block(&data_block2);

                    if (rc != 0) {
                        (*depth)--;
                        return rc;
                    }
                }
            }
        }
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // Apply Any Labeling, Timing Offsets and Calibration Actions to Data and Dimension (no Data or Dimension
    // substituting)

    UDA_LOG(UDA_LOG_DEBUG, "#Timing Before XML");
    print_data_block(*data_block);

    if (!client_block_.get_asis) {

        // All Signal Actions have Precedence over Signal_Desc Actions: Deselect if there is a conflict

        server_deselect_signal_xml(&_actions_desc, &_actions_sig);

        server_apply_signal_xml(client_block_, &meta_data, data_block, _actions_desc);
        server_apply_signal_xml(client_block_, &meta_data, data_block, _actions_sig);
    }

    UDA_LOG(UDA_LOG_DEBUG, "#Timing After XML");
    print_data_block(*data_block);

    //--------------------------------------------------------------------------------------------------------------------------
    // Subset Data or Map Data when all other actions have been applied

    if (is_derived && comp_id > -1) {
        UDA_LOG(UDA_LOG_DEBUG, "Calling server_subset_data (Derived)  {}", *depth);
        print_data_block(*data_block);

        if ((rc = server_subset_data(data_block, _actions_desc.action[comp_id], _log_malloc_list)) != 0) {
            (*depth)--;
            return rc;
        }
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // Subset Operations

    if (!serverside && !is_derived && meta_data.find("type") == "S") {
        for (int i = 0; i < _actions_desc.nactions; i++) {
            if (_actions_desc.action[i].actionType == (int)ActionType::Subset) {
                UDA_LOG(UDA_LOG_DEBUG, "Calling server_subset_data (Subset)   {}", *depth);
                print_data_block(*data_block);

                if ((rc = server_subset_data(data_block, _actions_desc.action[i], _log_malloc_list)) != 0) {
                    (*depth)--;
                    return rc;
                }
            }
        }
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // Server Side Operations

    if (serverside) {
        for (int i = 0; i < actions_serverside.nactions; i++) {
            if (actions_serverside.action[i].actionType == (int)ActionType::ServerSide) {
                for (int j = 0; j < actions_serverside.action[i].serverside.nsubsets; j++) {
                    UDA_LOG(UDA_LOG_DEBUG, "Calling server_subset_data (Serverside)   {}", *depth);
                    print_data_block(*data_block);

                    if ((rc = server_subset_data(data_block, actions_serverside.action[i], _log_malloc_list)) != 0) {
                        (*depth)--;
                        return rc;
                    }
                }
            }
        }
        free_actions(&actions_serverside);
    }

    //--------------------------------------------------------------------------------------------------------------------------

    (*depth)--;
    return 0;
}

int uda::server::Server::read_data(RequestData* request, DataBlock* data_block)
{
    // If err = 0 then standard signal data read
    // If err > 0 then an error occured
    // If err < 0 then unable to read signal because it is a derived type and details are in XML format

    char mapping[MaxMeta] = "";

    print_request_data(*request);

    //------------------------------------------------------------------------------
    // Test for Subsetting or Mapping XML: These require parsing First to identify the data signals needed.
    // The exception is XML defining Composite signals. These have a specific Request type.
    //------------------------------------------------------------------------------
#ifndef PROXYSERVER
    if (request->request != (int)Request::ReadXML) {
        if (STR_STARTSWITH(request->signal, "<?xml")) {
            _meta_data.set("type", 'C'); // Composite/Derived Type
            _meta_data.set("signal_name", ""); // The true signal is contained in the XML
            _meta_data.set("xml", request->signal); // XML is passed via the signal string
            _meta_data.set("format", request->format);
            _meta_data.set("path", request->path);
            _meta_data.set("filename", request->file);
            return -1;
        }
    }
#endif

    //------------------------------------------------------------------------------
    // Identify a Signal Mapping from a Legacy Name to an Alternative Name/Source
    // Conditional on client signal requested is not XML
    // Same archive and device assumed
    //
    // Overwrite request_block entries - Private to this function instance
    //
    // If the source is a private file, then ignore any xml corrections saved with the signal_desc record
    // Otherwise, respect xml corrections and append new mapping xml
    //
    // If the source is a private file and the Legacy Name has No Alternative record, then fail access
    // If the source is Not a private file (Generic Access method) and the Legacy Name has No Alternative record and
    // the Source Alias associated with the Legacy Name (Signal_Desc table) is Not used by ANY Alternative mapping with
    // the same Rank, then allow normal Generic lookup.
    //------------------------------------------------------------------------------
#ifndef PROXYSERVER
    if (client_block_.clientFlags & client_flags::AltData && request->request != (int)Request::ReadXML &&
        STR_STARTSWITH(request->signal, "<?xml")) {

        if (request->request != (int)Request::ReadGeneric) {
            // Must be a Private File so switch signal names
            strcpy(request->signal, _meta_data.find("signal_name").data());  // Alias or Generic have no context wrt private files
            _meta_data.set("xml", mapping); // Only mapping XML is applicable
            if (mapping[0] != '\0') {
                _meta_data.set("type", 'S');
            } // Switched data with mapping Transform in XML
        } else {
            if (!_meta_data.find("signal_alias").empty()) {
                strcpy(request->signal, _meta_data.find("signal_alias").data()); // Alias or Generic name is what is passed into sqlGeneric
            }
        }
    }
#endif
    //------------------------------------------------------------------------------
    // Identify the Signal Required from the Database if a Generic Signal Requested
    // Plugin sourced data (type 'P') will fail as there is no entry in the DataSource table so ignore
    //------------------------------------------------------------------------------

    if (request->request == (int)Request::ReadGeneric) {

        // Identify the required Plugin

        auto maybe_plugin = find_metadata_plugin(config_, _plugins);
        if (!maybe_plugin) {
            // No plugin so not possible to identify the requested data item
            UDA_THROW(778, "Unable to identify requested data item");
        }

        // If the plugin is registered as a FILE or LIBRARY type then call the default method as no method will have
        // been specified

        copy_string(maybe_plugin->default_method, request->function, StringLength);

        // Execute the plugin to resolve the identity of the data requested

        int err = call_metadata_plugin(config_, maybe_plugin.get(), request, _plugins, _meta_data);

        if (err != 0) {
            UDA_THROW(err, "No Record Found for this Generic Signal");
        }
        UDA_LOG(UDA_LOG_DEBUG, "Metadata Plugin Executed\nSignal Type: {}", _meta_data.find("type").data());

        // Plugin? Create a new Request Block to identify the request_id

        if (_meta_data.find("type") == "P") {
            strcpy(request->signal, _meta_data.find("signal_name").data());
            strcpy(request->source, _meta_data.find("path").data());
            make_server_request_data(config_, request, _plugins);
        }

    } // end of Request::ReadGeneric

    // Placeholder name-value substitution and additional name-value pairs
    // Modifies HEAP in request_block

    {
        int err = name_value_substitution(error_stack_, request->name_value_list, request->tpass);
        if (err != 0) {
            return err;
        }
    }

    //------------------------------------------------------------------------------
    // Client XML Specified Composite Signal
    //------------------------------------------------------------------------------

    if (request->request == static_cast<int>(Request::ReadXML)) {
        if (strlen(request->signal) > 0) {
            _meta_data.set("xml", request->signal); // XML is passed via the signal string
        } else if (strlen(request->path) > 0) {
            // XML is passed via a file
            FILE* xmlfile = nullptr;
            int nchar;
            errno = 0;
            xmlfile = fopen(request->path, "r");
            int serrno = errno;
            if (serrno != 0 || xmlfile == nullptr) {
                if (serrno != 0) {
                    add_error(error_stack_, ErrorType::System, "idamserverReadData", serrno, "");
                }
                if (xmlfile != nullptr) {
                    fclose(xmlfile);
                }
                UDA_THROW(122, "Unable to Open the XML File defining the signal");
            }
            nchar = 0;
            while (!feof(xmlfile) && nchar < MaxMeta) {
                request->signal[nchar++] = (char)getc(xmlfile);
            }
            request->signal[nchar - 2] = '\0'; // Remove EOF Character and replace with String Terminator
            _meta_data.set("xml", request->signal);
            fclose(xmlfile);
        } else {
            UDA_THROW(123, "There is NO XML defining the signal");
        }
        _meta_data.set("type", 'C');
        return -1;
    }

    //------------------------------------------------------------------------------
    // Read Data via a Suitable Registered Plugin using a standard interface
    //------------------------------------------------------------------------------

    // Test for known File formats and Server protocols

    {
        UdaPluginInterface plugin_interface;

        UDA_LOG(UDA_LOG_DEBUG, "creating the plugin interface structure");

        // Initialise the Data Block

        init_data_block(data_block);

        auto& plugin_list = _plugins.plugin_list();
        plugin_interface.interface_version = 1;
        plugin_interface.data_block = data_block;
        plugin_interface.client_block = &client_block_;
        plugin_interface.request_data = request;
        plugin_interface.meta_data = &_meta_data;
        plugin_interface.house_keeping = false;
        plugin_interface.change_plugin = false;
        plugin_interface.pluginList = &plugin_list;
        plugin_interface.user_defined_type_list = _user_defined_type_list;
        plugin_interface.log_malloc_list = _log_malloc_list;
        plugin_interface.error_stack = {};
        plugin_interface.config = &config_;

        int plugin_request = (int)Request::ReadUnknown;

        if (request->request != (int)Request::ReadGeneric && request->request != static_cast<int>(Request::ReadUnknown)) {
            plugin_request = request->request; // User has Specified a Plugin
            UDA_LOG(UDA_LOG_DEBUG, "Plugin Request {}", plugin_request);
        } else {
            const auto format = _meta_data.find("format");
            auto [id, maybe_plugin] = _plugins.find_by_name(format);
            if (maybe_plugin) {
                plugin_request = id;
                UDA_LOG(UDA_LOG_DEBUG, "findPluginRequestByFormat Plugin Request {}", plugin_request);
            }
        }

        UDA_LOG(UDA_LOG_DEBUG, "Number of PutData Blocks: {}", request->putDataBlockList.size());

        if (plugin_request != (int)Request::ReadUnknown) {

            auto maybe_plugin = _plugins.find_by_id(plugin_request);
            if (!maybe_plugin) {
                UDA_LOG(UDA_LOG_DEBUG, "Error locating data plugin {}", plugin_request);
                UDA_THROW(999, "Error locating data plugin");
            }

#ifndef ITERSERVER
            auto external_user = config_.get("server.external_user").as_or_default(false);
            if (maybe_plugin.get().is_private == UDA_PLUGIN_PRIVATE && external_user) {
                UDA_THROW(999, "Access to this data class is not available.");
            }
#endif
            if (maybe_plugin->handle != nullptr &&
                maybe_plugin->entry_func != nullptr) {

                const auto format = _meta_data.find("format");
                UDA_LOG(UDA_LOG_DEBUG, "[{}] {} Plugin Selected", plugin_request, format.data());

#ifndef FATCLIENT
                // Redirect Output to temporary file if no file handles passed
                int reset = 0;
                int rc;
                if ((rc = server_redirect_std_streams(config_, reset)) != 0) {
                    UDA_THROW(rc, "Error Redirecting Plugin Message Output");
                }
#endif

                // Call the plugin
                int err = maybe_plugin->entry_func(&plugin_interface);
                for (const auto& error : plugin_interface.error_stack) {
                    add_error(error_stack_, error.type, error.location, error.code, error.msg);
                }

#ifndef FATCLIENT
                // Reset Redirected Output
                reset = 1;
                if ((rc = server_redirect_std_streams(config_, reset)) != 0) {
                    UDA_THROW(rc, "Error Resetting Redirected Plugin Message Output");
                }
#endif

                if (err != 0) {
                    return err;
                }

                UDA_LOG(UDA_LOG_DEBUG, "returned from plugin called");

                // Save Provenance with socket stream protection

                server_redirect_std_streams(config_, 0);
                provenance_plugin(config_, &client_block_, request, _plugins, nullptr, _meta_data);
                server_redirect_std_streams(config_, 1);

                // If no structures to pass back (only regular data) then free the user defined type list

                if (data_block->opaque_block == nullptr) {
                    if (data_block->opaque_type == UDA_OPAQUE_TYPE_STRUCTURES && data_block->opaque_count > 0) {
                        UDA_THROW(999, "Opaque Data Block is Null Pointer");
                    }
                }

                if (!plugin_interface.change_plugin) {
                    // job done!
                    return 0;
                }

                request->request = static_cast<int>(Request::ReadGeneric); // Use a different Plugin
            }
        }
    }

    int plugin_request = static_cast<int>(Request::ReadUnknown);

    if (request->request != static_cast<int>(Request::ReadGeneric)) {
        plugin_request = request->request; // User API has Specified a Plugin
    } else {

        // Test for known File formats and Server protocols

        const auto format = _meta_data.find("format");
        auto [id, maybe_plugin] = _plugins.find_by_name(format);

        const auto external_user = config_.get("server.external_user");
        if (maybe_plugin && maybe_plugin->is_private == UDA_PLUGIN_PRIVATE && external_user) {
            UDA_THROW(999, "Access to this data class is not available.");
        }

        // Don't append the file name to the path - if it's already present!

        if (!boost::ends_with(_meta_data.find("path"), _meta_data.find("file"))) {
            if (_meta_data.find("path").size() + _meta_data.find("file").size() + 1 < MaxPath) {
                const auto new_path = std::string{_meta_data.find("path")} + "/" + _meta_data.find("file").data();
                _meta_data.set("path", new_path);
            } else {
                UDA_THROW(999, "Path + Filename too long");
            }
        }

        if (maybe_plugin) {
            plugin_request = id;
        }
    }

    if (plugin_request == static_cast<int>(Request::ReadUnknown)) {
        UDA_LOG(UDA_LOG_DEBUG, "No Plugin Selected");
    }
    UDA_LOG(UDA_LOG_DEBUG, "Archive      : {} ", _meta_data.find("archive").data());
    UDA_LOG(UDA_LOG_DEBUG, "Device Name  : {} ", _meta_data.find("device_name").data());
    UDA_LOG(UDA_LOG_DEBUG, "Signal Name  : {} ", _meta_data.find("signal_name").data());
    UDA_LOG(UDA_LOG_DEBUG, "File Path    : {} ", _meta_data.find("path").data());
    UDA_LOG(UDA_LOG_DEBUG, "File Name    : {} ", _meta_data.find("filename").data());
    UDA_LOG(UDA_LOG_DEBUG, "Pulse Number : {} ", _meta_data.find("exp_number").data());
    UDA_LOG(UDA_LOG_DEBUG, "Pass Number  : {} ", _meta_data.find("pass").data());

    //----------------------------------------------------------------------------
    // Initialise the Data Block Structure

    init_data_block(data_block);

    //----------------------------------------------------------------------------
    // Status values

    if (request->request == static_cast<int>(Request::ReadGeneric)) {
        data_block->source_status = _meta_data.find_as<int>("source.status");
        data_block->signal_status = _meta_data.find_as<int>("signal.status");
    }

    //----------------------------------------------------------------------------
    // Copy the Client Block into the Data Block to pass client requested properties into plugins

    data_block->client_block = client_block_;

    //----------------------------------------------------------------------------
    // Save Provenance with socket stream protection

    server_redirect_std_streams(config_, 0);
    provenance_plugin(config_, &client_block_, request, _plugins, nullptr, _meta_data);
    server_redirect_std_streams(config_, 1);

    return 0;
}
