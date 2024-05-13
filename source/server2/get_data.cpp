#include "get_data.hpp"

#include "apply_XML.hpp"
#include "clientserver/errorLog.h"
#include "clientserver/initStructs.h"
#include "clientserver/nameValueSubstitution.h"
#include "clientserver/printStructs.h"
#include "common/stringUtils.h"
#include "logging/logging.h"
#include "make_server_request_block.hpp"
#include "plugins.hpp"
#include "server.hpp"
#include "server_plugin.h"
#include "server_subset_data.h"
#include "uda/plugins.h"
#include <uda/types.h>

using namespace uda::client_server;
using namespace uda::plugins;
using namespace uda::logging;

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
        UDA_THROW_ERROR(7777, "Error Data Substitution Not Possible - Incompatible Lengths");
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
            UDA_THROW_ERROR(7777, "Dimension Data Substitution Not Possible - Incompatible Lengths");
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
                UDA_THROW_ERROR(7777, "Dimension Data Substitution Not Possible - Incompatible Lengths");
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
            UDA_THROW_ERROR(7777, "Dimension Error Data Substitution Not Possible - Incompatible Lengths");
        }
    }
    return 0;
}

int uda::server::Server::get_data(int* depth, RequestData* request_data, DataBlock* data_block, int protocol_version)
{
    int isDerived = 0, compId = -1, serverside = 0;

    RequestData request_block2;
    DataBlock data_block2;
    DataSource data_source2;
    Signal signal_rec2;
    SignalDesc signal_desc2;
    Actions actions_serverside;
    Actions actions_comp_desc, actions_comp_sig;
    Actions actions_comp_desc2, actions_comp_sig2;

    static int original_request = 0; // First entry value of the Plugin Request
    static int original_xml = 0;     // First entry flag that XML was passed in

    //--------------------------------------------------------------------------------------------------------------------------
    // Retain the original request (Needed to flag that signal/file details are in the Request or Action structures)

#ifndef PROXYSERVER
    if (original_request == 0 || *depth == 0) {
        original_request = request_data->request;
        if (request_data->request != REQUEST_READ_XML) {
            if (STR_STARTSWITH(request_data->signal, "<?xml")) {
                original_xml = 1;
            }
        }
    }

    SignalDesc* signal_desc = &_metadata_block.signal_desc;

    if (original_xml == 1 && *depth == 1) {
        _metadata_block.signal_desc.xml[0] = '\0';
    } // remove redirected XML after first recursive pass
#endif

    //--------------------------------------------------------------------------------------------------------------------------
    // Limit the Recursive Depth

    if (*depth == UDA_XML_MAX_RECURSIVE) {
        UDA_THROW_ERROR(7777, "Recursive Depth (Derived or Substitute Data) Exceeds Internal Limit");
    }

    (*depth)++;

    UDA_LOG(UDA_LOG_DEBUG, "GetData Recursive Depth = {}", *depth);

    // Can't use REQUEST_READ_SERVERSIDE because data must be read first using a 'real' data reader or
    // REQUEST_READ_GENERIC

    if (protocol_version < 6) {
        if (STR_IEQUALS(request_data->archive, "SS") || STR_IEQUALS(request_data->archive, "SERVERSIDE")) {
            if (!strncasecmp(request_data->signal, "Subset(", 7)) {
                serverside = 1;
                init_actions(&actions_serverside);
                int rc;
                if ((rc = server_parse_server_side(_config, request_data, &actions_serverside)) != 0) {
                    return rc;
                }
                // Erase original Subset request
                copy_string(trim_string(request_data->signal), _metadata_block.signal_desc.signal_name, MAXNAME);
            }
        }
    } else if (STR_IEQUALS(request_data->function, "subset")) {
        auto [_, maybe_plugin] = _plugins.find_by_name(request_data->archive);
        if (maybe_plugin) {
            if (maybe_plugin.get().entry_func_name == "serverside") {
                serverside = 1;
                init_actions(&actions_serverside);
                int rc;
                if ((rc = server_parse_server_side(_config, request_data, &actions_serverside)) != 0) {
                    return rc;
                }
                // Erase original Subset request
                copy_string(trim_string(request_data->signal), _metadata_block.signal_desc.signal_name, MAXNAME);
            }
        }
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // Read the Data (Returns rc < 0 if the signal is a derived type or is defined in an XML document)

    auto data_source = &_metadata_block.data_source;
    auto signal_rec = &_metadata_block.signal_rec;

    int rc = read_data(request_data, data_block);

    UDA_LOG(UDA_LOG_DEBUG, "After read_data rc = {}", rc);
    UDA_LOG(UDA_LOG_DEBUG, "Is the Signal a Composite? {}", _metadata_block.signal_desc.type == 'C');

    if (rc > 0) {
        (*depth)--;
        return rc; // An Error Occurred
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // If the Request is Not for a Generic Signal then exit - No XML source to apply to data as it is just regular data.
    // Allow Composites (C) or Signal Switch (S) through regardless of request type

    if (_metadata_block.signal_desc.type != 'C' && !serverside && _metadata_block.signal_desc.type != 'S' &&
        (!(request_data->request == REQUEST_READ_GENERIC || request_data->request == REQUEST_READ_XML))) {
        return 0;
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // Is the Signal a Derived or Signal Composite?

    if (_metadata_block.signal_desc.type == 'C') {
        // The Signal is a Derived/Composite Type so Parse the XML for the data signal identity and read the data

        UDA_LOG(UDA_LOG_DEBUG, "Derived/Composite Signal {}", request_data->signal);

        isDerived = 1; // is True

        // derived_signal_desc     = *signal_desc;                // Preserve details of Derived Signal Description
        // Record
        _metadata_block.data_source.exp_number =
            request_data->exp_number;                          // Needed for Pulse Number Range Check in XML Parser
        _metadata_block.data_source.pass = request_data->pass; // Needed for a Pass/Sequence Range Check in XML Parser

        // Allways Parse Signal XML to Identify the True Data Source for this Pulse Number - not subject to client
        // request: get_asis (First Valid Action Record found only - others ignored)

        init_actions(&actions_comp_desc);
        init_actions(&actions_comp_sig);

        UDA_LOG(UDA_LOG_DEBUG, "parsing XML for a COMPOSITE Signal");

        rc = server_parse_signal_xml(*data_source, *signal_rec, *signal_desc, &actions_comp_desc, &actions_comp_sig);

        UDA_LOG(UDA_LOG_DEBUG, "parsing XML RC? {}", rc);

        if (rc > 0) {
            free_actions(&actions_comp_desc);
            free_actions(&actions_comp_sig);
            (*depth)--;
            UDA_THROW_ERROR(8881, "Unable to Parse XML");
        }

        // Identify which XML statements are in Range (Only signal_desc xml need be checked as signal xml is specific to
        // a single pulse/pass)

        compId = -1;
        if (rc == 0) {
            for (int i = 0; i < actions_comp_desc.nactions; i++) {
                if (actions_comp_desc.action[i].actionType == UDA_COMPOSITE_TYPE &&
                    actions_comp_desc.action[i].inRange) {
                    compId = i;
                    break; // First Record found only!
                }
            }

            // Identify the data's signal

            if (compId >= 0) {
                if (strlen(actions_comp_desc.action[compId].composite.data_signal) > 0) {
                    // If we haven't a True Signal then can't identify the data required!

                    request_block2 =
                        *request_data; // Preserve details of the Original User Request (Do Not FREE Elements)

                    strcpy(request_block2.signal,
                           actions_comp_desc.action[compId].composite.data_signal); // True Signal Identity

                    // Does this Composite originate from a subsetting operation? If so then fill out any missing items
                    // in the composite record

                    if (actions_comp_desc.action[compId].composite.nsubsets > 0 ||
                        actions_comp_desc.action[compId].composite.nmaps > 0 ||
                        (strlen(actions_comp_desc.action[compId].composite.file) == 0 &&
                         strlen(_metadata_block.data_source.path) > 0)) {

                        // ******** If there is No subset then composite.file is missing!!!

                        if (strlen(actions_comp_desc.action[compId].composite.file) == 0 &&
                            strlen(_metadata_block.data_source.path) > 0) {
                            strcpy(actions_comp_desc.action[compId].composite.file, _metadata_block.data_source.path);
                        }

                        if (strlen(actions_comp_desc.action[compId].composite.format) == 0 &&
                            strlen(_metadata_block.data_source.format) > 0) {
                            strcpy(actions_comp_desc.action[compId].composite.format,
                                   _metadata_block.data_source.format);
                        }

                        if (strlen(actions_comp_desc.action[compId].composite.data_signal) > 0 &&
                            strlen(_metadata_block.signal_desc.signal_name) == 0) {
                            strcpy(_metadata_block.signal_desc.signal_name,
                                   actions_comp_desc.action[compId].composite.data_signal);
                        }
                    }

                    //=======>>> Experimental ============================================
                    // Need to change formats from GENERIC if Composite and Signal Description record only exists and
                    // format Not Generic!

                    if (request_data->request == REQUEST_READ_GENERIC && request_data->exp_number <= 0) {
                        request_data->request = REQUEST_READ_XML;
                    }

                    //=======>>>==========================================================

                    if (request_data->request == REQUEST_READ_XML || request_data->exp_number <= 0) {
                        if ((strlen(actions_comp_desc.action[compId].composite.file) == 0 ||
                             strlen(actions_comp_desc.action[compId].composite.format) == 0) &&
                            request_block2.exp_number <= 0) {
                            free_actions(&actions_comp_desc);
                            free_actions(&actions_comp_sig);
                            (*depth)--;
                            UDA_THROW_ERROR(8888,
                                            "User Specified Composite Data Signal Not Fully Defined: Format?, File?");
                        }
                        strcpy(request_block2.path, actions_comp_desc.action[compId].composite.file);

                        auto [id, maybe_plugin] = _plugins.find_by_name(actions_comp_desc.action[compId].composite.format);
                        if (maybe_plugin) {
                            request_block2.request = id;
                        } else {
                            request_block2.request = REQUEST_READ_UNKNOWN;
                        }

                        if (request_block2.request == REQUEST_READ_UNKNOWN) {
                            if (actions_comp_desc.action[compId].composite.format[0] == '\0' &&
                                request_block2.exp_number > 0) {
                                request_block2.request = REQUEST_READ_GENERIC;
                            } else {
                                free_actions(&actions_comp_desc);
                                free_actions(&actions_comp_sig);
                                (*depth)--;
                                UDA_THROW_ERROR(8889,
                                                "User Specified Composite Data Signal's File Format NOT Recognised");
                            }
                        }

                        if (request_block2.request == REQUEST_READ_HDF5) {
                            strcpy(_metadata_block.data_source.path,
                                   trim_string(request_block2.path)); // HDF5 File Location
                            strcpy(_metadata_block.signal_desc.signal_name,
                                   trim_string(request_block2.signal)); // HDF5 Variable Name
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

                    if (actions_comp_desc.action[compId].composite.order > -1) {
                        data_block->order = actions_comp_desc.action[compId].composite.order;
                    }
                } else {
                    if (rc == -1 || rc == 1) {
                        free_actions(&actions_comp_desc);
                        free_actions(&actions_comp_sig);
                        (*depth)--;
                        UDA_THROW_ERROR(7770, "Composite Data Signal Not Available - No XML Document to define it!");
                    }
                }
            }
        }

    } else {
        isDerived = 0;
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // Parse Qualifying Actionable XML

    if (isDerived) {
        // All Actions are applicable to the Derived/Composite Data Structure
        copy_actions(&_actions_desc, &actions_comp_desc);
        copy_actions(&_actions_sig, &actions_comp_sig);
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "parsing XML for a Regular Signal");

        if (!_client_block.get_asis) {

            // Regular Signal
            rc = server_parse_signal_xml(*data_source, *signal_rec, *signal_desc, &_actions_desc, &_actions_sig);

            if (rc == -1) {
                if (!serverside) {
                    (*depth)--;
                    return 0; // No XML to Apply so No More to be Done!
                }
            } else {
                if (rc == 1) {
                    (*depth)--;
                    UDA_THROW_ERROR(7770, "Error Parsing Signal XML Document");
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

    if (isDerived && compId > -1) {

        if (strlen(_actions_desc.action[compId].composite.error_signal) > 0) {

            UDA_LOG(UDA_LOG_DEBUG, "Substituting Error Data: {}",
                    _actions_desc.action[compId].composite.error_signal);

            request_block2 = *request_data;
            strcpy(request_block2.signal, _actions_desc.action[compId].composite.error_signal);

            // Recursive Call for Error Data

            init_actions(&actions_comp_desc2);
            init_actions(&actions_comp_sig2);
            init_data_block(&data_block2);
            init_data_source(&data_source2);
            init_signal(&signal_rec2);
            init_signal_desc(&signal_desc2);

            // Check if the source file was originally defined in the client API?

            if (original_xml) {
                strcpy(data_source2.format, request_data->format);
                strcpy(data_source2.path, request_data->path);
                strcpy(data_source2.filename, request_data->file);
            }

            rc = get_data(depth, request_data, data_block, 0);

            free_actions(&actions_comp_desc2);
            free_actions(&actions_comp_sig2);

            if (rc != 0) {
                freeDataBlock(&data_block2);
                (*depth)--;
                return rc;
            }

            // Replace Error Data

            rc = swap_signal_error(data_block, &data_block2, 0);
            freeDataBlock(&data_block2);

            if (rc != 0) {
                (*depth)--;
                return rc;
            }
        }

        if (strlen(_actions_desc.action[compId].composite.aserror_signal) > 0) {

            UDA_LOG(UDA_LOG_DEBUG, "Substituting Asymmetric Error Data: {}",
                    _actions_desc.action[compId].composite.aserror_signal);

            request_block2 = *request_data;
            strcpy(request_block2.signal, _actions_desc.action[compId].composite.aserror_signal);

            // Recursive Call for Error Data

            init_actions(&actions_comp_desc2);
            init_actions(&actions_comp_sig2);
            init_data_block(&data_block2);

            // Check if the source file was originally defined in the client API?

            if (original_xml) {
                strcpy(data_source2.format, request_data->format);
                strcpy(data_source2.path, request_data->path);
                strcpy(data_source2.filename, request_data->file);
            }

            rc = get_data(depth, &request_block2, data_block, 0);

            free_actions(&actions_comp_desc2);
            free_actions(&actions_comp_sig2);

            if (rc != 0) {
                freeDataBlock(&data_block2);
                (*depth)--;
                return rc;
            }

            // Replace Error Data

            rc = swap_signal_error(data_block, &data_block2, 1);
            freeDataBlock(&data_block2);

            if (rc != 0) {
                (*depth)--;
                return rc;
            }
        }
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // Swap Dimension Data if Required

    if (isDerived && compId > -1) {
        for (int i = 0; i < _actions_desc.action[compId].composite.ndimensions; i++) {
            if (_actions_desc.action[compId].composite.dimensions[i].dimType == UDA_DIM_COMPOSITE_TYPE) {
                if (strlen(_actions_desc.action[compId].composite.dimensions[i].dimcomposite.dim_signal) > 0) {

                    UDA_LOG(UDA_LOG_DEBUG, "Substituting Dimension Data");

                    strcpy(request_block2.format,
                           "GENERIC"); // Database Lookup if not specified in XML or by Client

                    // Replace signal name re-using the Local Working REQUEST Block

                    strcpy(request_block2.signal,
                           _actions_desc.action[compId].composite.dimensions[i].dimcomposite.dim_signal);

                    // Replace other properties if defined by the original client request or the XML DIMCOMPOSITE record

                    if (strlen(request_data->path) > 0) {
                        strcpy(request_block2.path, request_data->file);
                    }
                    if (strlen(request_data->format) > 0) {
                        strcpy(request_block2.format, request_data->format);
                    }

                    if (strlen(_actions_desc.action[compId].composite.file) > 0) {
                        strcpy(request_block2.path, _actions_desc.action[compId].composite.file);
                    }

                    if (strlen(_actions_desc.action[compId].composite.format) > 0) {
                        strcpy(request_block2.format, _actions_desc.action[compId].composite.format);
                    }

                    if (strlen(_actions_desc.action[compId].composite.dimensions[i].dimcomposite.file) > 0) {
                        strcpy(request_block2.path,
                               _actions_desc.action[compId].composite.dimensions[i].dimcomposite.file);
                    }

                    if (strlen(_actions_desc.action[compId].composite.dimensions[i].dimcomposite.format) > 0) {
                        strcpy(request_block2.format,
                               _actions_desc.action[compId].composite.dimensions[i].dimcomposite.format);
                    }

                    // Recursive Call for Data

                    init_actions(&actions_comp_desc2);
                    init_actions(&actions_comp_sig2);
                    init_data_block(&data_block2);
                    init_signal_desc(&signal_desc2); // Added 06Nov2008

                    // Check if the source file was originally defined in the client API?

                    strcpy(data_source2.format, request_block2.format);
                    strcpy(data_source2.path, request_block2.path);
                    strcpy(signal_desc2.signal_name, trim_string(request_block2.signal));

                    auto [id, maybe_plugin] = _plugins.find_by_name(request_block2.format);
                    if (maybe_plugin) {
                        request_block2.request = id;
                    } else {
                        request_block2.request = REQUEST_READ_UNKNOWN;
                    }

                    if (request_block2.request == REQUEST_READ_UNKNOWN) {
                        free_actions(&actions_comp_desc2);
                        free_actions(&actions_comp_sig2);
                        (*depth)--;
                        UDA_THROW_ERROR(9999,
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
                        freeDataBlock(&data_block2);
                        (*depth)--;
                        return rc;
                    }

                    // Replace Dimension Data

                    rc = swap_signal_dim(_actions_desc.action[compId].composite.dimensions[i].dimcomposite, data_block,
                                         &data_block2);

                    freeDataBlock(&data_block2);

                    if (rc != 0) {
                        (*depth)--;
                        return rc;
                    }
                }

                if (strlen(_actions_desc.action[compId].composite.dimensions[i].dimcomposite.dim_error) > 0) {

                    UDA_LOG(UDA_LOG_DEBUG, "Substituting Dimension Error Data");

                    request_block2 = *request_data;
                    strcpy(request_block2.signal,
                           _actions_desc.action[compId].composite.dimensions[i].dimcomposite.dim_error);

                    // Recursive Call for Data

                    init_actions(&actions_comp_desc2);
                    init_actions(&actions_comp_sig2);
                    init_data_block(&data_block2);

                    // Check if the source file was originally defined in the client API?

                    if (original_xml) {
                        strcpy(data_source2.format, request_data->format);
                        strcpy(data_source2.path, request_data->path);
                        strcpy(data_source2.filename, request_data->file);
                    }

                    rc = get_data(depth, &request_block2, data_block, 0);

                    free_actions(&actions_comp_desc2);
                    free_actions(&actions_comp_sig2);

                    if (rc != 0) {
                        freeDataBlock(&data_block2);
                        (*depth)--;
                        return rc;
                    }

                    // Replace Dimension Error Data

                    rc = swap_signal_dim_error(_actions_desc.action[compId].composite.dimensions[i].dimcomposite,
                                               data_block, &data_block2, 0);

                    freeDataBlock(&data_block2);

                    if (rc != 0) {
                        (*depth)--;
                        return rc;
                    }
                }

                if (strlen(_actions_desc.action[compId].composite.dimensions[i].dimcomposite.dim_aserror) > 0) {

                    UDA_LOG(UDA_LOG_DEBUG, "Substituting Dimension Asymmetric Error Data");

                    request_block2 = *request_data;
                    strcpy(request_block2.signal,
                           _actions_desc.action[compId].composite.dimensions[i].dimcomposite.dim_aserror);

                    // Recursive Call for Data

                    init_actions(&actions_comp_desc2);
                    init_actions(&actions_comp_sig2);
                    init_data_block(&data_block2);

                    // Check if the source file was originally defined in the client API?

                    if (original_xml) {
                        strcpy(data_source2.format, request_data->format);
                        strcpy(data_source2.path, request_data->path);
                        strcpy(data_source2.filename, request_data->file);
                    }

                    rc = get_data(depth, &request_block2, data_block, 0);

                    free_actions(&actions_comp_desc2);
                    free_actions(&actions_comp_sig2);

                    if (rc != 0) {
                        freeDataBlock(&data_block2);
                        (*depth)--;
                        return rc;
                    }

                    // Replace Dimension Asymmetric Error Data

                    rc = swap_signal_dim_error(_actions_desc.action[compId].composite.dimensions[i].dimcomposite,
                                               data_block, &data_block2, 1);
                    freeDataBlock(&data_block2);

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

    if (!_client_block.get_asis) {

        // All Signal Actions have Precedence over Signal_Desc Actions: Deselect if there is a conflict

        server_deselect_signal_xml(&_actions_desc, &_actions_sig);

        server_apply_signal_xml(_client_block, data_source, signal_rec, signal_desc, data_block, _actions_desc);
        server_apply_signal_xml(_client_block, data_source, signal_rec, signal_desc, data_block, _actions_sig);
    }

    UDA_LOG(UDA_LOG_DEBUG, "#Timing After XML");
    print_data_block(*data_block);

    //--------------------------------------------------------------------------------------------------------------------------
    // Subset Data or Map Data when all other actions have been applied

    if (isDerived && compId > -1) {
        UDA_LOG(UDA_LOG_DEBUG, "Calling server_subset_data (Derived)  {}", *depth);
        print_data_block(*data_block);

        if ((rc = server_subset_data(data_block, _actions_desc.action[compId], _log_malloc_list)) != 0) {
            (*depth)--;
            return rc;
        }
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // Subset Operations

    if (!serverside && !isDerived && _metadata_block.signal_desc.type == 'S') {
        for (int i = 0; i < _actions_desc.nactions; i++) {
            if (_actions_desc.action[i].actionType == UDA_SUBSET_TYPE) {
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
            if (actions_serverside.action[i].actionType == UDA_SERVER_SIDE_TYPE) {
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

    char mapping[MAXMETA] = "";

    print_request_data(*request);

    //------------------------------------------------------------------------------
    // Test for Subsetting or Mapping XML: These require parsing First to identify the data signals needed.
    // The exception is XML defining Composite signals. These have a specific Request type.
    //------------------------------------------------------------------------------
#ifndef PROXYSERVER
    if (request->request != REQUEST_READ_XML) {
        if (STR_STARTSWITH(request->signal, "<?xml")) {
            _metadata_block.signal_desc.type = 'C';                   // Composite/Derived Type
            _metadata_block.signal_desc.signal_name[0] = '\0';        // The true signal is contained in the XML
            strcpy(_metadata_block.signal_desc.xml, request->signal); // XML is passed via the signal string
            strcpy(_metadata_block.data_source.format, request->format);
            strcpy(_metadata_block.data_source.path, request->path);
            strcpy(_metadata_block.data_source.filename, request->file);
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
    if (_client_block.clientFlags & CLIENTFLAG_ALTDATA && request->request != REQUEST_READ_XML &&
        STR_STARTSWITH(request->signal, "<?xml")) {

        if (request->request != REQUEST_READ_GENERIC) {
            // Must be a Private File so switch signal names
            strcpy(request->signal,
                   _metadata_block.signal_desc.signal_name);  // Alias or Generic have no context wrt private files
            _metadata_block.signal_desc.xml[0] = '\0';        // No corrections to private data files
            strcpy(_metadata_block.signal_desc.xml, mapping); // Only mapping XML is applicable
            if (mapping[0] != '\0') {
                _metadata_block.signal_desc.type = 'S';
            } // Switched data with mapping Transform in XML
        } else {
            if (_metadata_block.signal_desc.signal_alias[0] != '\0') {
                strcpy(request->signal, _metadata_block.signal_desc
                                            .signal_alias); // Alias or Generic name is what is passed into sqlGeneric
            }
        }
    }
#endif
    //------------------------------------------------------------------------------
    // Identify the Signal Required from the Database if a Generic Signal Requested
    // Plugin sourced data (type 'P') will fail as there is no entry in the DataSource table so ignore
    //------------------------------------------------------------------------------

    if (request->request == REQUEST_READ_GENERIC) {

        // Identify the required Plugin

        auto maybe_plugin = find_metadata_plugin(_config, _plugins);
        if (!maybe_plugin) {
            // No plugin so not possible to identify the requested data item
            UDA_THROW_ERROR(778, "Unable to identify requested data item");
        }

        // If the plugin is registered as a FILE or LIBRARY type then call the default method as no method will have
        // been specified

        copy_string(maybe_plugin->default_method, request->function, STRING_LENGTH);

        // Execute the plugin to resolve the identity of the data requested

        int err = call_metadata_plugin(_config, maybe_plugin.get(), request, _plugins, _metadata_block);

        if (err != 0) {
            UDA_THROW_ERROR(err, "No Record Found for this Generic Signal");
        }
        UDA_LOG(UDA_LOG_DEBUG, "Metadata Plugin Executed\nSignal Type: {}", _metadata_block.signal_desc.type);

        // Plugin? Create a new Request Block to identify the request_id

        if (_metadata_block.signal_desc.type == 'P') {
            strcpy(request->signal, _metadata_block.signal_desc.signal_name);
            strcpy(request->source, _metadata_block.data_source.path);
            make_server_request_data(_config, request, _plugins);
        }

    } // end of REQUEST_READ_GENERIC

    // Placeholder name-value substitution and additional name-value pairs
    // Modifies HEAP in request_block

    {
        int err = name_value_substitution(&request->nameValueList, request->tpass);
        if (err != 0) {
            return err;
        }
    }

    //------------------------------------------------------------------------------
    // Client XML Specified Composite Signal
    //------------------------------------------------------------------------------

    if (request->request == REQUEST_READ_XML) {
        if (strlen(request->signal) > 0) {
            strcpy(_metadata_block.signal_desc.xml, request->signal); // XML is passed via the signal string
        } else if (strlen(request->path) > 0) {                       // XML is passed via a file
            FILE* xmlfile = nullptr;
            int nchar;
            errno = 0;
            xmlfile = fopen(request->path, "r");
            int serrno = errno;
            if (serrno != 0 || xmlfile == nullptr) {
                if (serrno != 0) {
                    add_error(UDA_SYSTEM_ERROR_TYPE, "idamserverReadData", serrno, "");
                }
                if (xmlfile != nullptr) {
                    fclose(xmlfile);
                }
                UDA_THROW_ERROR(122, "Unable to Open the XML File defining the signal");
            }
            nchar = 0;
            while (!feof(xmlfile) && nchar < MAXMETA) {
                request->signal[nchar++] = (char)getc(xmlfile);
            }
            request->signal[nchar - 2] = '\0'; // Remove EOF Character and replace with String Terminator
            strcpy(_metadata_block.signal_desc.xml, request->signal);
            fclose(xmlfile);
        } else {
            UDA_THROW_ERROR(123, "There is NO XML defining the signal");
        }
        _metadata_block.signal_desc.type = 'C';
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
        plugin_interface.client_block = &_client_block;
        plugin_interface.request_data = request;
        plugin_interface.data_source = &_metadata_block.data_source;
        plugin_interface.signal_desc = &_metadata_block.signal_desc;
        plugin_interface.house_keeping = 0;
        plugin_interface.change_plugin = 0;
        plugin_interface.pluginList = &plugin_list;
        plugin_interface.user_defined_type_list = _user_defined_type_list;
        plugin_interface.log_malloc_list = _log_malloc_list;
        plugin_interface.error_stack.nerrors = 0;
        plugin_interface.error_stack.idamerror = nullptr;

        int plugin_request = REQUEST_READ_UNKNOWN;

        if (request->request != REQUEST_READ_GENERIC && request->request != REQUEST_READ_UNKNOWN) {
            plugin_request = request->request; // User has Specified a Plugin
            UDA_LOG(UDA_LOG_DEBUG, "Plugin Request {}", plugin_request);
        } else {
            auto [id, maybe_plugin] = _plugins.find_by_name(_metadata_block.data_source.format);
            if (maybe_plugin) {
                plugin_request = id;
                UDA_LOG(UDA_LOG_DEBUG, "findPluginRequestByFormat Plugin Request {}", plugin_request);
            }
        }

        UDA_LOG(UDA_LOG_DEBUG, "Number of PutData Blocks: {}", request->putDataBlockList.blockCount);

        if (plugin_request != REQUEST_READ_UNKNOWN) {

            auto maybe_plugin = _plugins.find_by_id(plugin_request);
            if (!maybe_plugin) {
                UDA_LOG(UDA_LOG_DEBUG, "Error locating data plugin {}", plugin_request);
                UDA_THROW_ERROR(999, "Error locating data plugin");
            }

#ifndef ITERSERVER
            auto external_user = _config.get("server.external_user");
            if (maybe_plugin.get().is_private == UDA_PLUGIN_PRIVATE && external_user) {
                UDA_THROW_ERROR(999, "Access to this data class is not available.");
            }
#endif
            if (maybe_plugin->handle != nullptr &&
                maybe_plugin->entry_func != nullptr) {

                UDA_LOG(UDA_LOG_DEBUG, "[{}] {} Plugin Selected", plugin_request, _metadata_block.data_source.format);

#ifndef FATCLIENT
                // Redirect Output to temporary file if no file handles passed
                int reset = 0;
                int rc;
                if ((rc = server_redirect_std_streams(_config, reset)) != 0) {
                    UDA_THROW_ERROR(rc, "Error Redirecting Plugin Message Output");
                }
#endif

                // Call the plugin
                int err = maybe_plugin->entry_func(&plugin_interface);
                for (unsigned int i = 0; i < plugin_interface.error_stack.nerrors; ++i) {
                    auto error = &plugin_interface.error_stack.idamerror[i];
                    add_error(error->type, error->location, error->code, error->msg);
                }
                free_error_stack(&plugin_interface.error_stack);

#ifndef FATCLIENT
                // Reset Redirected Output
                reset = 1;
                if ((rc = server_redirect_std_streams(_config, reset)) != 0) {
                    UDA_THROW_ERROR(rc, "Error Resetting Redirected Plugin Message Output");
                }
#endif

                if (err != 0) {
                    return err;
                }

                UDA_LOG(UDA_LOG_DEBUG, "returned from plugin called");

                // Save Provenance with socket stream protection

                server_redirect_std_streams(_config, 0);
                provenance_plugin(_config, &_client_block, request, _plugins, nullptr, _metadata_block);
                server_redirect_std_streams(_config, 1);

                // If no structures to pass back (only regular data) then free the user defined type list

                if (data_block->opaque_block == nullptr) {
                    if (data_block->opaque_type == UDA_OPAQUE_TYPE_STRUCTURES && data_block->opaque_count > 0) {
                        UDA_THROW_ERROR(999, "Opaque Data Block is Null Pointer");
                    }
                }

                if (!plugin_interface.change_plugin) {
                    // job done!
                    return 0;
                }

                request->request = REQUEST_READ_GENERIC; // Use a different Plugin
            }
        }
    }

    int plugin_request = REQUEST_READ_UNKNOWN;

    if (request->request != REQUEST_READ_GENERIC) {
        plugin_request = request->request; // User API has Specified a Plugin
    } else {

        // Test for known File formats and Server protocols

        auto [id, maybe_plugin] = _plugins.find_by_name(_metadata_block.data_source.format);

        auto external_user = _config.get("server.external_user");
        if (maybe_plugin && maybe_plugin->is_private == UDA_PLUGIN_PRIVATE && external_user) {
            UDA_THROW_ERROR(999, "Access to this data class is not available.");
        }

        // Don't append the file name to the path - if it's already present!

        if (strstr(_metadata_block.data_source.path, _metadata_block.data_source.filename) == nullptr) {
            if (strlen(_metadata_block.data_source.path) + strlen(_metadata_block.data_source.filename) + 1 < MAXPATH) {
                strcat(_metadata_block.data_source.path, "/");
                strcat(_metadata_block.data_source.path, _metadata_block.data_source.filename);
            } else {
                UDA_THROW_ERROR(999, "Path + Filename too long");
            }
        }

        if (maybe_plugin) {
            plugin_request = id;
        }
    }

    if (plugin_request == REQUEST_READ_UNKNOWN) {
        UDA_LOG(UDA_LOG_DEBUG, "No Plugin Selected");
    }
    UDA_LOG(UDA_LOG_DEBUG, "Archive      : {} ", _metadata_block.data_source.archive);
    UDA_LOG(UDA_LOG_DEBUG, "Device Name  : {} ", _metadata_block.data_source.device_name);
    UDA_LOG(UDA_LOG_DEBUG, "Signal Name  : {} ", _metadata_block.signal_desc.signal_name);
    UDA_LOG(UDA_LOG_DEBUG, "File Path    : {} ", _metadata_block.data_source.path);
    UDA_LOG(UDA_LOG_DEBUG, "File Name    : {} ", _metadata_block.data_source.filename);
    UDA_LOG(UDA_LOG_DEBUG, "Pulse Number : {} ", _metadata_block.data_source.exp_number);
    UDA_LOG(UDA_LOG_DEBUG, "Pass Number  : {} ", _metadata_block.data_source.pass);

    //----------------------------------------------------------------------------
    // Initialise the Data Block Structure

    init_data_block(data_block);

    //----------------------------------------------------------------------------
    // Status values

    if (request->request == REQUEST_READ_GENERIC) {
        data_block->source_status = _metadata_block.data_source.status;
        data_block->signal_status = _metadata_block.signal_rec.status;
    }

    //----------------------------------------------------------------------------
    // Copy the Client Block into the Data Block to pass client requested properties into plugins

    data_block->client_block = _client_block;

    //----------------------------------------------------------------------------
    // Save Provenance with socket stream protection

    server_redirect_std_streams(_config, 0);
    provenance_plugin(_config, &_client_block, request, _plugins, nullptr, _metadata_block);
    server_redirect_std_streams(_config, 1);

    return 0;
}
