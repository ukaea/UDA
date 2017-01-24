//--------------------------------------------------------------------------------------------------------------------
// Generic Data Access Tool
// XML defined Derived Signals
// XML corrections to Data
//
// Return Codes:	0 => OK, otherwise Error
//
// Change History:
// v0.00   18Oct2006  D.G.Muir   Original Version
// v0.01   06Nov2006  D.G.Muir   Asymmetric Errors added
//	   13Dec2006  D.G.Muir   errparams now fixed length array rather than heap
//	   18Jan2007  D.G.Muir   Don't Parse XML from Regular Signals if the client has specified get_asis
//	   25Feb2007  D.G.Muir   get_asis return point changed to immediately after regular data obtained
//	   09Mar2007  D.G.Muir   HDF5 File Reader Plugin added and client specified XML Composite signals
//	   14Mar2007  D.G.Muir   UFile Plugin added
//	   22Mar2007  D.G.Muir   dumpFile Plugin added
//	   28Mar2007  D.G.Muir   Improved Client based XML processing of signals
//	   16Apr2007  D.G.MUIR	 data_source2, signal_rec2 and signal_desc2 now initialised prior to recursive call to idamserverGetData
// 	   09Jul2007  dgm	 debugon enabled
//	   25Jul2007  dgm	 PPF & JPF requests added
//	   26Sep2007  dgm	 Change to XML request of exp_number = 0, Composite Signal and Generic
//	   26Sep2007  dgm	 Identify signal format for XML specified Dimension signal
//	   22Oct2007  dgm	 idamErrorLog implemented
//	   29Oct2007  dgm	 server_block arguments and components removed
//	   18Jan2008  dgm	 Bug Fix: If the format is NIDA after a Generic access, the pluging selected
//				 is readBytes. Instead of the number of time slices returned, the number of
//		 		 bytes in the IDA file is returned.
//	   15Feb2008  dgm	 added tpass to sqlGeneric call
//				 added a specific call for TRANSP data via sqlGeneric when the Archive is TRANSP and the
//				 device is MAST. The TRANSP run id is converted to a pass number.
// 28Mar2008 dgm	Added test for signal names beginning <xml? => XML signal composite or signal subset
//			Added a call to idamserverSubset to apply subsetting to data
//			Modified test for Generic and XML requests : added Composite types also (after return from idamserverReadData)
// 30May2008 dgm	Added DUMP for MDS+
// 06Nov2008 dgm	Corrected Initialisation of signal_desc2
//			Added frrs of Action Data Structures for Composite types
// 20Aug2009 dgm	Copied client_block into data_block in function idamserverReadData to pass properties into plugins
// 01Oct2009 dgm	Changed the default request block format to GENERIC with dimensions swaps
// 23Nov2009 dgm	Added SQL request
// 04Mar2010 dgm	Added global gDBConnect to pass back the SQL socket if opened. (Poor design - simple fix needing improvement)
// 02Nov2010 dgm	Added a call to sqlAltData in idamserverReadData when clientFlag raised: mapping from legacy names to new names
// 19Apr2011 dgm	Added REQUEST_BLOCK to readCDF interface
// 28Sep2011 dgm	Added external shared library plugins via macro PLUGINTEST
// 12Mar2012 dgm	Removed TESTCODE compiler option - legacy code deleted.
// 28Jun2012 dgm	Corrected bug with Serverside help function execution
// 18Sep2012 dgm	Added call to sqlMapPrivateData to map signals when the data file is private
// 05Feb2013 dgm	pluginList added to idam_plugin_interface
// 02Oct2013 dgm	serverside help function removed - now delivered by an external plugin
// 20Nov2013 dgm	PLUGINTEST selections commented out
// 21Jan2016 dgm	Added a specific plugin to query the MetaData Catalog to resolve Generic Name mappings
//--------------------------------------------------------------------------------------------------------------------

#include <idamLog.h>
#include "idamserverGetData.h"

#include "dumpFile.h"
#include "applyXML.h"
#include "readIda.h"
#include "readBytesNonOptimally.h"
#include "TrimString.h"
#include "idamplugin.h"
#include "idamServerPlugin.h"
#include "initStructs.h"
#include "printStructs.h"
#include "mastArchiveFilePath.h"
#include "readMDS.h"
#include "readIdam.h"
#include "readCDF4.h"
#include "readHDF58.h"
#include "readUFile.h"
#include "readppf.h"
#include "readjpf.h"
#include "readNothing.h"
#include "readSQL.h"
#include "readHData.h"
#include "makeServerRequestBlock.h"
#include "freeDataBlock.h"
#include "struct.h"

int idamserverSubsetData(DATA_BLOCK* data_block, ACTION action);

int idamserverParseServerSide(REQUEST_BLOCK* request_block, ACTIONS* actions_serverside);

int idamserverGetData(PGconn* DBConnect, int* depth, REQUEST_BLOCK request_block, CLIENT_BLOCK client_block,
                      DATA_BLOCK* data_block, DATA_SOURCE* data_source,
                      SIGNAL* signal_rec, SIGNAL_DESC* signal_desc, ACTIONS* actions_desc, ACTIONS* actions_sig)
{

    int i, j, rc, err, isDerived = 0, compId = -1, serverside = 0;
    char* p;

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
            if (!strncmp(request_block.signal, "<?xml", 5)) original_xml = 1;
        }
    }

    if (original_xml == 1 && *depth == 1)
        signal_desc->xml[0] = '\0';    // remove redirected XML after first recursive pass
#endif

//--------------------------------------------------------------------------------------------------------------------------
// Limit the Recursive Depth


    if (*depth == XMLMAXRECURSIVE) {
        err = 7777;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverGetData", err,
                     "Recursive Depth (Derived or Substitute Data) Exceeds Internal Limit");
        return err;
    }

    (*depth)++;

    IDAM_LOGF(LOG_DEBUG, "idamserverGetData Recursive Depth = %d\n", *depth);

// Can't use REQUEST_READ_SERVERSIDE because data must be read first using a 'real' data reader or REQUEST_READ_GENERIC

    if (protocolVersion < 6) {
        if (!strcasecmp(request_block.archive, "SS") || !strcasecmp(request_block.archive, "SERVERSIDE")) {
            if (!strncasecmp(request_block.signal, "SUBSET(", 7)) {
                serverside = 1;
                initActions(&actions_serverside);
                if ((rc = idamserverParseServerSide(&request_block, &actions_serverside)) != 0) {
                    return rc;
                }
                copyString(TrimString(request_block.signal), signal_desc->signal_name,
                           MAXNAME);    // Erase original SUBSET request
            }
        }
    } else

    if (!strcasecmp(request_block.function, "subset")) {
        int id;
        if ((id = findPluginIdByFormat(request_block.archive, &pluginList)) >= 0) {
            if (!strcasecmp(pluginList.plugin[id].symbol, "serverside")) {
                serverside = 1;
                initActions(&actions_serverside);
                if ((rc = idamserverParseServerSide(&request_block, &actions_serverside)) != 0) return rc;
                copyString(TrimString(request_block.signal), signal_desc->signal_name,
                           MAXNAME);    // Erase original SUBSET request
            }
        }
    }

//--------------------------------------------------------------------------------------------------------------------------
// Read the Data (Returns rc < 0 if the signal is a derived type or is defined in an XML document)

    rc = idamserverReadData(DBConnect, request_block, client_block, data_block, data_source, signal_rec, signal_desc);

    IDAM_LOGF(LOG_DEBUG, "After idamserverReadData rc = %d\n", rc);
    IDAM_LOGF(LOG_DEBUG, "Is the Signal a Composite? %d\n", signal_desc->type == 'C');

    if (rc > 0) {
        (*depth)--;
        return rc;        // An Error Occurred
    }

#ifdef TIMETEST
    irc = gettimeofday(&tv_end[0], NULL);
    tv_start[1] = tv_end[0];
    testtime = (float)(tv_end[0].tv_sec-tv_start[0].tv_sec)*1.0E6 + (float)(tv_end[0].tv_usec - tv_start[0].tv_usec) ;
    IDAM_LOGF(LOG_DEBUG, "ReadData Timing: %.2f(microsecs)\n", testtime);
#endif

//--------------------------------------------------------------------------------------------------------------------------
// If the Request is Not for a Generic Signal then exit - No XML source to apply to data as it is just regular data.
// Allow Composites (C) or Signal Switch (S) through regardless of request type

    if (signal_desc->type != 'C' && !serverside && signal_desc->type != 'S' &&
        (!(request_block.request == REQUEST_READ_GENERIC || request_block.request == REQUEST_READ_XML)))
        return 0;

//--------------------------------------------------------------------------------------------------------------------------
// Is the Signal a Derived or Signal Composite?

    if (rc < 0 && signal_desc->type ==
                  'C') {    // The Signal is a Derived/Composite Type so Parse the XML for the data signal identity and read the data

        IDAM_LOGF(LOG_DEBUG, "Derived/Composite Signal %s\n", request_block.signal);

        isDerived = 1;                        // is True

        //derived_signal_desc     = *signal_desc;			// Preserve details of Derived Signal Description Record
        data_source->exp_number = request_block.exp_number;    // Needed for Pulse Number Range Check in XML Parser
        data_source->pass = request_block.pass;        // Needed for a Pass/Sequence Range Check in XML Parser

// Allways Parse Signal XML to Identify the True Data Source for this Pulse Number - not subject to client request: get_asis
// (First Valid Action Record found only - others ignored)

        initActions(&actions_comp_desc);
        initActions(&actions_comp_sig);

        IDAM_LOG(LOG_DEBUG, "parsing XML for a COMPOSITE Signal\n");

        rc = idamserverParseSignalXML(*data_source, *signal_rec, *signal_desc, &actions_comp_desc, &actions_comp_sig);

        IDAM_LOGF(LOG_DEBUG, "parsing XML RC? %d\n", rc);

        if (rc > 0) {
            err = 8881;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverGetData", err, "Unable to Parse XML");
            freeActions(&actions_comp_desc);
            freeActions(&actions_comp_sig);
            (*depth)--;
            return err;
        }

// Identify which XML statements are in Range (Only signal_desc xml need be checked as signal xml is specific to a single pulse/pass)

        compId = -1;
        if (rc == 0) {                    // #############
            for (i = 0; i < actions_comp_desc.nactions; i++) {
                if (actions_comp_desc.action[i].actionType == COMPOSITETYPE && actions_comp_desc.action[i].inRange) {
                    compId = i;
                    break;            // First Record found only!
                }
            }

// Identify the data's signal

            if (compId >= 0) {

                if (strlen(actions_comp_desc.action[compId].composite.data_signal) >
                    0) {    // If we haven't a True Signal then can't identify the data required!

                    request_block2 = request_block;                                // Preserve details of the Original User Request
                    strcpy(request_block2.signal,
                           actions_comp_desc.action[compId].composite.data_signal);    // True Signal Identity

// Does this Composite originate from a subsetting operation? If so then fill out any missing items in the composite record

                    if (actions_comp_desc.action[compId].composite.nsubsets > 0 ||
                        actions_comp_desc.action[compId].composite.nmaps > 0 ||
                        (strlen(actions_comp_desc.action[compId].composite.file) == 0 &&
                         strlen(data_source->path) > 0)) {

// ******** If there is No subset then composite.file is missing!!!

                        if (strlen(actions_comp_desc.action[compId].composite.file) == 0 &&
                            strlen(data_source->path) > 0)
                            strcpy(actions_comp_desc.action[compId].composite.file, data_source->path);

                        if (strlen(actions_comp_desc.action[compId].composite.format) == 0 &&
                            strlen(data_source->format) > 0)
                            strcpy(actions_comp_desc.action[compId].composite.format, data_source->format);

                        if (strlen(actions_comp_desc.action[compId].composite.data_signal) > 0 &&
                            strlen(signal_desc->signal_name) == 0)
                            strcpy(signal_desc->signal_name, actions_comp_desc.action[compId].composite.data_signal);
                    }

//=======>>> Experimental ============================================
// Need to change formats from GENERIC if Composite and Signal Description record only exists and format Not Generic!

                    if (request_block.request == REQUEST_READ_GENERIC && request_block.exp_number <= 0) {
                        request_block.request = REQUEST_READ_XML;
                    }

//=======>>>==========================================================

                    if (request_block.request == REQUEST_READ_XML || request_block.exp_number <= 0) {
//	       if(request_block.request == REQUEST_READ_XML){				// Has the User defined the signal explicitly?
                        if ((strlen(actions_comp_desc.action[compId].composite.file) == 0 ||
                             strlen(actions_comp_desc.action[compId].composite.format) == 0) &&
                            request_block2.exp_number <= 0) {
                            err = 8888;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverGetData", err,
                                         "User Specified Composite Data Signal Not Fully Defined: Format?, File?");
                            freeActions(&actions_comp_desc);
                            freeActions(&actions_comp_sig);
                            (*depth)--;
                            return err;
                        }
                        strcpy(request_block2.path, actions_comp_desc.action[compId].composite.file);

//#ifdef PLUGINTEST
                        request_block2.request = findPluginRequestByFormat(
                                actions_comp_desc.action[compId].composite.format, &pluginList);

                        if (request_block2.request == REQUEST_READ_UNKNOWN) {
                            if (actions_comp_desc.action[compId].composite.format[0] == '\0' &&
                                request_block2.exp_number > 0) {
                                request_block2.request = REQUEST_READ_GENERIC;
                            } else {
                                err = 8889;
                                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverGetData", err,
                                             "User Specified Composite Data Signal's File Format NOT Recognised");
                                freeActions(&actions_comp_desc);
                                freeActions(&actions_comp_sig);
                                (*depth)--;
                                return err;
                            }
                        }

                        if (request_block2.request == REQUEST_READ_HDF5) {
                            strcpy(data_source->path, TrimString(request_block2.path));        // HDF5 File Location
                            strcpy(signal_desc->signal_name,
                                   TrimString(request_block2.signal));    // HDF5 Variable Name
                        }
                    }

// Does the request type need an SQL socket?
// This is not passed back via the argument as only a 'by value' pointer is specified.
// Assign to a global to pass back - poor design that needs correcting at a later date!

//#ifdef GENERIC_ENABLE
#ifndef NOTGENERICENABLED
                    if (DBConnect == NULL && (request_block2.request == REQUEST_READ_GENERIC ||
                                              request_block2.request == REQUEST_READ_SQL)) {
                        if ((DBConnect = gDBConnect) == NULL) {
                            if (!(DBConnect = (PGconn*) startSQL())) {
                                if (DBConnect != NULL) PQfinish(DBConnect);
                                err = 777;
                                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverGetData", err,
                                             "SQL Database Server Connect Error");
                                freeActions(&actions_comp_desc);
                                freeActions(&actions_comp_sig);
                                (*depth)--;
                                return err;
                            }
                            gDBConnect = DBConnect;
                        }
                    }
#endif
// If the Archive is XML and the signal contains a ServerSide SUBSET function then parse and replace

                    if (!strcasecmp(request_block2.archive, "XML") &&
                        ((p = strstr(request_block2.signal, "SS::SUBSET")) != NULL ||
                         (p = strstr(request_block2.signal, "SERVERSIDE::SUBSET")) != NULL)) {
                        strcpy(request_block2.archive, "SS");
                        p = strstr(request_block2.signal, "::SUBSET");
                        strcpy(request_block2.signal, &p[2]);
                    }

                    IDAM_LOG(LOG_DEBUG, "Reading Composite Signal DATA\n");

// Recursive Call for True Data with XML Transformations Applied and Associated Meta Data

                    IDAM_LOG(LOG_DEBUG, "Reading Composite Signal DATA\n");

                    rc = idamserverGetData(DBConnect, depth, request_block2, client_block, data_block, data_source,
                                           signal_rec, signal_desc, actions_desc, actions_sig);

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

                    if (actions_comp_desc.action[compId].composite.order > -1)
                        data_block->order = actions_comp_desc.action[compId].composite.order;
                } else {
                    if (rc == -1) {
                        err = 7770;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverGetData", err,
                                     "Composite Data Signal Not Available - No XML Document to define it!");
                        freeActions(&actions_comp_desc);
                        freeActions(&actions_comp_sig);
                        (*depth)--;
                        return err;
                    } else {
                        if (rc == 1) {
                            err = 7770;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverGetData", err,
                                         "Composite Data Signal Not Available - No XML Document to define it!");
                            freeActions(&actions_comp_desc);
                            freeActions(&actions_comp_sig);
                            (*depth)--;
                            return err;
                        }
                    }
                }
            }
        }                    // #############

    } else isDerived = 0;

//--------------------------------------------------------------------------------------------------------------------------
// Parse Qualifying Actionable XML

    if (isDerived) {
        copyActions(actions_desc,
                    &actions_comp_desc);    // All Actions are applicable to the Derived/Composite Data Structure
        copyActions(actions_sig, &actions_comp_sig);
    } else {
        IDAM_LOG(LOG_DEBUG, "parsing XML for a Regular Signal\n");

        if (!client_block.get_asis) {

            rc = idamserverParseSignalXML(*data_source, *signal_rec, *signal_desc, actions_desc,
                                          actions_sig);    // Regular Signal

            if (rc == -1) {
                if (!serverside) {
                    (*depth)--;
                    return (0);    // No XML to Apply so No More to be Done!
                }
            } else {
                if (rc == 1) {
                    err = 7770;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverGetData", err,
                                 "Error Parsing Signal XML Document");
                    (*depth)--;
                    return err;
                }
            }
        } else {
// dgm 25Feb2007
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

            IDAM_LOGF(LOG_DEBUG, "Substituting Error Data: %s\n", actions_desc->action[compId].composite.error_signal);

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
                                   &signal_rec2, &signal_desc2, &actions_comp_desc2, &actions_comp_sig2);

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

            IDAM_LOGF(LOG_DEBUG, "Substituting Asymmetric Error Data: %s\n",
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
                                   &signal_rec2, &signal_desc2, &actions_comp_desc2, &actions_comp_sig2);

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

        for (i = 0; i < actions_desc->action[compId].composite.ndimensions; i++) {
            if (actions_desc->action[compId].composite.dimensions[i].dimType == DIMCOMPOSITETYPE) {
                if (strlen(actions_desc->action[compId].composite.dimensions[i].dimcomposite.dim_signal) > 0) {

                    IDAM_LOG(LOG_DEBUG, "Substituting Dimension Data\n");

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

                    request_block2.request = findPluginRequestByFormat(request_block2.format, &pluginList);

                    if (request_block2.request == REQUEST_READ_UNKNOWN) {
                        err = 9999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverGetData", err,
                                     "User Specified Composite Dimension Data Signal's File Format NOT Recognised");
                        freeActions(&actions_comp_desc2);
                        freeActions(&actions_comp_sig2);
                        (*depth)--;
                        return err;
                    }

// If the Archive is XML and the signal contains a ServerSide SUBSET function then parse and replace

                    if (((p = strstr(request_block2.signal, "SS::SUBSET")) != NULL ||
                         (p = strstr(request_block2.signal, "SERVERSIDE::SUBSET")) != NULL)) {
                        strcpy(request_block2.archive, "SS");
                        p = strstr(request_block2.signal, "::SUBSET");
                        strcpy(request_block2.signal, &p[2]);
                    }

// Recursive call

                    rc = idamserverGetData(DBConnect, depth, request_block2, client_block, &data_block2, &data_source2,
                                           &signal_rec2, &signal_desc2, &actions_comp_desc2, &actions_comp_sig2);

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

                    IDAM_LOG(LOG_DEBUG, "Substituting Dimension Error Data\n");

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
                                           &signal_rec2, &signal_desc2, &actions_comp_desc2, &actions_comp_sig2);

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

                    IDAM_LOG(LOG_DEBUG, "Substituting Dimension Asymmetric Error Data\n");

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
                                           &signal_rec2, &signal_desc2, &actions_comp_desc2, &actions_comp_sig2);

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

    IDAM_LOG(LOG_DEBUG, "#Timing Before XML\n");
    printDataBlock(*data_block);

    if (!client_block.get_asis) {

// All Signal Actions have Precedence over Signal_Desc Actions: Deselect if there is a conflict

        idamserverDeselectSignalXML(actions_desc, actions_sig);

        idamserverApplySignalXML(client_block, data_source, signal_rec, signal_desc, data_block, *actions_desc);
        idamserverApplySignalXML(client_block, data_source, signal_rec, signal_desc, data_block, *actions_sig);
    }

    IDAM_LOG(LOG_DEBUG, "#Timing After XML\n");
    printDataBlock(*data_block);

//--------------------------------------------------------------------------------------------------------------------------
// Subset Data or Map Data when all other actions have been applied

    if (isDerived && compId > -1) {
        IDAM_LOGF(LOG_DEBUG, "Calling idamserverSubsetData (Derived)  %d\n", *depth);
        printDataBlock(*data_block);

        if ((rc = idamserverSubsetData(data_block, actions_desc->action[compId])) != 0) {
            (*depth)--;
            return rc;
        }
    }

//--------------------------------------------------------------------------------------------------------------------------
// Subset Operations

    if (!serverside && !isDerived && signal_desc->type == 'S') {
        for (i = 0; i < actions_desc->nactions; i++) {
            if (actions_desc->action[i].actionType == SUBSETTYPE) {
                IDAM_LOGF(LOG_DEBUG, "Calling idamserverSubsetData (SUBSET)   %d\n", *depth);
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
        for (i = 0; i < actions_serverside.nactions; i++) {
            if (actions_serverside.action[i].actionType == SERVERSIDETYPE) {
                for (j = 0; j < actions_serverside.action[i].serverside.nsubsets; j++) {
                    IDAM_LOGF(LOG_DEBUG, "Calling idamserverSubsetData (Serverside)   %d\n", *depth);
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
    IDAM_LOGF(LOG_DEBUG, "XML Processing Timing: %.2f(microsecs)\n", testtime);
#endif

    (*depth)--;
    return 0;
}


int idamserverSwapSignalError(DATA_BLOCK* data_block, DATA_BLOCK* data_block2, int asymmetry)
{

    int err;

// Check Rank and Array Block Size are equal

    if (data_block->rank == data_block2->rank && data_block->data_n == data_block2->data_n) {

        if (!asymmetry) {
            if (data_block->errhi != NULL) free((void*) data_block->errhi);    // Free unwanted Error Data Heap
            data_block->errhi = data_block2->data;                // straight swap!
            data_block2->data = NULL;                        // Prevent Double Heap Free
            data_block->errasymmetry = 0;
        } else {
            if (data_block->errlo != NULL) free((void*) data_block->errlo);
            data_block->errlo = data_block2->data;
            data_block2->data = NULL;
            data_block->errasymmetry = 1;
        }

        data_block->error_type = data_block2->data_type;

    } else {
        err = 7777;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverSwapSignalError", err,
                     "Error Data Substitution Not Possible - Incompatible Lengths");
        return (err);
    }

    return 0;
}

int idamserverSwapSignalDim(DIMCOMPOSITE dimcomposite, DATA_BLOCK* data_block, DATA_BLOCK* data_block2)
{
    int i, err;
    void* cptr = NULL;

// Possible Swaps: Replace Dimension with Signal Data or with a Dimension of the Swap Signal Data

// Swap Signal Data

    if (dimcomposite.from_dim < 0 && dimcomposite.to_dim >= 0) {

        if (data_block->dims[dimcomposite.to_dim].dim_n == data_block2->data_n) {

            if ((cptr = (void*) data_block->dims[dimcomposite.to_dim].dim) != NULL)
                free(cptr); // Free unwanted dimension Heap
            if ((cptr = (void*) data_block->dims[dimcomposite.to_dim].sams) != NULL) free(cptr);
            if ((cptr = (void*) data_block->dims[dimcomposite.to_dim].offs) != NULL) free(cptr);
            if ((cptr = (void*) data_block->dims[dimcomposite.to_dim].ints) != NULL) free(cptr);
            if ((cptr = (void*) data_block->dims[dimcomposite.to_dim].errhi) != NULL) free(cptr);
            if ((cptr = (void*) data_block->dims[dimcomposite.to_dim].errlo) != NULL) free(cptr);

            data_block->dims[dimcomposite.to_dim].dim = NULL;                        // Prevent Double Heap Free
            data_block->dims[dimcomposite.to_dim].sams = NULL;
            data_block->dims[dimcomposite.to_dim].offs = NULL;
            data_block->dims[dimcomposite.to_dim].ints = NULL;
            data_block->dims[dimcomposite.to_dim].errhi = NULL;
            data_block->dims[dimcomposite.to_dim].errlo = NULL;

            data_block->dims[dimcomposite.to_dim].dim = data_block2->data;        // straight swap!
            data_block->dims[dimcomposite.to_dim].errhi = data_block2->errhi;
            data_block->dims[dimcomposite.to_dim].errlo = data_block2->errlo;
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
            err = 7777;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverSwapSignalDim", err,
                         "Dimension Data Substitution Not Possible - Incompatible Lengths");
            return (err);
        }

// Swap Signal Dimension Data

    } else {

        if (dimcomposite.from_dim >= 0 && dimcomposite.to_dim >= 0) {
            if (data_block->dims[dimcomposite.to_dim].dim_n == data_block2->dims[dimcomposite.from_dim].dim_n) {

                if ((cptr = (void*) data_block->dims[dimcomposite.to_dim].dim) != NULL)
                    free(cptr);  // Free unwanted dimension Heap
                if ((cptr = (void*) data_block->dims[dimcomposite.to_dim].errhi) != NULL) free(cptr);
                if ((cptr = (void*) data_block->dims[dimcomposite.to_dim].errlo) != NULL) free(cptr);
                if ((cptr = (void*) data_block->dims[dimcomposite.to_dim].sams) != NULL) free(cptr);
                if ((cptr = (void*) data_block->dims[dimcomposite.to_dim].offs) != NULL) free(cptr);
                if ((cptr = (void*) data_block->dims[dimcomposite.to_dim].ints) != NULL) free(cptr);

                data_block->dims[dimcomposite.to_dim].dim = data_block2->dims[dimcomposite.from_dim].dim;    // straight swap!
                data_block->dims[dimcomposite.to_dim].errhi = data_block2->dims[dimcomposite.from_dim].errhi;
                data_block->dims[dimcomposite.to_dim].errlo = data_block2->dims[dimcomposite.from_dim].errlo;
                data_block->dims[dimcomposite.to_dim].sams = data_block2->dims[dimcomposite.from_dim].sams;
                data_block->dims[dimcomposite.to_dim].offs = data_block2->dims[dimcomposite.from_dim].offs;
                data_block->dims[dimcomposite.to_dim].ints = data_block2->dims[dimcomposite.from_dim].ints;
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
                err = 7777;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverSwapSignalDim", err,
                             "Dimension Data Substitution Not Possible - Incompatible Lengths");
                return (err);
            }
        }
    }
    return 0;
}


int idamserverSwapSignalDimError(DIMCOMPOSITE dimcomposite, DATA_BLOCK* data_block, DATA_BLOCK* data_block2,
                                 int asymmetry)
{
    int err;
    void* cptr = NULL;

// Replace Dimension Error Data with Signal Data

    if (dimcomposite.from_dim < 0 && dimcomposite.to_dim >= 0) {

        if (data_block->dims[dimcomposite.to_dim].dim_n == data_block2->data_n) {

            if (!asymmetry) {
                if ((cptr = (void*) data_block->dims[dimcomposite.to_dim].errhi) != NULL)
                    free(cptr);    // Unwanted
                data_block->dims[dimcomposite.to_dim].errhi = data_block2->data;                // straight swap!
                data_block2->data = NULL;                                    // Prevent Double Heap Free
                data_block->dims[dimcomposite.to_dim].errasymmetry = 0;
            } else {
                if ((cptr = (void*) data_block->dims[dimcomposite.to_dim].errlo) != NULL) free(cptr);
                data_block->dims[dimcomposite.to_dim].errlo = data_block2->data;
                data_block2->data = NULL;
                data_block->dims[dimcomposite.to_dim].errasymmetry = 1;
            }
            data_block->dims[dimcomposite.to_dim].error_type = data_block2->data_type;

        } else {
            err = 7777;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverSwapSignalDimError", err,
                         "Dimension Error Data Substitution Not Possible - Incompatible Lengths");
            return (err);
        }
    }
    return 0;
}


int idamserverReadData(PGconn* DBConnect, REQUEST_BLOCK request_block, CLIENT_BLOCK client_block,
                       DATA_BLOCK* data_block, DATA_SOURCE* data_source, SIGNAL* signal_rec, SIGNAL_DESC* signal_desc)
{

// If err = 0 then standard signal data read
// If err > 0 then an error occured
// If err < 0 then unable to read signal because it is a derived type and details are in XML format

    int i, id, rc = 0, err = 0, serrno;
    int plugin_id = -1;

    char mapping[MAXMETA] = "";

    printRequestBlock(request_block);

//------------------------------------------------------------------------------
// Test for Subsetting or Mapping XML: These require parsing First to identify the data signals needed.
// The exception is XML defining Composite signals. These have a specific Request type.
//------------------------------------------------------------------------------
#ifndef PROXYSERVER
    if (request_block.request != REQUEST_READ_XML) {
        if (!strncmp(request_block.signal, "<?xml", 5)) {

            signal_desc->type = 'C';            // Composite/Derived Type
            signal_desc->signal_name[0] = '\0';            // The true signal is contained in the XML

            strcpy(signal_desc->xml, request_block.signal);    // XML is passed via the signal string

            strcpy(data_source->format, request_block.format);
            strcpy(data_source->path, request_block.path);
            strcpy(data_source->filename, request_block.file);
            return (-1);
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

        if (request_block.request != REQUEST_READ_GENERIC && client_block.altRank < 0) {
            mapping[0] = '\0';
            if ((rc = sqlMapPrivateData(DBConnect, request_block, signal_desc)) != 1) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err,
                             "Error Returned from Name Mapping lookup!");
                return err;
            }
        } else

        if ((rc = sqlAltData(DBConnect, request_block, client_block.altRank, signal_desc, mapping)) != 1) {
            err = 778;
            if (rc == 0)
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err,
                             "No Alternative Record Found for this Legacy Signal");
            else
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err,
                             "Error Returned from Alternative Data SQL Query");
            return err;
        }

        if (request_block.request != REQUEST_READ_GENERIC) {        // Must be a Private File so switch signal names
            strcpy(request_block.signal,
                   signal_desc->signal_name);    // Alias or Generic have no context wrt private files
            signal_desc->xml[0] = '\0';                    // No corrections to private data files
            strcpy(signal_desc->xml, mapping);                // Only mapping XML is applicable
            if (mapping[0] != '\0') signal_desc->type = 'S';        // Switched data with mapping Transform in XML
        } else {
            if (signal_desc->signal_alias[0] != '\0')
                strcpy(request_block.signal,
                       signal_desc->signal_alias);    // Alias or Generic name is what is passed into sqlGeneric
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
            (!strcasecmp(request_block.device_name, "MAST") &&
             (strlen(request_block.archive) == 0 || !strcasecmp(request_block.archive, request_block.device_name))) ||
            (!strcasecmp(request_block.archive, "MAST") &&
             (strlen(request_block.device_name) == 0 ||
              !strcasecmp(request_block.archive, request_block.device_name)))) {

            if ((rc = sqlGeneric(DBConnect, request_block.signal, request_block.exp_number, request_block.pass,
                                 request_block.tpass,
                                 signal_rec, signal_desc, data_source)) != 1 &&
                signal_desc->type != 'P') {
                err = 778;
                if (rc == 0)
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err,
                                 "No Record Found for this Generic Signal");
                else
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err,
                                 "Error Returned from Generic SQL Query");
                return err;
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
                    char* xml = (char*) malloc((strlen(signal_desc->xml) + 1) * sizeof(char));

                    reverseString(signal_desc->xml, xml);    // Find the final </signal> tag
                    p1 = strstr(xml, "langis");
                    if (p1 == NULL) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err,
                                     "Badly formed Action XML from the SignalDesc database record");
                        free((void*) xml);
                        return err;
                    }
                    p3 = &p1[6];
                    p1 = strchr(p3, '<');
                    if (p1 == NULL) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err,
                                     "Badly formed Action XML from the SignalDesc database record");
                        free((void*) xml);
                        return err;
                    }
                    reverseString(&p1[1], signal_desc->xml);

                    p3 = strstr(mapping, "signal");        // Find the first <signal> tag
                    p2 = strchr(p3, '>');

                    if (p2 != NULL) {
                        strcpy(signal_desc->xml, &p2[1]);    // Insert Mapping XML at the </signal> entry point
                    } else {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err,
                                     "Badly formed Action XML from the SignalAlt database record");
                        free((void*) xml);
                        return err;
                    }
                }
            }

        } else {

            if (strlen(request_block.archive) != 0 && strcasecmp(request_block.archive, "MAST") != 0 &&
                strcasecmp(request_block.archive, "TRANSP") != 0 &&
                (strlen(request_block.device_name) == 0 || !strcasecmp(request_block.device_name, "MAST"))) {

                if ((rc = sqlArchive(DBConnect, request_block.archive, data_source)) != 1) {
                    err = 779;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err,
                                 "Error Returned from Archive SQL Query");
                    return err;
                }

            } else {

// TRANSP Specific MAST Archive

                if ((strlen(request_block.device_name) == 0 || !strcasecmp(request_block.device_name, "MAST")) &&
                    !strcasecmp(request_block.archive, "TRANSP")) {
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
                                err = 778;
                                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err,
                                             "Please Correct the TRANSP Run ID Character Code");
                                return err;
                            }
                        }
                        if (isdigit(transp_number[0]) && isdigit(transp_number[1])) {
                            transp_pass = transp_pass + atoi(transp_number);
                        } else {
                            err = 778;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err,
                                         "Please Correct the TRANSP Run ID Number");
                            return err;
                        }
                    } else {
                        if ((strlen(request_block.tpass) == 0) && request_block.pass > -1) {
                            transp_pass = request_block.pass;
                        } else {
                            err = 778;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err,
                                         "Please Correct the TRANSP Run ID");
                            return err;
                        }
                    }

                    if ((rc = sqlGeneric(DBConnect, request_block.signal, request_block.exp_number, transp_pass, "",
                                         signal_rec, signal_desc, data_source)) != 1) {
                        err = 778;
                        if (rc == 0)
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err,
                                         "No Record Found for this TRANSP Signal");
                        else
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err,
                                         "Error Returned from Generic SQL Query");
                        return err;
                    }


                } else {

// External Sources of Data

                    if ((rc = sqlExternalGeneric(DBConnect, request_block.archive, request_block.device_name,
                                                 request_block.signal,
                                                 request_block.exp_number, request_block.pass,
                                                 signal_rec, signal_desc, data_source)) != 1) {
                        err = 779;
                        if (rc == 0)
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err,
                                         "No Record Found for this External Generic Signal");
                        else
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err,
                                         "Error Returned from External Generic SQL Query");
                        return err;
                    }
                }
            }
        }

        printDataSource(*data_source);
        printSignal(*signal_rec);
        printSignalDesc(*signal_desc);

// Composite or Derived? - Need to Parse XML to Identify the True Signal before re-reading data

        if (signal_desc->type == 'C') return (-1);

// Plugin? Create a new Request Block to identify the request_id

        if (signal_desc->type == 'P') {
            strcpy(request_block.signal, signal_desc->signal_name);
            makeServerRequestBlock(&request_block, pluginList);
        }

#else		// Plugin for Database Queries

        // Identify the required Plugin

              plugin_id = idamServerMetaDataPluginId(&pluginList);
              if(plugin_id < 0){	// No plugin so not possible to identify the requested data item
                 err = 778;
                 addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err, "No Metadata Catalog Implemented! "
                          "Unable to identify requested data item");
                 IDAM_LOG(LOG_ERROR, "Error: No Metadata Catalog Implemented! Unable to identify requested data item\n");
                return err;
              }

              IDAM_LOGF(LOG_DEBUG, "Metadata Plugin ID = %d\nExecuting the plugin\n", plugin_id);

        // If the plugin is registered as a FILE or LIBRARY type then call the default method as no method will have been specified

              strcpy(request_block.function, pluginList.plugin[plugin_id].method);

        // Execute the plugin to resolve the identity of the data requested

              err = idamServerMetaDataPlugin(&pluginList, plugin_id, &request_block, signal_desc, data_source);

              if(err != 0){
                 addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err, "No Record Found for this Generic Signal");
                 return err;
              }
              IDAM_LOGF(LOG_DEBUG, "Metadata Plugin Executed\nSignal Type: %c", signal_desc->type);

        // Plugin? Create a new Request Block to identify the request_id

              if(signal_desc->type == 'P'){
                 strcpy(request_block.signal,signal_desc->signal_name);
		 strcpy(request_block.source,data_source->path); 
                 makeServerRequestBlock(&request_block, pluginList);
              }

#endif		// NOTGENERICENABLED


    }        // end of REQUEST_READ_GENERIC

//------------------------------------------------------------------------------
// Client XML Specified Composite Signal
//------------------------------------------------------------------------------

    if (request_block.request == REQUEST_READ_XML) {
        if (strlen(request_block.signal) > 0)
            strcpy(signal_desc->xml, request_block.signal);    // XML is passed via the signal string
        else if (strlen(request_block.path) > 0) {            // XML is passed via a file
            FILE* xmlfile = NULL;
            int nchar;
            errno = 0;
            xmlfile = fopen(request_block.path, "r");
            serrno = errno;
            if (serrno != 0 || xmlfile == NULL) {
                err = 122;
                if (serrno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "idamserverReadData", serrno, "");
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err,
                             "Unable to Open the XML File defining the signal");
                if (xmlfile != NULL) fclose(xmlfile);
                return err;
            }
            nchar = 0;
            while (!feof(xmlfile) && nchar < MAXMETA) request_block.signal[nchar++] = getc(xmlfile);
            request_block.signal[nchar - 2] = '\0';    // Remove EOF Character and replace with String Terminator
            strcpy(signal_desc->xml, request_block.signal);
            fclose(xmlfile);
        } else {
            err = 123;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err,
                         "There is NO XML defining the signal");
            return err;
        }
        signal_desc->type = 'C';
        return (-1);
    }


//------------------------------------------------------------------------------
// Read Data via a Suitable Registered Plugin using a standard interface
//------------------------------------------------------------------------------

// Test for known File formats and Server protocols

    {
        int id, reset;
        IDAM_PLUGIN_INTERFACE idam_plugin_interface;

        IDAM_LOG(LOG_DEBUG, "creating the plugin interface structure\n");

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
        idam_plugin_interface.environment = &environment;
        idam_plugin_interface.sqlConnection = NULL;
        idam_plugin_interface.verbose = 0;
        idam_plugin_interface.housekeeping = 0;
        idam_plugin_interface.changePlugin = 0;
        idam_plugin_interface.pluginList = &pluginList;

        if (request_block.request != REQUEST_READ_GENERIC && request_block.request != REQUEST_READ_UNKNOWN) {
            plugin_id = request_block.request;            // User has Specified a Plugin
            IDAM_LOGF(LOG_DEBUG, "Plugin Request ID %d\n", plugin_id);
        } else {
            plugin_id = findPluginRequestByFormat(data_source->format, &pluginList);    // via Generic database query
            IDAM_LOGF(LOG_DEBUG, "findPluginRequestByFormat Plugin Request ID %d\n", plugin_id);
        }

        IDAM_LOGF(LOG_DEBUG, "(idamServerGetData) Number of PutData Blocks: %d\n",
                request_block.putDataBlockList.blockCount);

        if (request_block.request != REQUEST_READ_GENERIC && request_block.request != REQUEST_READ_UNKNOWN) {

            if ((id = findPluginIdByRequest(plugin_id, &pluginList)) == -1) {
                IDAM_LOGF(LOG_DEBUG, "Error locating data plugin %d\n", plugin_id);
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err, "Error locating data plugin");
                return err;
            }

#ifndef ITERSERVER
            if (pluginList.plugin[id].private == PLUGINPRIVATE && environment.external_user) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err,
                             "Access to this data class is not available.");
                return err;
            }
#endif
            if (pluginList.plugin[id].external == PLUGINEXTERNAL &&
                pluginList.plugin[id].status == PLUGINOPERATIONAL &&
                pluginList.plugin[id].pluginHandle != NULL &&
                pluginList.plugin[id].idamPlugin != NULL) {

                IDAM_LOGF(LOG_DEBUG, "[%d] %s Plugin Selected\n", plugin_id, data_source->format);

// Redirect Output to temporary file if no file handles passed

                reset = 0;
                if ((err = idamServerRedirectStdStreams(reset)) != 0) {
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err,
                                 "Error Redirecting Plugin Message Output");
                    return err;
                }

// Initialise general structure passing components

                logmalloclist = (LOGMALLOCLIST*) malloc(sizeof(LOGMALLOCLIST));
                initLogMallocList(logmalloclist);
                copyUserDefinedTypeList(
                        &userdefinedtypelist);                // Allocate and Copy the Master User Defined Type Lis

// Call the plugin

                err = pluginList.plugin[id].idamPlugin(&idam_plugin_interface);

// Reset Redirected Output

                reset = 1;
                if ((rc = idamServerRedirectStdStreams(reset)) != 0 || err != 0) {
                    if (rc != 0)
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", rc,
                                     "Error Resetting Redirected Plugin Message Output");
                    if (err != 0) return err;
                    return rc;
                }

                IDAM_LOG(LOG_DEBUG, "returned from plugin called\n");

// Save Provenance with socket stream protection

                idamServerRedirectStdStreams(0);
                idamProvenancePlugin(&client_block, &request_block, data_source, signal_desc, &pluginList, NULL);
                idamServerRedirectStdStreams(1);

// If no structures to pass back (only regular data) then free the user defined type list

                if (data_block->opaque_block == NULL) {

                    if (data_block->opaque_type == OPAQUE_TYPE_STRUCTURES && data_block->opaque_count > 0) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", rc,
                                     "Opaque Data Block is Null Pointer");
                        return err;
                    }

                    freeMallocLogList(logmalloclist);
                    free((void*) logmalloclist);
                    logmalloclist = NULL;
                    freeUserDefinedTypeList(userdefinedtypelist);
                    free((void*) userdefinedtypelist);
                    userdefinedtypelist = NULL;
                }

                if (!idam_plugin_interface.changePlugin) return 0;        // job done!

                request_block.request = REQUEST_READ_GENERIC;            // Use a different Plugin
            }
        }
    }


    plugin_id = REQUEST_READ_UNKNOWN;

    if (request_block.request != REQUEST_READ_GENERIC) {
        plugin_id = request_block.request;            // User API has Specified a Plugin
    } else {

// Test for known File formats and Server protocols

        id = -1;
        for (i = 0; i < pluginList.count; i++) {
            if (!strcasecmp(data_source->format, pluginList.plugin[i].format)) {
                plugin_id = pluginList.plugin[i].request;                // Found
                id = i;
                IDAM_LOGF(LOG_DEBUG, "[%d] %s Plugin Selected\n", plugin_id, data_source->format);
                break;
            }
        }

        if (id >= 0 && pluginList.plugin[id].private == PLUGINPRIVATE && environment.external_user) {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err,
                         "Access to this data class is not available.");
            return err;
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
            IDAM_LOGF(LOG_DEBUG, "MDS+ Tree Number %d\n", data_source->exp_number);
            IDAM_LOGF(LOG_DEBUG, "MDS+ Tree Path %s\n", signal_desc->signal_name);
        }

// Don't append the file name to the path - if it's already present!

        if (strlen(data_source->path) == 0) {        // No path in Data_Source record so must be on default Archive Path
            if (request_block.pass == -1 && request_block.tpass[0] == '\0') {    // Always LATEST
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
        IDAM_LOG(LOG_DEBUG, "IdamServer: No Plugin Selected\n");
    }
    IDAM_LOGF(LOG_DEBUG, "IdamServer: Archive      : %s \n", data_source->archive);
    IDAM_LOGF(LOG_DEBUG, "IdamServer: Device Name  : %s \n", data_source->device_name);
    IDAM_LOGF(LOG_DEBUG, "IdamServer: Signal Name  : %s \n", signal_desc->signal_name);
    IDAM_LOGF(LOG_DEBUG, "IdamServer: File Path    : %s \n", data_source->path);
    IDAM_LOGF(LOG_DEBUG, "IdamServer: File Name    : %s \n", data_source->filename);
    IDAM_LOGF(LOG_DEBUG, "IdamServer: Pulse Number : %d \n", data_source->exp_number);
    IDAM_LOGF(LOG_DEBUG, "IdamServer: Pass Number  : %d \n", data_source->pass);

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
    idamProvenancePlugin(&client_block, &request_block, data_source, signal_desc, &pluginList, NULL);
    idamServerRedirectStdStreams(1);

//----------------------------------------------------------------------------
// DUMPs ? (Not if redirected) (Legacy version - should be moved to plugins)

    err = 0;

#ifndef PROXYSERVER
    if (!strcasecmp(request_block.archive, "DUMP") && environment.server_proxy[0] == '\0') {

        IDAM_LOG(LOG_DEBUG, "Requested: DUMP File Contents.\n");

        if ((err = dumpFile(request_block, data_block)) != 0) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err, "Error Dumping IDA File Contents");
            return err;
        }
        printDataBlock(*data_block);

        return (err);
    }
#endif

//----------------------------------------------------------------------------
// Call the Requested Data Access Routine (Legacy version with embedded data readers)

// Redivert all message output to stdout and stderr to a temporary file

    int reset = 0;
    if ((err = idamServerRedirectStdStreams(reset)) != 0) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err,
                     "Error Redirecting Plugin Message Output");
        return err;
    }

    err = 0;

    switch (plugin_id) {

        case REQUEST_READ_IDA :
            IDAM_LOG(LOG_DEBUG, "Requested Data Access Routine = readIDA2 \n");
            if ((err = readIDA2(*data_source, *signal_desc, data_block)) != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err, "Error Accessing IDA Data");
            }
            break;

        case REQUEST_READ_MDS :
            IDAM_LOG(LOG_DEBUG, "Requested Data Access Routine = readMDS \n");
            if ((err = readMDS(*data_source, *signal_desc, data_block)) != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err, "Error Accessing MDS+ Data");
            }
            break;
#ifndef DGM8OCT14
        case REQUEST_READ_IDAM :
            if ((err = readIdam(*data_source, *signal_desc, request_block, data_block)) != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err, "Error Accessing IDAM Data");
            }
            IDAM_LOGF(LOG_DEBUG, "IDAM server to IDAM server request via readIDAM Error? %d\n", data_block->errcode);
            break;
#endif
        case REQUEST_READ_CDF :
            IDAM_LOG(LOG_DEBUG, "Requested Data Access Routine = readCDF \n");
            if ((err = readCDF(*data_source, *signal_desc, request_block, data_block)) != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err, "Error Accessing netCDF Data");
            }
            IDAM_LOG(LOG_DEBUG, "Returned from readCDF \n");
            break;

        case REQUEST_READ_HDF5 :
            IDAM_LOG(LOG_DEBUG, "Requested Data Access Routine = readHDF5 \n");
            if ((err = readHDF5(*data_source, *signal_desc, data_block)) != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err, "Error Accessing HDF5 Data");
            }
            break;

        case REQUEST_READ_UFILE :
            IDAM_LOG(LOG_DEBUG, "Requested Data Access Routine = readUFile \n");
            if ((err = readUFile(*data_source, *signal_desc, data_block)) != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err, "Error Accessing UFile Data");
            }
            break;

        case REQUEST_READ_PPF :
            IDAM_LOG(LOG_DEBUG, "Requested Data Access Routine = readPPF \n");
            if ((err = readPPF(*data_source, *signal_desc, data_block)) != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err, "Error Accessing PPF Data");
            }
            break;

        case REQUEST_READ_JPF :
            IDAM_LOG(LOG_DEBUG, "Requested Data Access Routine = readJPF \n");
            if ((err = readJPF(*data_source, *signal_desc, data_block)) != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err, "Error Accessing JPF Data");
            }
            break;

        case REQUEST_READ_FILE :
            IDAM_LOG(LOG_DEBUG, "Requested Data Access Routine = readBytes \n");
            if ((err = readBytes(*data_source, *signal_desc, data_block)) != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err,
                             "Error Accessing File Contents (Bytes)");
            }
            break;

        case REQUEST_READ_NOTHING :
            IDAM_LOG(LOG_DEBUG, "Requested No Data Access Routine\n");
            if ((err = readNothing(*data_source, *signal_desc, data_block)) != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err,
                             "Error Reading 'Nothing', i.e. Generating Test Data");
            }
            break;

        case REQUEST_READ_SQL :
            IDAM_LOG(LOG_DEBUG, "Requested SQL Plugin \n");
            if ((err = readSQL(DBConnect, request_block, *data_source, data_block)) != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err, "Error Reading SQL Data");
            }
            IDAM_LOG(LOG_DEBUG, "Returned from SQL Plugin \n");
            printDataBlock(*data_block);
            break;

        case REQUEST_READ_HDATA:
            IDAM_LOG(LOG_DEBUG, "Requested Hierarchical Data Plugin \n");
            if ((err = readHData(DBConnect, request_block, *data_source, data_block)) != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err,
                             "Error Reading Hierarchical Data");
            }
            printDataBlock(*data_block);
            break;

        default:
            IDAM_LOGF(LOG_DEBUG, "Unknown Requested Data Access Routine (%d) \n", request_block.request);
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", err,
                         "Unknown Data Accessor Requested - No IDAM Plug-in");
            break;
    }

// Reset Redirected Output

    reset = 1;
    if ((rc = idamServerRedirectStdStreams(reset)) != 0 || err != 0) {
        if (rc != 0)
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamserverReadData", rc,
                         "Error Resetting Redirected Plugin Message Output");
    }

    return err;
}


