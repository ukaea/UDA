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
*---------------------------------------------------------------------------------------------------------------*/
#include "efitmagxml.h"

#include <strings.h>

#include <client/accAPI.h>
#include <structures/struct.h>
#include <clientserver/initStructs.h>
#include <clientserver/stringUtils.h>
#include <clientserver/xmlStructs.h>

#include "parseHData.h"
#include "efitmagxmllib.h"

static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_get(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, EFIT* efit);

// NUMBER
static int getnfluxloops(EFIT* efit);
static int getnfluxloopcoords(EFIT* efit, const int n);
//static int getnlimiter(EFIT* efit);
//static int getnlimitercoords(EFIT* efit);
//static int getnpfsupplies(EFIT* efit);
static int getnpfcoils(EFIT* efit);
static int getnpfcoilcoords(EFIT* efit, const int n);
//static int getnpfcircuits(EFIT* efit);
//static int getnpfcircuitconnections(EFIT* efit, const int n);
//static int getnpfpassive(EFIT* efit);
//static int getnpfpassivecoords(EFIT* efit, const int n);
//static int getnplasmacurrent(EFIT* efit);
//static int getndiamagnetic(EFIT* efit);
//static int getntoroidalfield(EFIT* efit);
static int getnmagprobes(EFIT* efit);

// NAME
static char* getnamemagprobe(EFIT* efit, const int index);
static char* getnamefluxloop(EFIT* efit, const int index);
//static char* getnameplasmacurrent(EFIT* efit, const int index);
//static char* getnametoroidalfield(EFIT* efit, const int index);
static char* getnamepfcoils(EFIT* efit, const int index);
//static char* getnamepfpassive(EFIT* efit, const int index);
//static char* getnamepfsupplies(EFIT* efit, const int index);
//static char* getnamepfcircuit(EFIT* efit, const int index);

// ID
static char* getidmagprobe(EFIT* efit, const int index);
static char* getidfluxloop(EFIT* efit, const int index);
//static char* getidplasmacurrent(EFIT* efit, const int index);
//static char* getidtoroidalfield(EFIT* efit, const int index);
static char* getidpfcoils(EFIT* efit, const int index);
//static char* getidpfpassive(EFIT* efit, const int index);
//static char* getidpfsupplies(EFIT* efit, const int index);
//static char* getidpfcircuit(EFIT* efit, const int index);

int efitmagxml(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    idamSetLogLevel(UDA_LOG_DEBUG);

    int err = 0;

    static short init = 0;
    static EFIT efit;

    unsigned short housekeeping;

    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        IDAM_LOG(UDA_LOG_ERROR, "Plugin Interface Version Unknown to this plugin: Unable to execute the request!\n");
        THROW_ERROR(999, "Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    housekeeping = idam_plugin_interface->housekeeping;

//----------------------------------------------------------------------------------------
// Arguments and keywords 

    unsigned short int isReset = 0;

    int i;
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

    if (!STR_IEQUALS(request_block->function, "help")  &&
            (!init || STR_IEQUALS(request_block->function, "init") || STR_IEQUALS(request_block->function, "initialise"))) {

        // EFIT Data Structures

        initEfit(&efit);

        const char* xmlFile = NULL;
        FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, xmlFile);

        const char* dir = getenv("UDA_EFITMAGXML_XMLDIR");
        char* fullpath = strdup(xmlFile);
        if (dir != NULL) {
            fullpath = FormatString("%s/%s", dir, xmlFile);
        }

        IDAM_LOGF(UDA_LOG_DEBUG, "loading XML file %s\n", fullpath);

        // Parse the XML if the file has been identified

        if (parseEfitXML(fullpath, &efit) != 0) {
            THROW_ERROR(999, "EFIT++ Magnetics XML could Not be Parsed");
        }

        init = 1;
        if (!strcasecmp(request_block->function, "init") || !strcasecmp(request_block->function, "initialise")) {
            return 0;
        }
    }

    //----------------------------------------------------------------------------------------
    // Plugin Functions
    //----------------------------------------------------------------------------------------

    if (!strcasecmp(request_block->function, "get")) {
        err = do_get(idam_plugin_interface, &efit);
    } else if (!strcasecmp(request_block->function, "help")) {
        do_help(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "version")) {
        do_version(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "builddate")) {
        do_builddate(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "defaultmethod")) {
        do_defaultmethod(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "maxinterfaceversion")) {
        do_maxinterfaceversion(idam_plugin_interface);
    } else {
        THROW_ERROR(999, "Unknown function requested!");
    }

//--------------------------------------------------------------------------------------
// Housekeeping

    return err;
}

static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    char* help = "\nefitmagxml: Add Functions Names, Syntax, and Descriptions\n\n";
    const char* desc = "efitmagxml: help = description of this plugin";

    return setReturnDataString(idam_plugin_interface->data_block, help, desc);
}

static int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    return setReturnDataIntScalar(idam_plugin_interface->data_block, THISPLUGIN_VERSION, NULL);
}

static int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    return setReturnDataString(idam_plugin_interface->data_block, __DATE__, NULL);
}

static int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    return setReturnDataString(idam_plugin_interface->data_block, THISPLUGIN_DEFAULT_METHOD, NULL);
}

static int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    return setReturnDataIntScalar(idam_plugin_interface->data_block, THISPLUGIN_MAX_INTERFACE_VERSION, NULL);
}

/**
 * Mapping from magnetics IDS to XML objects.
 *
 * Example: Get(xmlFile=xmlFile, /UnitStartIndex])
 *
 * device					/Device
 *
 * flux_loop(:)				/FluxLoop, /Count
 * flux_loop(id)/signal			id=id, /FluxLoop, /Signal
 * flux_loop(id)/name			id=id, /FluxLoop, /Name
 * flux_loop(id)/identifier		id=id, /FluxLoop, /Identifier
 * flux_loop(id)/position(:)		id=id, /FluxLoop, /Position, Count
 * flux_loop(id)/position(index)/r		id=id, /FluxLoop, /Position, index=index, /r
 * flux_loop(id)/position(index)/z		id=id, /FluxLoop, /Position, index=index, /z
 * flux_loop(id)/position(index)/phi	id=id, /FluxLoop, /Position, index=index, /phi
 *
 * bpol_probe(:)				/MagProbe, /Count
 * bpol_probe(id)/name			id=id, /MagProbe, /Name
 * bpol_probe(id)/identifier		id=id, /MagProbe, /identifier
 * bpol_probe(id)/position/r		id=id, /MagProbe, /Position, /r
 * bpol_probe(id)/position/z		id=id, /MagProbe, /Position, /z
 * bpol_probe(id)/position/phi		id=id, /MagProbe, /Position, /phi
 *
 * Mapping from PF_ACTIVE IDS to XML objects
 *
 * pf_active.coil(:)						/PFActive, /Count
 * pf_active.coil(id).name						/PFActive, /Name
 * pf_active.coil(id).identifier					/PFActive, /Identifier
 * pf_active.coil(id).signal					/PFActive, /Signal
 * pf_active.coil(id).data_scaling					/PFActive, /DataScaling
 * pf_active.coil(id).time_scaling					/PFActive, /TimeScaling
 *
 * pf_active.coil(id).element(:)					id=id, /PFActive, /Element, /Count
 * pf_active.coil(id).element(index).geometry.rectangle.r		id=id, /PFActive, /Element, /Geometry, /Rectangle, index=index, /r
 * pf_active.coil(id).element(index).geometry.rectangle.z		id=id, /PFActive, /Element, /Geometry, /Rectangle, index=index, /z
 * pf_active.coil(id).element(index).geometry.rectangle.width	id=id, /PFActive, /Element, /Geometry, /Rectangle, index=index, /width
 * pf_active.coil(id).element(index).geometry.rectangle.height	id=id, /PFActive, /Element, /Geometry, /Rectangle, index=index, /height
 * pf_active.coil(id).element(index).turns_with_sign		id=id, /PFActive, /Element, /turns
 *
 * pf_active.coil(id).element(*).geometry.rectangle.r		id=id, /PFActive, /Geometry, /Rectangle, /r 		// Full array of element coordinate data
 * pf_active.coil(id).element(*).geometry.rectangle.z		id=id, /PFActive, /Geometry, /Rectangle, /z
 * pf_active.coil(id).element(*).geometry.rectangle.width		id=id, /PFActive, /Geometry, /Rectangle, /width
 * pf_active.coil(id).element(*).geometry.rectangle.height		id=id, /PFActive, /Geometry, /Rectangle, /height
 *
 * TODO:
 * 1> Reset when the file name changes
 * 2> Ignore data when the signal name is "noData"
 *
 * @param idam_plugin_interface
 * @param efit
 * @return
 */
static int do_get(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, EFIT* efit)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    initDataBlock(data_block);

    int count = 0;
    bool isCount = FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, count);

    int objectId;
    bool isObjectId = FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, objectId);

    int index;
    bool isIndex = FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, index);

    bool isFluxLoop = findValue(&idam_plugin_interface->request_block->nameValueList, "fluxloop");
    bool isPosition = findValue(&idam_plugin_interface->request_block->nameValueList, "position");
    bool isMagProbe = findValue(&idam_plugin_interface->request_block->nameValueList, "magprobe");
    bool isPFActive = findValue(&idam_plugin_interface->request_block->nameValueList, "pfactive");
    bool isElement = findValue(&idam_plugin_interface->request_block->nameValueList, "element");
    bool isSignal = findValue(&idam_plugin_interface->request_block->nameValueList, "signal");
    bool isDataScaling = findValue(&idam_plugin_interface->request_block->nameValueList, "datascaling");
    bool isTimeScaling = findValue(&idam_plugin_interface->request_block->nameValueList, "timescaling");
    bool isTurns = findValue(&idam_plugin_interface->request_block->nameValueList, "turns");
    bool isName = findValue(&idam_plugin_interface->request_block->nameValueList, "name");
    bool isIdentifier = findValue(&idam_plugin_interface->request_block->nameValueList, "identifier");
    bool isR = findValue(&idam_plugin_interface->request_block->nameValueList, "r");
    bool isZ = findValue(&idam_plugin_interface->request_block->nameValueList, "z");
    bool isPhi = findValue(&idam_plugin_interface->request_block->nameValueList, "phi");
    bool isWidth = findValue(&idam_plugin_interface->request_block->nameValueList, "width");
    bool isHeight = findValue(&idam_plugin_interface->request_block->nameValueList, "height");
    bool isUnitStartIndex = findValue(&idam_plugin_interface->request_block->nameValueList, "unitStartIndex");
    bool isGeometry = findValue(&idam_plugin_interface->request_block->nameValueList, "geometry");
    bool isRectangle = findValue(&idam_plugin_interface->request_block->nameValueList, "rectangle");
    bool isDevice = findValue(&idam_plugin_interface->request_block->nameValueList, "device");

    if (isUnitStartIndex) {
        // All C arrays begin with index 0
        if (isObjectId) objectId = objectId - 1;
        if (isIndex) index = index - 1;
    }

    if (isCount) {
        if (isFluxLoop) {
            if (isPosition) {
                count = getnfluxloopcoords(efit, objectId);
            } else {
                count = getnfluxloops(efit);
            }
        } else if (isMagProbe) {
            if (isPosition) {
                count = 1;    // The IMAS IDS can only have 1
            } else {
                count = getnmagprobes(efit);
            }
        } else if (isPFActive) {
            if (isElement) {
                count = getnpfcoilcoords(efit, objectId);
            } else {
                count = getnpfcoils(efit);
            }
        }
        return setReturnDataIntScalar(idam_plugin_interface->data_block, count, "efitmagxml: object count returned");
    } else if (isSignal) {
        char* signal = NULL;
        if (isFluxLoop) {
            signal = (efit->fluxloop[objectId].instance).signal;
        } else if (isMagProbe) {
            signal = (efit->magprobe[objectId].instance).signal;
        } else if (isPFActive) {
            signal = (efit->pfcoils[objectId].instance).signal;
        }
        return setReturnDataString(idam_plugin_interface->data_block, signal, "efitmagxml: object name returned");
    } else if (isDataScaling) {
        double scalingFactor = 0;
        if (isFluxLoop) {
            scalingFactor = (double)(efit->fluxloop[objectId].instance).factor;
        } else if (isMagProbe) {
            scalingFactor = (double)(efit->magprobe[objectId].instance).factor;
        } else if (isPFActive) {
            scalingFactor = (double)(efit->pfcoils[objectId].instance).factor;
        }
        return setReturnDataDblScalar(idam_plugin_interface->data_block, scalingFactor, "efitmagxml: object data scaling factor returned");
    } else if (isTimeScaling) {
        double scalingFactor = 0;
        scalingFactor = (double)1.0;    // Not encoded in xml at this time
        return setReturnDataDblScalar(idam_plugin_interface->data_block, scalingFactor, "efitmagxml: object time scaling factor returned");
    } else if (isTurns) {
        int turns = 0;
        if (isPFActive) {        // Not by Element in this XML!
            turns = efit->pfcoils[objectId].turns;
        }
        return setReturnDataIntScalar(idam_plugin_interface->data_block, turns, "efitmagxml: object turns returned");
    } else if (isName) {
        char* name = NULL;
        if (isFluxLoop) {
            name = getnamefluxloop(efit, objectId);
        } else if (isMagProbe) {
            name = getnamemagprobe(efit, objectId);
        } else if (isPFActive) {
            name = getnamepfcoils(efit, objectId);
        }
        return setReturnDataString(idam_plugin_interface->data_block, name, "efitmagxml: object name returned");
    } else if (isIdentifier) {
        char* id = NULL;
        if (isFluxLoop) {
            id = getidfluxloop(efit, objectId);
        } else if (isMagProbe) {
            id = getidmagprobe(efit, objectId);
        } else if (isPFActive) {
            id = getidpfcoils(efit, objectId);
        }
        return setReturnDataString(idam_plugin_interface->data_block, id, "efitmagxml: object id returned");
    } else if (isPosition) {
        double data = 0.0;
        if (isFluxLoop) {
            float* r = NULL;
            float* z = NULL;
            float* phi = NULL;
            float aerr, rerr;
            getfluxloop(efit, objectId, &r, &z, &phi, &aerr, &rerr);
            if (isR) {
                data = (double)r[index];
            } else if (isZ) {
                data = (double)z[index];
            } else if (isPhi) {
                data = (double)phi[index];
            }
        } else if (isMagProbe) {
            float r, z, phi, aerr, rerr;
            getmagprobe(efit, objectId, &r, &z, &phi, &aerr, &rerr);
            if (isR) {
                data = (double)r;
            } else if (isZ) {
                data = (double)z;
            } else if (isPhi) {
                data = (double)phi;
            }
        }
        return setReturnDataDblScalar(idam_plugin_interface->data_block, data, "efitmagxml: Position Coordinate returned");
    } else if (isPFActive && isGeometry && isRectangle) {
        if (isElement) {
            double data = 0.0;
            float* r = NULL;
            float* z = NULL;
            float* width = NULL;
            float* height = NULL;
            float aerr, rerr;
            int turns;
            getpfcoil(efit, objectId, &turns, &r, &z, &width, &height, &aerr, &rerr);
            if (isR) {
                data = (double)r[index];
            } else if (isZ) {
                data = (double)z[index];
            } else if (isWidth) {
                data = (double)width[index];
            } else if (isHeight) {
                data = (double)height[index];
            }
            return setReturnDataDblScalar(idam_plugin_interface->data_block, data, "efitmagxml: Element Geometry/Coordinate returned");
        } else {
            // Return all element data
            int count = getnpfcoilcoords(efit, objectId);
            double* data = (double*)malloc(count * sizeof(double));
            float* r = NULL, * z = NULL, * width = NULL, * height = NULL, aerr, rerr;
            int turns;
            getpfcoil(efit, objectId, &turns, &r, &z, &width, &height, &aerr, &rerr);
            if (isR) {
                int i;
                for (i = 0; i < count; i++) data[i] = (double)r[i];
            } else if (isZ) {
                int i;
                for (i = 0; i < count; i++) data[i] = (double)z[i];
            } else if (isWidth) {
                int i;
                for (i = 0; i < count; i++) data[i] = (double)width[i];
            } else if (isHeight) {
                int i;
                for (i = 0; i < count; i++) data[i] = (double)height[i];
            }
            data_block->rank = 0;
            data_block->data_n = count;
            data_block->data_type = TYPE_DOUBLE;
            strcpy(data_block->data_desc, "efitmagxml: Element Geometry/Coordinate Array returned");
            data_block->data = (char*)data;
            return 0;
        }
    } else if (isDevice) {
        const char* device = getdevice(efit);
        return setReturnDataString(idam_plugin_interface->data_block, device, "efitmagxml: device name returned");
    }

    return 0;
}

int getnfluxloops(EFIT* efit)
{
    return efit->nfluxloops;
}

int getnfluxloopcoords(EFIT* efit, const int n)
{
    return efit->fluxloop[n].nco;
}

//int getnlimiter(EFIT* efit)
//{
//    return efit->nlimiter;
//}

//int getnlimitercoords(EFIT* efit)
//{
//    return efit->limiter->nco;
//}

int getnmagprobes(EFIT* efit)
{
    return efit->nmagprobes;
}

//int getnpfsupplies(EFIT* efit)
//{
//    return efit->npfsupplies;
//}

int getnpfcoils(EFIT* efit)
{
    return efit->npfcoils;
}

int getnpfcoilcoords(EFIT* efit, const int n)
{
    return efit->pfcoils[n].nco;
}

//int getnpfcircuits(EFIT* efit)
//{
//    return efit->npfcircuits;
//}

//int getnpfcircuitconnections(EFIT* efit, const int n)
//{
//    return efit->pfcircuit[n].nco;
//}

//int getnpfpassive(EFIT* efit)
//{
//    return efit->npfpassive;
//}

//int getnpfpassivecoords(EFIT* efit, const int n)
//{
//    return efit->pfpassive[n].nco;
//}

//int getnplasmacurrent(EFIT* efit)
//{
//    return efit->nplasmacurrent;
//}

//int getndiamagnetic(EFIT* efit)
//{
//    return efit->ndiamagnetic;
//}

//int getntoroidalfield(EFIT* efit)
//{
//    return efit->ntoroidalfield;
//}

//=========================================================================
// NAME

char* getnamemagprobe(EFIT* efit, const int index)
{
    return efit->magprobe[index].instance.signal;
}

char* getnamefluxloop(EFIT* efit, const int index)
{
    return efit->fluxloop[index].instance.signal;
}

//char* getnameplasmacurrent(EFIT* efit, const int index)
//{
//    return efit->plasmacurrent[index].instance.signal;
//}

//char* getnametoroidalfield(EFIT* efit, const int index)
//{
//    return efit->toroidalfield[index].instance.signal;
//}

char* getnamepfcoils(EFIT* efit, const int index)
{
    return efit->pfcoils[index].instance.signal;
}

//char* getnamepfpassive(EFIT* efit, const int index)
//{
//    return efit->pfpassive[index].instance.signal;
//}

//char* getnamepfsupplies(EFIT* efit, const int index)
//{
//    return efit->pfsupplies[index].instance.signal;
//}

//char* getnamepfcircuit(EFIT* efit, const int index)
//{
//    return efit->pfcircuit[index].instance.signal;
//}

//=========================================================================
// ID

char* getidmagprobe(EFIT* efit, const int index)
{
    return efit->magprobe[index].id;
}

char* getidfluxloop(EFIT* efit, const int index)
{
    return efit->fluxloop[index].id;
}

//char* getidplasmacurrent(EFIT* efit, const int index)
//{
//    return efit->plasmacurrent[index].id;
//}

//char* getidtoroidalfield(EFIT* efit, const int index)
//{
//    return efit->toroidalfield[index].id;
//}

char* getidpfcoils(EFIT* efit, const int index)
{
    return efit->pfcoils[index].id;
}

//char* getidpfpassive(EFIT* efit, const int index)
//{
//    return efit->pfpassive[index].id;
//}

//char* getidpfsupplies(EFIT* efit, const int index)
//{
//    return efit->pfsupplies[index].id;
//}

//char* getidpfcircuit(EFIT* efit, const int index)
//{
//    return efit->pfcircuit[index].id;
//}
