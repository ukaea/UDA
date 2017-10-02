//--------------------------------------------------------------------------------------------------------------------
// Generic Data Access Tool
// XML defined Derived Signals
// XML corrections to Data
//
// Return Codes:	0 => OK, otherwise Error
//
//--------------------------------------------------------------------------------------------------------------------
#include <errno.h>
#include <strings.h>

#include <clientserver/stringUtils.h>
#include <clientserver/initStructs.h>
#include <clientserver/freeDataBlock.h>
#include <clientserver/printStructs.h>
#include <clientserver/protocol.h>
#include <clientserver/udaErrors.h>
#include <structures/struct.h>

#include <modules/bytes/readBytesNonOptimally.h>
#include <modules/hdata/readHData.h>
#include <modules/hdf58/readHDF58.h>
#include <modules/ida/readIda.h>
#include <modules/idam/readIdam.h>
#include <modules/jpf/readjpf.h>
#include <modules/mdsplus/readMDS.h>
#include <modules/netcdf4/readCDF4.h>
#include <modules/ppf/readppf.h>
#include <modules/readNothing/readNothing.h>
#include <modules/readsql/readSQL.h>
#include <modules/ufile/readUFile.h>

#include "serverGetData.h"
#include "dumpFile.h"
#include "applyXML.h"
#include "serverPlugin.h"
#include "mastArchiveFilePath.h"
#include "makeServerRequestBlock.h"
#include "sqllib.h"
#include "getServerEnvironment.h"

int idamserverSubsetData(DATA_BLOCK* data_block, ACTION action);

int idamserverParseServerSide(REQUEST_BLOCK* request_block, ACTIONS* actions_serverside);

int idamserverGetData(PGconn* DBConnect, int* depth, REQUEST_BLOCK request_block, CLIENT_BLOCK client_block,
                      DATA_BLOCK* data_block, DATA_SOURCE* data_source, SIGNAL* signal_rec, SIGNAL_DESC* signal_desc,
                      ACTIONS* actions_desc, ACTIONS* actions_sig, const PLUGINLIST* pluginlist,
                      LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist, SOCKETLIST* socket_list)
{
    int isDerived = 0, compId = -1, serverside = 0;

    REQUEST_BLOCK request_block2;
    DATA_BLOCK data_block2;
    DATA_SOURCE data_source2;
    SIGNAL signal_rec2;
    SIGNAL_DESC signal_desc2;
    ACTIONS actions_serverside;
    ACTIONS actions_comp_desc, actions_comp_sig;
    ACTIONS actions_comp_desc2, actions_comp_sig2;

    static int original_request = 0;        // First entry value of the Plugin Request
    static int original_xml = 0;        // First entry flag that XML was passed in

#ifdef TIMETEST
    struct timeval tv_start[2];
    struct timeval tv_end[2];
    float testtime ;
    int irc = gettimeofday(&tv_start[0], NULL);
#endif

    //--------------------------------------------------------------------------------------------------------------------------
    // Retain the original request (Needed to flag that signal/file details are in the Request or Action structures)

#ifndef PROXYSERVER
    if (original_request == 0 || *depth == 0) {
        original_request = request_block.request;
        if (request_block.request != REQUEST_READ_XML) {
            if (STR_EQUALS(request_block.signal, "<?xml")) original_xml = 1;
        }
    }

    if (original_xml == 1 && *depth == 1) {
        signal_desc->xml[0] = '\0';
    }    // remove redirected XML after first recursive pass
#endif

    //--------------------------------------------------------------------------------------------------------------------------
    // Limit the Recursive Depth

    if (*depth == XMLMAXRECURSIVE) {
        THROW_ERROR(7777, "Recursive Depth (Derived or Substitute Data) Exceeds Internal Limit");
    }

    (*depth)++;

    IDAM_LOGF(UDA_LOG_DEBUG, "idamserverGetData Recursive Depth = %d\n", *depth);

    // Can't use REQUEST_READ_SERVERSIDE because data must be read first using a 'real' data reader or REQUEST_READ_GENERIC

    if (protocolVersion < 6) {
        if (STR_IEQUALS(request_block.archive, "SS") || STR_IEQUALS(request_block.archive, "SERVERSIDE")) {
            if (!strncasecmp(request_block.signal, "SUBSET(", 7)) {
                serverside = 1;
                initActions(&actions_serverside);
                int rc;
                if ((rc = idamserverParseServerSide(&request_block, &actions_serverside)) != 0) {
                    return rc;
                }
                // Erase original SUBSET request
                copyString(TrimString(request_block.signal), signal_desc->signal_name, MAXNAME);
            }
        }
    } else if (STR_IEQUALS(request_block.function, "subset")) {
        int id;
        if ((id = findPluginIdByFormat(request_block.archive, pluginlist)) >= 0) {
            if (STR_IEQUALS(pluginlist->plugin[id].symbol, "serverside")) {
                serverside = 1;
                initActions(&actions_serverside);
                int rc;
                if ((rc = idamserverParseServerSide(&request_block, &actions_serverside)) != 0) {
                    return rc;
                }
                // Erase original SUBSET request
                copyString(TrimString(request_block.signal), signal_desc->signal_name, MAXNAME);
            }
        }
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // Read the Data (Returns rc < 0 if the signal is a derived type or is defined in an XML document)

    int rc = idamserverReadData(DBConnect, request_block, client_block, data_block, data_source, signal_rec, signal_desc,
                            pluginlist, logmalloclist, userdefinedtypelist, socket_list);

    IDAM_LOGF(UDA_LOG_DEBUG, "After idamserverReadData rc = %d\n", rc);
    IDAM_LOGF(UDA_LOG_DEBUG, "Is the Signal a Composite? %d\n", signal_desc->type == 'C');

    if (rc > 0) {
        (*depth)--;
        return rc;        // An Error Occurred
    }

#ifdef TIMETEST
    irc = gettimeofday(&tv_end[0], NULL);
    tv_start[1] = tv_end[0];
    testtime = (float)(tv_end[0].tv_sec-tv_start[0].tv_sec)*1.0E6 + (float)(tv_end[0].tv_usec - tv_start[0].tv_usec) ;
    IDAM_LOGF(UDA_LOG_DEBUG, "ReadData Timing: %.2f(microsecs)\n", testtime);
#endif

    //--------------------------------------------------------------------------------------------------------------------------
    // If the Request is Not for a Generic Signal then exit - No XML source to apply to data as it is just regular data.
    // Allow Composites (C) or Signal Switch (S) through regardless of request type

    if (signal_desc->type != 'C' && !serverside && signal_desc->type != 'S' &&
        (!(request_block.request == REQUEST_READ_GENERIC || request_block.request == REQUEST_READ_XML)))
        return 0;

    //--------------------------------------------------------------------------------------------------------------------------
    // Is the Signal a Derived or Signal Composite?

    if (rc < 0 && signal_desc->type == 'C') {
        // The Signal is a Derived/Composite Type so Parse the XML for the data signal identity and read the data

        IDAM_LOGF(UDA_LOG_DEBUG, "Derived/Composite Signal %s\n", request_block.signal);

        isDerived = 1;                        // is True

        //derived_signal_desc     = *signal_desc;			    // Preserve details of Derived Signal Description Record
        data_source->exp_number = request_block.exp_number;     // Needed for Pulse Number Range Check in XML Parser
        data_source->pass = request_block.pass;                 // Needed for a Pass/Sequence Range Check in XML Parser

        // Allways Parse Signal XML to Identify the True Data Source for this Pulse Number - not subject to client request: get_asis
        // (First Valid Action Record found only - others ignored)

        initActions(&actions_comp_desc);
        initActions(&actions_comp_sig);

        IDAM_LOG(UDA_LOG_DEBUG, "parsing XML for a COMPOSITE Signal\n");

        rc = idamserverParseSignalXML(*data_source, *signal_rec, *signal_desc, &actions_comp_desc, &actions_comp_sig);

        IDAM_LOGF(UDA_LOG_DEBUG, "parsing XML RC? %d\n", rc);

        if (rc > 0) {
            freeActions(&actions_comp_desc);
            freeActions(&actions_comp_sig);
            (*depth)--;
            THROW_ERROR(8881, "Unable to Parse XML");
        }

        // Identify which XML statements are in Range (Only signal_desc xml need be checked as signal xml is specific to a single pulse/pass)

        compId = -1;
        if (rc == 0) {
            int i;
            for (i = 0; i < actions_comp_desc.nactions; i++) {
                if (actions_comp_desc.action[i].actionType == COMPOSITETYPE && actions_comp_desc.action[i].inRange) {
                    compId = i;
                    break;            // First Record found only!
                }
            }

            // Identify the data's signal

            if (compId >= 0) {
                if (strlen(actions_comp_desc.action[compId].composite.data_signal) > 0) {
                    // If we haven't a True Signal then can't identify the data required!

                    request_block2 = request_block;                                // Preserve details of the Original User Request
                    strcpy(request_block2.signal, actions_comp_desc.action[compId].composite.data_signal);  // True Signal Identity

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

                    if (request_block.request == REQUEST_READ_GENERIC && request_block.exp_number <= 0) {
                        request_block.request = REQUEST_READ_XML;
                    }

                    //=======>>>==========================================================

                    if (request_block.request == REQUEST_READ_XML || request_block.exp_number <= 0) {
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

#ifndef NOTGENERICENABLED
                    if (DBConnect == NULL && (request_block2.request == REQUEST_READ_GENERIC ||
                                              request_block2.request == REQUEST_READ_SQL)) {
                        if ((DBConnect = gDBConnect) == NULL) {
                            if (!(DBConnect = startSQL())) {
                                if (DBConnect != NULL) {
                                    PQfinish(DBConnect);
                                }
                                freeActions(&actions_comp_desc);
                                freeActions(&actions_comp_sig);
                                (*depth)--;
                                THROW_ERROR(777, "SQL Database Server Connect Error");
                            }
                            gDBConnect = DBConnect;
                        }
                    }
#endif
                    // If the Archive is XML and the signal contains a ServerSide SUBSET function then parse and replace

                    if (STR_IEQUALS(request_block2.archive, "XML") &&
                            (strstr(request_block2.signal, "SS::SUBSET") != NULL ||
                                    strstr(request_block2.signal, "SERVERSIDE::SUBSET") != NULL)) {
                        strcpy(request_block2.archive, "SS");
                        char* p = strstr(request_block2.signal, "::SUBSET");
                        strcpy(request_block2.signal, &p[2]);
                    }

                    IDAM_LOG(UDA_LOG_DEBUG, "Reading Composite Signal DATA\n");

                    // Recursive Call for True Data with XML Transformations Applied and Associated Meta Data

                    IDAM_LOG(UDA_LOG_DEBUG, "Reading Composite Signal DATA\n");

                    rc = idamserverGetData(DBConnect, depth, request_block2, client_block, data_block, data_source,
                                           signal_rec, signal_desc, actions_desc, actions_sig, pluginlist,
                                           logmalloclist, userdefinedtypelist, socket_list);

                    if (DBConnect == NULL && gDBConnect != NULL)
                        DBConnect = gDBConnect;    // Pass back SQL Socket from idamserverGetData

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

    } else isDerived = 0;

    //--------------------------------------------------------------------------------------------------------------------------
    // Parse Qualifying Actionable XML

    if (isDerived) {
        // All Actions are applicable to the Derived/Composite Data Structure
        copyActions(actions_desc, &actions_comp_desc);
        copyActions(actions_sig, &actions_comp_sig);
    } else {
        IDAM_LOG(UDA_LOG_DEBUG, "parsing XML for a Regular Signal\n");

        if (!client_block.get_asis) {

            // Regular Signal
            rc = idamserverParseSignalXML(*data_source, *signal_rec, *signal_desc, actions_desc, actions_sig);

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

            IDAM_LOGF(UDA_LOG_DEBUG, "Substituting Error Data: %s\n",
                      actions_desc->action[compId].composite.error_signal);

            request_block2 = request_block;
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
                strcpy(data_source2.format, request_block.format);
                strcpy(data_source2.path, request_block.path);
                strcpy(data_source2.filename, request_block.file);
            }

            rc = idamserverGetData(DBConnect, depth, request_block2, client_block, &data_block2, &data_source2,
                                   &signal_rec2, &signal_desc2, &actions_comp_desc2, &actions_comp_sig2, pluginlist,
                                   logmalloclist, userdefinedtypelist, socket_list);

            if (DBConnect == NULL && gDBConnect != NULL)
                DBConnect = gDBConnect;    // Pass back SQL Socket from idamserverGetData

            freeActions(&actions_comp_desc2);
            freeActions(&actions_comp_sig2);

            if (rc != 0) {
                freeDataBlock(&data_block2);
                (*depth)--;
                return rc;
            }

            // Replace Error Data

            rc = idamserverSwapSignalError(data_block, &data_block2, 0);
            freeDataBlock(&data_block2);

            if (rc != 0) {
                (*depth)--;
                return rc;
            }
        }

        if (strlen(actions_desc->action[compId].composite.aserror_signal) > 0) {

            IDAM_LOGF(UDA_LOG_DEBUG, "Substituting Asymmetric Error Data: %s\n",
                      actions_desc->action[compId].composite.aserror_signal);

            request_block2 = request_block;
            strcpy(request_block2.signal, actions_desc->action[compId].composite.aserror_signal);

            // Recursive Call for Error Data

            initActions(&actions_comp_desc2);
            initActions(&actions_comp_sig2);
            initDataBlock(&data_block2);

            // Check if the source file was originally defined in the client API?

            if (original_xml) {
                strcpy(data_source2.format, request_block.format);
                strcpy(data_source2.path, request_block.path);
                strcpy(data_source2.filename, request_block.file);
            }

            rc = idamserverGetData(DBConnect, depth, request_block2, client_block, &data_block2, &data_source2,
                                   &signal_rec2, &signal_desc2, &actions_comp_desc2, &actions_comp_sig2, pluginlist,
                                   logmalloclist, userdefinedtypelist, socket_list);

            if (DBConnect == NULL && gDBConnect != NULL)
                DBConnect = gDBConnect;    // Pass back SQL Socket from idamserverGetData

            freeActions(&actions_comp_desc2);
            freeActions(&actions_comp_sig2);

            if (rc != 0) {
                freeDataBlock(&data_block2);
                (*depth)--;
                return rc;
            }

            // Replace Error Data

            rc = idamserverSwapSignalError(data_block, &data_block2, 1);
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
        int i;
        for (i = 0; i < actions_desc->action[compId].composite.ndimensions; i++) {
            if (actions_desc->action[compId].composite.dimensions[i].dimType == DIMCOMPOSITETYPE) {
                if (strlen(actions_desc->action[compId].composite.dimensions[i].dimcomposite.dim_signal) > 0) {

                    IDAM_LOG(UDA_LOG_DEBUG, "Substituting Dimension Data\n");

                    strcpy(request_block2.format,
                           "GENERIC");        // Database Lookup if not specified in XML or by Client

                    // Replace signal name re-using the Local Working REQUEST Block

                    strcpy(request_block2.signal,
                           actions_desc->action[compId].composite.dimensions[i].dimcomposite.dim_signal);

                    // Replace other properties if defined by the original client request or the XML DIMCOMPOSITE record

                    if (strlen(request_block.path) > 0) strcpy(request_block2.path, request_block.file);
                    if (strlen(request_block.format) > 0) strcpy(request_block2.format, request_block.format);

                    if (strlen(actions_desc->action[compId].composite.file) > 0)
                        strcpy(request_block2.path, actions_desc->action[compId].composite.file);

                    if (strlen(actions_desc->action[compId].composite.format) > 0)
                        strcpy(request_block2.format, actions_desc->action[compId].composite.format);

                    if (strlen(actions_desc->action[compId].composite.dimensions[i].dimcomposite.file) > 0)
                        strcpy(request_block2.path,
                               actions_desc->action[compId].composite.dimensions[i].dimcomposite.file);

                    if (strlen(actions_desc->action[compId].composite.dimensions[i].dimcomposite.format) > 0)
                        strcpy(request_block2.format,
                               actions_desc->action[compId].composite.dimensions[i].dimcomposite.format);

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
                        THROW_ERROR(9999, "User Specified Composite Dimension Data Signal's File Format NOT Recognised");
                    }

                    // If the Archive is XML and the signal contains a ServerSide SUBSET function then parse and replace

                    if ((strstr(request_block2.signal, "SS::SUBSET") != NULL ||
                         strstr(request_block2.signal, "SERVERSIDE::SUBSET") != NULL)) {
                        strcpy(request_block2.archive, "SS");
                        char* p = strstr(request_block2.signal, "::SUBSET");
                        strcpy(request_block2.signal, &p[2]);
                    }

                    // Recursive call

                    rc = idamserverGetData(DBConnect, depth, request_block2, client_block, &data_block2, &data_source2,
                                           &signal_rec2, &signal_desc2, &actions_comp_desc2, &actions_comp_sig2,
                                           pluginlist, logmalloclist, userdefinedtypelist, socket_list);

                    if (DBConnect == NULL && gDBConnect != NULL)
                        DBConnect = gDBConnect;    // Pass back SQL Socket from idamserverGetData

                    freeActions(&actions_comp_desc2);
                    freeActions(&actions_comp_sig2);

                    if (rc != 0) {
                        freeDataBlock(&data_block2);
                        (*depth)--;
                        return rc;
                    }

                    // Replace Dimension Data

                    rc = idamserverSwapSignalDim(actions_desc->action[compId].composite.dimensions[i].dimcomposite,
                                                 data_block, &data_block2);

                    freeDataBlock(&data_block2);

                    if (rc != 0) {
                        (*depth)--;
                        return rc;
                    }
                }

                if (strlen(actions_desc->action[compId].composite.dimensions[i].dimcomposite.dim_error) > 0) {

                    IDAM_LOG(UDA_LOG_DEBUG, "Substituting Dimension Error Data\n");

                    request_block2 = request_block;
                    strcpy(request_block2.signal,
                           actions_desc->action[compId].composite.dimensions[i].dimcomposite.dim_error);

                    // Recursive Call for Data

                    initActions(&actions_comp_desc2);
                    initActions(&actions_comp_sig2);
                    initDataBlock(&data_block2);

                    // Check if the source file was originally defined in the client API?

                    if (original_xml) {
                        strcpy(data_source2.format, request_block.format);
                        strcpy(data_source2.path, request_block.path);
                        strcpy(data_source2.filename, request_block.file);
                    }

                    rc = idamserverGetData(DBConnect, depth, request_block2, client_block, &data_block2, &data_source2,
                                           &signal_rec2, &signal_desc2, &actions_comp_desc2, &actions_comp_sig2,
                                           pluginlist, logmalloclist, userdefinedtypelist, socket_list);

                    if (DBConnect == NULL && gDBConnect != NULL)
                        DBConnect = gDBConnect;    // Pass back SQL Socket from idamserverGetData

                    freeActions(&actions_comp_desc2);
                    freeActions(&actions_comp_sig2);

                    if (rc != 0) {
                        freeDataBlock(&data_block2);
                        (*depth)--;
                        return rc;
                    }

                    // Replace Dimension Error Data

                    rc = idamserverSwapSignalDimError(actions_desc->action[compId].composite.dimensions[i].dimcomposite,
                                                      data_block, &data_block2, 0);

                    freeDataBlock(&data_block2);

                    if (rc != 0) {
                        (*depth)--;
                        return rc;
                    }
                }

                if (strlen(actions_desc->action[compId].composite.dimensions[i].dimcomposite.dim_aserror) > 0) {

                    IDAM_LOG(UDA_LOG_DEBUG, "Substituting Dimension Asymmetric Error Data\n");

                    request_block2 = request_block;
                    strcpy(request_block2.signal,
                           actions_desc->action[compId].composite.dimensions[i].dimcomposite.dim_aserror);

                    // Recursive Call for Data

                    initActions(&actions_comp_desc2);
                    initActions(&actions_comp_sig2);
                    initDataBlock(&data_block2);

                    // Check if the source file was originally defined in the client API?

                    if (original_xml) {
                        strcpy(data_source2.format, request_block.format);
                        strcpy(data_source2.path, request_block.path);
                        strcpy(data_source2.filename, request_block.file);
                    }

                    rc = idamserverGetData(DBConnect, depth, request_block2, client_block, &data_block2, &data_source2,
                                           &signal_rec2, &signal_desc2, &actions_comp_desc2, &actions_comp_sig2,
                                           pluginlist, logmalloclist, userdefinedtypelist, socket_list);

                    if (DBConnect == NULL && gDBConnect != NULL)
                        DBConnect = gDBConnect;    // Pass back SQL Socket from idamserverGetData


                    freeActions(&actions_comp_desc2);
                    freeActions(&actions_comp_sig2);

                    if (rc != 0) {
                        freeDataBlock(&data_block2);
                        (*depth)--;
                        return rc;
                    }

                    // Replace Dimension Asymmetric Error Data

                    rc = idamserverSwapSignalDimError(actions_desc->action[compId].composite.dimensions[i].dimcomposite,
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

    IDAM_LOG(UDA_LOG_DEBUG, "#Timing Before XML\n");
    printDataBlock(*data_block);

    if (!client_block.get_asis) {

        // All Signal Actions have Precedence over Signal_Desc Actions: Deselect if there is a conflict

        idamserverDeselectSignalXML(actions_desc, actions_sig);

        idamserverApplySignalXML(client_block, data_source, signal_rec, signal_desc, data_block, *actions_desc);
        idamserverApplySignalXML(client_block, data_source, signal_rec, signal_desc, data_block, *actions_sig);
    }

    IDAM_LOG(UDA_LOG_DEBUG, "#Timing After XML\n");
    printDataBlock(*data_block);

    //--------------------------------------------------------------------------------------------------------------------------
    // Subset Data or Map Data when all other actions have been applied

    if (isDerived && compId > -1) {
        IDAM_LOGF(UDA_LOG_DEBUG, "Calling idamserverSubsetData (Derived)  %d\n", *depth);
        printDataBlock(*data_block);

        if ((rc = idamserverSubsetData(data_block, actions_desc->action[compId])) != 0) {
            (*depth)--;
            return rc;
        }
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // Subset Operations

    if (!serverside && !isDerived && signal_desc->type == 'S') {
        int i;
        for (i = 0; i < actions_desc->nactions; i++) {
            if (actions_desc->action[i].actionType == SUBSETTYPE) {
                IDAM_LOGF(UDA_LOG_DEBUG, "Calling idamserverSubsetData (SUBSET)   %d\n", *depth);
                printDataBlock(*data_block);

                if ((rc = idamserverSubsetData(data_block, actions_desc->action[i])) != 0) {
                    (*depth)--;
                    return rc;
                }
            }
        }
    }

    //--------------------------------------------------------------------------------------------------------------------------
    // Server Side Operations

    if (serverside) {
        int i;
        for (i = 0; i < actions_serverside.nactions; i++) {
            if (actions_serverside.action[i].actionType == SERVERSIDETYPE) {
                int j;
                for (j = 0; j < actions_serverside.action[i].serverside.nsubsets; j++) {
                    IDAM_LOGF(UDA_LOG_DEBUG, "Calling idamserverSubsetData (Serverside)   %d\n", *depth);
                    printDataBlock(*data_block);

                    if ((rc = idamserverSubsetData(data_block, actions_serverside.action[i])) != 0) {
                        (*depth)--;
                        return rc;
                    }
                }
            }
        }
        freeActions(&actions_serverside);
    }

//--------------------------------------------------------------------------------------------------------------------------

#ifdef TIMETEST
    irc = gettimeofday(&tv_end[1], NULL);
    testtime = (float)(tv_end[1].tv_sec-tv_start[1].tv_sec)*1.0E6 + (float)(tv_end[1].tv_usec - tv_start[1].tv_usec) ;
    IDAM_LOGF(UDA_LOG_DEBUG, "XML Processing Timing: %.2f(microsecs)\n", testtime);
#endif

    (*depth)--;
    return 0;
}


int idamserverSwapSignalError(DATA_BLOCK* data_block, DATA_BLOCK* data_block2, int asymmetry)
{
    // Check Rank and Array Block Size are equal

    if (data_block->rank == data_block2->rank && data_block->data_n == data_block2->data_n) {

        if (!asymmetry) {
            if (data_block->errhi != NULL) free((void*)data_block->errhi);    // Free unwanted Error Data Heap
            data_block->errhi = data_block2->data;                // straight swap!
            data_block2->data = NULL;                        // Prevent Double Heap Free
            data_block->errasymmetry = 0;
        } else {
            if (data_block->errlo != NULL) free((void*)data_block->errlo);
            data_block->errlo = data_block2->data;
            data_block2->data = NULL;
            data_block->errasymmetry = 1;
        }

        data_block->error_type = data_block2->data_type;

    } else {
        THROW_ERROR(7777, "Error Data Substitution Not Possible - Incompatible Lengths");
    }

    return 0;
}

int idamserverSwapSignalDim(DIMCOMPOSITE dimcomposite, DATA_BLOCK* data_block, DATA_BLOCK* data_block2)
{
    void* cptr = NULL;

    // Possible Swaps: Replace Dimension with Signal Data or with a Dimension of the Swap Signal Data

    // Swap Signal Data

    if (dimcomposite.from_dim < 0 && dimcomposite.to_dim >= 0) {

        if (data_block->dims[dimcomposite.to_dim].dim_n == data_block2->data_n) {

            if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].dim) != NULL) {
                free(cptr);
            } // Free unwanted dimension Heap
            if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].sams) != NULL) free(cptr);
            if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].offs) != NULL) free(cptr);
            if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].ints) != NULL) free(cptr);
            if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].errhi) != NULL) free(cptr);
            if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].errlo) != NULL) free(cptr);

            data_block->dims[dimcomposite.to_dim].dim = NULL;                        // Prevent Double Heap Free
            data_block->dims[dimcomposite.to_dim].sams = NULL;
            data_block->dims[dimcomposite.to_dim].offs = NULL;
            data_block->dims[dimcomposite.to_dim].ints = NULL;
            data_block->dims[dimcomposite.to_dim].errhi = NULL;
            data_block->dims[dimcomposite.to_dim].errlo = NULL;

            data_block->dims[dimcomposite.to_dim].dim = data_block2->data;        // straight swap!
            data_block->dims[dimcomposite.to_dim].errhi = data_block2->errhi;
            data_block->dims[dimcomposite.to_dim].errlo = data_block2->errlo;
            int i;
            for (i = 0; i < data_block2->error_param_n; i++) {
                data_block->dims[dimcomposite.to_dim].errparams[i] = data_block2->errparams[i];
            }
            data_block2->data = NULL;                            // Prevent Double Heap Free
            data_block2->errhi = NULL;
            data_block2->errlo = NULL;

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

                if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].dim) != NULL) {
                    free(cptr);
                }  // Free unwanted dimension Heap
                if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].errhi) != NULL) free(cptr);
                if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].errlo) != NULL) free(cptr);
                if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].sams) != NULL) free(cptr);
                if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].offs) != NULL) free(cptr);
                if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].ints) != NULL) free(cptr);

                data_block->dims[dimcomposite.to_dim].dim = data_block2->dims[dimcomposite.from_dim].dim;    // straight swap!
                data_block->dims[dimcomposite.to_dim].errhi = data_block2->dims[dimcomposite.from_dim].errhi;
                data_block->dims[dimcomposite.to_dim].errlo = data_block2->dims[dimcomposite.from_dim].errlo;
                data_block->dims[dimcomposite.to_dim].sams = data_block2->dims[dimcomposite.from_dim].sams;
                data_block->dims[dimcomposite.to_dim].offs = data_block2->dims[dimcomposite.from_dim].offs;
                data_block->dims[dimcomposite.to_dim].ints = data_block2->dims[dimcomposite.from_dim].ints;
                int i;
                for (i = 0; i < data_block2->dims[dimcomposite.from_dim].error_param_n; i++) {
                    data_block->dims[dimcomposite.to_dim].errparams[i] = data_block2->dims[dimcomposite.from_dim].errparams[i];
                }
                data_block2->dims[dimcomposite.from_dim].dim = NULL;                        // Prevent Double Heap Free
                data_block2->dims[dimcomposite.from_dim].errhi = NULL;
                data_block2->dims[dimcomposite.from_dim].errlo = NULL;
                data_block2->dims[dimcomposite.from_dim].sams = NULL;
                data_block2->dims[dimcomposite.from_dim].offs = NULL;
                data_block2->dims[dimcomposite.from_dim].ints = NULL;

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


int idamserverSwapSignalDimError(DIMCOMPOSITE dimcomposite, DATA_BLOCK* data_block, DATA_BLOCK* data_block2,
                                 int asymmetry)
{
    void* cptr = NULL;

// Replace Dimension Error Data with Signal Data

    if (dimcomposite.from_dim < 0 && dimcomposite.to_dim >= 0) {

        if (data_block->dims[dimcomposite.to_dim].dim_n == data_block2->data_n) {

            if (!asymmetry) {
                if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].errhi) != NULL) {
                    free(cptr);
                }    // Unwanted
                data_block->dims[dimcomposite.to_dim].errhi = data_block2->data;                // straight swap!
                data_block2->data = NULL;                                    // Prevent Double Heap Free
                data_block->dims[dimcomposite.to_dim].errasymmetry = 0;
            } else {
                if ((cptr = (void*)data_block->dims[dimcomposite.to_dim].errlo) != NULL) free(cptr);
                data_block->dims[dimcomposite.to_dim].errlo = data_block2->data;
                data_block2->data = NULL;
                data_block->dims[dimcomposite.to_dim].errasymmetry = 1;
            }
            data_block->dims[dimcomposite.to_dim].error_type = data_block2->data_type;

        } else {
            THROW_ERROR(7777, "Dimension Error Data Substitution Not Possible - Incompatible Lengths");
        }
    }
    return 0;
}

int idamserverReadData(PGconn* DBConnect, REQUEST_BLOCK request_block, CLIENT_BLOCK client_block,
                       DATA_BLOCK* data_block, DATA_SOURCE* data_source, SIGNAL* signal_rec, SIGNAL_DESC* signal_desc,
                       const PLUGINLIST* pluginlist, LOGMALLOCLIST* logmalloclist,
                       USERDEFINEDTYPELIST* userdefinedtypelist, SOCKETLIST* socket_list)
{
    // If err = 0 then standard signal data read
    // If err > 0 then an error occured
    // If err < 0 then unable to read signal because it is a derived type and details are in XML format

    char mapping[MAXMETA] = "";

    printRequestBlock(request_block);

    //------------------------------------------------------------------------------
    // Test for Subsetting or Mapping XML: These require parsing First to identify the data signals needed.
    // The exception is XML defining Composite signals. These have a specific Request type.
    //------------------------------------------------------------------------------
#ifndef PROXYSERVER
    if (request_block.request != REQUEST_READ_XML) {
        if (STR_EQUALS(request_block.signal, "<?xml")) {
            signal_desc->type = 'C';                            // Composite/Derived Type
            signal_desc->signal_name[0] = '\0';                 // The true signal is contained in the XML
            strcpy(signal_desc->xml, request_block.signal);     // XML is passed via the signal string
            strcpy(data_source->format, request_block.format);
            strcpy(data_source->path, request_block.path);
            strcpy(data_source->filename, request_block.file);
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
    if (client_block.clientFlags & CLIENTFLAG_ALTDATA && request_block.request != REQUEST_READ_XML &&
        strncmp(request_block.signal, "<?xml", 5) != 0) {

        int rc;
        if (request_block.request != REQUEST_READ_GENERIC && client_block.altRank < 0) {
            mapping[0] = '\0';
            if (sqlMapPrivateData(DBConnect, request_block, signal_desc) != 1) {
                THROW_ERROR(999, "Error Returned from Name Mapping lookup!");
            }
        } else if ((rc = sqlAltData(DBConnect, request_block, client_block.altRank, signal_desc, mapping)) != 1) {
            if (rc == 0) {
                THROW_ERROR(778, "No Alternative Record Found for this Legacy Signal");
            }
            THROW_ERROR(778, "Error Returned from Alternative Data SQL Query");
        }

        if (request_block.request != REQUEST_READ_GENERIC) {                // Must be a Private File so switch signal names
            strcpy(request_block.signal, signal_desc->signal_name);         // Alias or Generic have no context wrt private files
            signal_desc->xml[0] = '\0';                                     // No corrections to private data files
            strcpy(signal_desc->xml, mapping);                              // Only mapping XML is applicable
            if (mapping[0] != '\0') signal_desc->type = 'S';                // Switched data with mapping Transform in XML
        } else {
            if (signal_desc->signal_alias[0] != '\0') {
                strcpy(request_block.signal, signal_desc->signal_alias);    // Alias or Generic name is what is passed into sqlGeneric
            }
        }
    }
#endif
    //------------------------------------------------------------------------------
    // Identify the Signal Required from the Database if a Generic Signal Requested
    // Plugin sourced data (type 'P') will fail as there is no entry in the DATA_SOURCE table so ignore
    //------------------------------------------------------------------------------

    if (request_block.request == REQUEST_READ_GENERIC) {

#ifndef NOTGENERICENABLED    // Legacy embedded SQL functions
        // Query the Database: Internal or External Data Sources; Specified Archive or Hierarchical

        if ((strlen(request_block.archive) == 0 && strlen(request_block.device_name) == 0) ||
            (STR_IEQUALS(request_block.device_name, "MAST") &&
             (strlen(request_block.archive) == 0 || STR_IEQUALS(request_block.archive, request_block.device_name))) ||
            (STR_IEQUALS(request_block.archive, "MAST") &&
             (strlen(request_block.device_name) == 0 ||
              STR_IEQUALS(request_block.archive, request_block.device_name)))) {

            int rc;
            if ((rc = sqlGeneric(DBConnect, request_block.signal, request_block.exp_number, request_block.pass,
                                 request_block.tpass,
                                 signal_rec, signal_desc, data_source)) != 1 && signal_desc->type != 'P') {
                if (rc == 0) {
                    THROW_ERROR(778, "No Record Found for this Generic Signal");
                }
                THROW_ERROR(778, "Error Returned from Generic SQL Query");
            }

            if (client_block.clientFlags & CLIENTFLAG_ALTDATA && mapping[0] != '\0') {

                signal_desc->type = 'S';            // Switched data with mapping Transform in XML

                // Merge XML
                // Insert mapping XML into SignalDesc action XML
                // Single entry points: Immediately before the closing </signal> tag.

                if (signal_desc->xml[0] == '\0') {
                    strcpy(signal_desc->xml, mapping);    // Copy mapping if no SignalDesc XML exists
                } else {
                    char* p1, * p2, * p3;
                    char* xml = (char*)malloc((strlen(signal_desc->xml) + 1) * sizeof(char));

                    reverseString(signal_desc->xml, xml);    // Find the final </signal> tag
                    p1 = strstr(xml, "langis");
                    if (p1 == NULL) {
                        free((void*)xml);
                        THROW_ERROR(999, "Badly formed Action XML from the SignalDesc database record");
                    }
                    p3 = &p1[6];
                    p1 = strchr(p3, '<');
                    if (p1 == NULL) {
                        free((void*)xml);
                        THROW_ERROR(999, "Badly formed Action XML from the SignalDesc database record");
                    }
                    reverseString(&p1[1], signal_desc->xml);

                    p3 = strstr(mapping, "signal");        // Find the first <signal> tag
                    p2 = strchr(p3, '>');

                    if (p2 != NULL) {
                        strcpy(signal_desc->xml, &p2[1]);    // Insert Mapping XML at the </signal> entry point
                    } else {
                        free((void*)xml);
                        THROW_ERROR(999, "Badly formed Action XML from the SignalAlt database record");
                    }
                }
            }

        } else {

            if (strlen(request_block.archive) != 0 && strcasecmp(request_block.archive, "MAST") != 0 &&
                strcasecmp(request_block.archive, "TRANSP") != 0 &&
                (strlen(request_block.device_name) == 0 || STR_IEQUALS(request_block.device_name, "MAST"))) {

                if (sqlArchive(DBConnect, request_block.archive, data_source) != 1) {
                    THROW_ERROR(779, "Error Returned from Archive SQL Query");
                }

            } else {

                // TRANSP Specific MAST Archive

                if ((strlen(request_block.device_name) == 0 || STR_IEQUALS(request_block.device_name, "MAST")) &&
                    STR_IEQUALS(request_block.archive, "TRANSP")) {
                    int transp_pass;

                    // Convert the Run ID to a Pass Number

                    if (strlen(request_block.tpass) == 3 && request_block.pass == -1) {
                        char transp_letter = request_block.tpass[0];
                        char* transp_number = request_block.tpass + 1;
                        if (transp_letter >= 'a' && transp_letter <= 'z') {
                            transp_pass = (transp_letter - 'a' + 1) * 100;
                        } else {
                            if (transp_letter >= 'A' && transp_letter <= 'Z') {
                                transp_pass = (transp_letter - 'A' + 1) * 100;
                            } else {
                                THROW_ERROR(778, "Please Correct the TRANSP Run ID Character Code");
                            }
                        }
                        if (isdigit(transp_number[0]) && isdigit(transp_number[1])) {
                            transp_pass = transp_pass + (int)strtol(transp_number, NULL, 10);
                        } else {
                            THROW_ERROR(778, "Please Correct the TRANSP Run ID Number");
                        }
                    } else {
                        if ((strlen(request_block.tpass) == 0) && request_block.pass > -1) {
                            transp_pass = request_block.pass;
                        } else {
                            THROW_ERROR(778, "Please Correct the TRANSP Run ID");
                        }
                    }

                    int rc;
                    if ((rc = sqlGeneric(DBConnect, request_block.signal, request_block.exp_number, transp_pass, "",
                                         signal_rec, signal_desc, data_source)) != 1) {
                        if (rc == 0) {
                            THROW_ERROR(778, "No Record Found for this TRANSP Signal");
                        }
                        THROW_ERROR(778, "Error Returned from Generic SQL Query");
                    }


                } else {

                    // External Sources of Data

                    int rc;
                    if ((rc = sqlExternalGeneric(DBConnect, request_block.archive, request_block.device_name,
                                                 request_block.signal,
                                                 request_block.exp_number, request_block.pass,
                                                 signal_rec, signal_desc, data_source)) != 1) {
                        if (rc == 0) {
                            THROW_ERROR(779, "No Record Found for this External Generic Signal");
                        }
                        THROW_ERROR(779, "Error Returned from External Generic SQL Query");
                    }
                }
            }
        }

        printDataSource(*data_source);
        printSignal(*signal_rec);
        printSignalDesc(*signal_desc);

        // Composite or Derived? - Need to Parse XML to Identify the True Signal before re-reading data

        if (signal_desc->type == 'C') {
            return -1;
        }

        // Plugin? Create a new Request Block to identify the request_id

        if (signal_desc->type == 'P') {
            strcpy(request_block.signal, signal_desc->signal_name);
            makeServerRequestBlock(&request_block, *pluginlist);
        }

#else // Plugin for Database Queries

        // Identify the required Plugin

        plugin_id = idamServerMetaDataPluginId(pluginlist);
        if (plugin_id < 0) {
            // No plugin so not possible to identify the requested data item
            THROW_ERROR(778, "Unable to identify requested data item");
        }

        IDAM_LOGF(UDA_LOG_DEBUG, "Metadata Plugin ID = %d\nExecuting the plugin\n", plugin_id);

        // If the plugin is registered as a FILE or LIBRARY type then call the default method as no method will have been specified

        strcpy(request_block.function, pluginlist->plugin[plugin_id].method);

        // Execute the plugin to resolve the identity of the data requested

        err = idamServerMetaDataPlugin(pluginlist, plugin_id, &request_block, signal_desc, data_source);

        if (err != 0) {
            THROW_ERROR(err, "No Record Found for this Generic Signal");
        }
        IDAM_LOGF(UDA_LOG_DEBUG, "Metadata Plugin Executed\nSignal Type: %c", signal_desc->type);

        // Plugin? Create a new Request Block to identify the request_id

        if(signal_desc->type == 'P') {
            strcpy(request_block.signal,signal_desc->signal_name);
            strcpy(request_block.source,data_source->path);
            makeServerRequestBlock(&request_block, pluginList);
        }
#endif // NOTGENERICENABLED

    } // end of REQUEST_READ_GENERIC

    //------------------------------------------------------------------------------
    // Client XML Specified Composite Signal
    //------------------------------------------------------------------------------

    if (request_block.request == REQUEST_READ_XML) {
        if (strlen(request_block.signal) > 0) {
            strcpy(signal_desc->xml, request_block.signal);     // XML is passed via the signal string
        } else if (strlen(request_block.path) > 0) {            // XML is passed via a file
            FILE* xmlfile = NULL;
            int nchar;
            errno = 0;
            xmlfile = fopen(request_block.path, "r");
            int serrno = errno;
            if (serrno != 0 || xmlfile == NULL) {
                if (serrno != 0) {
                    addIdamError(SYSTEMERRORTYPE, "idamserverReadData", serrno, "");
                }
                if (xmlfile != NULL) {
                    fclose(xmlfile);
                }
                THROW_ERROR(122, "Unable to Open the XML File defining the signal");
            }
            nchar = 0;
            while (!feof(xmlfile) && nchar < MAXMETA) {
                request_block.signal[nchar++] = (char)getc(xmlfile);
            }
            request_block.signal[nchar - 2] = '\0';    // Remove EOF Character and replace with String Terminator
            strcpy(signal_desc->xml, request_block.signal);
            fclose(xmlfile);
        } else {
            THROW_ERROR(123, "There is NO XML defining the signal");
        }
        signal_desc->type = 'C';
        return -1;
    }

    ENVIRONMENT* environment = getIdamServerEnvironment();

    //------------------------------------------------------------------------------
    // Read Data via a Suitable Registered Plugin using a standard interface
    //------------------------------------------------------------------------------

    // Test for known File formats and Server protocols

    {
        int id, reset;
        IDAM_PLUGIN_INTERFACE idam_plugin_interface;

        IDAM_LOG(UDA_LOG_DEBUG, "creating the plugin interface structure\n");

        // Initialise the Data Block

        initDataBlock(data_block);

        idam_plugin_interface.interfaceVersion = 1;
        idam_plugin_interface.pluginVersion = 0;
        idam_plugin_interface.sqlConnectionType = 0;
        idam_plugin_interface.data_block = data_block;
        idam_plugin_interface.client_block = &client_block;
        idam_plugin_interface.request_block = &request_block;
        idam_plugin_interface.data_source = data_source;
        idam_plugin_interface.signal_desc = signal_desc;
        idam_plugin_interface.environment = environment;
        idam_plugin_interface.sqlConnection = NULL;
        idam_plugin_interface.verbose = 0;
        idam_plugin_interface.housekeeping = 0;
        idam_plugin_interface.changePlugin = 0;
        idam_plugin_interface.pluginList = pluginlist;
        idam_plugin_interface.userdefinedtypelist = userdefinedtypelist;
        idam_plugin_interface.logmalloclist = logmalloclist;

        int plugin_id;

        if (request_block.request != REQUEST_READ_GENERIC && request_block.request != REQUEST_READ_UNKNOWN) {
            plugin_id = request_block.request;            // User has Specified a Plugin
            IDAM_LOGF(UDA_LOG_DEBUG, "Plugin Request ID %d\n", plugin_id);
        } else {
            plugin_id = findPluginRequestByFormat(data_source->format, pluginlist);    // via Generic database query
            IDAM_LOGF(UDA_LOG_DEBUG, "findPluginRequestByFormat Plugin Request ID %d\n", plugin_id);
        }

        IDAM_LOGF(UDA_LOG_DEBUG, "(idamServerGetData) Number of PutData Blocks: %d\n",
                  request_block.putDataBlockList.blockCount);

        if (request_block.request != REQUEST_READ_GENERIC && request_block.request != REQUEST_READ_UNKNOWN) {

            if ((id = findPluginIdByRequest(plugin_id, pluginlist)) == -1) {
                IDAM_LOGF(UDA_LOG_DEBUG, "Error locating data plugin %d\n", plugin_id);
                THROW_ERROR(999, "Error locating data plugin");
            }

#ifndef ITERSERVER
            if (pluginlist->plugin[id].is_private == PLUGINPRIVATE && environment->external_user) {
                THROW_ERROR(999, "Access to this data class is not available.");
            }
#endif
            if (pluginlist->plugin[id].external == PLUGINEXTERNAL &&
                pluginlist->plugin[id].status == PLUGINOPERATIONAL &&
                pluginlist->plugin[id].pluginHandle != NULL &&
                pluginlist->plugin[id].idamPlugin != NULL) {

                IDAM_LOGF(UDA_LOG_DEBUG, "[%d] %s Plugin Selected\n", plugin_id, data_source->format);

                // Redirect Output to temporary file if no file handles passed

                reset = 0;
                int err;
                if ((err = idamServerRedirectStdStreams(reset)) != 0) {
                    THROW_ERROR(err, "Error Redirecting Plugin Message Output");
                }

                // Initialise general structure passing components

                copyUserDefinedTypeList(&userdefinedtypelist); // Allocate and Copy the Master User Defined Type Lis

                // Call the plugin

                err = pluginlist->plugin[id].idamPlugin(&idam_plugin_interface);

                // Reset Redirected Output

                reset = 1;
                int rc;
                if ((rc = idamServerRedirectStdStreams(reset)) != 0 || err != 0) {
                    if (rc != 0) {
                        addIdamError(CODEERRORTYPE, "idamserverReadData", rc,
                                     "Error Resetting Redirected Plugin Message Output");
                    }
                    if (err != 0) {
                        return err;
                    }
                    return rc;
                }

                IDAM_LOG(UDA_LOG_DEBUG, "returned from plugin called\n");

                // Save Provenance with socket stream protection

                idamServerRedirectStdStreams(0);
                idamProvenancePlugin(&client_block, &request_block, data_source, signal_desc, pluginlist, NULL);
                idamServerRedirectStdStreams(1);

                // If no structures to pass back (only regular data) then free the user defined type list

                if (data_block->opaque_block == NULL) {

                    if (data_block->opaque_type == UDA_OPAQUE_TYPE_STRUCTURES && data_block->opaque_count > 0) {
                        THROW_ERROR(999, "Opaque Data Block is Null Pointer");
                    }

                    freeMallocLogList(logmalloclist);
                    free((void*)logmalloclist);
                    logmalloclist = NULL;
                    freeUserDefinedTypeList(userdefinedtypelist);
                    free((void*)userdefinedtypelist);
                    userdefinedtypelist = NULL;
                }

                if (!idam_plugin_interface.changePlugin) {
                    // job done!
                    return 0;
                }

                request_block.request = REQUEST_READ_GENERIC;            // Use a different Plugin
            }
        }
    }

    int plugin_id = REQUEST_READ_UNKNOWN;

    if (request_block.request != REQUEST_READ_GENERIC) {
        plugin_id = request_block.request;            // User API has Specified a Plugin
    } else {

        // Test for known File formats and Server protocols

        int id = -1;
        int i;
        for (i = 0; i < pluginlist->count; i++) {
            if (STR_IEQUALS(data_source->format, pluginlist->plugin[i].format)) {
                plugin_id = pluginlist->plugin[i].request;                // Found
                id = i;
                IDAM_LOGF(UDA_LOG_DEBUG, "[%d] %s Plugin Selected\n", plugin_id, data_source->format);
                break;
            }
        }

        if (id >= 0 && pluginlist->plugin[id].is_private == PLUGINPRIVATE && environment->external_user) {
            THROW_ERROR(999, "Access to this data class is not available.");
        }

        // Legacy data reader ...
        if (plugin_id == REQUEST_READ_MDS) {
            if (data_source->exp_number == 0) data_source->exp_number = request_block.exp_number;
            if (strlen(data_source->path) > 0) {
                strcpy(signal_desc->signal_alias, data_source->path);
                strcat(signal_desc->signal_alias, ".");
                strcat(signal_desc->signal_alias, signal_desc->signal_name);
                strcpy(signal_desc->signal_name, signal_desc->signal_alias);
            }
            IDAM_LOGF(UDA_LOG_DEBUG, "MDS+ Tree Number %d\n", data_source->exp_number);
            IDAM_LOGF(UDA_LOG_DEBUG, "MDS+ Tree Path %s\n", signal_desc->signal_name);
        }

        // Don't append the file name to the path - if it's already present!

        if (strlen(data_source->path) == 0) {        // No path in Data_Source record so must be on default Archive Path
            if (request_block.pass == -1 && request_block.tpass[0] == '\0') {    // Always LATES
                mastArchiveFilePath(request_block.exp_number, request_block.pass, data_source->filename,
                                    data_source->path);
            } else {                                // Specific PASS
                mastArchiveFilePath(request_block.exp_number, data_source->pass, data_source->filename,
                                    data_source->path);
            }
        } else {
            if (strstr(data_source->path, data_source->filename) == NULL) {
                strcat(data_source->path, "/");
                strcat(data_source->path, data_source->filename);
            }
        }
    }

    if (plugin_id == REQUEST_READ_UNKNOWN) {
        IDAM_LOG(UDA_LOG_DEBUG, "IdamServer: No Plugin Selected\n");
    }
    IDAM_LOGF(UDA_LOG_DEBUG, "IdamServer: Archive      : %s \n", data_source->archive);
    IDAM_LOGF(UDA_LOG_DEBUG, "IdamServer: Device Name  : %s \n", data_source->device_name);
    IDAM_LOGF(UDA_LOG_DEBUG, "IdamServer: Signal Name  : %s \n", signal_desc->signal_name);
    IDAM_LOGF(UDA_LOG_DEBUG, "IdamServer: File Path    : %s \n", data_source->path);
    IDAM_LOGF(UDA_LOG_DEBUG, "IdamServer: File Name    : %s \n", data_source->filename);
    IDAM_LOGF(UDA_LOG_DEBUG, "IdamServer: Pulse Number : %d \n", data_source->exp_number);
    IDAM_LOGF(UDA_LOG_DEBUG, "IdamServer: Pass Number  : %d \n", data_source->pass);

    //----------------------------------------------------------------------------
    // Initialise the Data Block Structure

    initDataBlock(data_block);

    //----------------------------------------------------------------------------
    // Status values

    if (request_block.request == REQUEST_READ_GENERIC) {
        data_block->source_status = data_source->status;
        data_block->signal_status = signal_rec->status;
    }

    //----------------------------------------------------------------------------
    // Copy the Client Block into the Data Block to pass client requested properties into plugins

    data_block->client_block = client_block;

    //----------------------------------------------------------------------------
    // Save Provenance with socket stream protection

    idamServerRedirectStdStreams(0);
    idamProvenancePlugin(&client_block, &request_block, data_source, signal_desc, pluginlist, NULL);
    idamServerRedirectStdStreams(1);

    //----------------------------------------------------------------------------
    // DUMPs ? (Not if redirected) (Legacy version - should be moved to plugins)

#ifndef PROXYSERVER
    if (STR_IEQUALS(request_block.archive, "DUMP") && environment->server_proxy[0] == '\0') {
        IDAM_LOG(UDA_LOG_DEBUG, "Requested: DUMP File Contents.\n");

        int err;
        if ((err = dumpFile(request_block, data_block)) != 0) {
            THROW_ERROR(err, "Error Dumping IDA File Contents");
        }
        printDataBlock(*data_block);

        return 0;
    }
#endif

    //----------------------------------------------------------------------------
    // Call the Requested Data Access Routine (Legacy version with embedded data readers)

    // Redivert all message output to stdout and stderr to a temporary file

    int reset = 0;
    int err;
    if ((err = idamServerRedirectStdStreams(reset)) != 0) {
        THROW_ERROR(err, "Error Redirecting Plugin Message Output");
    }

    switch (plugin_id) {
        case REQUEST_READ_IDA:
            IDAM_LOG(UDA_LOG_DEBUG, "Requested Data Access Routine = readIDA2 \n");
            if ((err = readIDA2(*data_source, *signal_desc, data_block)) != 0) {
                addIdamError(CODEERRORTYPE, "idamserverReadData", err, "Error Accessing IDA Data");
            }
            break;

        case REQUEST_READ_MDS:
            IDAM_LOG(UDA_LOG_DEBUG, "Requested Data Access Routine = readMDS \n");
            if ((err = readMDS(*data_source, *signal_desc, data_block, socket_list)) != 0) {
                addIdamError(CODEERRORTYPE, "idamserverReadData", err, "Error Accessing MDS+ Data");
            }
            break;

        case REQUEST_READ_IDAM:
            if ((err = readIdam(*data_source, *signal_desc, request_block, data_block)) != 0) {
                addIdamError(CODEERRORTYPE, "idamserverReadData", err, "Error Accessing IDAM Data");
            }
            IDAM_LOGF(UDA_LOG_DEBUG, "IDAM server to IDAM server request via readIDAM Error? %d\n",
                      data_block->errcode);
            break;

        case REQUEST_READ_CDF:
            IDAM_LOG(UDA_LOG_DEBUG, "Requested Data Access Routine = readCDF \n");
            if ((err = readCDF(*data_source, *signal_desc, request_block, data_block, &logmalloclist, &userdefinedtypelist)) != 0) {
                addIdamError(CODEERRORTYPE, "idamserverReadData", err, "Error Accessing netCDF Data");
            }
            IDAM_LOG(UDA_LOG_DEBUG, "Returned from readCDF \n");
            break;

        case REQUEST_READ_HDF5:
            IDAM_LOG(UDA_LOG_DEBUG, "Requested Data Access Routine = readHDF5 \n");
            if ((err = readHDF5(*data_source, *signal_desc, data_block)) != 0) {
                addIdamError(CODEERRORTYPE, "idamserverReadData", err, "Error Accessing HDF5 Data");
            }
            break;

        case REQUEST_READ_UFILE:
            IDAM_LOG(UDA_LOG_DEBUG, "Requested Data Access Routine = readUFile \n");
            if ((err = readUFile(*data_source, *signal_desc, data_block)) != 0) {
                addIdamError(CODEERRORTYPE, "idamserverReadData", err, "Error Accessing UFile Data");
            }
            break;

        case REQUEST_READ_PPF:
            IDAM_LOG(UDA_LOG_DEBUG, "Requested Data Access Routine = readPPF \n");
            if ((err = readPPF(*data_source, *signal_desc, data_block)) != 0) {
                addIdamError(CODEERRORTYPE, "idamserverReadData", err, "Error Accessing PPF Data");
            }
            break;

        case REQUEST_READ_JPF:
            IDAM_LOG(UDA_LOG_DEBUG, "Requested Data Access Routine = readJPF \n");
            if ((err = readJPF(*data_source, *signal_desc, data_block)) != 0) {
                addIdamError(CODEERRORTYPE, "idamserverReadData", err, "Error Accessing JPF Data");
            }
            break;

        case REQUEST_READ_FILE:
            IDAM_LOG(UDA_LOG_DEBUG, "Requested Data Access Routine = readBytes \n");
            if ((err = readBytes(*data_source, *signal_desc, data_block)) != 0) {
                addIdamError(CODEERRORTYPE, "idamserverReadData", err,
                             "Error Accessing File Contents (Bytes)");
            }
            break;

        case REQUEST_READ_NOTHING:
            IDAM_LOG(UDA_LOG_DEBUG, "Requested No Data Access Routine\n");
            if ((err = readNothing(*data_source, *signal_desc, data_block)) != 0) {
                addIdamError(CODEERRORTYPE, "idamserverReadData", err,
                             "Error Reading 'Nothing', i.e. Generating Test Data");
            }
            break;

        case REQUEST_READ_SQL:
            IDAM_LOG(UDA_LOG_DEBUG, "Requested SQL Plugin \n");
            if ((err = readSQL(DBConnect, request_block, *data_source, data_block, userdefinedtypelist)) != 0) {
                addIdamError(CODEERRORTYPE, "idamserverReadData", err, "Error Reading SQL Data");
            }
            IDAM_LOG(UDA_LOG_DEBUG, "Returned from SQL Plugin \n");
            printDataBlock(*data_block);
            break;

        case REQUEST_READ_HDATA:
            IDAM_LOG(UDA_LOG_DEBUG, "Requested Hierarchical Data Plugin \n");
            if ((err = readHData(DBConnect, request_block, *data_source, data_block)) != 0) {
                addIdamError(CODEERRORTYPE, "idamserverReadData", err,
                             "Error Reading Hierarchical Data");
            }
            printDataBlock(*data_block);
            break;

        default:
            IDAM_LOGF(UDA_LOG_DEBUG, "Unknown Requested Data Access Routine (%d) \n", request_block.request);
            err = 999;
            addIdamError(CODEERRORTYPE, "idamserverReadData", err,
                         "Unknown Data Accessor Requested - No IDAM Plug-in");
            break;
    }

    // Reset Redirected Output

    reset = 1;
    int rc;
    if ((rc = idamServerRedirectStdStreams(reset)) != 0) {
        THROW_ERROR(rc, "Error Resetting Redirected Plugin Message Output");
    }

    return err;
}


