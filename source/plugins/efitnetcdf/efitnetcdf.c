/*---------------------------------------------------------------
* v1 IDAM Plugin: Extract Data from a EFIT++ netCDF output file
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

#include "idamclientpublic.h"
#include "idamclientserver.h"
#include "idamserver.h"
#include "idamserverfiles.h"

#include "idamgenstruct.h"
#include <mcheck.h>
#include <time.h>

#include "idamplugin.h"
#include "efitnetcdf.h"

#ifndef USE_PLUGIN_DIRECTLY

#include "/home/dgm/IDAM/source/clientserver/TrimString.c"
#endif

static FILE* dbgout, * errout;

#ifndef USE_PLUGIN_DIRECTLY
static ENVIRONMENT environment;
IDAMERRORSTACK* idamErrorStack;    // Pointer to the Server's Error Stack. Global scope within this plugin library
#endif

int efitnetcdf(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int i, j, k, err = 0, offset;
    char* p;

    static short init = 0;
    static EFIT efit;

//----------------------------------------------------------------------------------------
// Standard v1 Plugin Interface

    DATA_BLOCK* data_block;
    REQUEST_BLOCK* request_block;
    DATA_SOURCE* data_source;
    SIGNAL_DESC* signal_desc;

    PLUGINLIST* pluginList;    // List of all data reader plugins (internal and external shared libraries)

    FILE* old_stdout, * old_stderr;

    USERDEFINEDTYPE usertype;
    COMPOUNDFIELD field;

#ifndef USE_PLUGIN_DIRECTLY
    idamErrorStack = getIdamServerPluginErrorStack();        // Server's error stack
    LOGMALLOCLIST* logmalloclist = getIdamServerLogMallocList();
    USERDEFINEDTYPELIST* userdefinedtypelist = getIdamServerUserDefinedTypeList();

    initIdamErrorStack(&idamerrorstack);        // Initialise Local Error Stack (defined in idamclientserver.h)
#else
    IDAMERRORSTACK *idamErrorStack = &idamerrorstack;		// local and server are the same!
#endif

    unsigned short housekeeping;

    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        err = 999;
        if (verbose) {
            fprintf(errout,
                    "ERROR efitnetcdf: Plugin Interface Version Unknown to this plugin: Unable to execute the request!\n");
        }
        addIdamError(&idamerrorstack, CODEERRORTYPE, "efitnetcdf", err,
                     "Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
        concatIdamError(idamerrorstack, idamErrorStack);
        return err;
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    data_block = idam_plugin_interface->data_block;
    request_block = idam_plugin_interface->request_block;
    data_source = idam_plugin_interface->data_source;
    signal_desc = idam_plugin_interface->signal_desc;

    pluginList = idam_plugin_interface->pluginList;

    dbgout = idam_plugin_interface->dbgout;
    errout = idam_plugin_interface->errout;

    housekeeping = idam_plugin_interface->housekeeping;

    debugon = (dbgout != NULL);
    verbose = (errout != NULL);

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

        init = 0;

        if (!isReset) return 0;        // Step to Initialisation
    }

//----------------------------------------------------------------------------------------
// Initialise 

    if (!init || !strcasecmp(request_block->function, "init")
        || !strcasecmp(request_block->function, "initialise")) {

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

        unsigned short int isObjectId = 0, isIndex = 0, isCount = 0, isIndex = 0, isElement = 0, isName = 0,
                isIdentifier, isSignal = 0, isGeometry = 0,
                isR = 0, isZ = 0, isHeight = 0, isWidth = 0,
                isToroidalAngle = 0, isPoloidalAngle = 0, isTurnsWithSign = 0,
                isUnitStartIndex = 0;
        int objectId = -1, index = -1;

// Arguments and keywords 

        for (i = 0; i < request_block->nameValueList.pairCount; i++) {

            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "id")) {
                isObjectId = 1;
                objectId = atoi(request_block->nameValueList.nameValue[i].value);
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "index")) {
                isIndex = 1;
                index = atoi(request_block->nameValueList.nameValue[i].value);
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "Element")) {
                isElement = 1;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "Count")) {
                isCount = 1;
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
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "Height")) {
                isR = 1;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "Width")) {
                isZ = 1;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "ToroidalAngle")) {
                isPhi = 1;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "PoloidalAngle")) {
                isPhi = 1;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "TurnsWithSign")) {
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
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "Geometry")) {
                isGeometry = 1;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "UnitStartIndex")) {
                isUnitStartIndex = 1;
                continue;
            }

        }

        if (isUnitStartIndex) {
            if (isObjectId) objectId = objectId - 1;    // All C arrays begin with index 0
            if (isIndex) index = index - 1;
        }

//---------------------------------------------------------------------------------------- 
// get(File=File, /UnitStartIndex) 

/* Mapping from PF_ACTIVE IDS names to data objects

PF_ACTIVE/COIL/SHAPE_OF							/Count							db query: select count(*) from pf_active_ids where exp_number is in range   
PF_ACTIVE/COIL[id]/ELEMENT/SHAPE_OF					id=id, /Element, /Count					db query: select count(*) from signal_desc where exp_number is in range and type='P' and source_alias='imas' and 
																		signal_alias ilike "PF_ACTIVE/COIL/[id]/ELEMENT/%/TURNS_WITH_SIGN"
PF_ACTIVE/COIL[id]/CURRENT/DATA						IMAS::Source 
PF_ACTIVE/COIL[id]/CURRENT/TIME						IMAS::Source 

PF_ACTIVE/COIL[id]/NAME							id=id, /Name						db query: select name from pf_active_ids where exp_number is in range and identifier=id
PF_ACTIVE/COIL[id]/IDENTIFIER						id=id, /Identifier					db query: select identifier from pf_active_ids where exp_number is in range and identifier=id

PF_ACTIVE/COIL[id]/ELEMENT[index]/TURNS_WITH_SIGN			id=id, /Element, index=index, /Turns			netCDF File: /input/pfSystem/pfCoilsGeometry
PF_ACTIVE/COIL[id]/ELEMENT[index]/GEOMETRY/RECTANGLE/R			id=id, /Element, index=index, /Geometry, /R		netCDF File: /input/pfSystem/pfCoilsGeometry.r
PF_ACTIVE/COIL[id]/ELEMENT[index]/GEOMETRY/RECTANGLE/Z			id=id, /Element, index=index, /Geometry, /Z		netCDF FIle: /input/pfSystem/pfCoilsGeometry.z
PF_ACTIVE/COIL[id]/ELEMENT[index]/GEOMETRY/RECTANGLE/WIDTH		id=id, /Element, index=index, /Geometry, /Width		netCDF File: /input/pfSystem/pfCoilsGeometry.dr
PF_ACTIVE/COIL[id]/ELEMENT[index]/GEOMETRY/RECTANGLE/HEIGHT		id=id, /Element, index=index, /Geometry, /Height	netCDF FIle: /input/pfSystem/pfCoilsGeometry.dz
PF_ACTIVE/COIL[id]/ELEMENT[index]/GEOMETRY/RECTANGLE/TOROIDAL_ANGLE	id=id, /Element, index=index, /Geometry, /ToroidalAngle	netCDF File: /input/pfSystem/pfCoilsGeometry.angle1
PF_ACTIVE/COIL[id]/ELEMENT[index]/GEOMETRY/RECTANGLE/POLOIDAL_ANGLE	id=id, /Element, index=index, /Geometry, /PoloidalAngle	netCDF FIle: /input/pfSystem/pfCoilsGeometry.angle2

ToDo:

1> Reset when the file name changes
2> Ignore data when the signal name is "noData"

3> Use the database plugin provided - it must have a general query api - not this plugin's concern what the database technology is

*/

        if (!strcasecmp(request_block->function, "get")) {
            initDataBlock(data_block);

            int count;
            float r, z, phi;
            char* p, * name;

            if (isCount) {
                if (isElement) {
                    // test for id
                    count = dbquery(select
                    count(*)
                    from
                            signal_desc
                    where
                            exp_number
                    is
                            in
                    range
                            and
                    type = 'P'
                    and
                            source_alias = 'imas'
                    and
                            signal_alias
                    ilike
                    "PF_ACTIVE/COIL/[id]/ELEMENT/%/TURNS_WITH_SIGN");
                } else {
                    count = dbquery(select
                }
                count(*)
                from
                        pf_active_ids
                where
                        exp_number
                is
                        in
                range);
            }

            data_block->data_n = 1;
            data_block->rank = 0;
            data_block->data_type = TYPE_INT;
            strcpy(data_block->data_desc, "efitnetcdf: object count returned");
            int* data = (int*) malloc(sizeof(int));
            *data = count;
            data_block->data = (char*) data;
            break;
        } else if (isSignal) {
            // test for id
            p = dbquery(db
            query:
            select
                    signal_alias
            from
                    pf_active_ids
            where
                    exp_number
            is
                    in
            range
            and identifier = id;
            data_block->data_n = strlen(p) + 1;
            data_block->rank = 0;
            data_block->data_type = TYPE_STRING;
            strcpy(data_block->data_desc, "efitnetcdf: object name returned");
            data_block->data = (char*) malloc(data_block->data_n * sizeof(char));
            strcpy(data_block->data, p);
            break;
        } else if (isName || isIdentifier) {

            p = dbquery(select
            name
                    from
            pf_active_ids
                    where
            exp_number
                    is
            in
                    range
            and
            identifier = id);

            data_block->data_n = strlen(p) + 1;
            data_block->rank = 0;
            data_block->data_type = TYPE_STRING;
            strcpy(data_block->data_desc, "efitnetcdf: object name returned");
            data_block->data = (char*) malloc(data_block->data_n * sizeof(char));
            strcpy(data_block->data, p);
            break;
        } else if (isGeometry) {

// Read "/input/pfSystem/pfCoilsGeometry" from file "/home/dgm/IDAM/test/source/plugins/livedisplay/EFIT/efitOut.nc"	 
// cast to structure	 	 	 

            float* data = (float*) malloc(sizeof(float));
            *data = 0.0;
            int rc;
            float r, z, phi, aerr, rerr;
            rc = netcdf();
            if (isR) {
                *data = r;
            } else if (isZ) {
                *data = z;
            } else if (isPhi) {
                *data = phi;
            }
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data_type = TYPE_FLOAT;
            strcpy(data_block->data_desc, "efitnetcdf: Position Coordinate returned");
            data_block->data = (char*) data;
            break;
        } else {
            break;
        }
    } else

//----------------------------------------------------------------------------------------
// HELP Documentation
//----------------------------------------------------------------------------------------

// Help: A Description of library functionality

    if (!strcasecmp(request_block->function, "help")) {

        p = (char*) malloc(sizeof(char) * 2 * 1024);

        strcpy(p, "\nefitnetcdf: Add Functions Names, Syntax, and Descriptions\n\n");

        initDataBlock(data_block);

        data_block->rank = 1;
        data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
        for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

        data_block->data_type = TYPE_STRING;
        strcpy(data_block->data_desc, "efitnetcdf: help = description of this plugin");

        data_block->data = (char*) p;

        data_block->dims[0].data_type = TYPE_UNSIGNED_INT;
        data_block->dims[0].dim_n = strlen(p) + 1;
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
        data_block->data_n = strlen(__DATE__) + 1;
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
        data_block->data_n = strlen(THISPLUGIN_DEFAULT_METHOD) + 1;
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
        addIdamError(&idamerrorstack, CODEERRORTYPE, "efitnetcdf", err, "Unknown function requested!");
        break;
    }

}

while(0);

//--------------------------------------------------------------------------------------
// Housekeeping

concatIdamError(idamerrorstack, idamErrorStack
);    // Combine local errors with the Server's error stack

#ifndef USE_PLUGIN_DIRECTLY
closeIdamError(&idamerrorstack);            // Free local plugin error stack
#endif

return
err;
}

