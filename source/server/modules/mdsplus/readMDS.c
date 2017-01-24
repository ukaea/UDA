//! $LastChangedRevision: 353 $
//! $LastChangedDate: 2013-11-18 15:32:28 +0000 (Mon, 18 Nov 2013) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/plugins/mdsplus/readMDS.c $

/*---------------------------------------------------------------
* IDAM Plugin data Reader to Access DATA from MDS+ Trees
*
* Input Arguments:	DATA_SOURCE data_source
*			SIGNAL_DESC signal_desc
*
* Returns:		readMDS		0 if read was successful
*					otherwise a Error Code is returned
*			DATA_BLOCK	Data read from the MDS+ Tree
*
* Calls		readMDSDim 	for Dimensional MDS+ Data
*		freeDataBlock	to free Heap memory if an Error Occurs
*
* Notes: 	All memory required to hold data received from the Tree is
*		allocated dynamically in heap storage. Pointers to these
*		areas of memory are held by the passed DATA_BLOCK structure.
*		Local memory allocation are freed on exit. However, the blocks
*		reserved for data are not and MUST BE freed by the calling
*		routine.
*
* ToDo:		Calling pingServer multiple times causes a hang! Needs further
*		investigation if the IDAM Server is to serve multiple client
*		requests with server checking enabled.
*
* Revision 1.0  06Jul2005	D.G.Muir
* v1.1   12Jan2006	D.G.Muir	call to initDimBlock added
* v1.2   19Jul2006	D.G.Muir	Free units correctly prior to return
* v1.3   12Feb2007	D.G.Muir	If the Tree Name is missing then assume a TDI function has been called
* v1.4   21Mar2007	D.G.Muir	server_socketlist argument added to socket management functions
* v1.5   09Jul2007	D.G.Muir	debugon, verbose enabled
* 29Oct2007	dgm	ERRORSTACK Components added
* 08Jan2008	dgm	Changed local to localhost - avoiding the use of mdsconnect
* 10Jan2008	dgm	If the first part of the server URL is localhost, then the second part is the path
*			to the data tree. An environment variable is then set up to identify where MDS+
*			can find the data.
* 17Jul2008	dgm	If Unable to Retrieve the Data Units Length then ignore Data Units (Previously error exit)
* 17Jul2008	dgm	Call MdsClose only if a Tree was opened.
* 22Sep2008	dgm	All Standard data types added to float only
* 20Oct2008	dgm	Long node names or TDI Arguments can alternatively be passed in the xml structure member
*                       'signal' string length changed from STRING_LENGTH to MAXMETA+5
* 07Nov2008	dgm	Tidy up of Path names attached to Server names
* 18May2010	dgm	Added initialisation of JET MDS+ Sand-Box with compiler option MDSSANDBOX
* 08Jul2010	dgm	Given precedence of / over . for usr like path names
* 25Mar2011	dgm	MDS+ uses printf for error messages so if a problem occurs MDS+ writes to the XDR socket. To
*			avoid this, create an exception file handle to catch these messages. The SANDBOX prints exceptions to
*			the MDS+ stderr file descriptor. It is not necessary to initialise MDS+ unless the local client is used
*			rather than a server.
* 13Jun2011	dgm	There is a problem with LD_PRELOAD on 64bit architecture: If fails to load 32 bit libraries. To
*			switch off the sandbox for localhost use the environment variable IDAM_BAD_PRELOAD
* 22Dec2011	dgm	changed order from 0 to -1 as time dimension is not known.
* 09Mar2012	dgm	Message output redirection now external to this routine.
* 09Mar2012	dgm	Disable use of 'localhost' MDS+ if the security sandbox is not operating (if the
*			environment variable IDAM_BAD_PRELOAD is set).
*--------------------------------------------------------------*/

#include "readMDS.h"

#include <clientserver/idamErrors.h>
#include "idamServerStartup.h"
#include "idamErrorLog.h"
#include "idamserverconfig.h"

#ifdef NOMDSPLUSPLUGIN

int readMDS(DATA_SOURCE data_source,
            SIGNAL_DESC signal_desc,
            DATA_BLOCK *data_block) {
    int err = 999;
    addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDS", err, "MDSPLUS PLUGIN NOT ENABLED");
    return(err);
}

#else

#include <mdslib.h>
#include <mdstypes.h>
#include <mdsdescrip.h>
#include <mdsshr.h>

#include "readMDSDim.h"
#include "manageSockets.h"
#include "TrimString.h"
#include "freeDataBlock.h"
#include "initStructs.h"
#include "idamLog.h"

#define status_ok(status) (((status) & 1) == 1)

#ifdef MDSSANDBOX
void __mdscall_init();		// MDS+ Security sand-box
#endif

int readMDSType(int type) {
    switch(type) {
    case(DTYPE_NATIVE_FLOAT):
        return TYPE_FLOAT;			// float
    case(DTYPE_NATIVE_DOUBLE):
        return TYPE_DOUBLE;			// double
    case(DTYPE_FLOAT):
        return TYPE_FLOAT;			// float
    case(DTYPE_DOUBLE):
        return TYPE_DOUBLE;			// double
//	case(DTYPE_F):			return TYPE_FLOAT;			// 4 byte float
//	case(DTYPE_D):			return TYPE_DOUBLE;			// 8 byte float
    case(DTYPE_G):
        return TYPE_DOUBLE;			// 8 byte float
    case(DTYPE_UCHAR):
        return TYPE_UNSIGNED_CHAR;		// unsigned char
//	case(DTYPE_BU):			return TYPE_UNSIGNED_CHAR;		// unsigned byte
    case(DTYPE_CHAR):
        return TYPE_CHAR;			// char
//	case(DTYPE_B):			return TYPE_CHAR;			// signed byte
    case(DTYPE_USHORT):
        return TYPE_UNSIGNED_SHORT;		// unsigned short
//	case(DTYPE_WU):			return TYPE_UNSIGNED_SHORT;		// unsigned 2 byte word
    case(DTYPE_SHORT):
        return TYPE_SHORT;			// signed short
//	case(DTYPE_W):			return TYPE_SHORT;			// signed 2 byte word
    case(DTYPE_ULONG):
        return TYPE_UNSIGNED ;			// unsigned integer
//	case(DTYPE_LU):			return TYPE_UNSIGNED ;			// unsigned 4 byte word
    case(DTYPE_LONG):
        return TYPE_INT;			// signed integer
//	case(DTYPE_L):			return TYPE_INT;			// signed 4 byte word
    case(DTYPE_ULONGLONG):
        return TYPE_UNSIGNED_LONG64 ;		// unsigned long long integer
//	case(DTYPE_QU):			return TYPE_UNSIGNED_LONG64 ;		// unsigned 8 byte word
    case(DTYPE_LONGLONG):
        return TYPE_LONG64;			// signed long long byte word
//	case(DTYPE_Q):			return TYPE_LONG64;			// signed 8 byte word
    default:
        return TYPE_UNKNOWN;
    }
    return TYPE_UNKNOWN;
}

int readMDS(DATA_SOURCE data_source,
            SIGNAL_DESC signal_desc,
            DATA_BLOCK *data_block) {

    int dtype_float  = DTYPE_FLOAT;
    int dtype_double = DTYPE_DOUBLE;

    int dtype_char   = DTYPE_CHAR;
    int dtype_uchar  = DTYPE_UCHAR;
    int dtype_short  = DTYPE_SHORT;
    int dtype_ushort = DTYPE_USHORT;
    int dtype_int    = DTYPE_LONG;
    int dtype_uint   = DTYPE_ULONG;
    int dtype_long64  = DTYPE_LONGLONG;
    int dtype_ulong64 = DTYPE_ULONGLONG;

    int dtype_str    = DTYPE_CSTRING;

    //struct descrip tdiarg;

    int i, err, rc=0, status, type, lpath=0;
    int socket = 0;
    //int treeno;
    int rank, size, null, desc, lunits;
    long lint = 1;
    int  len  = 0;


    char  *sdim = NULL;
    char  *units= NULL;
    void  *data = NULL;
    DIMS  *ddim = NULL;

    char server[MAXSERVER];
    char env[MAXSERVER];
    char signal[MAXMETA+5];
    char *tree  ;
    char *node  ;
    char *path  ;
    char *cmd    = NULL;
    char *envtst = NULL;
    int  treenum;
    int  mdsport = 0;

    SOCKET mdssocket = -1;

    int server_called = 0;

    //float test;
    float *ptest;

    static int sand_box = 0;		// Flags sand-box initialisation called - Once only

//----------------------------------------------------------------------
// Client Identifies the MDS Server (Lower case) and Tree

    tree    = data_source.filename;
    node    = signal_desc.signal_name;
    treenum = data_source.exp_number;

    if(node[0] == '\0') node = signal_desc.xml;				// Must be passed in the XML tag (temporary fix!)

    strncpy(server, data_source.server, MAXSERVER-1) ; 			// case is significant?
    server[MAXSERVER-1] = '\0';

//----------------------------------------------------------------------
// Error Management Loop

    err    = 0;
    null   = 0;
    env[0] = '\0';

    do {

//----------------------------------------------------------------------
// Create any Required environment variables for LOCALHOST Service (MDS+ Servers run in their own private environment)
// Paths can take two forms: /a/b/c or .a.b.c (. changed to /)
// If a mixture is used, / has precedence (. preserved)
//
// localhost path setting is NOT allowed for EXTERNAL clients because of security issues with MDS+

        if(!strncasecmp(data_source.server, "localhost.", 10)) data_source.server[9] = '/';	// For Parsing

        if((path = strstr(data_source.server,"/")) != NULL) {			// The Server contains the path to the data
            strncpy(server, data_source.server,path-data_source.server) ; 		// Extract the Server Name
            server[path-data_source.server] = '\0';

            if(!strcasecmp(server, "localhost")) {

                lpath = strlen(path);

#ifdef FATCLIENT
                if(getenv("IDAM_EXTERNAL_USER") != NULL) {		// Used for Testing purposes only
#else
                if(environment.external_user) {
#endif
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDS", err, "Disabled: Creation of the MDS+ TREE Environment Variable");
                    idamLog(LOG_DEBUG, "readMDS: Disabled: Creation of the Environment Variable to %s\n", path);
                    break;
                } else {

                    if(IsLegalFilePath(path)) {						// Check for unusual embedded characters
                        if(lpath > 1 && (cmd = strchr(&path[1], '/')) == NULL) {		// Check for / path delimiter. If not found then
                            for(i=1; i<lpath; i++) if(path[i]=='.') path[i]='/';		// change from URL Notation to Path Tree Notation
                        }
                        sprintf(env,"%s_path",tree);
                        if((rc = setenv(env,path,1)) != 0) { 				// Set an Environment Variable for MDS+ to locate the tree
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDS", err, "Unable to Create TREE Environment Variable");
                            idamLog(LOG_DEBUG, "readMDS: Error Creating Environment Variable %s [%d][%d]\n", env, rc, errno);
                            break;
                        }

                        idamLog(LOG_DEBUG, "readMDS: Creating Environment Variable %s [%d]\n", env, rc);
                        if((envtst = getenv(env)) != NULL) {
                            idamLog(LOG_DEBUG, "readMDS: Testing Environment OK %s \n", envtst);
                        } else {
                            idamLog(LOG_DEBUG, "readMDS: Environment Variable %s does not exist!\n", env );
                        }

                    } else {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDS", err, "Syntax Error in Path to Data Tree");
                        idamLog(LOG_DEBUG, "readMDS: Syntax Error in Path to Data Tree %s\n", path);
                        break;
                    }
                }
            } else {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDS", err, "Unable to Set Tree Paths for Remote MDSPlus Servers");
                idamLog(LOG_DEBUG, "readMDS: Unable to Set Tree Paths for Remote MDSPlus Servers - %s\n", path);
                break;
            }
        }

        if(strlen(server) == 0) {
            strcpy(server, LOCAL_MDSPLUS_SERVER);		// Need a Server!!! - Use the Default

            char *p = getenv("IDAM_MDSPLUSHOST");
            if(p != NULL) strcpy(server, p);		// Over-ruled by the server script
        }

        idamLog(LOG_DEBUG, "readMDS: Server Name: %s \n", server);
        idamLog(LOG_DEBUG, "readMDS: Tree Name  : %s \n", tree);
        idamLog(LOG_DEBUG, "readMDS: Node Name  : %s \n", node);
        idamLog(LOG_DEBUG, "readMDS: Tree Number: %d \n", treenum);

//----------------------------------------------------------------------
// Connect to an MDS+ Server if Not Local (All local MDS+ servers are protected by the sandbox)

        if (strcasecmp(server, "localhost") != 0) {

            type   = TYPE_MDSPLUS_SERVER;
            status = 1;
            if((rc = getSocket(&server_socketlist, type, &status, server, mdsport, &socket)) == 1) {	// Check if the Server is Connected
                mdssocket = -1;
                MdsSetSocket(&mdssocket);	// Flags that a New Socket is to be opened without closing the previous one
                socket = MdsConnect(server);	// Not Found or Not Open
                status = 1;
            } else {
                mdssocket = (SOCKET) socket;
                MdsSetSocket(&mdssocket);	// Switch to a different existing and Open Socket
            }

            if ( socket == -1 || !status) {
                err = MDS_ERROR_CONNECTING_TO_SERVER;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDS", err, "Unable to Connect to MDSPlus Server");
                break;
            } else
                rc = addSocket(&server_socketlist, type, status, server, mdsport, socket);

            server_called = 1;

            idamLog(LOG_DEBUG, "readMDS: Socket fd: %d \n", socket);

        } else {

            idamLog(LOG_DEBUG, "readMDS: LOCALHOST Service\n");

#ifdef MDSSANDBOX
            if(!sand_box) {
                if(getenv("IDAM_BAD_PRELOAD") == NULL) {	// LD_PRELOAD of 32bit libraries doesn't work on 64bit host!
                    // initialise the MDS+ sand-box for the local MDS+ client
                    __mdscall_init();		// Ensure libmdscall sand-box is called before any MDS+ function
                    // The MDS+ server has its own private sand-box!
                } else {
                    idamLog(LOG_DEBUG,"MDSPlus Security Sand-Box Disabled because of Bad Pre-Load\n");

                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDS", err, "localhost MDS+ disabled on this host");
                    idamLog(LOG_DEBUG, "readMDS: localhost MDS+ disabled on this host\n");
                    break;
                }
                sand_box = 1;

                idamLog(LOG_DEBUG,"MDSPlus Security Sand-Box Initialisation called\n");
            }
#endif

            mdssocket = -1;
            MdsSetSocket(&mdssocket); // Don't use an existing socket connection to a server => local MDS+ service

        }

//----------------------------------------------------------------------
// Open the MDS+ Tree (If Zero length then a TDI Function is being called)
//
//    d1 = descr( &dtype_long, &status, &null);
//    d2 = descr( &dtype_cstring, &text, &null, &lentext);
//    status = MdsValue("getmsg($)\0", &d1, &d2, &null, &len);
//    printf("%s\n",text);

//
//  alternative: (char *)MdsGetMsg(status));


        if(strlen(tree) > 0) {

            status = MdsOpen(tree, &treenum);	// Open the Tree

            if ( !status_ok(status) ) {
                err = MDS_ERROR_OPENING_TREE;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDS", err, "Unable to Open the MDSPlus Tree");
                idamLog(LOG_DEBUG, "readMDS: Error Opening MDS+ Tree \n");
                break;
            }

            idamLog(LOG_DEBUG, "readMDS: Tree Opened OK \n");

        } else {

            strcpy(signal, "_sig=");				// This name is private to the server

            if(signal_desc.signal_name[0] == '\0') {
                strcat(signal, signal_desc.xml); 		// The TDI Function to be called passed in the XML tag (temporary fix!)
            } else {
                strcat(signal, signal_desc.signal_name); 	// The TDI Function to be called
            }

            node = signal;

            idamLog(LOG_DEBUG, "readMDS: Executing Server Side TDI Function: %s\n", node);
            
            desc   = descr(&dtype_int, &lint, &null);
            status = MdsValue(node, &desc, &null, &len);

            if ( !status_ok(status) ) {
                err = MDS_ERROR_MDSVALUE_DATA ;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDS", err, "Unable to Access Data via the TDI Function");

// Any MDS+ messages?

                {
                    int lentext = 256;
                    char text[256];
                    int len = 0;
                    int d1 = descr( &dtype_int, &status, &null);
                    int d2 = descr( &dtype_str, &text, &null, &lentext);
                    status = MdsValue("getmsg($)\0", &d1, &d2, &null, &len);
                    idamLog(LOG_DEBUG, "%s\n", text);
                }

                break;
            }

            idamLog(LOG_DEBUG, "readMDS: Server Side TDI Function: %s Executed.\n", node);

            strcpy(node, "_sig"); 		// Internal Reference to the Signal's Data from this point forward
        }


//----------------------------------------------------------------------
// Data Rank ?

        null = 0;
        sdim = (char *) malloc((7+strlen(node))*sizeof(char) );

        if (sdim == NULL) {
            err = MDS_ERROR_ALLOCATING_HEAP_TDI_RANK ;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDS", err, "Unable to Allocate Heap for MdsValue Rank Enquiry");
            break;
        }

        sprintf(sdim,"RANK(%s)",node);

        desc = descr(&dtype_int, &rank, &null);
        status = MdsValue(sdim, &desc, &null, 0);

        if ( !status_ok(status) ) {
            err = MDS_ERROR_MDSVALUE_RANK ;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDS", err, "Unable to Retrieve the Data Rank");
            break;
        }

        data_block->rank = rank;

        idamLog(LOG_DEBUG,"readMDS: Data Rank %d\n", rank);

//----------------------------------------------------------------------
// Length of Data Array

        null = 0;
        sdim = (char *) realloc((void *)sdim, (size_t)(7+strlen(node))*sizeof(char) );

        if (sdim == NULL) {
            err = MDS_ERROR_ALLOCATING_HEAP_TDI_SIZE ;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDS", err, "Unable to Allocate Heap for MdsValue Size Enquiry");
            break;
        }

        sprintf(sdim,"SIZE(%s)",node);

        desc = descr(&dtype_int, &size, &null);
        status = MdsValue(sdim, &desc, &null, 0);

        if ( !status_ok(status) || size < 1) {
            err = MDS_ERROR_MDSVALUE_SIZE ;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDS", err, "Unable to Retrieve the Data Size");
            idamLog(LOG_DEBUG,"readMDS: Unable to Retrieve the Data Size\n");
            break;
        }

        data_block->data_n = size;

        idamLog(LOG_DEBUG,"readMDS: Length of Data Array %d\n", size);

//----------------------------------------------------------------------
// Data Type

        null = 0;
        sdim = (char *) realloc((void *)sdim, (size_t)(13+strlen(node))*sizeof(char) );

        sprintf(sdim,"KIND(DATA(%s))",node);

        desc   = descr(&dtype_int, &type, &null);
        status = MdsValue(sdim, &desc, &null, 0);

        if ( !status_ok(status) || size < 1) {
            err = MDS_ERROR_MDSVALUE_TYPE ;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDS", err, "Unable to Retrieve the Data Type");
            break;
        }

        idamLog(LOG_DEBUG,"readMDS: Type of Data %d [Float: %d]\n", type, DTYPE_NATIVE_FLOAT);


//----------------------------------------------------------------------
// Identify the Equivalent IDAM type

        if ((data_block->data_type = readMDSType(type)) == TYPE_UNKNOWN) {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDS", err, "Unknown Data Type");
            break;
        }

//----------------------------------------------------------------------
// Allocate Heap for Data, and define descriptor

        null = 0;

        switch(data_block->data_type) {
        case(TYPE_FLOAT): {
            data = malloc(size * sizeof(float));				// allocate memory for the signal
            desc = descr(&dtype_float, (float *)data, &size, &null);	// descriptor for this signal
            break;
        }
        case(TYPE_DOUBLE): {
            data = malloc(size * sizeof(double));
            desc = descr(&dtype_double, (double *)data, &size, &null);
            break;
        }
        case(TYPE_UNSIGNED_CHAR): {
            data = malloc(size * sizeof(unsigned char));
            desc = descr(&dtype_uchar, (unsigned char *)data, &size, &null);
            break;
        }
        case(TYPE_CHAR): {
            data = malloc(size * sizeof(char));
            desc = descr(&dtype_char, (char *)data, &size, &null);
            break;
        }
        case(TYPE_UNSIGNED_SHORT): {
            data = malloc(size * sizeof(unsigned short));
            desc = descr(&dtype_ushort, (unsigned short *)data, &size, &null);
            break;
        }
        case(TYPE_SHORT): {
            data = malloc(size * sizeof(short));
            desc = descr(&dtype_short, (short *)data, &size, &null);
            break;
        }
        case(TYPE_UNSIGNED): {
            data = malloc(size * sizeof(unsigned int));
            desc = descr(&dtype_uint, (unsigned int *)data, &size, &null);
            break;
        }
        case(TYPE_INT): {
            data = malloc(size * sizeof(int));
            desc = descr(&dtype_int, (int *)data, &size, &null);
            break;
        }
        case(TYPE_UNSIGNED_LONG64): {
            data = malloc(size * sizeof(unsigned long long));
            desc = descr(&dtype_ulong64, (unsigned long long*)data, &size, &null);
            break;
        }
        case(TYPE_LONG64): {
            data = malloc(size * sizeof(long long));
            desc = descr(&dtype_long64, (long long*)data, &size, &null);
            break;
        }
        }

        if (data == NULL) {
            err = MDS_ERROR_ALLOCATING_HEAP_DATA_BLOCK ;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDS", err, "Unable to Allocate Heap for the Data");
            break;
        }

//----------------------------------------------------------------------
// Read Data Array

        status = MdsValue(node, &desc, &null, 0 );		// retrieve signal

        if ( !status_ok(status) ) {
            err = MDS_ERROR_MDSVALUE_DATA ;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDS", err, "Unable to Retrieve the Data");
            break;
        }

        data_block->data = (char *)data;

//----------------------------------------------------------------------
// length of Data Units String

        null = 0;
        sdim = (char *) realloc((void *)sdim, (size_t)(16+strlen(node))*sizeof(char) );

        if (sdim == NULL) {
            err = MDS_ERROR_ALLOCATING_HEAP_TDI_LEN_UNITS ;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDS", err, "Unable to Allocate Heap for MdsValue Units Length Enquiry");
            break;
        }

        sprintf(sdim,"LEN(UNITS_OF(%s))",node);

        desc = descr(&dtype_int, &lunits, &null);
        status = MdsValue(sdim, &desc, &null, 0);

        if ( !status_ok(status) || lunits < 1) lunits = 0;

        lunits++;	// 1 Byte For Null Terminator

        idamLog(LOG_DEBUG,"Length of Units String %d\n", lunits);

//----------------------------------------------------------------------
// Data Units

        if(lunits > 1) {

            null = 0;
            sdim = (char *) realloc((void *)sdim, (size_t)(12+strlen(node))*sizeof(char) );

            if (sdim == NULL ) {
                err = MDS_ERROR_ALLOCATING_HEAP_TDI_UNITS ;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDS", err, "Unable to Allocate Heap for MdsValue Units Enquiry");
                break;
            }

            sprintf(sdim,"UNITS_OF(%s)",node);

            units = (char *)malloc(lunits*sizeof(char));

            if (units == NULL) {
                err = MDS_ERROR_ALLOCATING_HEAP_UNITS ;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDS", err, "Unable to Allocate Heap for the Data Units");
                break;
            }

            desc = descr(&dtype_str, units, &null, &lunits);
            status = MdsValue(sdim, &desc, &null, 0);

            if ( !status_ok(status) ) {
                err = MDS_ERROR_MDSVALUE_UNITS ;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDS", err, "Unable to Retrieve the Data Units");
                break;
            }

            units[lunits-1] = '\0';
            TrimString(units);

            strcpy(data_block->data_units, units);
            LeftTrimString(data_block->data_units);

        } else {
            units = NULL;
            strcpy(data_block->data_units, "");
        }

        idamLog(LOG_DEBUG,"readMDS: Data Units of signal %s\n", data_block->data_units);

//----------------------------------------------------------------------
// Length of Dimensional Arrays (The Order of the Time Dimension is Unknown!)

        ddim = (DIMS *)malloc(rank * sizeof(DIMS));
        idamLog(LOG_DEBUG,"ddim %p\n",ddim);

        if (ddim == NULL) {
            err = MDS_ERROR_ALLOCATING_HEAP_DATA_BLOCK ;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDS", err, "Unable to Allocate Heap for the Data Dimensions");
            break;
        }


        for (i = 0; i< rank; i++) {
            initDimBlock(&ddim[i]);
            if((err = readMDSDim( node, i, &ddim[i])) == 0) {

                idamLog(LOG_DEBUG,"readMDS: Dimension %d, Size %d, Units %s \n", i,
                                       ddim[i].dim_n, ddim[i].dim_units);

            } else {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDS", err, "Unable to Retrieve the Dimensional Data");
                break;
            }
        }

        data_block->dims = ddim;

        if(rank > 0) {
            float tot = 0.0;
            size = data_block->dims[0].dim_n;
            idamLog(LOG_DEBUG,"%d\n", size);
            idamLog(LOG_DEBUG,"%p\n", data_block->dims);

            ptest = (float *)data_block->dims[0].dim;
            if(size >20) for(i=0; i<20; i++) idamLog(LOG_DEBUG, "dim[0] %d  %f\n", i,ptest[i]);
            tot=0.0;
            for(i=0; i<size; i++) tot=tot+ptest[i];
            idamLog(LOG_DEBUG," Dim Data Sum = %f\n",tot);
        }

    }
    while(0);	// Always exit the Error Management Loop

//----------------------------------------------------------------------
// Log Error Status

    idamLog(LOG_DEBUG,"readMDS: Final Error Status = %d\n", err);

//----------------------------------------------------------------------

    if(err != 0)
        freeDataBlock( data_block );
    else {
        data_block->handle    = 1;	// Default values
        data_block->order     = -1;
        data_block->data_label[0] = '\0';
        data_block->data_desc[0]  = '\0';
    }

// Free Local Heap Memory

    if (units != NULL) free( (void *)units);
    if (sdim  != NULL) free( (void *)sdim );

// Clear any set Environment Variable

    if(env[0] !='\0') rc = unsetenv(env);

// Close the Tree: MDS+ Server Management ... (List of Open Servers maintained for Re-use)

    if(strlen(tree) > 0) status = MdsClose(tree, &treenum);

    return(err);
}

#include "readMDSDim.c"

#endif

