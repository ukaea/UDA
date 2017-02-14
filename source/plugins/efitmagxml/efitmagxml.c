
/*---------------------------------------------------------------
* v1 IDAM Plugin: Parse and server EFIT++ legacy Magnetics XML
*
* Input Arguments:	IDAM_PLUGIN_INTERFACE *idam_plugin_interface
*
* Returns:		0 if the plugin functionality was successful
*			otherwise a Error Code is returned 
*
* Standard functionality:
*
*	help	a description of what this plugin does together with a list of functions available
*
*	reset	frees all previously allocated heap, closes file handles and resets all static parameters.
*		This has the same functionality as setting the housekeeping directive in the plugin interface
*		data structure to TRUE (1)
*
*	init	Initialise the plugin: read all required data and process. Retain staticly for
*		future reference.	
*
* Change History
*
* 06Sept2016	dgm	Original Version
*---------------------------------------------------------------------------------------------------------------*/
#include "efitmagxml.h"

#include <strings.h>

#include <client/accAPI_C.h>
#include <structures/struct.h>
#include <structures/accessors.h>
#include <clientserver/stringUtils.h>
#include <plugins/idamPlugin.h>
#include <clientserver/initStructs.h>

int efitmagxml(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int i, err = 0;

    static short init = 0;
    static EFIT efit;

//----------------------------------------------------------------------------------------
// Standard v1 Plugin Interface

    DATA_BLOCK* data_block;
    REQUEST_BLOCK* request_block;

    IDAMERRORSTACK* idamErrorStack = getIdamServerPluginErrorStack();        // Server library functions

    initIdamErrorStack(&idamerrorstack);

    unsigned short housekeeping;

    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        err = 999;
        idamLog(LOG_ERROR,
                "ERROR efitmagxml: Plugin Interface Version Unknown to this plugin: Unable to execute the request!\n");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "efitmagxml", err,
                     "Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
        concatIdamError(idamerrorstack, idamErrorStack);
        return err;
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    data_block = idam_plugin_interface->data_block;
    request_block = idam_plugin_interface->request_block;

    housekeeping = idam_plugin_interface->housekeeping;

#ifndef USE_PLUGIN_DIRECTLY
// Don't copy the structure if housekeeping is requested - may dereference a NULL or freed pointer!     
    if (!housekeeping && idam_plugin_interface->environment != NULL) environment = *idam_plugin_interface->environment;
#endif

// Additional interface components (must be defined at the bottom of the standard data structure)
// Versioning must be consistent with the macro THISPLUGIN_MAX_INTERFACE_VERSION and the plugin registration with the server

    //if(idam_plugin_interface->interfaceVersion >= 2){
    // NEW COMPONENTS
    //}

//----------------------------------------------------------------------------------------
// Arguments and keywords 

    unsigned short int isReset = 0;
    for (i = 0; i < request_block->nameValueList.pairCount; i++) {
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "reset") ||
            !strcasecmp(request_block->nameValueList.nameValue[i].name, "initialise")) {
            isReset = 1;
            break;
        }
    }

//----------------------------------------------------------------------------------------
// Heap Housekeeping 

// Plugin must maintain a list of open file handles and sockets: loop over and close all files and sockets
// Plugin must maintain a list of plugin functions called: loop over and reset state and free heap.
// Plugin must maintain a list of calls to other plugins: loop over and call each plugin with the housekeeping request
// Plugin must destroy lists at end of housekeeping

// A plugin only has a single instance on a server. For multiple instances, multiple servers are needed.
// Plugins can maintain state so recursive calls (on the same server) must respect this.
// If the housekeeping action is requested, this must be also applied to all plugins called.
// A list must be maintained to register these plugin calls to manage housekeeping.
// Calls to plugins must also respect access policy and user authentication policy

    if (isReset || housekeeping || !strcasecmp(request_block->function, "reset")) {

        if (!init) return 0;        // Not previously initialised: Nothing to do!

// Free Heap & reset counters

        freeEfit(&efit);

        init = 0;

        if (!isReset) return 0;        // Step to Initialisation
    }

//----------------------------------------------------------------------------------------
// Initialise 

    if (!init || !strcasecmp(request_block->function, "init")
        || !strcasecmp(request_block->function, "initialise")) {

// EFIT Data Structures

        initEfit(&efit);

// Arguments and keywords 

        char* xmlFile = NULL;
        unsigned short int isXmlFile = 0;
        for (i = 0; i < request_block->nameValueList.pairCount; i++) {
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "xmlfile")) {
                isXmlFile = 1;
                xmlFile = request_block->nameValueList.nameValue[i].value;
                break;
            }
        }

// Parse the XML if the file has been identified      

        if (isXmlFile && parseEfitXML(xmlFile, &efit) != 0) {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "efitmagxml", err, "EFIT++ Magnetics XML could Not be Parsed");
            return err;
        }

        init = 1;
        if (!strcasecmp(request_block->function, "init") || !strcasecmp(request_block->function, "initialise")) {
            return 0;
        }
    }

//----------------------------------------------------------------------------------------
// Plugin Functions 
//----------------------------------------------------------------------------------------

    do {

//---------------------------------------------------------------------------------------- 
// Common Passed name-value pairs and Keywords 

        unsigned short int isObjectId = 0, isFluxLoop = 0, isMagProbe = 0, isCount = 0, isIndex = 0, isPosition = 0, isName = 0,
                isIdentifier, isSignal = 0, isR = 0, isZ = 0, isPhi = 0, isUnitStartIndex = 0, isDevice = 0;
        int objectId = -1, index = -1;

// Arguments and keywords 

        for (i = 0; i < request_block->nameValueList.pairCount; i++) {

            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "id")) {
                isObjectId = 1;
                objectId = atoi(request_block->nameValueList.nameValue[i].value);
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "FluxLoop")) {
                isFluxLoop = 1;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "MagProbe")) {
                isMagProbe = 1;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "Position")) {
                isPosition = 1;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "Count")) {
                isCount = 1;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "index")) {
                isIndex = 1;
                index = atoi(request_block->nameValueList.nameValue[i].value);
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "R")) {
                isR = 1;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "Z")) {
                isZ = 1;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "Phi")) {
                isPhi = 1;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "Signal")) {
                isSignal = 1;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "Name")) {
                isName = 1;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "Identifier")) {
                isIdentifier = 1;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "UnitStartIndex")) {
                isUnitStartIndex = 1;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "Device")) {
                isDevice = 1;
                continue;
            }

        }

        if (isUnitStartIndex) {
            if (isObjectId) objectId = objectId - 1;    // All C arrays begin with index 0
            if (isIndex) index = index - 1;
        }

//---------------------------------------------------------------------------------------- 
// Get(xmlFile=xmlFile, /UnitStartIndex]) 

/* Mapping from magnetics IDS to XML objects

device					/Device

flux_loop(:)				/FluxLoop, /Count
flux_loop(id)/signal			id=id, /FluxLoop, /Signal 
flux_loop(id)/name			id=id, /FluxLoop, /Name 
flux_loop(id)/identifier		id=id, /FluxLoop, /Identifier  
flux_loop(id)/position(:)		id=id, /FluxLoop, /Position, Count  
flux_loop(id)/position(index)/r		id=id, /FluxLoop, /Position, index=index, /r  
flux_loop(id)/position(index)/z		id=id, /FluxLoop, /Position, index=index, /z  
flux_loop(id)/position(index)/phi	id=id, /FluxLoop, /Position, index=index, /phi 

bpol_probe(:)				/MagProbe, /Count
bpol_probe(id)/name			id=id, /MagProbe, /Name
bpol_probe(id)/identifier		id=id, /MagProbe, /identifier    
bpol_probe(id)/position/r		id=id, /MagProbe, /Position, /r  
bpol_probe(id)/position/z		id=id, /MagProbe, /Position, /z  
bpol_probe(id)/position/phi		id=id, /MagProbe, /Position, /phi  

ToDo:

1> Reset when the file name changes
2> Ignore data when the signal name is "noData"

*/

        if (!strcasecmp(request_block->function, "get")) {
            initDataBlock(data_block);

            int count = 0;

            if (isCount) {
                if (isFluxLoop) {
                    if (isPosition) {
                        count = getnfluxloopcoords(&efit, objectId);
                    } else {
                        count = getnfluxloops(&efit);
                    }
                } else if (isMagProbe) {
                    if (isPosition) {
                        count = 1;    // The IMAS IDS can only have 1
                    } else {
                        count = getnmagprobes(&efit);
                    }
                }

                data_block->data_n = 1;
                data_block->rank = 0;
                data_block->data_type = TYPE_INT;
                strcpy(data_block->data_desc, "efitmagxml: object count returned");
                int* data = (int*) malloc(sizeof(int));
                *data = count;
                data_block->data = (char*) data;
                break;
            } else if (isSignal) {
                char* p = NULL;
                if (isFluxLoop) {
                    p = (efit.fluxloop[objectId].instance).signal;
                } else if (isMagProbe) {
                    p = (efit.magprobe[objectId].instance).signal;
                }
                data_block->data_n = (int)(strlen(p) + 1);
                data_block->rank = 0;
                data_block->data_type = TYPE_STRING;
                strcpy(data_block->data_desc, "efitmagxml: object name returned");
                data_block->data = (char*) malloc(data_block->data_n * sizeof(char));
                strcpy(data_block->data, p);
                break;
            } else if (isName || isIdentifier) {
                char* p = NULL;
                if (isFluxLoop) {
                    p = getidfluxloop(&efit, objectId);
                } else if (isMagProbe) {
                    p = getidmagprobe(&efit, objectId);
                }
                data_block->data_n = (int)(strlen(p) + 1);
                data_block->rank = 0;
                data_block->data_type = TYPE_STRING;
                strcpy(data_block->data_desc, "efitmagxml: object name returned");
                data_block->data = (char*) malloc(data_block->data_n * sizeof(char));
                strcpy(data_block->data, p);
                break;
            } else if (isPosition) {
                double* data = (double*) malloc(sizeof(double));
                *data = 0.0;
                if (isFluxLoop) {
                    float* r = NULL, * z = NULL, * phi = NULL, aerr, rerr;
                    getfluxloop(&efit, objectId, &r, &z, &phi, &aerr, &rerr);
                    if (isR) {
                        *data = (double) r[index];
                    } else if (isZ) {
                        *data = (double) z[index];
                    } else if (isPhi) {
                        *data = (double) phi[index];
                    }
                } else if (isMagProbe) {
                    float r, z, phi, aerr, rerr;
                    getmagprobe(&efit, objectId, &r, &z, &phi, &aerr, &rerr);
                    if (isR) {
                        *data = (double) r;
                    } else if (isZ) {
                        *data = (double) z;
                    } else if (isPhi) {
                        *data = (double) phi;
                    }
                }
                data_block->rank = 0;
                data_block->data_n = 1;
                data_block->data_type = TYPE_DOUBLE;
                strcpy(data_block->data_desc, "efitmagxml: Position Coordinate returned");
                data_block->data = (char*) data;
                break;
            } else if (isDevice) {
                char* p = getdevice(&efit);
                data_block->data_n = (int)(strlen(p) + 1);
                data_block->rank = 0;
                data_block->data_type = TYPE_STRING;
                strcpy(data_block->data_desc, "efitmagxml: device name returned");
                data_block->data = (char*) malloc(data_block->data_n * sizeof(char));
                strcpy(data_block->data, p);
            }
            break;
        } else

//----------------------------------------------------------------------------------------
// HELP Documentation
//----------------------------------------------------------------------------------------

// Help: A Description of library functionality

        if (!strcasecmp(request_block->function, "help")) {

            char* p = (char*) malloc(sizeof(char) * 2 * 1024);

            strcpy(p, "\nefitmagxml: Add Functions Names, Syntax, and Descriptions\n\n");

            initDataBlock(data_block);

            data_block->rank = 1;
            data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
            for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

            data_block->data_type = TYPE_STRING;
            strcpy(data_block->data_desc, "efitmagxml: help = description of this plugin");

            data_block->data = (char*) p;

            data_block->dims[0].data_type = TYPE_UNSIGNED_INT;
            data_block->dims[0].dim_n = (int)(strlen(p) + 1);
            data_block->dims[0].compressed = 1;
            data_block->dims[0].dim0 = 0.0;
            data_block->dims[0].diff = 1.0;
            data_block->dims[0].method = 0;

            data_block->data_n = data_block->dims[0].dim_n;

            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            break;
        } else

//----------------------------------------------------------------------------------------    
// Standard methods: version, builddate, defaultmethod, maxinterfaceversion 

        if (!strcasecmp(request_block->function, "version")) {
            initDataBlock(data_block);
            data_block->data_type = TYPE_INT;
            data_block->rank = 0;
            data_block->data_n = 1;
            int* data = (int*) malloc(sizeof(int));
            data[0] = THISPLUGIN_VERSION;
            data_block->data = (char*) data;
            strcpy(data_block->data_desc, "Plugin version number");
            strcpy(data_block->data_label, "version");
            strcpy(data_block->data_units, "");
            break;
        } else

// Plugin Build Date

        if (!strcasecmp(request_block->function, "builddate")) {
            initDataBlock(data_block);
            data_block->data_type = TYPE_STRING;
            data_block->rank = 0;
            data_block->data_n = (int)(strlen(__DATE__) + 1);
            char* data = (char*) malloc(data_block->data_n * sizeof(char));
            strcpy(data, __DATE__);
            data_block->data = (char*) data;
            strcpy(data_block->data_desc, "Plugin build date");
            strcpy(data_block->data_label, "date");
            strcpy(data_block->data_units, "");
            break;
        } else

// Plugin Default Method

        if (!strcasecmp(request_block->function, "defaultmethod")) {
            initDataBlock(data_block);
            data_block->data_type = TYPE_STRING;
            data_block->rank = 0;
            data_block->data_n = (int)(strlen(THISPLUGIN_DEFAULT_METHOD) + 1);
            char* data = (char*) malloc(data_block->data_n * sizeof(char));
            strcpy(data, THISPLUGIN_DEFAULT_METHOD);
            data_block->data = (char*) data;
            strcpy(data_block->data_desc, "Plugin default method");
            strcpy(data_block->data_label, "method");
            strcpy(data_block->data_units, "");
            break;
        } else

// Plugin Maximum Interface Version

        if (!strcasecmp(request_block->function, "maxinterfaceversion")) {
            initDataBlock(data_block);
            data_block->data_type = TYPE_INT;
            data_block->rank = 0;
            data_block->data_n = 1;
            int* data = (int*) malloc(sizeof(int));
            data[0] = THISPLUGIN_MAX_INTERFACE_VERSION;
            data_block->data = (char*) data;
            strcpy(data_block->data_desc, "Maximum Interface Version");
            strcpy(data_block->data_label, "version");
            strcpy(data_block->data_units, "");
            break;
        } else {

//======================================================================================
// Error ...

            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "efitmagxml", err, "Unknown function requested!");
            break;
        }

    } while (0);

//--------------------------------------------------------------------------------------
// Housekeeping

    concatIdamError(idamerrorstack, idamErrorStack);    // Combine local errors with the Server's error stack

#ifndef USE_PLUGIN_DIRECTLY
    closeIdamError(&idamerrorstack);            // Free local plugin error stack
#endif

    return err;
}

int getnfluxloops(EFIT* efit)
{
    return (efit->nfluxloops);
}

int getnfluxloopcoords(EFIT* efit, const int n)
{
    return (efit->fluxloop[n].nco);
}

int getnlimiter(EFIT* efit)
{
    return (efit->nlimiter);
}

int getnlimitercoords(EFIT* efit)
{
    return (efit->limiter->nco);
}

int getnmagprobes(EFIT* efit)
{
    return (efit->nmagprobes);
}

int getnpfsupplies(EFIT* efit)
{
    return (efit->npfsupplies);
}

int getnpfcoils(EFIT* efit)
{
    return (efit->npfcoils);
}

int getnpfcoilcoords(EFIT* efit, const int n)
{
    return (efit->pfcoils[n].nco);
}

int getnpfcircuits(EFIT* efit)
{
    return (efit->npfcircuits);
}

int getnpfcircuitconnections(EFIT* efit, const int n)
{
    return (efit->pfcircuit[n].nco);
}

int getnpfpassive(EFIT* efit)
{
    return (efit->npfpassive);
}

int getnpfpassivecoords(EFIT* efit, const int n)
{
    return (efit->pfpassive[n].nco);
}

int getnplasmacurrent(EFIT* efit)
{
    return (efit->nplasmacurrent);
}

int getndiamagnetic(EFIT* efit)
{
    return (efit->ndiamagnetic);
}

int getntoroidalfield(EFIT* efit)
{
    return (efit->ntoroidalfield);
}

//=========================================================================

char* getidmagprobe(EFIT* efit, const int index)
{
    return (efit->magprobe[index].id);
}

char* getidfluxloop(EFIT* efit, const int index)
{
    return (efit->fluxloop[index].id);
}

char* getidplasmacurrent(EFIT* efit, const int index)
{
    return (efit->plasmacurrent[index].id);
}

char* getidtoroidalfield(EFIT* efit, const int index)
{
    return (efit->toroidalfield[index].id);
}

char* getidpfcoils(EFIT* efit, const int index)
{
    return (efit->pfcoils[index].id);
}

char* getidpfpassive(EFIT* efit, const int index)
{
    return (efit->pfpassive[index].id);
}

char* getidpfsupplies(EFIT* efit, const int index)
{
    return (efit->pfsupplies[index].id);
}

char* getidpfcircuit(EFIT* efit, const int index)
{
    return (efit->pfcircuit[index].id);
}
