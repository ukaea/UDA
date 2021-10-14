#include "serverGetData.h"

#include <cerrno>
#if defined(__GNUC__)
#  include <strings.h>
#else
#  define strncasecmp _strnicmp
#endif

#include <clientserver/initStructs.h>
#include <clientserver/printStructs.h>
#include <clientserver/stringUtils.h>
#include <clientserver/makeRequestBlock.h>
#include <clientserver/nameValueSubstitution.h>
#include <structures/struct.h>

#include "applyXML.h"
#include "getServerEnvironment.h"
#include "makeServerRequestBlock.h"
#include "serverPlugin.h"
#include "serverSubsetData.h"

static int swap_signal_error(DATA_BLOCK* data_block, DATA_BLOCK* data_block2, int asymmetry);
static int swap_signal_dim(DIMCOMPOSITE dimcomposite, DATA_BLOCK* data_block, DATA_BLOCK* data_block2);
static int swap_signal_dim_error(DIMCOMPOSITE dimcomposite, DATA_BLOCK* data_block, DATA_BLOCK* data_block2,
                                 int asymmetry);
static int read_data(REQUEST_DATA* request, CLIENT_BLOCK client_block, DATA_BLOCK* data_block,
                     DATA_SOURCE* data_source, SIGNAL* signal_rec, SIGNAL_DESC* signal_desc,
                     const PLUGINLIST* pluginlist, LOGMALLOCLIST* logmalloclist,
                     USERDEFINEDTYPELIST* userdefinedtypelist);

int udaGetData(int* depth, REQUEST_DATA* request_data, CLIENT_BLOCK client_block,
               DATA_BLOCK* data_block, DATA_SOURCE* data_source, SIGNAL* signal_rec, SIGNAL_DESC* signal_desc,
               ACTIONS* actions_desc, ACTIONS* actions_sig, const PLUGINLIST* pluginlist,
               LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist, SOCKETLIST* socket_list,
               int protocolVersion)
{
    int isDerived = 0, compId = -1, serverside = 0;

    REQUEST_DATA request_block2;
    DATA_BLOCK data_block2;
    DATA_SOURCE data_source2;
    SIGNAL signal_rec2;
    SIGNAL_DESC signal_desc2;
    ACTIONS actions_serverside;
    ACTIONS actions_comp_desc, actions_comp_sig;
    ACTIONS actions_comp_desc2, actions_comp_sig2;

    static int original_request = 0;        // First entry value of the Plugin Request
    static int original_xml = 0;        // First entry flag that XML was passed in

    //--------------------------------------------------------------------------------------------------------------------------
    // Retain the original request (Needed to flag that signal/file details are in the Request or Action structures)

#ifndef PROXYSERVER
    if (original_request == 0 || *depth == 0) {
        original_request = request_data->request;
        if (request_data->request != REQUEST_READ_XML) {
            if (STR_STARTSWITH(request_data->signal, "<?xml")) original_xml = 1;
        }
    }

    if (original_xml == 1 && *depth == 1) {
        signal_desc->xml[0] = '\0';
    }    // remove redirected XML after first recursive pass
#endif

    //--------------------------------------------------------------------------------------------------------------------------
    // Limit the Recursive Depth

    if (*depth == UDA_XML_MAX_RECURSIVE) {
        THROW_ERROR(7777, "Recursive Depth (Derived or Substitute Data) Exceeds Internal Limit");
    }

    (*depth)++;

    UDA_LOG(UDA_LOG_DEBUG, "udaGetData Recursive Depth = %d\n", *depth);

    // Can't use REQUEST_READ_SERVERSIDE because data must be read first using a 'real' data reader or REQUEST_READ_GENERIC

    if (protocolVersion < 6) {
        if (STR_IEQUALS(request_data->archive, "SS") || STR_IEQUALS(request_data->archive, "SERVERSIDE")) {
            if (!strncasecmp(request_data->signal, "SUBSET(", 7)) {
                serverside = 1;
                initActions(&actions_serverside);
                int rc;
                if ((rc = serverParseServerSide(request_data, &actions_serverside, pluginlist)) != 0) {
                    return rc;
                }
                // Erase original SUBSET request
                copyString(TrimString(request_data->signal), signal_desc->signal_name, MAXNAME);
            }
        }
    } else if (STR_IEQUALS(request_data->function, "subset")) {
        int id;
        if ((id = findPluginIdByFormat(request_data->archive, pluginlist)) >= 0) {
            if (STR_IEQUALS(pluginlist->plugin[id].symbol, "serverside")) {
                serverside = 1;
                initActions(&actions_serverside);
                int rc;
                if ((rc = serverParseServerSide(request_data, &actions_serverside, pluginlist)) != 0) {
                    return rc;
                }
                // Erase original SUBSET request
                copyString(TrimString(request_data->signal), signal_desc->signal_name, MAXNAME);
            }
        }
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // Read the Data (Returns rc < 0 if the signal is a derived type or is defined in an XML document)

    int rc = read_data(request_data, client_block, data_block, data_source, signal_rec, signal_desc,
                       pluginlist, logmalloclist, userdefinedtypelist);

    UDA_LOG(UDA_LOG_DEBUG, "After read_data rc = %d\n", rc);
    UDA_LOG(UDA_LOG_DEBUG, "Is the Signal a Composite? %d\n", signal_desc->type == 'C');

    if (rc > 0) {
        (*depth)--;
        return rc;        // An Error Occurred
    }

    // Perform data subsetting if requested

    if (request_data->datasubset.nbound > 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Calling serverSubsetData (SUBSET)   %d\n", *depth);
        ACTION action = {};
        initAction(&action);
        action.actionType = UDA_SUBSET_TYPE;
        action.subset = request_data->datasubset;
        if ((rc = serverSubsetData(data_block, action, logmalloclist)) != 0) {
            (*depth)--;
            return rc;
        }
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // If the Request is Not for a Generic Signal then exit - No XML source to apply to data as it is just regular data.
    // Allow Composites (C) or Signal Switch (S) through regardless of request type

    if (signal_desc->type != 'C' && !serverside && signal_desc->type != 'S' &&
        (!(request_data->request == REQUEST_READ_GENERIC || request_data->request == REQUEST_READ_XML))) {
        return 0;
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // Is the Signal a Derived or Signal Composite?

    if (signal_desc->type == 'C') {
        // The Signal is a Derived/Composite Type so Parse the XML for the data signal identity and read the data

        UDA_LOG(UDA_LOG_DEBUG, "Derived/Composite Signal %s\n", request_data->signal);

        isDerived = 1;                        // is True

        //derived_signal_desc     = *signal_desc;			    // Preserve details of Derived Signal Description Record
        data_source->exp_number = request_data->exp_number;     // Needed for Pulse Number Range Check in XML Parser
        data_source->pass = request_data->pass;                 // Needed for a Pass/Sequence Range Check in XML Parser

        // Allways Parse Signal XML to Identify the True Data Source for this Pulse Number - not subject to client request: get_asis
        // (First Valid Action Record found only - others ignored)

        initActions(&actions_comp_desc);
        initActions(&actions_comp_sig);

        UDA_LOG(UDA_LOG_DEBUG, "parsing XML for a COMPOSITE Signal\n");

        rc = serverParseSignalXML(*data_source, *signal_rec, *signal_desc, &actions_comp_desc, &actions_comp_sig);

        UDA_LOG(UDA_LOG_DEBUG, "parsing XML RC? %d\n", rc);

        if (rc > 0) {
            freeActions(&actions_comp_desc);
            freeActions(&actions_comp_sig);
            (*depth)--;
            THROW_ERROR(8881, "Unable to Parse XML");
        }

        // Identify which XML statements are in Range (Only signal_desc xml need be checked as signal xml is specific to a single pulse/pass)

        compId = -1;
        if (rc == 0) {
            for (int i = 0; i < actions_comp_desc.nactions; i++) {
                if (actions_comp_desc.action[i].actionType == UDA_COMPOSITE_TYPE && actions_comp_desc.action[i].inRange) {
                    compId = i;
                    break;            // First Record found only!
                }
            }

            // Identify the data's signal

            if (compId >= 0) {
                if (strlen(actions_comp_desc.action[compId].composite.data_signal) > 0) {
                    // If we haven't a True Signal then can't identify the data required!

                    request_block2 = *request_data; // Preserve details of the Original User Request (Do Not FREE Elements)

                    strcpy(request_block2.signal,
                           actions_comp_desc.action[compId].composite.data_signal);  // True Signal Identity

                    // Does this Composite originate from a subsetting operation? If so then fill out any missing items in the composite record

                    if (actions_comp_desc.action[compId].composite.nsubsets > 0 ||
                        actions_comp_desc.action[compId].composite.nmaps > 0 ||
                        (strlen(actions_comp_desc.action[compId].composite.file) == 0 &&
                         strlen(data_source->path) > 0)) {

                        // ******** If there is No subset then composite.file is missing!!!

                        if (strlen(actions_comp_desc.action[compId].composite.file) == 0
                            && strlen(data_source->path) > 0) {
                            strcpy(actions_comp_desc.action[compId].composite.file, data_source->path);
                        }

                        if (strlen(actions_comp_desc.action[compId].composite.format) == 0
                            && strlen(data_source->format) > 0) {
                            strcpy(actions_comp_desc.action[compId].composite.format, data_source->format);
                        }

                        if (strlen(actions_comp_desc.action[compId].composite.data_signal) > 0
                            && strlen(signal_desc->signal_name) == 0) {
                            strcpy(signal_desc->signal_name, actions_comp_desc.action[compId].composite.data_signal);
                        }
                    }

                    //=======>>> Experimental ============================================
                    // Need to change formats from GENERIC if Composite and Signal Description record only exists and format Not Generic!

                    if (request_data->request == REQUEST_READ_GENERIC && request_data->exp_number <= 0) {
                        request_data->request = REQUEST_READ_XML;
                    }

                    //=======>>>==========================================================

                    if (request_data->request == REQUEST_READ_XML || request_data->exp_number <= 0) {
                        if ((strlen(actions_comp_desc.action[compId].composite.file) == 0 ||
                             strlen(actions_comp_desc.action[compId].composite.format) == 0) &&
                            request_block2.exp_number <= 0) {
                            freeActions(&actions_comp_desc);
                            freeActions(&actions_comp_sig);
                            (*depth)--;
                            THROW_ERROR(8888, "User Specified Composite Data Signal Not Fully Defined: Format?, File?");
                        }
                        strcpy(request_block2.path, actions_comp_desc.action[compId].composite.file);

                        request_block2.request = findPluginRequestByFormat(
                                actions_comp_desc.action[compId].composite.format, pluginlist);

                        if (request_block2.request == REQUEST_READ_UNKNOWN) {
                            if (actions_comp_desc.action[compId].composite.format[0] == '\0' &&
                                request_block2.exp_number > 0) {
                                request_block2.request = REQUEST_READ_GENERIC;
                            } else {
                                freeActions(&actions_comp_desc);
                                freeActions(&actions_comp_sig);
                                (*depth)--;
                                THROW_ERROR(8889, "User Specified Composite Data Signal's File Format NOT Recognised");
                            }
                        }

                        if (request_block2.request == REQUEST_READ_HDF5) {
                            strcpy(data_source->path, TrimString(request_block2.path));          // HDF5 File Location
                            strcpy(signal_desc->signal_name, TrimString(request_block2.signal)); // HDF5 Variable Name
                        }
                    }

                    // Does the request type need an SQL socket?
                    // This is not passed back via the argument as only a 'by value' pointer is specified.
                    // Assign to a global to pass back - poor design that needs correcting at a later date!

                    // If the Archive is XML and the signal contains a ServerSide SUBSET function then parse and replace

                    if (STR_IEQUALS(request_block2.archive, "XML") &&
                        (strstr(request_block2.signal, "SS::SUBSET") != nullptr ||
                         strstr(request_block2.signal, "SERVERSIDE::SUBSET") != nullptr)) {
                        strcpy(request_block2.archive, "SS");
                        char* p = strstr(request_block2.signal, "::SUBSET");
                        strcpy(request_block2.signal, &p[2]);
                    }

                    UDA_LOG(UDA_LOG_DEBUG, "Reading Composite Signal DATA\n");

                    // Recursive Call for True Data with XML Transformations Applied and Associated Meta Data

                    UDA_LOG(UDA_LOG_DEBUG, "Reading Composite Signal DATA\n");

                    rc = udaGetData(depth, &request_block2, client_block, data_block, data_source, signal_rec, signal_desc,
                                    actions_desc, actions_sig, pluginlist, logmalloclist, userdefinedtypelist, socket_list,
                                    protocolVersion);

                    freeActions(actions_desc);        // Added 06Nov2008
                    freeActions(actions_sig);

                    if (rc != 0) {        // Error
                        freeActions(&actions_comp_desc);
                        freeActions(&actions_comp_sig);
                        (*depth)--;
                        return rc;
                    }

                    // Has a Time Dimension been Identified?

                    if (actions_comp_desc.action[compId].composite.order > -1) {
                        data_block->order = actions_comp_desc.action[compId].composite.order;
                    }
                } else {
                    if (rc == -1 || rc == 1) {
                        freeActions(&actions_comp_desc);
                        freeActions(&actions_comp_sig);
                        (*depth)--;
                        THROW_ERROR(7770, "Composite Data Signal Not Available - No XML Document to define it!");
                    }
                }
            }
        }

    } else { isDerived = 0; }

    //--------------------------------------------------------------------------------------------------------------------------
    // Parse Qualifying Actionable XML

    if (isDerived) {
        // All Actions are applicable to the Derived/Composite Data Structure
        copyActions(actions_desc, &actions_comp_desc);
        copyActions(actions_sig, &actions_comp_sig);
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "parsing XML for a Regular Signal\n");

        if (!client_block.get_asis) {

            // Regular Signal
            rc = serverParseSignalXML(*data_source, *signal_rec, *signal_desc, actions_desc, actions_sig);

            if (rc == -1) {
                if (!serverside) {
                    (*depth)--;
                    return 0;    // No XML to Apply so No More to be Done!
                }
            } else {
                if (rc == 1) {
                    (*depth)--;
                    THROW_ERROR(7770, "Error Parsing Signal XML Document");
                }
            }
        } else {
            (*depth)--;
            return 0;    // Ignore All XML so nothing to be done! Done!
        }
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // Swap Error Data if Required

    // ***************************   Need to Replicate the process used with Dimension Replacement?
    // ***************************

    if (isDerived && compId > -1) {

        if (strlen(actions_desc->action[compId].composite.error_signal) > 0) {

            UDA_LOG(UDA_LOG_DEBUG, "Substituting Error Data: %s\n",
                    actions_desc->action[compId].composite.error_signal);

            request_block2 = *request_data;
            strcpy(request_block2.signal, actions_desc->action[compId].composite.error_signal);

            // Recursive Call for Error Data

            initActions(&actions_comp_desc2);
            initActions(&actions_comp_sig2);
            initDataBlock(&data_block2);
            initDataSource(&data_source2);
            initSignal(&signal_rec2);
            initSignalDesc(&signal_desc2);

            // Check if the source file was originally defined in the client API?

            if (original_xml) {
                strcpy(data_source2.format, request_data->format);
                strcpy(data_source2.path, request_data->path);
                strcpy(data_source2.filename, request_data->file);
            }

            rc = udaGetData(depth, &request_block2, client_block, &data_block2, &data_source2,
                            &signal_rec2, &signal_desc2, &actions_comp_desc2, &actions_comp_sig2, pluginlist,
                            logmalloclist, userdefinedtypelist, socket_list, protocolVersion);

            freeActions(&actions_comp_desc2);
            freeActions(&actions_comp_sig2);

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

        if (strlen(actions_desc->action[compId].composite.aserror_signal) > 0) {

            UDA_LOG(UDA_LOG_DEBUG, "Substituting Asymmetric Error Data: %s\n",
                    actions_desc->action[compId].composite.aserror_signal);

            request_block2 = *request_data;
            strcpy(request_block2.signal, actions_desc->action[compId].composite.aserror_signal);

            // Recursive Call for Error Data

            initActions(&actions_comp_desc2);
            initActions(&actions_comp_sig2);
            initDataBlock(&data_block2);

            // Check if the source file was originally defined in the client API?

            if (original_xml) {
                strcpy(data_source2.format, request_data->format);
                strcpy(data_source2.path, request_data->path);
                strcpy(data_source2.filename, request_data->file);
            }

            rc = udaGetData(depth, &request_block2, client_block, &data_block2, &data_source2,
                            &signal_rec2, &signal_desc2, &actions_comp_desc2, &actions_comp_sig2, pluginlist,
                            logmalloclist, userdefinedtypelist, socket_list, protocolVersion);

            freeActions(&actions_comp_desc2);
            freeActions(&actions_comp_sig2);

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
        for (int i = 0; i < actions_desc->action[compId].composite.ndimensions; i++) {
            if (actions_desc->action[compId].composite.dimensions[i].dimType == UDA_DIM_COMPOSITE_TYPE) {
                if (strlen(actions_desc->action[compId].composite.dimensions[i].dimcomposite.dim_signal) > 0) {

                    UDA_LOG(UDA_LOG_DEBUG, "Substituting Dimension Data\n");

                    strcpy(request_block2.format,
                           "GENERIC");        // Database Lookup if not specified in XML or by Client

                    // Replace signal name re-using the Local Working REQUEST Block

                    strcpy(request_block2.signal,
                           actions_desc->action[compId].composite.dimensions[i].dimcomposite.dim_signal);

                    // Replace other properties if defined by the original client request or the XML DIMCOMPOSITE record

                    if (strlen(request_data->path) > 0) strcpy(request_block2.path, request_data->file);
                    if (strlen(request_data->format) > 0) strcpy(request_block2.format, request_data->format);

                    if (strlen(actions_desc->action[compId].composite.file) > 0) {
                        strcpy(request_block2.path, actions_desc->action[compId].composite.file);
                    }

                    if (strlen(actions_desc->action[compId].composite.format) > 0) {
                        strcpy(request_block2.format, actions_desc->action[compId].composite.format);
                    }

                    if (strlen(actions_desc->action[compId].composite.dimensions[i].dimcomposite.file) > 0) {
                        strcpy(request_block2.path,
                               actions_desc->action[compId].composite.dimensions[i].dimcomposite.file);
                    }

                    if (strlen(actions_desc->action[compId].composite.dimensions[i].dimcomposite.format) > 0) {
                        strcpy(request_block2.format,
                               actions_desc->action[compId].composite.dimensions[i].dimcomposite.format);
                    }

                    // Recursive Call for Data

                    initActions(&actions_comp_desc2);
                    initActions(&actions_comp_sig2);
                    initDataBlock(&data_block2);
                    initSignalDesc(&signal_desc2);        // Added 06Nov2008

                    // Check if the source file was originally defined in the client API?

                    strcpy(data_source2.format, request_block2.format);
                    strcpy(data_source2.path, request_block2.path);
                    strcpy(signal_desc2.signal_name, TrimString(request_block2.signal));

                    request_block2.request = findPluginRequestByFormat(request_block2.format, pluginlist);

                    if (request_block2.request == REQUEST_READ_UNKNOWN) {
                        freeActions(&actions_comp_desc2);
                        freeActions(&actions_comp_sig2);
                        (*depth)--;
                        THROW_ERROR(9999,
                                    "User Specified Composite Dimension Data Signal's File Format NOT Recognised");
                    }

                    // If the Archive is XML and the signal contains a ServerSide SUBSET function then parse and replace

                    if ((strstr(request_block2.signal, "SS::SUBSET") != nullptr ||
                         strstr(request_block2.signal, "SERVERSIDE::SUBSET") != nullptr)) {
                        strcpy(request_block2.archive, "SS");
                        char* p = strstr(request_block2.signal, "::SUBSET");
                        strcpy(request_block2.signal, &p[2]);
                    }

                    // Recursive call

                    rc = udaGetData(depth, &request_block2, client_block, &data_block2, &data_source2,
                                    &signal_rec2, &signal_desc2, &actions_comp_desc2, &actions_comp_sig2,
                                    pluginlist, logmalloclist, userdefinedtypelist, socket_list, protocolVersion);

                    freeActions(&actions_comp_desc2);
                    freeActions(&actions_comp_sig2);

                    if (rc != 0) {
                        freeDataBlock(&data_block2);
                        (*depth)--;
                        return rc;
                    }

                    // Replace Dimension Data

                    rc = swap_signal_dim(actions_desc->action[compId].composite.dimensions[i].dimcomposite,
                                         data_block, &data_block2);

                    freeDataBlock(&data_block2);

                    if (rc != 0) {
                        (*depth)--;
                        return rc;
                    }
                }

                if (strlen(actions_desc->action[compId].composite.dimensions[i].dimcomposite.dim_error) > 0) {

                    UDA_LOG(UDA_LOG_DEBUG, "Substituting Dimension Error Data\n");

                    request_block2 = *request_data;
                    strcpy(request_block2.signal,
                           actions_desc->action[compId].composite.dimensions[i].dimcomposite.dim_error);

                    // Recursive Call for Data

                    initActions(&actions_comp_desc2);
                    initActions(&actions_comp_sig2);
                    initDataBlock(&data_block2);

                    // Check if the source file was originally defined in the client API?

                    if (original_xml) {
                        strcpy(data_source2.format, request_data->format);
                        strcpy(data_source2.path, request_data->path);
                        strcpy(data_source2.filename, request_data->file);
                    }

                    rc = udaGetData(depth, &request_block2, client_block, &data_block2, &data_source2,
                                    &signal_rec2, &signal_desc2, &actions_comp_desc2, &actions_comp_sig2,
                                    pluginlist, logmalloclist, userdefinedtypelist, socket_list, protocolVersion);

                    freeActions(&actions_comp_desc2);
                    freeActions(&actions_comp_sig2);

                    if (rc != 0) {
                        freeDataBlock(&data_block2);
                        (*depth)--;
                        return rc;
                    }

                    // Replace Dimension Error Data

                    rc = swap_signal_dim_error(actions_desc->action[compId].composite.dimensions[i].dimcomposite,
                                               data_block, &data_block2, 0);

                    freeDataBlock(&data_block2);

                    if (rc != 0) {
                        (*depth)--;
                        return rc;
                    }
                }

                if (strlen(actions_desc->action[compId].composite.dimensions[i].dimcomposite.dim_aserror) > 0) {

                    UDA_LOG(UDA_LOG_DEBUG, "Substituting Dimension Asymmetric Error Data\n");

                    request_block2 = *request_data;
                    strcpy(request_block2.signal,
                           actions_desc->action[compId].composite.dimensions[i].dimcomposite.dim_aserror);

                    // Recursive Call for Data

                    initActions(&actions_comp_desc2);
                    initActions(&actions_comp_sig2);
                    initDataBlock(&data_block2);

                    // Check if the source file was originally defined in the client API?

                    if (original_xml) {
                        strcpy(data_source2.format, request_data->format);
                        strcpy(data_source2.path, request_data->path);
                        strcpy(data_source2.filename, request_data->file);
                    }

                    rc = udaGetData(depth, &request_block2, client_block, &data_block2, &data_source2,
                                    &signal_rec2, &signal_desc2, &actions_comp_desc2, &actions_comp_sig2,
                                    pluginlist, logmalloclist, userdefinedtypelist, socket_list, protocolVersion);

                    freeActions(&actions_comp_desc2);
                    freeActions(&actions_comp_sig2);

                    if (rc != 0) {
                        freeDataBlock(&data_block2);
                        (*depth)--;
                        return rc;
                    }

                    // Replace Dimension Asymmetric Error Data

                    rc = swap_signal_dim_error(actions_desc->action[compId].composite.dimensions[i].dimcomposite,
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
    // Apply Any Labeling, Timing Offsets and Calibration Actions to Data and Dimension (no Data or Dimension substituting)

    UDA_LOG(UDA_LOG_DEBUG, "#Timing Before XML\n");
    printDataBlock(*data_block);

    if (!client_block.get_asis) {

        // All Signal Actions have Precedence over Signal_Desc Actions: Deselect if there is a conflict

        serverDeselectSignalXML(actions_desc, actions_sig);

        serverApplySignalXML(client_block, data_source, signal_rec, signal_desc, data_block, *actions_desc);
        serverApplySignalXML(client_block, data_source, signal_rec, signal_desc, data_block, *actions_sig);
    }

    UDA_LOG(UDA_LOG_DEBUG, "#Timing After XML\n");
    printDataBlock(*data_block);

    //--------------------------------------------------------------------------------------------------------------------------
    // Subset Data or Map Data when all other actions have been applied

    if (isDerived && compId > -1) {
        UDA_LOG(UDA_LOG_DEBUG, "Calling serverSubsetData (Derived)  %d\n", *depth);
        printDataBlock(*data_block);

        if ((rc = serverSubsetData(data_block, actions_desc->action[compId], logmalloclist)) != 0) {
            (*depth)--;
            return rc;
        }
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // Subset Operations

    if (!serverside && !isDerived && signal_desc->type == 'S') {
        for (int i = 0; i < actions_desc->nactions; i++) {
            if (actions_desc->action[i].actionType == UDA_SUBSET_TYPE) {
                UDA_LOG(UDA_LOG_DEBUG, "Calling serverSubsetData (SUBSET)   %d\n", *depth);
                printDataBlock(*data_block);

                if ((rc = serverSubsetData(data_block, actions_desc->action[i], logmalloclist)) != 0) {
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
                    UDA_LOG(UDA_LOG_DEBUG, "Calling serverSubsetData (Serverside)   %d\n", *depth);
                    printDataBlock(*data_block);

                    if ((rc = serverSubsetData(data_block, actions_serverside.action[i], logmalloclist)) != 0) {
                        (*depth)--;
                        return rc;
                    }
                }
            }
        }
        freeActions(&actions_serverside);
    }

//--------------------------------------------------------------------------------------------------------------------------

    (*depth)--;
    return 0;
}

int swap_signal_error(DATA_BLOCK* data_block, DATA_BLOCK* data_block2, int asymmetry)
{
    // Check Rank and Array Block Size are equal

    if (data_block->rank == data_block2->rank && data_block->data_n == data_block2->data_n) {

        if (!asymmetry) {
            if (data_block->errhi != nullptr) free(data_block->errhi);    // Free unwanted Error Data Heap
            data_block->errhi = data_block2->data;                // straight swap!
            data_block2->data = nullptr;                        // Prevent Double Heap Free
            data_block->errasymmetry = 0;
        } else {
            if (data_block->errlo != nullptr) free(data_block->errlo);
            data_block->errlo = data_block2->data;
            data_block2->data = nullptr;
            data_block->errasymmetry = 1;
        }

        data_block->error_type = data_block2->data_type;

    } else {
        THROW_ERROR(7777, "Error Data Substitution Not Possible - Incompatible Lengths");
    }

    return 0;
}

int swap_signal_dim(DIMCOMPOSITE dimcomposite, DATA_BLOCK* data_block, DATA_BLOCK* data_block2)
{
    void* cptr = nullptr;

    // Possible Swaps: Replace Dimension with Signal Data or with a Dimension of the Swap Signal Data

    // Swap Signal Data

    if (dimcomposite.from_dim < 0 && dimcomposite.to_dim >= 0) {

        if (data_block->dims[dimcomposite.to_dim].dim_n == data_block2->data_n) {

            if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].dim) != nullptr) {
                free(cptr);
            } // Free unwanted dimension Heap
            if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].sams) != nullptr) free(cptr);
            if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].offs) != nullptr) free(cptr);
            if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].ints) != nullptr) free(cptr);
            if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].errhi) != nullptr) free(cptr);
            if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].errlo) != nullptr) free(cptr);

            data_block->dims[dimcomposite.to_dim].dim = nullptr;                        // Prevent Double Heap Free
            data_block->dims[dimcomposite.to_dim].sams = nullptr;
            data_block->dims[dimcomposite.to_dim].offs = nullptr;
            data_block->dims[dimcomposite.to_dim].ints = nullptr;
            data_block->dims[dimcomposite.to_dim].errhi = nullptr;
            data_block->dims[dimcomposite.to_dim].errlo = nullptr;

            data_block->dims[dimcomposite.to_dim].dim = data_block2->data;        // straight swap!
            data_block->dims[dimcomposite.to_dim].errhi = data_block2->errhi;
            data_block->dims[dimcomposite.to_dim].errlo = data_block2->errlo;
            for (int i = 0; i < data_block2->error_param_n; i++) {
                data_block->dims[dimcomposite.to_dim].errparams[i] = data_block2->errparams[i];
            }
            data_block2->data = nullptr;                            // Prevent Double Heap Free
            data_block2->errhi = nullptr;
            data_block2->errlo = nullptr;

            data_block->dims[dimcomposite.to_dim].dim_n = data_block2->data_n;
            data_block->dims[dimcomposite.to_dim].data_type = data_block2->data_type;
            data_block->dims[dimcomposite.to_dim].error_type = data_block2->error_type;
            data_block->dims[dimcomposite.to_dim].errasymmetry = data_block2->errasymmetry;
            data_block->dims[dimcomposite.to_dim].compressed = 0;                // Not Applicable to Signal Data
            data_block->dims[dimcomposite.to_dim].dim0 = 0.0E0;
            data_block->dims[dimcomposite.to_dim].diff = 0.0E0;
            data_block->dims[dimcomposite.to_dim].method = 0;
            data_block->dims[dimcomposite.to_dim].udoms = 0;
            data_block->dims[dimcomposite.to_dim].error_model = data_block2->error_model;
            data_block->dims[dimcomposite.to_dim].error_param_n = data_block2->error_param_n;

            strcpy(data_block->dims[dimcomposite.to_dim].dim_units, data_block2->data_units);
            strcpy(data_block->dims[dimcomposite.to_dim].dim_label, data_block2->data_label);

        } else {
            THROW_ERROR(7777, "Dimension Data Substitution Not Possible - Incompatible Lengths");
        }

        // Swap Signal Dimension Data

    } else {

        if (dimcomposite.from_dim >= 0 && dimcomposite.to_dim >= 0) {
            if (data_block->dims[dimcomposite.to_dim].dim_n == data_block2->dims[dimcomposite.from_dim].dim_n) {

                if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].dim) != nullptr) {
                    free(cptr);
                }  // Free unwanted dimension Heap
                if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].errhi) != nullptr) free(cptr);
                if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].errlo) != nullptr) free(cptr);
                if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].sams) != nullptr) free(cptr);
                if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].offs) != nullptr) free(cptr);
                if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].ints) != nullptr) free(cptr);

                data_block->dims[dimcomposite.to_dim].dim = data_block2->dims[dimcomposite.from_dim].dim;    // straight swap!
                data_block->dims[dimcomposite.to_dim].errhi = data_block2->dims[dimcomposite.from_dim].errhi;
                data_block->dims[dimcomposite.to_dim].errlo = data_block2->dims[dimcomposite.from_dim].errlo;
                data_block->dims[dimcomposite.to_dim].sams = data_block2->dims[dimcomposite.from_dim].sams;
                data_block->dims[dimcomposite.to_dim].offs = data_block2->dims[dimcomposite.from_dim].offs;
                data_block->dims[dimcomposite.to_dim].ints = data_block2->dims[dimcomposite.from_dim].ints;
                for (int i = 0; i < data_block2->dims[dimcomposite.from_dim].error_param_n; i++) {
                    data_block->dims[dimcomposite.to_dim].errparams[i] = data_block2->dims[dimcomposite.from_dim].errparams[i];
                }
                data_block2->dims[dimcomposite.from_dim].dim = nullptr;                        // Prevent Double Heap Free
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
                data_block->dims[dimcomposite.to_dim].error_model = data_block2->dims[dimcomposite.from_dim].errasymmetry;
                data_block->dims[dimcomposite.to_dim].error_model = data_block2->dims[dimcomposite.from_dim].error_model;
                data_block->dims[dimcomposite.to_dim].error_param_n = data_block2->dims[dimcomposite.from_dim].error_param_n;

                strcpy(data_block->dims[dimcomposite.to_dim].dim_units,
                       data_block2->dims[dimcomposite.from_dim].dim_units);
                strcpy(data_block->dims[dimcomposite.to_dim].dim_label,
                       data_block2->dims[dimcomposite.from_dim].dim_label);

            } else {
                THROW_ERROR(7777, "Dimension Data Substitution Not Possible - Incompatible Lengths");
            }
        }
    }
    return 0;
}


int swap_signal_dim_error(DIMCOMPOSITE dimcomposite, DATA_BLOCK* data_block, DATA_BLOCK* data_block2,
                          int asymmetry)
{
    void* cptr = nullptr;

// Replace Dimension Error Data with Signal Data

    if (dimcomposite.from_dim < 0 && dimcomposite.to_dim >= 0) {

        if (data_block->dims[dimcomposite.to_dim].dim_n == data_block2->data_n) {

            if (!asymmetry) {
                if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].errhi) != nullptr) {
                    free(cptr);
                }    // Unwanted
                data_block->dims[dimcomposite.to_dim].errhi = data_block2->data;                // straight swap!
                data_block2->data = nullptr;                                    // Prevent Double Heap Free
                data_block->dims[dimcomposite.to_dim].errasymmetry = 0;
            } else {
                if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].errlo) != nullptr) free(cptr);
                data_block->dims[dimcomposite.to_dim].errlo = data_block2->data;
                data_block2->data = nullptr;
                data_block->dims[dimcomposite.to_dim].errasymmetry = 1;
            }
            data_block->dims[dimcomposite.to_dim].error_type = data_block2->data_type;

        } else {
            THROW_ERROR(7777, "Dimension Error Data Substitution Not Possible - Incompatible Lengths");
        }
    }
    return 0;
}

int read_data(REQUEST_DATA* request, CLIENT_BLOCK client_block,
              DATA_BLOCK* data_block, DATA_SOURCE* data_source, SIGNAL* signal_rec, SIGNAL_DESC* signal_desc,
              const PLUGINLIST* pluginlist, LOGMALLOCLIST* logmalloclist,
              USERDEFINEDTYPELIST* userdefinedtypelist)
{
    // If err = 0 then standard signal data read
    // If err > 0 then an error occured
    // If err < 0 then unable to read signal because it is a derived type and details are in XML format

    char mapping[MAXMETA] = "";

    printRequestData(*request);

    //------------------------------------------------------------------------------
    // Test for Subsetting or Mapping XML: These require parsing First to identify the data signals needed.
    // The exception is XML defining Composite signals. These have a specific Request type.
    //------------------------------------------------------------------------------
#ifndef PROXYSERVER
    if (request->request != REQUEST_READ_XML) {
        if (STR_STARTSWITH(request->signal, "<?xml")) {
            signal_desc->type = 'C';                             // Composite/Derived Type
            signal_desc->signal_name[0] = '\0';                  // The true signal is contained in the XML
            strcpy(signal_desc->xml, request->signal);     // XML is passed via the signal string
            strcpy(data_source->format, request->format);
            strcpy(data_source->path, request->path);
            strcpy(data_source->filename, request->file);
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
    // the Source Alias associated with the Legacy Name (Signal_Desc table) is Not used by ANY Alternative mapping with the same
    // Rank, then allow normal Generic lookup.
    //------------------------------------------------------------------------------
#ifndef PROXYSERVER
    if (client_block.clientFlags & CLIENTFLAG_ALTDATA && request->request != REQUEST_READ_XML &&
        STR_STARTSWITH(request->signal, "<?xml")) {

        if (request->request != REQUEST_READ_GENERIC) {
            // Must be a Private File so switch signal names
            strcpy(request->signal, signal_desc->signal_name);        // Alias or Generic have no context wrt private files
            signal_desc->xml[0] = '\0';                                     // No corrections to private data files
            strcpy(signal_desc->xml, mapping);                              // Only mapping XML is applicable
            if (mapping[0] != '\0') {
                signal_desc->type = 'S';
            }                // Switched data with mapping Transform in XML
        } else {
            if (signal_desc->signal_alias[0] != '\0') {
                strcpy(request->signal, signal_desc->signal_alias);   // Alias or Generic name is what is passed into sqlGeneric
            }
        }
    }
#endif
    //------------------------------------------------------------------------------
    // Identify the Signal Required from the Database if a Generic Signal Requested
    // Plugin sourced data (type 'P') will fail as there is no entry in the DATA_SOURCE table so ignore
    //------------------------------------------------------------------------------

    if (request->request == REQUEST_READ_GENERIC) {

        // Identify the required Plugin

        int plugin_id = udaServerMetaDataPluginId(pluginlist, getServerEnvironment());
        if (plugin_id < 0) {
            // No plugin so not possible to identify the requested data item
            THROW_ERROR(778, "Unable to identify requested data item");
        }

        UDA_LOG(UDA_LOG_DEBUG, "Metadata Plugin ID = %d\nExecuting the plugin\n", plugin_id);

        // If the plugin is registered as a FILE or LIBRARY type then call the default method as no method will have been specified

        strcpy(request->function, pluginlist->plugin[plugin_id].method);

        // Execute the plugin to resolve the identity of the data requested

        int err = udaServerMetaDataPlugin(pluginlist, plugin_id, request, signal_desc, signal_rec, data_source,
                                          getServerEnvironment());

        if (err != 0) {
            THROW_ERROR(err, "No Record Found for this Generic Signal");
        }
        UDA_LOG(UDA_LOG_DEBUG, "Metadata Plugin Executed\nSignal Type: %c\n", signal_desc->type);

        // Plugin? Create a new Request Block to identify the request_id

        if (signal_desc->type == 'P') {
            strcpy(request->signal, signal_desc->signal_name);
            strcpy(request->source, data_source->path);
            makeServerRequestData(request, *pluginlist);
        }

    } // end of REQUEST_READ_GENERIC

    // Placeholder name-value substitution and additional name-value pairs
    // Modifes HEAP in request_block

    {
        int err = name_value_substitution(&request->nameValueList, request->tpass);
        if (err != 0) return err;
    }

    //------------------------------------------------------------------------------
    // Client XML Specified Composite Signal
    //------------------------------------------------------------------------------

    if (request->request == REQUEST_READ_XML) {
        if (strlen(request->signal) > 0) {
            strcpy(signal_desc->xml, request->signal);     // XML is passed via the signal string
        } else if (strlen(request->path) > 0) {            // XML is passed via a file
            FILE* xmlfile = nullptr;
            int nchar;
            errno = 0;
            xmlfile = fopen(request->path, "r");
            int serrno = errno;
            if (serrno != 0 || xmlfile == nullptr) {
                if (serrno != 0) {
                    addIdamError(SYSTEMERRORTYPE, "idamserverReadData", serrno, "");
                }
                if (xmlfile != nullptr) {
                    fclose(xmlfile);
                }
                THROW_ERROR(122, "Unable to Open the XML File defining the signal");
            }
            nchar = 0;
            while (!feof(xmlfile) && nchar < MAXMETA) {
                request->signal[nchar++] = (char)getc(xmlfile);
            }
            request->signal[nchar - 2] = '\0';    // Remove EOF Character and replace with String Terminator
            strcpy(signal_desc->xml, request->signal);
            fclose(xmlfile);
        } else {
            THROW_ERROR(123, "There is NO XML defining the signal");
        }
        signal_desc->type = 'C';
        return -1;
    }

    ENVIRONMENT* environment = getServerEnvironment();

    //------------------------------------------------------------------------------
    // Read Data via a Suitable Registered Plugin using a standard interface
    //------------------------------------------------------------------------------

    // Test for known File formats and Server protocols

    {
        IDAM_PLUGIN_INTERFACE idam_plugin_interface;

        UDA_LOG(UDA_LOG_DEBUG, "creating the plugin interface structure\n");

        // Initialise the Data Block

        initDataBlock(data_block);

        idam_plugin_interface.interfaceVersion = 1;
        idam_plugin_interface.pluginVersion = 0;
        idam_plugin_interface.sqlConnectionType = 0;
        idam_plugin_interface.data_block = data_block;
        idam_plugin_interface.client_block = &client_block;
        idam_plugin_interface.request_data = request;
        idam_plugin_interface.data_source = data_source;
        idam_plugin_interface.signal_desc = signal_desc;
        idam_plugin_interface.environment = environment;
        idam_plugin_interface.sqlConnection = nullptr;
        idam_plugin_interface.verbose = 0;
        idam_plugin_interface.housekeeping = 0;
        idam_plugin_interface.changePlugin = 0;
        idam_plugin_interface.pluginList = pluginlist;
        idam_plugin_interface.userdefinedtypelist = userdefinedtypelist;
        idam_plugin_interface.logmalloclist = logmalloclist;
        idam_plugin_interface.error_stack.nerrors = 0;
        idam_plugin_interface.error_stack.idamerror = nullptr;

        int plugin_id;

        if (request->request != REQUEST_READ_GENERIC && request->request != REQUEST_READ_UNKNOWN) {
            plugin_id = request->request;            // User has Specified a Plugin
            UDA_LOG(UDA_LOG_DEBUG, "Plugin Request ID %d\n", plugin_id);
        } else {
            plugin_id = findPluginRequestByFormat(data_source->format, pluginlist);    // via Generic database query
            UDA_LOG(UDA_LOG_DEBUG, "findPluginRequestByFormat Plugin Request ID %d\n", plugin_id);
        }

        UDA_LOG(UDA_LOG_DEBUG, "Number of PutData Blocks: %d\n", request->putDataBlockList.blockCount);

        if (plugin_id != REQUEST_READ_UNKNOWN) {

            int id;
            if ((id = findPluginIdByRequest(plugin_id, pluginlist)) == -1) {
                UDA_LOG(UDA_LOG_DEBUG, "Error locating data plugin %d\n", plugin_id);
                THROW_ERROR(999, "Error locating data plugin");
            }

#ifndef ITERSERVER
            if (pluginlist->plugin[id].is_private == UDA_PLUGIN_PRIVATE && environment->external_user) {
                THROW_ERROR(999, "Access to this data class is not available.");
            }
#endif
            if (pluginlist->plugin[id].external == UDA_PLUGIN_EXTERNAL &&
                pluginlist->plugin[id].status == UDA_PLUGIN_OPERATIONAL &&
                pluginlist->plugin[id].pluginHandle != nullptr &&
                pluginlist->plugin[id].idamPlugin != nullptr) {

                UDA_LOG(UDA_LOG_DEBUG, "[%d] %s Plugin Selected\n", plugin_id, data_source->format);

#ifndef FATCLIENT
                // Redirect Output to temporary file if no file handles passed
                int reset = 0;
                int rc;
                if ((rc = udaServerRedirectStdStreams(reset)) != 0) {
                    THROW_ERROR(rc, "Error Redirecting Plugin Message Output");
                }
#endif

                // Call the plugin
                int err = pluginlist->plugin[id].idamPlugin(&idam_plugin_interface);
                for (unsigned int i = 0; i < idam_plugin_interface.error_stack.nerrors; ++i) {
                    auto error = &idam_plugin_interface.error_stack.idamerror[i];
                    addIdamError(error->type, error->location, error->code, error->msg);
                }
                freeIdamErrorStack(&idam_plugin_interface.error_stack);

#ifndef FATCLIENT
                // Reset Redirected Output
                reset = 1;
                if ((rc = udaServerRedirectStdStreams(reset)) != 0) {
                    THROW_ERROR(rc, "Error Resetting Redirected Plugin Message Output");
                }
#endif

                if (err != 0) {
                    return err;
                }

                UDA_LOG(UDA_LOG_DEBUG, "returned from plugin called\n");

                // Save Provenance with socket stream protection

                udaServerRedirectStdStreams(0);
                udaProvenancePlugin(&client_block, request, data_source, signal_desc, pluginlist, nullptr,
                                    getServerEnvironment());
                udaServerRedirectStdStreams(1);

                // If no structures to pass back (only regular data) then free the user defined type list

                if (data_block->opaque_block == nullptr) {
                    if (data_block->opaque_type == UDA_OPAQUE_TYPE_STRUCTURES && data_block->opaque_count > 0) {
                        THROW_ERROR(999, "Opaque Data Block is Null Pointer");
                    }
                }

                if (!idam_plugin_interface.changePlugin) {
                    // job done!

                    data_block->source_status = data_source->status;
                    data_block->signal_status = signal_rec->status;

                    return 0;
                }

                request->request = REQUEST_READ_GENERIC;            // Use a different Plugin
            }
        }
    }

    int plugin_id = REQUEST_READ_UNKNOWN;

    if (request->request != REQUEST_READ_GENERIC) {
        plugin_id = request->request;            // User API has Specified a Plugin
    } else {

        // Test for known File formats and Server protocols

        int id = -1;
        for (int i = 0; i < pluginlist->count; i++) {
            if (STR_IEQUALS(data_source->format, pluginlist->plugin[i].format)) {
                plugin_id = pluginlist->plugin[i].request;                // Found
                id = i;
                UDA_LOG(UDA_LOG_DEBUG, "[%d] %s Plugin Selected\n", plugin_id, data_source->format);
                break;
            }
        }

        if (id >= 0 && pluginlist->plugin[id].is_private == UDA_PLUGIN_PRIVATE && environment->external_user) {
            THROW_ERROR(999, "Access to this data class is not available.");
        }

        // Don't append the file name to the path - if it's already present!

        if (strstr(data_source->path, data_source->filename) == nullptr) {
            strcat(data_source->path, "/");
            strcat(data_source->path, data_source->filename);
        }
    }

    if (plugin_id == REQUEST_READ_UNKNOWN) {
        UDA_LOG(UDA_LOG_DEBUG, "No Plugin Selected\n");
    }
    UDA_LOG(UDA_LOG_DEBUG, "Archive      : %s \n", data_source->archive);
    UDA_LOG(UDA_LOG_DEBUG, "Device Name  : %s \n", data_source->device_name);
    UDA_LOG(UDA_LOG_DEBUG, "Signal Name  : %s \n", signal_desc->signal_name);
    UDA_LOG(UDA_LOG_DEBUG, "File Path    : %s \n", data_source->path);
    UDA_LOG(UDA_LOG_DEBUG, "File Name    : %s \n", data_source->filename);
    UDA_LOG(UDA_LOG_DEBUG, "Pulse Number : %d \n", data_source->exp_number);
    UDA_LOG(UDA_LOG_DEBUG, "Pass Number  : %d \n", data_source->pass);

    //----------------------------------------------------------------------------
    // Initialise the Data Block Structure

    initDataBlock(data_block);

    //----------------------------------------------------------------------------
    // Status values

    if (request->request == REQUEST_READ_GENERIC) {
        data_block->source_status = data_source->status;
        data_block->signal_status = signal_rec->status;
    }

    //----------------------------------------------------------------------------
    // Copy the Client Block into the Data Block to pass client requested properties into plugins

    data_block->client_block = client_block;

    //----------------------------------------------------------------------------
    // Save Provenance with socket stream protection

    udaServerRedirectStdStreams(0);
    udaProvenancePlugin(&client_block, request, data_source, signal_desc, pluginlist, nullptr,
                        getServerEnvironment());
    udaServerRedirectStdStreams(1);

    return 0;
}
