/*---------------------------------------------------------------
 * v1 IDAM Plugin Template: PPF plugin  ...
 *
 * Input Arguments:	IDAM_PLUGIN_INTERFACE *idam_plugin_interface
 *
 * Returns:		0 if the plugin functionality was successful
 *			otherwise a Error Code is returned
 *
 * Standard functionality:
 *
 *	help	a description of what this plugin does together with a
 *               list of functions available
 *
 *	reset	frees all previously allocated heap, closes file handles and
 *               resets all static parameters.
 *		This has the same functionality as setting the housekeeping
 *               directive in the plugin interface
 *		data structure to TRUE (1)
 *
 *	init	Initialise the plugin: read all required data and process.
 Retain staticly for future reference.
 *      api     Call the PPF API
 *
 *---------------------------------------------------------------------*/

#include "ppf.h"
#include "ppf_vars.h"

#include <string.h>

#include <include/idamserver.h>
#include <clientserver/initStructs.h>
#include <clientserver/idamTypes.h>
#include <clientserver/idamErrorLog.h>
#include <clientserver/stringUtils.h>
#include <logging/idamLog.h>

#define PLUGIN_NAME "ppf2plugin"
#define PLUGIN_CURRENT_VERSION 1

FILE * g_dbgout;

IDAMERRORSTACK * idamErrorStack;

static short g_init = 0;

int printf_(FILE * fp, const char * fmt, ...);

static int reset(IDAM_PLUGIN_INTERFACE * ipi)
{
    g_init = 0;
    return 0;
}

static int init(IDAM_PLUGIN_INTERFACE * ipi)
{
    g_init = 1;

    return 0;
}

static int help(IDAM_PLUGIN_INTERFACE * ipi)
{
    DATA_BLOCK * data_block = ipi->data_block;
    int i;
    char * p = (char *) malloc(sizeof(char) * 2 * 1024);

    strcpy(p, "PPF2 plugin: Functions Names\n\n"
            "f=PPFGO,shot=n, seq=n\n"
            "f=PDMSEQ,pulse=n,seq=n,owner=s,dda=s\n"
            "f=PPFUID,userid=s,rw=s\n"
            "f=PPFERR,rname=s,error_code=n\n"
            "f=PPFGQI, shot=n\n"
            "f=PPFGID, rw=s\n"
            "f=PPFPOK\n"
            "f=PDAINF\n"
            "f=PPFSEQ, shot=d, ndda=n\n"
            "f=PPFINF, comlen=n, ndda=n\n"
            "f=DDAINF, shot=n, dda=s, nwcom=n, ndt=n\n"
            "f=PDLPPF, shot=n, uid=n, dup=n, nseq=n\n"
            "f=PDLUSR, shot=n, dda=s, nusers=n\n"
            "f=PDMSDT, date=n, time=s\n"
            "f=PDMSHT\n"
            "f=PDSTAT\n"
            "f=PDSRCH, shot=n, dda=s, ndda=n\n"
            "f=PDSTD, shot=n\n"
            "f=PDTINF, shot=n, dda=s, ndt=n\n"
            "f=PPFDAT, shot=n, seq=n, dda=s, dtype=s, nx=d, nt=d\n"
            "f=PPFDTI, shot=n, seq=n, dda=s, dtype=s\n"
            "f=PPFDDA, shot=n, nseq=n, dda=s\n"
            "f=PPFGET, shot=n, dda=s, dtype=s\n"
            "f=PPFGSF, shot=n, seq=n, dda=s, dtype=s\n"
            "f=PPFGTS, shot=n, dda=s, dtype=s, twant=f\n"
            "f=PPFMOD, shot=n\n"
            "f=PPFONDISK, shot=n, dda=s\n"
            "f=PPFOWNERINFO, shot=n, seq=n, flag=n\n"
            "f=PPFSETDEVICE, device=s\n"
            "f=PPFSIZ, shot=n, seq=n, dda=s, user=s\n"
    );

    initDataBlock(data_block);

    data_block->rank = 1;
    data_block->dims = (DIMS *) malloc(data_block->rank * sizeof(DIMS));
    for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

    data_block->data_type = TYPE_STRING;
    strcpy(data_block->data_desc, "dsa Plugin: help = description of this plugin");

    data_block->data = (char *) p;

    data_block->dims[0].data_type = TYPE_UNSIGNED_INT;
    data_block->dims[0].dim_n = strlen(p) + 1;
    data_block->dims[0].compressed = 1;
    data_block->dims[0].dim0 = 0.0;
    data_block->dims[0].diff = 1.0;
    data_block->dims[0].method = 0;

    data_block->data_n = data_block->dims[0].dim_n;

    strcpy(data_block->data_label, "");
    strcpy(data_block->data_units, "");
    return 0;
}

int call_pdmseq(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfuid(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfgo(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfclo(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppferr(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfsqi(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfgqi(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfok(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfgid(IDAM_PLUGIN_INTERFACE * ipi);

int call_pdainf(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfseq(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfinf(IDAM_PLUGIN_INTERFACE * ipi);

int call_ddainf(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfdel(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfwri(IDAM_PLUGIN_INTERFACE * ipi);

int call_pdlppf(IDAM_PLUGIN_INTERFACE * ipi);

int call_pdlusr(IDAM_PLUGIN_INTERFACE * ipi);

int call_pdmsdt(IDAM_PLUGIN_INTERFACE * ipi);

int call_pdmsht(IDAM_PLUGIN_INTERFACE * ipi);

int call_pdstat(IDAM_PLUGIN_INTERFACE * ipi);

int call_pdstd(IDAM_PLUGIN_INTERFACE * ipi);

int call_pdsrch(IDAM_PLUGIN_INTERFACE * ipi);

int call_pdtinf(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfdat(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfdti(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfdda(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfget(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfgmd(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfgsf(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfgts(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfmod(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfopn(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfondisk(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfownerinfo(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfsetdevice(IDAM_PLUGIN_INTERFACE * ipi);

int call_ppfsiz(IDAM_PLUGIN_INTERFACE * ipi);

const char * pszErrorStrings[] = {
        "ERR_NONE", "ERR_UNKNOWN_FUNCTION", "ERR_FUNCTION_NOT_SUPPORTED", "ERR_PDMSEQ_SETUP", "ERR_PDMSEQ_EXECUTE",
        "ERR_PPFUID_SETUP", "ERR_PPFGO_SETUP", "ERR_PPFCLO_SETUP", "ERR_PPFERR_SETUP", "ERR_PPFSQI_SETUP",
        "ERR_PPFGQI_SETUP", "ERR_PPFGID_SETUP", "ERR_PPFPOK_SETUP", "ERR_PDAINF_SETUP", "ERR_PPFSEQ_SETUP",
        "ERR_PPFINF_SETUP", "ERR_DDAINF_SETUP", "ERR_PPFDEL_SETUP", "ERR_PPFWRI_SETUP", "ERR_PDLPPF_SETUP",
        "ERR_PDLPPPF_EXECUTE", "ERR_PDLUSR_SETUP", "ERR_PDMSDT_SETUP", "ERR_PDSTAT_SETUP", "ERR_PDSTD_SETUP",
        "ERR_PDSRCH_SETUP", "ERR_PDTINF_SETUP", "ERR_PPFDAT_SETUP", "ERR_PPFDTI_SETUP", "ERR_PPFDDA_SETUP",
        "ERR_PPFGET_SETUP", "ERR_PPFGSF_SETUP", "ERR_PPFGTS_SETUP", "ERR_PPFMOD_SETUP", "ERR_PPFONDISK_SETUP",
        "ERR_PPFOWNERINFO_SETUP", "ERR_PPFSETDEVICE_SETUP", "ERR_PPFIZ_SETUP"

};


const char * getErrorString(enum ERRORS err)
{
    const char * psz = NULL;

    if (err > ERR_NONE && err < ERR_LAST)
        psz = pszErrorStrings[err];

    return psz;
}

static int api_call(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    //assert( ERR_LAST == NELEMS( pszErrorStrings ) );

    if (ipi) {
        //DATA_BLOCK* data_block    = ipi->data_block;
        REQUEST_BLOCK * request_block = ipi->request_block;

        //PUTDATA_BLOCK_LIST* pdl = &(request_block->putDataBlockList);

        const NAMEVALUELIST * nvl = &(request_block->nameValueList);
        const NAMEVALUE * nv = nvl_name_find(nvl, STR_FUNCTION_TAG);

        if (nv) {
            const char * val = nv_value(nv);

            if (!strcmp("PDMSEQ", val))
                err = call_pdmseq(ipi);
            else if (!strcmp("PPFUID", val))
                err = call_ppfuid(ipi);
            else if (!strcmp("PPFGO", val))
                err = call_ppfgo(ipi);
            else if (!strcmp("PPFCLO", val))
                err = call_ppfclo(ipi);
                //err = ERR_FUNCTION_NOT_SUPPORTED;
            else if (!strcmp("PPFERR", val))
                err = call_ppferr(ipi);
            else if (!strcmp("PPFSQI", val))
                err = call_ppfsqi(ipi);
            else if (!strcmp("PPFGQI", val))
                err = call_ppfgqi(ipi);
            else if (!strcmp("PPFGID", val))
                err = call_ppfgid(ipi);
            else if (!strcmp("PPFPOK", val))
                err = call_ppfok(ipi);
            else if (!strcmp("PDAINF", val))
                err = call_pdainf(ipi);
            else if (!strcmp("PPFSEQ", val))
                err = call_ppfseq(ipi);
            else if (!strcmp("PPFINF", val))
                err = call_ppfinf(ipi);
            else if (!strcmp("DDAINF", val))
                err = call_ddainf(ipi);
            else if (!strcmp("PPFDEL", val))
                err = call_ppfdel(ipi);
            else if (!strcmp("PPFWRI", val))
                err = call_ppfwri(ipi);
            else if (!strcmp("PDLPPF", val))
                err = call_pdlppf(ipi);
            else if (!strcmp("PDLUSR", val))
                err = call_pdlusr(ipi);
            else if (!strcmp("PDMSDT", val))
                err = call_pdmsdt(ipi);
            else if (!strcmp("PDMSHT", val))
                err = call_pdmsht(ipi);
            else if (!strcmp("PDSTAT", val))
                err = call_pdstat(ipi);
            else if (!strcmp("PDSTD", val))
                err = call_pdstd(ipi);
            else if (!strcmp("PDSRCH", val))
                err = call_pdsrch(ipi);
            else if (!strcmp("PDTINF", val))
                err = call_pdtinf(ipi);
            else if (!strcmp("PPFDAT", val))
                err = call_ppfdat(ipi);
            else if (!strcmp("PPFDTI", val))
                err = call_ppfdti(ipi);
            else if (!strcmp("PPFDDA", val))
                err = call_ppfdda(ipi);
            else if (!strcmp("PPFGET", val))
                err = call_ppfget(ipi);
            else if (!strcmp("PPFGMD", val))
                err = call_ppfgmd(ipi);
            else if (!strcmp("PPFGSF", val))
                err = call_ppfgsf(ipi);
            else if (!strcmp("PPFGTS", val))
                err = call_ppfgts(ipi);
            else if (!strcmp("PPFMOD", val))
                err = call_ppfmod(ipi);
            else if (!strcmp("PPFOPN", val))
                err = call_ppfopn(ipi);
            else if (!strcmp("PPFONDISK", val))
                err = call_ppfondisk(ipi);
            else if (!strcmp("PPFOWNERINFO", val))
                err = call_ppfownerinfo(ipi);
            else if (!strcmp("PPFSETDEVICE", val))
                err = call_ppfsetdevice(ipi);
            else if (!strcmp("PPFSIZ", val))
                err = call_ppfsiz(ipi);
            else
                err = ERR_UNKNOWN_FUNCTION;

            if (err) {
                const char * pszError = getErrorString(err);

                addIdamError(&idamerrorstack, CODEERRORTYPE, PLUGIN_NAME, err, pszError);
            }
        }
    }

    return err;
}


static int readppf(IDAM_PLUGIN_INTERFACE * ipi);

int plugin_entry(IDAM_PLUGIN_INTERFACE * idam_plugin_interface)
{
    int err = 0;

    REQUEST_BLOCK * request_block = NULL;
    unsigned short housekeeping = 0;

    idamErrorStack = getIdamServerPluginErrorStack();
    initIdamErrorStack(&idamerrorstack);

    if (idam_plugin_interface->interfaceVersion >= 1) {

        idam_plugin_interface->pluginVersion = PLUGIN_CURRENT_VERSION;    // This plugin's version number

        request_block = idam_plugin_interface->request_block;
        housekeeping = idam_plugin_interface->housekeeping;

        // Don't copy the structure if housekeeping is requested
        // - may dereference a NULL or freed pointer!
        if (!housekeeping && idam_plugin_interface->environment != NULL)
            environment = *idam_plugin_interface->environment;
    } else {
        err = 999;
        idamLog(LOG_ERROR, "ERROR templatePlugin: Plugin Interface Version Unknown\n");

        addIdamError(&idamerrorstack, CODEERRORTYPE, PLUGIN_NAME, err,
                     "Plugin Interface Version is Not Known: Unable to execute the request!");
        concatIdamError(idamerrorstack, idamErrorStack);
        return err;
    }

    //-----------------------------------------------------------------------------
    // Heap Housekeeping

    // Plugin must maintain a list of open file handles and sockets:
    // loop over and close all files and sockets
    // Plugin must maintain a list of plugin functions called:
    //  loop over and reset state and free heap.
    // Plugin must maintain a list of calls to other plugins:
    //loop over and call each plugin with the housekeeping request
    // Plugin must destroy lists at end of housekeeping

    // A plugin only has a single instance on a server.
    //For multiple instances, multiple servers are needed.
    // Plugins can maintain state so recursive calls (on the same server)
    // must respect this.
    // If the housekeeping action is requested, this must be also applied
    // to all plugins called.
    // A list must be maintained to register these plugin calls to
    // manage housekeeping.
    // Calls to plugins must also respect access policy
    //and user authentication policy


    //---------------------------------------------------------------------------
    // Plugin Functions
    //---------------------------------------------------------------------------
    //  raise( SIGSTOP );
    const char * pszFunction = request_block->function;
    if (housekeeping || !strcasecmp(pszFunction, "reset")) {
        return reset(idam_plugin_interface);
    }
    else if (!strcasecmp(pszFunction, "init")
             || !strcasecmp(pszFunction, "initialise")) {

        return init(idam_plugin_interface);
    }


    if (!strcasecmp(pszFunction, "help")) {
        help(idam_plugin_interface);
    }
    else if (!strcasecmp(pszFunction, "read")) {
        readppf(idam_plugin_interface);
    }
    else if (!strcasecmp(pszFunction, "api")) {
        api_call(idam_plugin_interface);
    }
    else {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, PLUGIN_NAME, err, "Unknown function requested!");
    }


    //-------------------------------------------------------------------------
    // Housekeeping
    // Combine local errors with the Server's error stack
    concatIdamError(idamerrorstack, idamErrorStack);


    closeIdamError(&idamerrorstack);        // Free local plugin error stack

    return err;
}


#define NWCOM     20        // 80 Byte DDA Comment
#define NDTNAMS   500        // Overkill?

#define TEST    0        // Output Debug Data

static int readppf(IDAM_PLUGIN_INTERFACE * idam_plugin_interface)
{

    int nwcom = NWCOM, ndt = NDTNAMS, lxtv[2 * NDTNAMS], err = 0, err2 = 0, lowner = 0, xsubset = 0;
    int pulno, pass, rank, order, nx, nt, lun = 0, ndmax, dtid, i;
    int irdat[13], iwdat[13];
    int swap[] = { 0, 1, 0 };            // Finding the Correct Dimension Index

    char ddacom[4 * NWCOM + 1], dtnams[4 * NDTNAMS + 1];
    char dda[5], dtype[5], ihdat[60], msg[81];
    char test[5];

    float * dvec = NULL, * xvec = NULL, * tvec = NULL, * sdvec = NULL;    //Data, x vector and T vector arrays

    int * plxtv = lxtv;

    DATA_BLOCK * data_block = idam_plugin_interface->data_block;
    //REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;
    SIGNAL_DESC * signal_desc = idam_plugin_interface->signal_desc;
    DATA_SOURCE * data_source = idam_plugin_interface->data_source;


    static int pwdstatus = 1;

    //--------------------------------------------------------------------
    // Extract Signal from SubSet Instruction if present.
    // Model: "dtyp(n)" where dtyp is the Datatype Name and n is the Dimension Slice to Subset >= 1
    // Default is X Slice: No syntax specifying which dimension
    // Reduce Rank by 1 and subset the X-Dimension as requested

    TrimString(signal_desc->signal_name);

    char *p2 = strrchr(signal_desc->signal_name, ')');
    if (p2 != NULL) {
        if (TEST)fprintf(stdout, "Non-Standard PPF Signal: %s\n", signal_desc->signal_name);

        p2[0] = '\0';
        strcpy(dtype, "    ");
        
	char *p1 = strrchr(signal_desc->signal_name, '(');
	if (p1 != NULL && p2-p1 > 1) {
           p1[0] = '\0';
           if((xsubset = atoi(&p1[1])) == 0){
	      err = 999;
              addIdamError(&idamerrorstack, CODEERRORTYPE, "readPPF", err, "Subsetting the X-Dimension begins at slice 1 not 0!");
              return err;
           }
	} else {
	    err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, PLUGIN_NAME, err, "Non-Standard PPF Signal Name Syntax Error!");
            return err;
        }
        strcpy(dtype, signal_desc->signal_name);
        if (TEST){
	    fprintf(stdout, "Data Type         : %s\n", dtype);
            fprintf(stdout, "X Dimension Subset: %d\n", xsubset);	    
        }
    } else {
        if (TEST) fprintf(stdout, "Standard PPF Signal: %s\n", signal_desc->signal_name);
        strncpy(dtype, signal_desc->signal_name, 4);
        dtype[4] = '\0';
    }

// Bug Fix: D.G.Muir 20Dec2016
/*
    if (signal_desc->signal_name[ldtype - 1] == ')') {
        if (TEST)fprintf(stdout, "Non-Standard PPF Signal: %s\n", signal_desc->signal_name);

        signal_desc->signal_name[ldtype - 1] = '\0';
        strcpy(dtype, "    ");
        for (i = 0; i < 5; i++) {
            if (signal_desc->signal_name[i] != '(') {
                if (i <= 3) dtype[i] = signal_desc->signal_name[i];
            } else {
                xsubset = atoi(signal_desc->signal_name + i + 1);
                if (TEST) {
                    fprintf(stdout, "Data Type         : %s\n", dtype);
                    fprintf(stdout, "X Dimension Subset: %d\n", xsubset);
                }
            }
        }
    } else {
        if (TEST) fprintf(stdout, "Standard PPF Signal: %s\n", signal_desc->signal_name);
        strncpy(dtype, signal_desc->signal_name, 4);
        dtype[4] = '\0';
    }
*/

    //--------------------------------------------------------------------
    // Signal etc.

    strncpy(dda, data_source->filename, 4);    // Only 4 Characters Allowed!
    dda[4] = '\0';

    strupr(dda);                    // Change Case to Upper
    strupr(dtype);
    TrimString(dtype);

    pulno = (int) data_source->exp_number;
    pass = (int) data_source->pass;        // PPF Sequence Number

    if (TEST) {
        fprintf(stdout, "Pulse : %d\n", pulno);
        fprintf(stdout, "Seq.  : %d\n", pass);
        fprintf(stdout, "DDA   : %s\n", dda);
        fprintf(stdout, "Signal: %s\n", dtype);
        fprintf(stdout, "Owner : %s\n", data_source->path);
    }

    //--------------------------------------------------------------------
    // Setup PPFUID call - specify the user name used for reading data

#ifndef FATCLIENT
    if (pwdstatus) {
        PPFPWD("ppfread", "ppfread", 8, 8);            // Read Only Permission (No .netrc needed)
        pwdstatus = 0;
    }
#endif

    if ((lowner = strlen(data_source->path)) > 0)
        PPFUID(data_source->path, "R", lowner, 1);        // Private PPF's Only
    else
        PPFUID("JETPPF", "R", 7, 1);            // Public PPF's Only

    //--------------------------------------------------------------------
    // Call PPFGO with the specified shot and sequence number

    if (pass >= 0)
        PPFGO(&pulno, &pass, &lun, &err);
    else
        PPFGO(&lun, &lun, &lun, &err);

    if (err != 0) {
        PPFERR("PPFGO", &err, msg, &err2, 6, 81);
        msg[80] = '\0';
        TrimString(msg);
        if (err2 != 0)
            addIdamError(&idamerrorstack, CODEERRORTYPE, PLUGIN_NAME, err, "PPFGO Error");
        else
            addIdamError(&idamerrorstack, CODEERRORTYPE, PLUGIN_NAME, err, msg);
        PPFUID("JETPPF", "R", 7, 1);            // Reset to reading Public PPF's Only
        return err;
    }


    //--------------------------------------------------------------------
    // Get DDA and Datatype Information

    ddacom[4 * NWCOM] = '\0';
    dtnams[4 * NDTNAMS] = '\0';

    unsigned int idda_len = 5;
    unsigned int iddacom_len = 4 * NWCOM + 1;
    unsigned int idtnams_len = 4 * NDTNAMS + 1;

    DDAINF(&pulno, dda, &nwcom, ddacom, &ndt, dtnams, plxtv, &err, idda_len, iddacom_len, idtnams_len);

    if (err != 0) {
        PPFERR("DDAINF", &err, msg, &err2, 7, 81);
        msg[80] = '\0';
        TrimString(msg);
        if (err2 != 0)
            addIdamError(&idamerrorstack, CODEERRORTYPE, PLUGIN_NAME, err, "DDAINF Error");
        else
            addIdamError(&idamerrorstack, CODEERRORTYPE, PLUGIN_NAME, err, msg);
        PPFUID("JETPPF", "R", 7, 1);            // Reset to reading Public PPF's Only
        return err;
    }

    if (TEST) {
        fprintf(stdout, "DDA Comment   : %s\n", ddacom);
        fprintf(stdout, "No. Data Types: %d\n", ndt);
        fprintf(stdout, "Signal List   : %s\n", dtnams);
    }

    //--------------------------------------------------------------------
    // Extract Requested DataType Info

    dtid = -1;

    for (i = 0; i < ndt; i++) {
        test[4] = '\0';
        strncpy(test, dtnams + i * 4, 4);    // Choose a data type name for testing
        TrimString(test);
        if (!strcmp(dtype, test)) {
            dtid = i;            // Found a Match
            if (TEST) {
                fprintf(stdout, "Signal Located: %d\n", dtid);
                fprintf(stdout, "Signal Name   : %s\n", dtnams + dtid * 4);
            }
            break;
        }
    }

    if (TEST) for (i = 0; i < 2 * ndt; i++)fprintf(stdout, "lxtv+%d = %d\n", i, lxtv[i]);

    if (dtid >= 0) {
        nx = lxtv[2 * dtid];        // Length of the X Dimension
        nt = lxtv[2 * dtid + 1];        // Length of the T Dimension
        if (TEST) {
            fprintf(stdout, "nx: %d\n", nx);
            fprintf(stdout, "nt: %d\n", nt);
        }
    } else {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, PLUGIN_NAME, err,
                     "Unable to Identify the PPF DDA Data-Type Requested");
        PPFUID("JETPPF", "R", 7, 1);
        return err;
    }


    // Subsetting indicies begin with value 1
    if (xsubset > 0 && xsubset > nx) {
        err = 995;
        addIdamError(&idamerrorstack, CODEERRORTYPE, PLUGIN_NAME, err,
                     "The Requested X-Dimension Subset Exceeds the Valid X-Dimensions");
        PPFUID("JETPPF", "R", 7, 1);
        return err;
    }

    ndmax = 1;
    if (nx > 0) ndmax = ndmax * nx;
    if (nt > 0) ndmax = ndmax * nt;

    //--------------------------------------------------------------------
    // Allocate Heap Memory for Data

    if ((dvec = (float *) malloc(ndmax * sizeof(float))) == NULL) {
        err = 998;
        addIdamError(&idamerrorstack, CODEERRORTYPE, PLUGIN_NAME, err, "Data Heap Memory Allocation Failed");
        PPFUID("JETPPF", "R", 7, 1);
        return err;
    }

    if (nt > 0) {
        if ((tvec = (float *) malloc(nt * sizeof(float))) == NULL) {
            err = 997;
            addIdamError(&idamerrorstack, CODEERRORTYPE, PLUGIN_NAME, err, "Tvec Heap Memory Allocation Failed");
            PPFUID("JETPPF", "R", 7, 1);
            return err;
        }
    }

    if (nx > 0) {
        if ((xvec = (float *) malloc(nx * sizeof(float))) == NULL) {
            err = 996;
            addIdamError(&idamerrorstack, CODEERRORTYPE, PLUGIN_NAME, err, "Xvec Heap Memory Allocation Failed");
            PPFUID("JETPPF", "R", 7, 1);
            return err;
        }
    }

    //--------------------------------------------------------------------
    // Setup PPFGET call
    // array sizes to hold retrieved X, T and DATA vectors

    irdat[0] = 0;
    irdat[1] = nx;
    irdat[2] = 0;
    irdat[3] = nt;
    irdat[4] = ndmax;        // Length of the Data Block
    irdat[5] = nx;        // X DImension
    irdat[6] = nt;        // T Dimendion
    irdat[7] = 0;        // Set flags to return x- and t-vectors
    irdat[8] = 0;

    if (nx <= 0)irdat[7] = -1;
    if (nt <= 0)irdat[8] = -1;

    // Call PPFGET to retrieve DataType.

    PPFGET(&pulno, dda, dtype, irdat, ihdat, iwdat, dvec, xvec, tvec, &err,
           5, 5, 60);

    if (err != 0) {
        PPFERR("PPFGET", &err, msg, &err2, 7, 81);
        msg[80] = '\0';
        TrimString(msg);
        if (err2 != 0)
            addIdamError(&idamerrorstack, CODEERRORTYPE, PLUGIN_NAME, err, "PPFGET Error");
        else
            addIdamError(&idamerrorstack, CODEERRORTYPE, PLUGIN_NAME, err, msg);
        if (dvec != NULL)free((void *) dvec);
        if (tvec != NULL)free((void *) tvec);
        if (xvec != NULL)free((void *) xvec);
        PPFUID("JETPPF", "R", 7, 1);            // Reset to reading Public PPF's Only
        return err;
    }

    //--------------------------------------------------------------------
    // Copy Data into Structures for Returning

    // Observational Data

    if (ihdat[24] == 'F') {
        data_block->data_type = TYPE_FLOAT;
    } else {
        if (ihdat[24] == 'I') {
            data_block->data_type = TYPE_INT;
        } else {
            data_block->data_type = TYPE_UNKNOWN;
        }
    }

    //fprintf(stdout,"Data is of Type: %c\n",ihdat[24]);

    data_block->error_type = TYPE_UNKNOWN;

    data_block->data = (char *) dvec;
    data_block->data_n = iwdat[4];

    rank = 0;
    if (nt > 0)rank++;

    if (nx > 1 && xsubset == 0)rank++;    // X Dimension must be > 1 in length

    data_block->rank = rank;

    data_block->source_status = iwdat[10];        // PPF/DDA Status
    data_block->signal_status = iwdat[11];        // Signal Status

    // Based on ECM1/PRFL data from PPFGET are arranged: data[t][x] => t dimension is order 1 not 0

    order = 0;
    if (rank == 2) order = 1;
    data_block->order = order;

    //fprintf(stdout,"Order = %d\n",order);
    //fprintf(stdout,"Rank  = %d\n",rank);

    strncpy(data_block->data_units, ihdat, 8);
    data_block->data_units[8] = '\0';

    strcpy(data_block->data_label, "");
    strncpy(data_block->data_desc, ihdat + 36, 24);
    data_block->data_desc[24] = '\0';
    TrimString(data_block->data_desc);

    // Dimension Data

    if (rank > 0) {
        data_block->dims = (DIMS *) malloc(data_block->rank * sizeof(DIMS));
    } else {
        data_block->dims = NULL;
    }

    for (i = 0; i < rank; i++)initDimBlock(&data_block->dims[i]);

    // T axis

    if (nt > 0 && order >= 0) {
        data_block->dims[order].dim_n = iwdat[6];
        data_block->dims[order].dim = (char *) tvec;
        strncpy(data_block->dims[order].dim_units, ihdat + 16, 8);
        data_block->dims[order].dim_units[8] = '\0';
        strcpy(data_block->dims[order].dim_label, "Time");

        if ('F' == ihdat[32]) {
            data_block->dims[order].data_type = TYPE_FLOAT;
        } else {
            if ('I' == ihdat[32]) {
                data_block->dims[order].data_type = TYPE_INT;
            } else {
                data_block->dims[order].data_type = TYPE_UNKNOWN;
            }
        }
    }

    // X axis (Only filled if no subsetting occurs)

    if (nx > 1 && 0 == xsubset) {
        //fprintf(stdout,"Filling X-Dim(%d)\n",swap[order+1]);
        data_block->dims[swap[order + 1]].dim_n = iwdat[5];
        data_block->dims[swap[order + 1]].dim = (char *) xvec;
        strncpy(data_block->dims[swap[order + 1]].dim_units, ihdat + 8, 8);
        data_block->dims[swap[order + 1]].dim_units[8] = '\0';
        strcpy(data_block->dims[swap[order + 1]].dim_label, "X");
        if ('F' == ihdat[28]) {
            data_block->dims[swap[order + 1]].data_type = TYPE_FLOAT;
        } else {
            if ('I' == ihdat[28]) {
                data_block->dims[swap[order + 1]].data_type = TYPE_INT;
            } else {
                data_block->dims[swap[order + 1]].data_type = TYPE_UNKNOWN;
            }
        }
    }

    //--------------------------------------------------------------------
    // Sub-Set the X Dimension: create New Heap, Copy and Free the Old Data Block

    if (xsubset > 0) {

        if ((sdvec = (float *) malloc(nt * sizeof(float))) == NULL) {
            err = 998;
            addIdamError(&idamerrorstack, CODEERRORTYPE, PLUGIN_NAME, err, "Xvec Subset Heap Memory Allocation Failed");
            PPFUID("JETPPF", "R", 7, 1);
            return err;
        }

        // Arranged as: data[t][x]

        for (i = 0; i < nt; i++)
            sdvec[i] = dvec[xsubset - 1 + nx * i];

        //for(i=0;i<nt;i++)fprintf(stdout,"%d %10.4e\n",i,(float)sdvec[i]);

        free((void *) dvec);
        free((void *) xvec);

        dvec = NULL;
        xvec = NULL;

        data_block->data = (char *) sdvec;
        data_block->data_n = nt;

    }

    if (nx == 1 && xsubset == 0 && xvec != NULL) free((void *) xvec);

    //--------------------------------------------------------------------
    // Housekeeping

    PPFUID("JETPPF", "R", 7, 1);            // Reset to reading Public PPF's Only

    return 0;
}

//#endif


int call_pdmsdt(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    VAR_PDMSDT v;

    err = PDMSDT_init(&v);

    if (!err)
        err = PDMSDT_setup(&v, ipi);

    if (!err)
        err = PDMSDT_execute(&v);

    if (!err)
        err = PDMSDT_DATABLOCK(&v, ipi->data_block);

    return err;
}

int call_pdmsht(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    VAR_PDMSHT v;

    err = PDMSHT_init(&v);

    //if ( !err )
    //  err = PDMSDT_setup( &v, ipi );

    if (!err)
        err = PDMSHT_execute(&v);

    if (!err)
        err = PDMSHT_DATABLOCK(&v, ipi->data_block);

    return err;
}

int call_pdmseq(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;
    DATA_BLOCK * data_block = ipi->data_block;
    //REQUEST_BLOCK* request_block = ipi->request_block;
    //PUTDATA_BLOCK_LIST* pdl = &(request_block->putDataBlockList);
    //const NAMEVALUELIST* nvl = &(request_block->nameValueList);
    //const NAMEVALUE* nv = nvl_name_find( nvl, STR_FUNCTION_TAG );

    VAR_PDMSEQ var_pdmseq;

    PDMSEQ_init(&var_pdmseq);
    err = PDMSEQ_setup(&var_pdmseq, ipi);

    if (!err)
        err = PDMSEQ_execute(&var_pdmseq);

    if (!err)
        err = PDMSEQ_DATABLOCK(&var_pdmseq, data_block);

    return err;
}

int call_ppfuid(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;
    VAR_PPFUID var;
    DATA_BLOCK * data_block = ipi->data_block;

    PPFUID_init(&var);
    err = PPFUID_setup(&var, ipi);

    if (!err)
        err = PPFUID_execute(&var);

    if (!err)
        PPFUID_DATABLOCK(&var, data_block);
    /*
     if( err )
      {
      addIdamError(&idamerrorstack, CODEERRORTYPE, PLUGIN_NAME
    	 , err, "PPFUID ERROR");
      }
    */
    return err;
}

int call_ppfgo(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    DATA_BLOCK * data_block = ipi->data_block;

    VAR_PPFGO var;

    PPFGO_init(&var);
    err = PPFGO_setup(&var, ipi);

    if (!err)
        err = PPFGO_execute(&var);

    if (!err)
        PPFGO_DATABLOCK(&var, data_block);

    if (err) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, PLUGIN_NAME, err, "PPFGO ERROR");
    }
    return err;
}

int call_ppfclo(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;
    err = ERR_FUNCTION_NOT_SUPPORTED;
    /*
    VAR_PPFCLO var;

    DATA_BLOCK* data_block    = ipi->data_block;

    PPFCLO_init( &var );

    err = PPFCLO_setup( &var, ipi );

    if( !err )
      err = PPFCLO_execute( &var );

    if( !err )
      PPFCLO_DATABLOCK( &var, data_block );

     if( err )
      {
      addIdamError(&idamerrorstack, CODEERRORTYPE, PLUGIN_NAME
    	 , err, "PPFCLO ERROR");
      }
    */
    return err;
}

int call_ppferr(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;
    DATA_BLOCK * data_block = ipi->data_block;
    VAR_PPFERR var;

    PPFERR_init(&var);

    err = PPFERR_setup(&var, ipi);

    if (!err)
        err = PPFERR_execute(&var);

    if (!err)
        PPFERR_DATABLOCK(&var, data_block);


    if (err) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, PLUGIN_NAME, err, "PPFERR ERROR");
    }

    return err;
}

int call_ppfsqi(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;
    err = ERR_FUNCTION_NOT_SUPPORTED;
    return err;
    /*

    VAR_PPFSQI var;
    DATA_BLOCK* data_block    = ipi->data_block;

    PPFSQI_init( &var );

    err = PPFSQI_setup( &var, ipi );

    if( !err )
      err = PPFSQI_execute( &var );

    if( !err )
      PPFSQI_DATABLOCK( &var, data_block );

    if( err )
      {
      addIdamError(&idamerrorstack, CODEERRORTYPE, PLUGIN_NAME
    	 , err, "PPFSQI ERROR");
      }


    return err;
    */
}

int call_ppfgqi(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;
    VAR_PPFGQI var;
    DATA_BLOCK * data_block = ipi->data_block;

    PPFGQI_init(&var);

    err = PPFGQI_setup(&var, ipi);

    if (!err)
        err = PPFGQI_execute(&var);

    if (!err)
        PPFGQI_DATABLOCK(&var, data_block);

    if (err) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, PLUGIN_NAME, err, "PPFGQI ERROR");
    }

    return err;
}

int call_ppfok(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;
    VAR_PPFPOK var;
    DATA_BLOCK * data_block = ipi->data_block;

    PPFPOK_init(&var);

    err = PPFPOK_setup(&var, ipi);

    if (!err)
        err = PPFPOK_execute(&var);

    if (!err)
        PPFPOK_DATABLOCK(&var, data_block);

    if (err) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, PLUGIN_NAME, err, "PPFOK ERROR");
    }
    return err;
}

int call_ppfgid(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;
    VAR_PPFGID var;
    DATA_BLOCK * data_block = ipi->data_block;

    PPFGID_init(&var);

    err = PPFGID_setup(&var, ipi);

    if (!err)
        err = PPFGID_execute(&var);

    if (!err)
        PPFGID_DATABLOCK(&var, data_block);

    if (err) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, PLUGIN_NAME, err, "PPFGID ERROR");
    }

    return err;
}

int call_pdainf(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;
    VAR_PDAINF var;
    DATA_BLOCK * data_block = ipi->data_block;
    PDAINF_init(&var);

    err = PDAINF_setup(&var, ipi);

    if (!err)
        err = PDAINF_execute(&var);

    if (!err)
        PDAINF_DATABLOCK(&var, data_block);

    if (err) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, PLUGIN_NAME, err, "PDAINF ERROR");
    }

    return err;
}

int call_ppfseq(IDAM_PLUGIN_INTERFACE * ipi)
{
    //raise( SIGSTOP);
    int err = 0;

    DATA_BLOCK * data_block = ipi->data_block;
    REQUEST_BLOCK * request_block = ipi->request_block;

    PUTDATA_BLOCK_LIST * pdl = &(request_block->putDataBlockList);

    VAR_PPFSEQ var;
    int i = 0;

    printf_(g_dbgout, "PPFSEQ\n");

    PPFSEQ_init(&var);

    err = PPFSEQ_setup(&var, ipi);

    printf_(g_dbgout, "COUNT:%d\n", pdl->blockCount);

    for (i = 0; i < pdl->blockCount; ++i) {
        PUTDATA_BLOCK * pd = pdl->putDataBlock + i;
        printf_(g_dbgout, "%d:%s\n", i, pd->blockName);

        int j = 0;
        int * pData = (int *) (pd->data);
        for (j = 0; j < pd->count; ++j) {
            printf_(g_dbgout, "%d\n", pData[j]);
        }
    }

    if (!err)
        err = PPFSEQ_execute(&var);

    if (!err)
        PPFSEQ_DATABLOCK(&var, data_block);

    if (!err)
        err = PPFSEQ_free(&var);

    if (err) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, PLUGIN_NAME, err, "PPFSEQ ERROR");
    }


    return err;
}

int call_ppfinf(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;
    VAR_PPFINF v;
    PPFINF_init(&v);

    err = PPFINF_setup(&v, ipi);

    if (!err)
        err = PPFINF_execute(&v);

    if (!err)
        err = PPFINF_DATABLOCK(&v, ipi->data_block);

    if (err) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, PLUGIN_NAME, err, "PPFINF ERROR");
    }

    return err;
}


int call_ddainf(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;
    VAR_DDAINF v;

    err = DDAINF_init(&v);

    if (!err)
        err = DDAINF_setup(&v, ipi);

    if (!err)
        err = DDAINF_execute(&v);

    if (!err)
        err = DDAINF_DATABLOCK(&v, ipi->data_block);

    DDAINF_free(&v);

    return err;
}

int call_ppfdel(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = ERR_FUNCTION_NOT_SUPPORTED;
    /*
    VAR_PPFDEL v;

    err = PPFDEL_init( &v );

    if( !err )
      err = PPFDEL_setup( &v, ipi );

    if( !err )
      err = PPFDEL_execute( &v );

    if( !err )
      err = PPFDEL_DATABLOCK( &v, ipi->data_block );
    */
    return err;
}

int call_ppfwri(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;
    err = ERR_FUNCTION_NOT_SUPPORTED;
    /*
    VAR_PPFWRI v;

    err = PPFWRI_init( &v );

    if( !err )
      err = PPFWRI_setup( &v, ipi );

    if( !err )
      err = PPFWRI_execute( &v );

    if( !err )
      err = PPFWRI_DATABLOCK( &v, ipi->data_block );
    */
    return err;
}

int call_pdlppf(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    VAR_PDLPPF v;

    err = PDLPPF_init(&v);

    if (!err)
        err = PDLPPF_setup(&v, ipi);

    if (!err)
        err = PDLPPF_execute(&v);

    if (!err)
        err = PDLPPF_DATABLOCK(&v, ipi->data_block);

    PDLPPF_free(&v);

    return err;
}

int call_pdlusr(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    VAR_PDLUSR v;

    err = PDLUSR_init(&v);

    if (!err)
        err = PDLUSR_setup(&v, ipi);

    if (!err)
        err = PDLUSR_execute(&v);

    if (!err)
        err = PDLUSR_DATABLOCK(&v, ipi->data_block);

    PDLUSR_free(&v);

    return err;
}

int call_pdstat(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    VAR_PDSTAT v;

    err = PDSTAT_init(&v);

    if (!err)
        err = PDSTAT_setup(&v, ipi);

    if (!err)
        err = PDSTAT_execute(&v);

    if (!err)
        err = PDSTAT_DATABLOCK(&v, ipi->data_block);

    return err;
}

int call_pdstd(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    VAR_PDSTD v;

    err = PDSTD_init(&v);

    if (!err)
        err = PDSTD_setup(&v, ipi);

    if (!err)
        err = PDSTD_execute(&v);

    if (!err)
        err = PDSTD_DATABLOCK(&v, ipi->data_block);

    return err;
}

int call_pdsrch(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    VAR_PDSRCH v;

    err = PDSRCH_init(&v);

    if (!err)
        err = PDSRCH_setup(&v, ipi);

    if (!err)
        err = PDSRCH_execute(&v);

    if (!err)
        err = PDSRCH_DATABLOCK(&v, ipi->data_block);

    PDSRCH_free(&v);

    return err;
}

int call_pdtinf(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    VAR_PDTINF v;

    err = PDTINF_init(&v);

    if (!err)
        err = PDTINF_setup(&v, ipi);

    if (!err)
        err = PDTINF_execute(&v);

    if (!err)
        err = PDTINF_DATABLOCK(&v, ipi->data_block);

    PDTINF_free(&v);

    return err;
}

int call_ppfdat(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    VAR_PPFDAT v;

    err = PPFDAT_init(&v);

    if (!err)
        err = PPFDAT_setup(&v, ipi);

    if (!err)
        err = PPFDAT_execute(&v);

    if (!err)
        err = PPFDAT_DATABLOCK(&v, ipi->data_block);

    PPFDAT_free(&v);

    return err;

}

int call_ppfdti(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    VAR_PPFDTI v;

    err = PPFDTI_init(&v);

    if (!err)
        err = PPFDTI_setup(&v, ipi);

    if (!err)
        err = PPFDTI_execute(&v);

    if (!err)
        err = PPFDTI_DATABLOCK(&v, ipi->data_block);

    //PPFDAT_free( &v );

    return err;

}


int call_ppfdda(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    VAR_PPFDDA v;

    err = PPFDDA_init(&v);

    if (!err)
        err = PPFDDA_setup(&v, ipi);

    if (!err)
        err = PPFDDA_execute(&v);

    if (!err)
        err = PPFDDA_DATABLOCK(&v, ipi->data_block);

    PPFDDA_free(&v);


    return err;

}


int call_ppfget(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;

    VAR_PPFGET v;
    //raise( SIGSTOP );
    err = PPFGET_init(&v);

    if (!err)
        err = PPFGET_setup(&v, ipi);


    if (!err)
        err = PPFGET_execute(&v);

    if (!err)
        err = PPFGET_DATABLOCK(&v, ipi->data_block);

    PPFGET_free(&v);

    return err;

}

int call_ppfgmd(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = ERR_FUNCTION_NOT_SUPPORTED;
    return err;
}

int call_ppfgsf(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;
    VAR_PPFGSF v;

    err = PPFGSF_init(&v);

    if (!err)
        err = PPFGSF_setup(&v, ipi);

    if (!err)
        err = PPFGSF_execute(&v);

    if (!err)
        err = PPFGSF_DATABLOCK(&v, ipi->data_block);

    return err;
}

int call_ppfgts(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;
    VAR_PPFGTS v;

    err = PPFGTS_init(&v);

    if (!err)
        err = PPFGTS_setup(&v, ipi);

    if (!err)
        err = PPFGTS_execute(&v);

    if (!err)
        err = PPFGTS_DATABLOCK(&v, ipi->data_block);

    //PPFGTS_free( &v );

    return err;
}

int call_ppfmod(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;
    VAR_PPFMOD v;

    err = PPFMOD_init(&v);

    if (!err)
        err = PPFMOD_setup(&v, ipi);

    if (!err)
        err = PPFMOD_execute(&v);

    if (!err)
        err = PPFMOD_DATABLOCK(&v, ipi->data_block);

    return err;
}


int call_ppfopn(IDAM_PLUGIN_INTERFACE * ipi)
{
    return ERR_FUNCTION_NOT_SUPPORTED;
}

int call_ppfondisk(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;
    VAR_PPFONDISK v;

    err = PPFONDISK_init(&v);

    if (!err)
        err = PPFONDISK_setup(&v, ipi);

    if (!err)
        err = PPFONDISK_execute(&v);

    if (!err)
        err = PPFONDISK_DATABLOCK(&v, ipi->data_block);

    return err;
}

int call_ppfownerinfo(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;
    VAR_PPFOWNERINFO v;

    err = PPFOWNERINFO_init(&v);

    if (!err)
        err = PPFOWNERINFO_setup(&v, ipi);

    if (!err)
        err = PPFOWNERINFO_execute(&v);

    if (!err)
        err = PPFOWNERINFO_DATABLOCK(&v, ipi->data_block);

    return err;
}

int call_ppfsetdevice(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;
    VAR_PPFSETDEVICE v;

    err = PPFSETDEVICE_init(&v);

    if (!err)
        err = PPFSETDEVICE_setup(&v, ipi);

    if (!err)
        err = PPFSETDEVICE_execute(&v);

    if (!err)
        err = PPFSETDEVICE_DATABLOCK(&v, ipi->data_block);

    return err;
}

int call_ppfsiz(IDAM_PLUGIN_INTERFACE * ipi)
{
    int err = 0;
    VAR_PPFSIZ v;

    err = PPFSIZ_init(&v);

    if (!err)
        err = PPFSIZ_setup(&v, ipi);

    if (!err)
        err = PPFSIZ_execute(&v);

    if (!err)
        err = PPFSIZ_DATABLOCK(&v, ipi->data_block);

    return err;
}


