/*---------------------------------------------------------------
* IDAM Plugin data Reader to Create a proxy MAGENTICS IDS 
*
* Input Arguments:	IDAM_PLUGIN_INTERFACE *idam_plugin_interface
*
* Returns:		livedisplay	0 if read was successful
*					otherwise a Error Code is returned
*			DATA_BLOCK	Structure with Data from the File
*
* Calls		freeDataBlock	to free Heap memory if an Error Occurs
*
* Notes: 	All memory required to hold data is allocated dynamically
*		    in heap storage. Pointers to these areas of memory are held
*		    by the passed DATA_BLOCK structure. Local memory allocations
*		    are freed on exit. However, the blocks reserved for data are
*		    not and MUST BE FREED by the calling routine.

test1	CODE
test2	MAGNETICS_TEST2
test3 - test7	FLUX_LOOP
test8	FLUX_LOOP with measurement data
test9x	FLUX_LOOP measurement data only
test9	FLUX_LOOP with machine description cache
test10x	BPOL_PROBE measurement data only
test10y	BPOL_PROBE machine description only with cache	
test10	BPOL_PROBE	

magnetics || test11	MAGNETICS_PROXY
limiter			EQUILIBRIUM_PROXY
pfactive		PF_ACTIVE_PROXY		

Options: test11 for all data or
         test11 for first pass, then test9x for Flux Loop measurment data only followed by test10x for Bpol Probe data only

test11 is timed at 5.5 seconds on initialisation and machine description caching (46 flux loops and 78 bpol probes = 124 signals)
Subsequent calls for cached data and subset measurement data are 2.2 seconds
=> 18ms per signal (this could be speeded up by using faster database lookup and memcache on the server's IDAM client (not available with live data!) )

The option to have multiple calls is slower: 7.5s to initialise and 3.6s for the measurement data only.
=> 29ms per signal!

Issues: MAST data specific

*
* Change History
*
* 28Sept2016	D.G.Muir	Original Version
*---------------------------------------------------------------------------------------------------------------*/
#include "livedisplay.h"

#include "livedisplay.h"

#include <stdlib.h>
#include <strings.h>

#include <client/accAPI.h>
#include <client/udaGetAPI.h>
#include <structures/struct.h>
#include <structures/accessors.h>
#include <clientserver/stringUtils.h>
#include <client/udaClient.h>
#include <server/makeServerRequestBlock.h>
#include <clientserver/initStructs.h>
#include <clientserver/freeDataBlock.h>

void defineIDSStructures()
{
    int offset = 0; //, stringLength;

    USERDEFINEDTYPE usertype;
    COMPOUNDFIELD field;

//#ifndef USE_PLUGIN_DIRECTLY
    USERDEFINEDTYPELIST* userdefinedtypelist = getIdamServerUserDefinedTypeList();
//#endif

    initUserDefinedType(&usertype);   // New structure definition

    strcpy(usertype.name, "CODE");
    usertype.size = sizeof(CODE);
    strcpy(usertype.source, "LiveDisplay");
    usertype.ref_id = 0;
    usertype.imagecount = 0;       // No Structure Image data
    usertype.image = NULL;
    usertype.idamclass = TYPE_COMPOUND;

    offset = 0;

    defineField(&field, "name", "Code Name", &offset, SCALARSTRING);
    addCompoundField(&usertype, field);

    defineField(&field, "version", "Code Version", &offset, SCALARSTRING);
    addCompoundField(&usertype, field);
    defineField(&field, "parameters", "Code Parameters", &offset, SCALARSTRING);
    addCompoundField(&usertype, field);
    defineField(&field, "output_flag_count", "The array count", &offset, SCALARINT);
    addCompoundField(&usertype, field);
    defineField(&field, "output_flag", "The Code output flags", &offset, ARRAYINT);
    addCompoundField(&usertype, field);

    addUserDefinedType(userdefinedtypelist, usertype);    // CODE


    initUserDefinedType(&usertype);   // New structure definition

    strcpy(usertype.name, "FLUX_LOOP");
    usertype.size = sizeof(FLUX_LOOP);
    strcpy(usertype.source, "LiveDisplay");
    usertype.ref_id = 0;
    usertype.imagecount = 0;       // No Structure Image data
    usertype.image = NULL;
    usertype.idamclass = TYPE_COMPOUND;

    offset = 0;

    defineField(&field, "identifier", "Identifier", &offset, SCALARSTRING);
    addCompoundField(&usertype, field);
    defineField(&field, "name", "name", &offset, SCALARSTRING);
    addCompoundField(&usertype, field);
    defineField(&field, "position_count", "Position Count", &offset, SCALARINT);
    addCompoundField(&usertype, field);
    defineField(&field, "r", "Major Radius", &offset, ARRAYDOUBLE);
    addCompoundField(&usertype, field);
    defineField(&field, "z", "Height", &offset, ARRAYDOUBLE);
    addCompoundField(&usertype, field);
    defineField(&field, "phi", "Toroidal Angle", &offset, ARRAYDOUBLE);
    addCompoundField(&usertype, field);
    defineField(&field, "xdata_count", "Measurement data count", &offset, SCALARINT);
    addCompoundField(&usertype, field);
    defineField(&field, "xdata", "Measurement data", &offset, ARRAYDOUBLE);        // Doesn't like 'data' !!!
    addCompoundField(&usertype, field);
    defineField(&field, "xtime", "Measurement time", &offset, ARRAYDOUBLE);
    addCompoundField(&usertype, field);

    addUserDefinedType(userdefinedtypelist, usertype);    // FLUX_LOOP


    initUserDefinedType(&usertype);   // New structure definition

    strcpy(usertype.name, "BPOL_PROBE");
    usertype.size = sizeof(BPOL_PROBE);
    strcpy(usertype.source, "LiveDisplay");
    usertype.ref_id = 0;
    usertype.imagecount = 0;       // No Structure Image data
    usertype.image = NULL;
    usertype.idamclass = TYPE_COMPOUND;

    offset = 0;

    defineField(&field, "identifier", "Identifier", &offset, SCALARSTRING);
    addCompoundField(&usertype, field);
    defineField(&field, "name", "name", &offset, SCALARSTRING);
    addCompoundField(&usertype, field);
    defineField(&field, "r", "Major Radius", &offset, SCALARDOUBLE);
    addCompoundField(&usertype, field);
    defineField(&field, "z", "Height", &offset, SCALARDOUBLE);
    addCompoundField(&usertype, field);
    defineField(&field, "phi", "Toroidal Angle", &offset, SCALARDOUBLE);
    addCompoundField(&usertype, field);
    defineField(&field, "xdata_count", "Measurement data count", &offset, SCALARINT);
    addCompoundField(&usertype, field);
    defineField(&field, "xdata", "Measurement data", &offset, ARRAYDOUBLE);        // Doesn't like 'data' !!!
    addCompoundField(&usertype, field);
    defineField(&field, "xtime", "Measurement time", &offset, ARRAYDOUBLE);
    addCompoundField(&usertype, field);

    addUserDefinedType(userdefinedtypelist, usertype);    // BPOL_PROBE


    initUserDefinedType(&usertype);   // New structure definition

    strcpy(usertype.name, "METHOD_DATA");
    usertype.size = sizeof(METHOD_DATA);
    strcpy(usertype.source, "LiveDisplay");
    usertype.ref_id = 0;
    usertype.imagecount = 0;       // No Structure Image data
    usertype.image = NULL;
    usertype.idamclass = TYPE_COMPOUND;

    offset = 0;

    defineField(&field, "identifier", "Identifier", &offset, SCALARSTRING);
    addCompoundField(&usertype, field);
    defineField(&field, "name", "name", &offset, SCALARSTRING);
    addCompoundField(&usertype, field);
    defineField(&field, "xdata_count", "Measurement data count", &offset, SCALARINT);
    addCompoundField(&usertype, field);
    defineField(&field, "xdata", "Measurement data", &offset, ARRAYDOUBLE);        // Doesn't like 'data' !!!
    addCompoundField(&usertype, field);
    defineField(&field, "xtime", "Measurement time", &offset, ARRAYDOUBLE);
    addCompoundField(&usertype, field);

    addUserDefinedType(userdefinedtypelist, usertype);    // METHOD_DATA

/*
typedef struct{
   STRING *name;
   METHOD_DATA *ip;
   METHOD_DATA *diamagnetic_flux;
} METHOD;
*/
    initUserDefinedType(&usertype);   // New structure definition

    strcpy(usertype.name, "METHOD");
    usertype.size = sizeof(METHOD);
    strcpy(usertype.source, "LiveDisplay");
    usertype.ref_id = 0;
    usertype.imagecount = 0;       // No Structure Image data
    usertype.image = NULL;
    usertype.idamclass = TYPE_COMPOUND;

    offset = 0;

    defineField(&field, "name", "name", &offset, SCALARSTRING);
    addCompoundField(&usertype, field);

    initCompoundField(&field);
    strcpy(field.name, "ip");
    field.atomictype = TYPE_UNKNOWN;
    strcpy(field.type, "METHOD_DATA");
    strcpy(field.desc, "Plasma Current");
    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = NULL;
    field.size = field.count * sizeof(void*);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    offset = field.offset + field.size;
    addCompoundField(&usertype, field);

    initCompoundField(&field);
    strcpy(field.name, "diamagnetic_flux");
    field.atomictype = TYPE_UNKNOWN;
    strcpy(field.type, "METHOD_DATA");
    strcpy(field.desc, "Diamagnetic Flux");
    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = NULL;
    field.size = field.count * sizeof(void*);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    offset = field.offset + field.size;
    addCompoundField(&usertype, field);

    addUserDefinedType(userdefinedtypelist, usertype);    // METHOD


/*
typedef struct{
   STRING * comment;
   int homogeneous_time;
   int flux_loop_count;
   FLUX_LOOP *flux_loop;		// Array of Flux Loops
   int bpol_probe_count;
   BPOL_PROBE *bpol_probe;		// Array of Magnetic Probes
   int method_count;
   METHOD *method;			// Array of measurement methods and values
   //CODE code;
} MAGNETICS_PROXY;
*/
    initUserDefinedType(&usertype);   // New structure definition

    strcpy(usertype.name, "MAGNETICS_PROXY");
    usertype.size = sizeof(MAGNETICS_PROXY);
    strcpy(usertype.source, "LiveDisplay");
    usertype.ref_id = 0;
    usertype.imagecount = 0;       // No Structure Image data
    usertype.image = NULL;
    usertype.idamclass = TYPE_COMPOUND;

    offset = 0;

    //defineField(&field, "comment", "comment", &offset, SCALARSTRING);
    //addCompoundField(&usertype, field);
    //defineField(&field, "homogeneous_time", "homogeneous time", &offset, SCALARINT);
    //addCompoundField(&usertype, field);

    defineField(&field, "flux_loop_count", "Flux Loop count", &offset, SCALARINT);
    addCompoundField(&usertype, field);

// FLUX_LOOP structure array

    initCompoundField(&field);
    strcpy(field.name, "flux_loop");
    field.atomictype = TYPE_UNKNOWN;
    strcpy(field.type, "FLUX_LOOP");
    strcpy(field.desc, "Array of Flux Loop data");
    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = NULL;            // Needed when rank >= 1
    field.size = field.count * sizeof(void*);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    offset = field.offset + field.size;
    addCompoundField(&usertype, field);

    defineField(&field, "bpol_probe_count", "Bpol Probe count", &offset, SCALARINT);
    addCompoundField(&usertype, field);

// BPOL_PROBE structure array

    initCompoundField(&field);
    strcpy(field.name, "bpol_probe");
    field.atomictype = TYPE_UNKNOWN;
    strcpy(field.type, "BPOL_PROBE");
    strcpy(field.desc, "Array of Bpol Probe data");
    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = NULL;            // Needed when rank >= 1
    field.size = field.count * sizeof(void*);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    offset = field.offset + field.size;
    addCompoundField(&usertype, field);

    defineField(&field, "method_count", "Method count", &offset, SCALARINT);
    addCompoundField(&usertype, field);

// METHOD structure array

    initCompoundField(&field);
    strcpy(field.name, "method");
    field.atomictype = TYPE_UNKNOWN;
    strcpy(field.type, "METHOD");
    strcpy(field.desc, "Array of Method measurement data");
    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = NULL;            // Needed when rank >= 1
    field.size = field.count * sizeof(void*);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    offset = field.offset + field.size;
    addCompoundField(&usertype, field);

/*
// CODE structure array

	 initCompoundField(&field);
         strcpy(field.name, "code");
         field.atomictype = TYPE_UNKNOWN;
         strcpy(field.type, "CODE");
         strcpy(field.desc, "Code");
         field.pointer    = 0;
         field.count      = 1;
         field.rank       = 0;
	 field.shape      = NULL;			// Needed when rank >= 1
	 field.size       = field.count*sizeof(CODE);
	 field.offset     = newoffset(offset,field.type);
	 field.offpad     = padding(offset,field.type);
         field.alignment  = getalignmentof(field.type);
	 offset           = field.offset + field.size;
         addCompoundField(&usertype, field);
*/
    addUserDefinedType(userdefinedtypelist, usertype);    // METHOD_DATA




/*
typedef struct{
   int   count;
   double *r;
   double *z;
} ACTIVE_LIMITER_POINT;
*/
    initUserDefinedType(&usertype);   // New structure definition

    strcpy(usertype.name, "STATIC_LIMITER");
    usertype.size = sizeof(STATIC_LIMITER);
    strcpy(usertype.source, "LiveDisplay");
    usertype.ref_id = 0;
    usertype.imagecount = 0;       // No Structure Image data
    usertype.image = NULL;
    usertype.idamclass = TYPE_COMPOUND;

    offset = 0;

    defineField(&field, "count", "Number of coordinates", &offset, SCALARINT);
    addCompoundField(&usertype, field);
    defineField(&field, "r", "Major Radius", &offset, ARRAYDOUBLE);
    addCompoundField(&usertype, field);
    defineField(&field, "z", "Height", &offset, ARRAYDOUBLE);
    addCompoundField(&usertype, field);

    addUserDefinedType(userdefinedtypelist, usertype);    // STATIC_LIMITER

/*
typedef struct{
   int data_count;			// Number of data Measurements
   double *r0;				// Major Radius of Measurement (m)
   double *b0;				// Vacuum Toroidal Magnetic Field at the Measurement Major Radius (T)
   double *rb0;				// Product r0 * b0 (Tm)
   double *time;				// Measurement Time+
} TF_PROXY;
*/

    initUserDefinedType(&usertype);   // New structure definition

    strcpy(usertype.name, "TF_PROXY");
    usertype.size = sizeof(TF_PROXY);
    strcpy(usertype.source, "LiveDisplay");
    usertype.ref_id = 0;
    usertype.imagecount = 0;       // No Structure Image data
    usertype.image = NULL;
    usertype.idamclass = TYPE_COMPOUND;

    offset = 0;

    defineField(&field, "data_count", "Measurement data count", &offset, SCALARINT);
    addCompoundField(&usertype, field);
    defineField(&field, "r0", "Major Radius", &offset, ARRAYDOUBLE);
    addCompoundField(&usertype, field);
    defineField(&field, "b0", "Vacuum Magnetic Field at r0", &offset, ARRAYDOUBLE);
    addCompoundField(&usertype, field);
    defineField(&field, "rb0", "Product r0*b0", &offset, ARRAYDOUBLE);
    addCompoundField(&usertype, field);
    defineField(&field, "time", "Measurement Time", &offset, ARRAYDOUBLE);
    addCompoundField(&usertype, field);

    addUserDefinedType(userdefinedtypelist, usertype);    // Toroidal Magnetic Field IDS Proxy

/*
typedef struct{
   double r;				// Radial Position array
   double z;				// Z Position array
   double width;			// Width array
   double height;			// Height array
} PF_ACTIVE_ELEMENT;
*/
    initUserDefinedType(&usertype);   // New structure definition

    strcpy(usertype.name, "PF_ACTIVE_ELEMENT");
    usertype.size = sizeof(PF_ACTIVE_ELEMENT);
    strcpy(usertype.source, "LiveDisplay");
    usertype.ref_id = 0;
    usertype.imagecount = 0;
    usertype.image = NULL;
    usertype.idamclass = TYPE_COMPOUND;

    offset = 0;

    defineField(&field, "r", "Major Radius", &offset, SCALARDOUBLE);
    addCompoundField(&usertype, field);
    defineField(&field, "z", "Z position", &offset, SCALARDOUBLE);
    addCompoundField(&usertype, field);
    defineField(&field, "width", "Width", &offset, SCALARDOUBLE);
    addCompoundField(&usertype, field);
    defineField(&field, "height", "Height", &offset, SCALARDOUBLE);
    addCompoundField(&usertype, field);

    addUserDefinedType(userdefinedtypelist, usertype);    // PF_ACTIVE_ELEMENT

/*
typedef struct{
   STRING * identifier;
   STRING * name;
   int  element_count;			// Number of Coil Elements positions
   PF_ACTIVE_ELEMENT *element;		// Coil Element properties
   int data_count;			// Number of data points
   double *data;			// Measurement data array
   double *time;			// Measurement time array
} PF_ACTIVE;
*/

    initUserDefinedType(&usertype);

    strcpy(usertype.name, "PF_ACTIVE");
    usertype.size = sizeof(PF_ACTIVE);
    strcpy(usertype.source, "LiveDisplay");
    usertype.ref_id = 0;
    usertype.imagecount = 0;
    usertype.image = NULL;
    usertype.idamclass = TYPE_COMPOUND;

    offset = 0;

    defineField(&field, "identifier", "Identifier", &offset, SCALARSTRING);
    addCompoundField(&usertype, field);
    defineField(&field, "name", "name", &offset, SCALARSTRING);
    addCompoundField(&usertype, field);
    defineField(&field, "element_count", "Count of Coil Elements/Coordinates", &offset, SCALARINT);
    addCompoundField(&usertype, field);

// PF_ACTIVE_ELEMENT structure array

    initCompoundField(&field);
    strcpy(field.name, "element");
    field.atomictype = TYPE_UNKNOWN;
    strcpy(field.type, "PF_ACTIVE_ELEMENT");
    strcpy(field.desc, "Array of Coil Element properties");
    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = NULL;            // Needed when rank >= 1
    field.size = field.count * sizeof(void*);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    offset = field.offset + field.size;
    addCompoundField(&usertype, field);

    defineField(&field, "xdata_count", "Measurement data count", &offset, SCALARINT);
    addCompoundField(&usertype, field);
    defineField(&field, "xdata", "Measurement data", &offset, ARRAYDOUBLE);        // Doesn't like 'data' !!!
    addCompoundField(&usertype, field);
    defineField(&field, "xtime", "Measurement time", &offset, ARRAYDOUBLE);
    addCompoundField(&usertype, field);

    addUserDefinedType(userdefinedtypelist, usertype);    // PF_ACTIVE


/*
typedef struct{
   int pfactive_count;
   PF_ACTIVE *pfactive;			// Array of PF Active Coils
} PF_ACTIVE_PROXY;			// Proxy for the PF_Active IDS
*/

    initUserDefinedType(&usertype);

    strcpy(usertype.name, "PF_ACTIVE_PROXY");
    usertype.size = sizeof(PF_ACTIVE_PROXY);
    strcpy(usertype.source, "LiveDisplay");
    usertype.ref_id = 0;
    usertype.imagecount = 0;
    usertype.image = NULL;
    usertype.idamclass = TYPE_COMPOUND;

    offset = 0;

    defineField(&field, "pfactive_count", "Count of PF Active Coils", &offset, SCALARINT);
    addCompoundField(&usertype, field);

// PF_ACTIVE structure array

    initCompoundField(&field);
    strcpy(field.name, "pfactive");
    field.atomictype = TYPE_UNKNOWN;
    strcpy(field.type, "PF_ACTIVE");
    strcpy(field.desc, "Array of PF Active Coil properties");
    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = NULL;
    field.size = field.count * sizeof(void*);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    offset = field.offset + field.size;
    addCompoundField(&usertype, field);

    addUserDefinedType(userdefinedtypelist, usertype);    // PF_ACTIVE_PROXY
}

void initRB(TF_PROXY* str)
{
    str->data_count = 0;
    str->r0 = NULL;
    str->b0 = NULL;
    str->rb0 = NULL;
    str->time = NULL;
}

void initStaticLimiter(STATIC_LIMITER* str)
{
    str->count = 0;
    str->r = NULL;
    str->z = NULL;
}

void initFluxLoop(FLUX_LOOP* floop)
{
    floop->identifier = NULL;
    floop->name = NULL;
    floop->position_count = 0;
    floop->r = NULL;
    floop->z = NULL;
    floop->phi = NULL;
    floop->data_count = 0;
    floop->data = NULL;
    floop->time = NULL;
}

void freeFluxLoop(FLUX_LOOP* floop)
{
    if (floop->identifier != NULL) free((void*)floop->identifier);
    if (floop->name != NULL) free((void*)floop->name);
    if (floop->r != NULL) free((void*)floop->r);
    if (floop->z != NULL) free((void*)floop->z);
    if (floop->phi != NULL) free((void*)floop->phi);
    if (floop->data != NULL) free((void*)floop->data);
    if (floop->time != NULL) free((void*)floop->time);
    initFluxLoop(floop);
}

void initBpolProbe(BPOL_PROBE* str)
{
    str->identifier = NULL;
    str->name = NULL;
    str->r = 0.0;
    str->z = 0.0;
    str->phi = 0.0;
    str->data_count = 0;
    str->data = NULL;
    str->time = NULL;
}

void freeBpolProbe(BPOL_PROBE* str)
{
    if (str->identifier != NULL) free((void*)str->identifier);
    if (str->name != NULL) free((void*)str->name);
    if (str->data != NULL) free((void*)str->data);
    if (str->time != NULL) free((void*)str->time);
    initBpolProbe(str);
}

void initMethodData(METHOD_DATA* str)
{
    str->identifier = NULL;
    str->name = NULL;
    str->count = 0;
    str->data = NULL;
    str->time = NULL;
}

void freeMethodData(METHOD_DATA* str)
{
    if (str->identifier != NULL) free((void*)str->identifier);
    if (str->name != NULL) free((void*)str->name);
    if (str->data != NULL) free((void*)str->data);
    if (str->time != NULL) free((void*)str->time);
    initMethodData(str);
}

void initMethod(METHOD* str)
{
    str->name = NULL;
    str->ip = NULL;
    str->diamagnetic_flux = NULL;
}

void initBpolProbe2(BPOL_PROBE_TEST2* str)
{
    str->identifier = NULL;
    str->name = NULL;
    str->r = 0.0;
    str->z = 0.0;
    str->phi = 0.0;
}

void freeBpolProbe2(BPOL_PROBE_TEST2* str)
{
    if (str->identifier != NULL) free((void*)str->identifier);
    if (str->name != NULL) free((void*)str->name);
    initBpolProbe2(str);
}

void initPFActiveElement(PF_ACTIVE_ELEMENT* str)
{
    str->r = 0.0;
    str->z = 0.0;
    str->width = 0.0;
    str->height = 0.0;
}

void initPFActive(PF_ACTIVE* str)
{
    str->identifier = NULL;
    str->name = NULL;
    str->element_count = 0;
    str->element = NULL;
    str->data_count = 0;
    str->data = NULL;
    str->time = NULL;
}

void initPFActiveProxy(PF_ACTIVE_PROXY* str)
{
    str->pfactive_count = 0;
    str->pfactive = NULL;
}

void freePFActive(PF_ACTIVE* str)
{
    if (str->identifier != NULL) free((void*)str->identifier);
    if (str->name != NULL) free((void*)str->name);
    if (str->element != NULL) free((void*)str->element);
    initPFActive(str);
}

void freePFActiveProxy(PF_ACTIVE_PROXY* str)
{
    int i;
    if (str->pfactive != NULL) for (i = 0; i < str->pfactive_count; i++) freePFActive(&str->pfactive[i]);
    initPFActiveProxy(str);
}

// Return Count Data

int returnCount(int count, DATA_BLOCK* data_block)
{
    data_block->data_type = TYPE_INT;
    data_block->rank = 0;
    data_block->data_n = 1;
    int* data = (int*)malloc(sizeof(int));
    data[0] = count;
    data_block->data = (char*)data;
    return 0;
}

// Subset the data
int dataSubset(double* times, int data_count, double startTime, int isEndTime, double endTime, int isFirst, int isLast,
               int isNearest,
               int* subset_count, int* index1, int* index2)
{
    int j, err = 0;
    if (data_count == 0) return 1;

    *index1 = -1;
    *index2 = -1;
    *subset_count = 0;

    if (startTime > times[0] && startTime < times[data_count - 1]) {
        for (j = 0; j < data_count; j++) {
            if (*index1 < 0 && times[j] >= startTime) {
                *index1 = j;
                if (!isEndTime) break;
            }
            if (*index1 >= 0 && *index2 < 0 && times[j] >= endTime) {
                *index2 = j - 1;
                if (*index2 < 0) *index2 = 0;
                if (times[j] == endTime) *index2 = j;
                break;
            }
        }
    } else {
        if (startTime <= times[0]) {
            *index1 = 0;
        } else {
            *index1 = data_count - 1;
        }
    }

    if (isEndTime) {    // Subset of the data
        if (*index2 == -1) *index2 = data_count - 1;
        *subset_count = *index2 - *index1 + 1;
    } else {        // Single value
        *subset_count = 1;
        *index2 = *index1;
    }

    if (isFirst) {
        *subset_count = 1;
        *index2 = *index1;
    } else if (isLast) {
        *subset_count = 1;
        *index1 = *index2;
    } else if (isNearest) {
        *subset_count = 1;
        if (*index1 > 0 && ((startTime - times[*index1 - 1]) > (times[*index1] - startTime))) *index1 = *index1 - 1;
        *index2 = *index1;
    }

    if (*index1 == -1 || *index2 == -1) err = 1;

    return err;
}

extern int livedisplay(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;
    static short init = 0;

    static FLUX_LOOP* flux_loop_cache = NULL;            // maintain a copy of the time invarient machine description data
    static unsigned short flux_loop_cache_count = 0;        // How many flux loops
    static char flux_loop_source[256];                // The re-cache trigger
    static BPOL_PROBE* bpol_probe_cache = NULL;
    static unsigned short bpol_probe_cache_count = 0;
    static char bpol_probe_source[256];

    static PF_ACTIVE* pfactive_cache = NULL;
    static unsigned short pfactive_cache_count = 0;
    static char pfactive_source[256];

    USERDEFINEDTYPE usertype;
    COMPOUNDFIELD field;
    unsigned short housekeeping;

    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        THROW_ERROR(999, "Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
    }

    idam_plugin_interface->pluginVersion = 1;

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    const PLUGINLIST* pluginList = idam_plugin_interface->pluginList;

    housekeeping = idam_plugin_interface->housekeeping;

//----------------------------------------------------------------------------------------
// Heap Housekeeping

    if (housekeeping || !strcasecmp(request_block->function, "reset")) {

        //idamLog(UDA_LOG_DEBUG, "LiveDisplay: reset function called.\n");

        if (flux_loop_cache != NULL) {
            int i;
            for (i = 0; i < flux_loop_cache_count; i++) freeFluxLoop(&flux_loop_cache[i]);
            free((void*)flux_loop_cache);
            flux_loop_cache = NULL;
            flux_loop_cache_count = 0;
        }
        flux_loop_source[0] = '\0';

        if (bpol_probe_cache != NULL) {
            int i;
            for (i = 0; i < bpol_probe_cache_count; i++) freeBpolProbe(&bpol_probe_cache[i]);
            free((void*)bpol_probe_cache);
            bpol_probe_cache = NULL;
            bpol_probe_cache_count = 0;
        }
        bpol_probe_source[0] = '\0';

        if (!init) return 0;        // Not previously initialised: Nothing to do!

        init = 0;        // Ready to re-initialise
        return 0;
    }

//----------------------------------------------------------------------------------------
// Initialise if requested

    if (!init || !strcasecmp(request_block->function, "init")
        || !strcasecmp(request_block->function, "initialise")) {

        flux_loop_cache = NULL;
        flux_loop_cache_count = 0;
        flux_loop_source[0] = '\0';

        bpol_probe_cache = NULL;
        bpol_probe_cache_count = 0;
        bpol_probe_source[0] = '\0';

        init = 1;

        if (!strcasecmp(request_block->function, "init") || !strcasecmp(request_block->function, "initialise")) {
            return 0;
        }
    }

//----------------------------------------------------------------------------------------
// Name Value pairs and Keywords

    unsigned short isExp_number = 0, isPass = 0, isDevice = 0, isStartTime = 0, isEndTime = 0, isCache = 0, isStructureVersion = 0;
    unsigned short isAverage = 0, isNearest = 0, isFirst = 0, isLast = 0, isCount = 0;
    int exp_number, pass, structureVersion = 1;
    double startTime, endTime;
    char* device = NULL;

    exp_number = request_block->exp_number;
    pass = request_block->pass;                // 'Pass' as a text string (tpass) is not used

// Keyword have higher priority

    int i;
    for (i = 0; i < request_block->nameValueList.pairCount; i++) {

        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "exp_number") ||
            !strcasecmp(request_block->nameValueList.nameValue[i].name, "shot") ||
            !strcasecmp(request_block->nameValueList.nameValue[i].name, "pulno")) {
            if (IsNumber(request_block->nameValueList.nameValue[i].value)) {
                isExp_number = 1;
                exp_number = atoi(request_block->nameValueList.nameValue[i].value);
                continue;
            } else {
                err = 888;
                break;
            }
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "pass")) {
            if (IsNumber(request_block->nameValueList.nameValue[i].value)) {
                isPass = 1;
                pass = atoi(request_block->nameValueList.nameValue[i].value);
                continue;
            } else {
                err = 888;
                break;
            }
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "device")) {
            isDevice = 1;
            device = request_block->nameValueList.nameValue[i].value;
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "startTime")) {
            isStartTime = 1;
            startTime = atof(request_block->nameValueList.nameValue[i].value);
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "endTime")) {
            isEndTime = 1;
            endTime = atof(request_block->nameValueList.nameValue[i].value);
            continue;
        }

        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "structureVersion")) {
            if (IsNumber(request_block->nameValueList.nameValue[i].value)) {
                isStructureVersion = 1;
                structureVersion = atoi(request_block->nameValueList.nameValue[i].value);
                continue;
            } else {
                err = 888;
                break;
            }
        }

// Keywords

        if (!strcasecmp(request_block->nameValueList.nameValue[i].name,
                        "average")) {        // Average data within the time window (startTime and endTime must be specified)
            isAverage = 1;
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name,
                        "nearest")) {        // The data measurement nearest startTime.  endTime is ignored.
            isNearest = 1;
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name,
                        "first")) {        // The first data measurement within the time window (startTime and endTime must be specified)
            isFirst = 1;
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name,
                        "last")) {        // The last data measurement within the time window (startTime and endTime must be specified)
            isLast = 1;
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name,
                        "cache")) {        // Cache the machine description data
            isCache = 1;
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name,
                        "count")) {        // Cache the Count of the coil or probe
            isCount = 1;
            continue;
        }
    }

    USERDEFINEDTYPELIST* userdefinedtypelist = idam_plugin_interface->userdefinedtypelist;

    //----------------------------------------------------------------------------------------
    // Functions

    err = 0;

    do {

//----------------------------------------------------------------------------------------
// Test Methods
//----------------------------------------------------------------------------------------

        if (!strcasecmp(request_block->function, "test1")) {    // CODE data structure

// Create the Returned Structure Definitions

            defineIDSStructures();

// Build the Returned Structures

            CODE* code = (CODE*)malloc(sizeof(CODE));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)code, 1, sizeof(CODE), "CODE");

            size_t stringLength = 56;
            code->name = (char*)malloc(stringLength * sizeof(char));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)code->name, 1, stringLength * sizeof(char),
                      "STRING");    // Scalar String
            strcpy(code->name, "IDAM LiveDisplay Plugin");

            stringLength = 56;
            code->version = (char*)malloc(stringLength * sizeof(char));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)code->version, 1, stringLength * sizeof(char),
                      "STRING");    // Scalar String
            sprintf(code->version, "LiveDisplay Plugin Version: %d", THISPLUGIN_VERSION);

            stringLength = strlen(request_block->signal) + strlen(request_block->source) + 7;
            code->parameters = (char*)malloc(stringLength * sizeof(char));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)code->parameters, 1, stringLength * sizeof(char),
                      "STRING");    // Scalar String
            sprintf(code->parameters, "\"%s\", \"%s\"", request_block->signal, request_block->source);

            code->output_flag_count = 3;

            code->output_flag = (int*)malloc(code->output_flag_count * sizeof(int));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)code->output_flag, code->output_flag_count,
                      sizeof(int), "int");    // Integer Array
            code->output_flag[0] = 12345;
            code->output_flag[1] = 67890;
            code->output_flag[2] = 99999;

// Pass Data

            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*)code;

            strcpy(data_block->data_desc, "Proxy for a Magnetics IDS CODE structure");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "CODE", 0);

            break;

        } else if (!strcasecmp(request_block->function, "test2")) {

// Create the Returned Structure Definitions

            defineIDSStructures();

            initUserDefinedType(&usertype);        // New structure definition

            strcpy(usertype.name, "MAGNETICS_TEST2");
            usertype.size = sizeof(MAGNETICS_TEST2);
            strcpy(usertype.source, "LiveDisplay");
            usertype.ref_id = 0;
            usertype.imagecount = 0;                 // No Structure Image data
            usertype.image = NULL;
            usertype.idamclass = TYPE_COMPOUND;

            int offset = 0;

            defineField(&field, "comment", "Comment", &offset, SCALARSTRING);
            addCompoundField(&usertype, field);
            defineField(&field, "homogeneous_time", "homogeneous_time", &offset, SCALARINT);
            addCompoundField(&usertype, field);
            defineField(&field, "flux_loop_count", "flux_loop array count", &offset, SCALARINT);
            addCompoundField(&usertype, field);
            defineField(&field, "bpol_probe_count", "bpol_probe array count", &offset, SCALARINT);
            addCompoundField(&usertype, field);
            defineField(&field, "method_count", "method array count", &offset, SCALARINT);
            addCompoundField(&usertype, field);

            initCompoundField(&field);
            strcpy(field.name, "code");
            field.atomictype = TYPE_UNKNOWN;
            strcpy(field.type, "CODE");
            strcpy(field.desc, "Code Information");
            field.pointer = 0;
            field.count = 1;
            field.rank = 0;
            field.shape = NULL;                    // Needed when rank >= 1
            field.size = field.count * sizeof(CODE);
            field.offset = newoffset(offset, field.type);
            field.offpad = padding(offset, field.type);
            field.alignment = getalignmentof(field.type);
            offset = field.offset + field.size;        // Next Offset
            addCompoundField(&usertype, field);        // Single Structure element

            addUserDefinedType(userdefinedtypelist, usertype);

// Build the Returned Structures

            CODE code;

            size_t stringLength = 56;
            code.name = (char*)malloc(stringLength * sizeof(char));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)code.name, 1, stringLength * sizeof(char),
                      "STRING");    // Scalar String
            strcpy(code.name, "IDAM LiveDisplay Plugin");

            stringLength = 56;
            code.version = (char*)malloc(stringLength * sizeof(char));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)code.version, 1, stringLength * sizeof(char),
                      "STRING");    // Scalar String
            sprintf(code.version, "LiveDisplay Plugin Version: %d", THISPLUGIN_VERSION);

            stringLength = strlen(request_block->signal) + strlen(request_block->source) + 7;
            code.parameters = (char*)malloc(stringLength * sizeof(char));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)code.parameters, 1, stringLength * sizeof(char),
                      "STRING");    // Scalar String
            sprintf(code.parameters, "\"%s\", \"%s\"", request_block->signal, request_block->source);

            code.output_flag_count = 3;
            code.output_flag = (int*)malloc(code.output_flag_count * sizeof(int));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)code.output_flag, code.output_flag_count,
                      sizeof(int), "int");    // Integer Array
            code.output_flag[0] = 12345;
            code.output_flag[1] = 67890;
            code.output_flag[2] = 99999;

            MAGNETICS_TEST2* magnetics = (MAGNETICS_TEST2*)malloc(sizeof(MAGNETICS_TEST2));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)magnetics, 1, sizeof(MAGNETICS_TEST2),
                      "MAGNETICS_TEST2");

            stringLength = 128;
            magnetics->comment = (char*)malloc(stringLength * sizeof(char));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)magnetics->comment, 1, stringLength * sizeof(char),
                      "STRING");    // Scalar String
            strcpy(magnetics->comment, "This is the LiveDisplay prototype plugin for Magnetics IDS data");

            magnetics->homogeneous_time = 1;
            magnetics->flux_loop_count = 2;
            magnetics->bpol_probe_count = 3;
            magnetics->method_count = 4;

            magnetics->code = code;

// Pass Data

            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*)magnetics;

            strcpy(data_block->data_desc, "Proxy for a Magnetics IDS");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "MAGNETICS_TEST2", 0);

            break;

        } else if (!strcasecmp(request_block->function, "test3")) {    // FLUX_LOOP data structure

// Create the Returned Structure Definitions

            defineIDSStructures();

// Build the Returned Structures

            FLUX_LOOP* floop = (FLUX_LOOP*)malloc(sizeof(FLUX_LOOP));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)floop, 1, sizeof(FLUX_LOOP), "FLUX_LOOP");

            size_t stringLength = 56;
            floop->identifier = (char*)malloc(stringLength * sizeof(char));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)floop->identifier, 1, stringLength * sizeof(char),
                      "STRING");    // Scalar String
            strcpy(floop->identifier, "IDAM LiveDisplay Plugin");

            stringLength = 56;
            floop->name = (char*)malloc(stringLength * sizeof(char));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)floop->name, 1, stringLength * sizeof(char),
                      "STRING");    // Scalar String
            strcpy(floop->name, "Flux Loop #1");

            floop->position_count = 1;

            floop->r = (double*)malloc(floop->position_count * sizeof(double));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)floop->r, floop->position_count, sizeof(double),
                      "double");
            floop->r[0] = 1.2345;
            floop->z = (double*)malloc(floop->position_count * sizeof(double));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)floop->z, floop->position_count, sizeof(double),
                      "double");
            floop->z[0] = 6.7890;
            floop->phi = (double*)malloc(floop->position_count * sizeof(double));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)floop->phi, floop->position_count, sizeof(double),
                      "double");
            floop->phi[0] = 9.9999;

            floop->data_count = 1;

            floop->data = (double*)malloc(floop->data_count * sizeof(double));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)floop->data, floop->data_count, sizeof(double),
                      "double");
            floop->data[0] = 3.1415927;

            floop->time = (double*)malloc(floop->data_count * sizeof(double));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)floop->time, floop->data_count, sizeof(double),
                      "double");
            floop->time[0] = 2.71828;


// Pass Data

            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*)floop;

            strcpy(data_block->data_desc, "Proxy for a Magnetics IDS CODE structure");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "FLUX_LOOP", 0);

            break;

        } else if (!strcasecmp(request_block->function, "test4")) {    // FLUX_LOOP data structure

// Create the Returned Structure Definitions

            defineIDSStructures();

// Build the Returned Structures

            int flux_loop_count = 2;

            FLUX_LOOP* floop = (FLUX_LOOP*)malloc(flux_loop_count * sizeof(FLUX_LOOP));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)floop, flux_loop_count, sizeof(FLUX_LOOP),
                      "FLUX_LOOP");

            for (i = 0; i < flux_loop_count; i++) {

                size_t stringLength = 56;
                floop[i].identifier = (char*)malloc(stringLength * sizeof(char));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].identifier, 1,
                          stringLength * sizeof(char), "STRING");    // Scalar String
                strcpy(floop[i].identifier, "IDAM LiveDisplay Plugin");

                stringLength = 56;
                floop[i].name = (char*)malloc(stringLength * sizeof(char));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].name, 1, stringLength * sizeof(char),
                          "STRING");
                strcpy(floop[i].name, "Flux Loop #1");

                floop[i].position_count = 1;

                floop[i].r = (double*)malloc(floop[i].position_count * sizeof(double));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].r, floop[i].position_count,
                          sizeof(double), "double");
                floop[i].r[0] = 1.2345;
                floop[i].z = (double*)malloc(floop[i].position_count * sizeof(double));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].z, floop[i].position_count,
                          sizeof(double), "double");
                floop[i].z[0] = 6.7890;
                floop[i].phi = (double*)malloc(floop[i].position_count * sizeof(double));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].phi, floop[i].position_count,
                          sizeof(double), "double");
                floop[i].phi[0] = 9.9999;

                floop[i].data_count = 1;

                floop[i].data = (double*)malloc(floop[i].data_count * sizeof(double));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].data, floop[i].data_count,
                          sizeof(double), "double");
                floop[i].data[0] = 3.1415927;

                floop[i].time = (double*)malloc(floop[i].data_count * sizeof(double));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].time, floop[i].data_count,
                          sizeof(double), "double");
                floop[i].time[0] = 2.71828;
            }

// Pass Data

            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*)floop;

            strcpy(data_block->data_desc, "Proxy for a Magnetics IDS CODE structure");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "FLUX_LOOP", 0);

            break;

        } else if (!strcasecmp(request_block->function, "test5")) {    // FLUX_LOOP data structure

            char signal[256], source[256];
            int stringLength;

            sprintf(source, "%s%s%d", request_block->device_name, request_block->api_delim, exp_number);

// Create the Returned Structure Definitions

            defineIDSStructures();

// Build the Returned Structures

            int flux_loop_count = 2;

            FLUX_LOOP* floop = (FLUX_LOOP*)malloc(flux_loop_count * sizeof(FLUX_LOOP));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)floop, flux_loop_count, sizeof(FLUX_LOOP),
                      "FLUX_LOOP");

            for (i = 0; i < flux_loop_count; i++) {

                initFluxLoop(&floop[i]);

                sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/NAME", i + 1);

                int handle = idamGetAPI(signal, source);

                if (handle < 0 || getIdamErrorCode(handle) > 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Unable to access Machine Description data!");
                    idamFree(handle);
                    break;
                }

                if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_STRING) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Machine Description data has Incorrect properties!");
                    idamFree(handle);
                    break;
                }

                char* name = (char*)getIdamData(handle);
                stringLength = strlen(name) + 1;
                floop[i].name = (char*)malloc(stringLength * sizeof(char));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].name, 1, stringLength * sizeof(char),
                          "STRING");    // Scalar String
                strcpy(floop[i].name, name);

                sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/IDENTIFIER", i + 1);

                handle = idamGetAPI(signal, source);

                if (handle < 0 || getIdamErrorCode(handle) > 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Unable to access Machine Description data!");
                    idamFree(handle);
                    break;
                }

                if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_STRING) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Machine Description data has Incorrect properties!");
                    idamFree(handle);
                    break;
                }

                char* identifier = (char*)getIdamData(handle);
                stringLength = strlen(identifier) + 1;
                floop[i].identifier = (char*)malloc(stringLength * sizeof(char));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].identifier, 1,
                          stringLength * sizeof(char), "STRING");    // Scalar String
                strcpy(floop[i].identifier, identifier);

                floop[i].position_count = 1;

                floop[i].r = (double*)malloc(floop[i].position_count * sizeof(double));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].r, floop[i].position_count,
                          sizeof(double), "double");
                floop[i].r[0] = 1.2345;
                floop[i].z = (double*)malloc(floop[i].position_count * sizeof(double));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].z, floop[i].position_count,
                          sizeof(double), "double");
                floop[i].z[0] = 6.7890;
                floop[i].phi = (double*)malloc(floop[i].position_count * sizeof(double));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].phi, floop[i].position_count,
                          sizeof(double), "double");
                floop[i].phi[0] = 9.9999;

                floop[i].data_count = 1;

                floop[i].data = (double*)malloc(floop[i].data_count * sizeof(double));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].data, floop[i].data_count,
                          sizeof(double), "double");
                floop[i].data[0] = 3.1415927;

                floop[i].time = (double*)malloc(floop[i].data_count * sizeof(double));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].time, floop[i].data_count,
                          sizeof(double), "double");
                floop[i].time[0] = 2.71828;
            }

// Pass Data

            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*)floop;

            strcpy(data_block->data_desc, "Proxy for a Magnetics IDS CODE structure");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "FLUX_LOOP", 0);

            break;

        } else if (!strcasecmp(request_block->function, "test6")) {    // FLUX_LOOP data structure with MAST data

            char signal[256], source[256];
            int stringLength;

// Create the Returned Structure Definitions

            defineIDSStructures();

// Access MAST machine description data: Flux Loops
// Use the IDAM client API with IMAS name abstraction

            sprintf(source, "MAST::%d", exp_number);

// 1. Number of Flux Loops
            sprintf(signal, "MAGNETICS/FLUX_LOOP/SHAPE_OF");

            int handle = idamGetAPI(signal, source);

            if (handle < 0 || getIdamErrorCode(handle) > 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                             "Unable to access Machine Description data!");
                idamFree(handle);
                break;
            }

            if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_INT || getIdamDataNum(handle) != 1) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                             "Machine Description data has Incorrect properties!");
                idamFree(handle);
                break;
            }

            int flux_loop_count = *((int*)getIdamData(handle));

            //flux_loop_count = 1;

// Build the Returned Structures

            FLUX_LOOP* floop = (FLUX_LOOP*)malloc(flux_loop_count * sizeof(FLUX_LOOP));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)floop, flux_loop_count, sizeof(FLUX_LOOP),
                      "FLUX_LOOP");


// 2. Loop over all Flux Loops

            for (i = 0; i < flux_loop_count; i++) {

                //initFluxLoop(floop);

                sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/NAME", i + 1);

                int handle = idamGetAPI(signal, source);

                if (handle < 0 || getIdamErrorCode(handle) > 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Unable to access Machine Description data!");
                    idamFree(handle);
                    break;
                }

                if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_STRING) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Machine Description data has Incorrect properties!");
                    idamFree(handle);
                    break;
                }

                char* name = (char*)getIdamData(handle);
                stringLength = strlen(name) + 1;
                floop[i].name = (char*)malloc(stringLength * sizeof(char));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].name, 1, stringLength * sizeof(char),
                          "STRING");    // Scalar String
                strcpy(floop[i].name, name);

                sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/IDENTIFIER", i + 1);

                handle = idamGetAPI(signal, source);

                if (handle < 0 || getIdamErrorCode(handle) > 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Unable to access Machine Description data!");
                    idamFree(handle);
                    break;
                }

                if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_STRING) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Machine Description data has Incorrect properties!");
                    idamFree(handle);
                    break;
                }

                char* identifier = (char*)getIdamData(handle);
                stringLength = strlen(identifier) + 1;
                floop[i].identifier = (char*)malloc(stringLength * sizeof(char));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].identifier, 1,
                          stringLength * sizeof(char), "STRING");    // Scalar String
                strcpy(floop[i].identifier, identifier);

// 3. Number of Coordinates

                sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/POSITION/SHAPE_OF", i + 1);

                handle = idamGetAPI(signal, source);

                if (handle < 0 || getIdamErrorCode(handle) > 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Unable to access Machine Description data!");
                    idamFree(handle);
                    break;
                }

                if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_INT || getIdamDataNum(handle) != 1) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Machine Description data has Incorrect properties!");
                    idamFree(handle);
                    break;
                }

                floop[i].position_count = *((int*)getIdamData(handle));

                floop[i].position_count = 1;

                floop[i].r = (double*)malloc(floop[i].position_count * sizeof(double));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].r, floop[i].position_count,
                          sizeof(double), "double");
                floop[i].z = (double*)malloc(floop[i].position_count * sizeof(double));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].z, floop[i].position_count,
                          sizeof(double), "double");
                floop[i].phi = (double*)malloc(floop[i].position_count * sizeof(double));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].phi, floop[i].position_count,
                          sizeof(double), "double");

// 4. Loop over coordinates

                int j;
                for (j = 0; j < floop[i].position_count; j++) {

                    sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/POSITION/%d/R", i + 1, j + 1);

                    handle = idamGetAPI(signal, source);

                    if (handle < 0 || getIdamErrorCode(handle) > 0) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Unable to access Machine Description data!");
                        idamFree(handle);
                        break;
                    }

                    if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_DOUBLE ||
                        getIdamDataNum(handle) != 1) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Machine Description data has Incorrect properties!");
                        idamFree(handle);
                        break;
                    }

                    floop[i].r[j] = *((double*)getIdamData(handle));

                }

                if (err != 0) break;

/*
MAGNETICS/FLUX_LOOP/1/NAME
MAGNETICS/FLUX_LOOP/1/IDENTIFIER
MAGNETICS/FLUX_LOOP/1/POSITION/SHAPE_OF
MAGNETICS/FLUX_LOOP/1/POSITION/1/R
MAGNETICS/FLUX_LOOP/1/POSITION/1/Z
MAGNETICS/FLUX_LOOP/1/POSITION/1/PHI
MAGNETICS/FLUX_LOOP/1/FLUX/DATA
MAGNETICS/FLUX_LOOP/1/FLUX/TIME
*/


                floop[i].z[0] = 6.7890;
                floop[i].phi[0] = 9.9999;

                floop[i].data_count = 1;

                floop[i].data = (double*)malloc(floop[i].data_count * sizeof(double));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].data, floop[i].data_count,
                          sizeof(double), "double");
                floop[i].data[0] = 3.1415927;

                floop[i].time = (double*)malloc(floop[i].data_count * sizeof(double));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].time, floop[i].data_count,
                          sizeof(double), "double");
                floop[i].time[0] = 2.71828;
            }

// Pass Data

            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*)floop;

            strcpy(data_block->data_desc, "Proxy for a Magnetics IDS CODE structure");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "FLUX_LOOP", 0);

            break;

        } else if (!strcasecmp(request_block->function, "test7")) {    // FLUX_LOOP data structure with MAST data

            char signal[256], source[256];
            int stringLength;

// Create the Returned Structure Definitions

            defineIDSStructures();

// Access MAST machine description data: Flux Loops
// Use the IDAM client API with IMAS name abstraction

            sprintf(source, "MAST::%d", exp_number);

// 1. Number of Flux Loops
            sprintf(signal, "MAGNETICS/FLUX_LOOP/SHAPE_OF");

            int handle = idamGetAPI(signal, source);

            if (handle < 0 || getIdamErrorCode(handle) > 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                             "Unable to access Machine Description data!");
                idamFree(handle);
                break;
            }

            if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_INT || getIdamDataNum(handle) != 1) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                             "Machine Description data has Incorrect properties!");
                idamFree(handle);
                break;
            }

            int flux_loop_count = *((int*)getIdamData(handle));

            //flux_loop_count = 1;

// Build the Returned Structures

            FLUX_LOOP* floop = (FLUX_LOOP*)malloc(flux_loop_count * sizeof(FLUX_LOOP));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)floop, flux_loop_count, sizeof(FLUX_LOOP),
                      "FLUX_LOOP");

// 2. Loop over all Flux Loops

            for (i = 0; i < flux_loop_count; i++) {

                //initFluxLoop(floop);

                sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/NAME", i + 1);

                int handle = idamGetAPI(signal, source);

                if (handle < 0 || getIdamErrorCode(handle) > 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Unable to access Machine Description data!");
                    idamFree(handle);
                    break;
                }

                if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_STRING) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Machine Description data has Incorrect properties!");
                    idamFree(handle);
                    break;
                }

                char* name = (char*)getIdamData(handle);
                stringLength = strlen(name) + 1;
                floop[i].name = (char*)malloc(stringLength * sizeof(char));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].name, 1, stringLength * sizeof(char),
                          "STRING");    // Scalar String
                strcpy(floop[i].name, name);

                sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/IDENTIFIER", i + 1);

                handle = idamGetAPI(signal, source);

                if (handle < 0 || getIdamErrorCode(handle) > 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Unable to access Machine Description data!");
                    idamFree(handle);
                    break;
                }

                if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_STRING) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Machine Description data has Incorrect properties!");
                    idamFree(handle);
                    break;
                }

                char* identifier = (char*)getIdamData(handle);
                stringLength = strlen(identifier) + 1;
                floop[i].identifier = (char*)malloc(stringLength * sizeof(char));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].identifier, 1,
                          stringLength * sizeof(char), "STRING");    // Scalar String
                strcpy(floop[i].identifier, identifier);

// 3. Number of Coordinates

                sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/POSITION/SHAPE_OF", i + 1);

                handle = idamGetAPI(signal, source);

                if (handle < 0 || getIdamErrorCode(handle) > 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Unable to access Machine Description data!");
                    idamFree(handle);
                    break;
                }

                if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_INT || getIdamDataNum(handle) != 1) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Machine Description data has Incorrect properties!");
                    idamFree(handle);
                    break;
                }

                floop[i].position_count = *((int*)getIdamData(handle));

                floop[i].position_count = 1;

                floop[i].r = (double*)malloc(floop[i].position_count * sizeof(double));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].r, floop[i].position_count,
                          sizeof(double), "double");
                floop[i].z = (double*)malloc(floop[i].position_count * sizeof(double));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].z, floop[i].position_count,
                          sizeof(double), "double");
                floop[i].phi = (double*)malloc(floop[i].position_count * sizeof(double));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].phi, floop[i].position_count,
                          sizeof(double), "double");

// 4. Loop over coordinates

                int j;
                for (j = 0; j < floop[i].position_count; j++) {

                    sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/POSITION/%d/R", i + 1, j + 1);

                    handle = idamGetAPI(signal, source);

                    if (handle < 0 || getIdamErrorCode(handle) > 0) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Unable to access Machine Description data!");
                        idamFree(handle);
                        break;
                    }

                    if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_DOUBLE ||
                        getIdamDataNum(handle) != 1) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Machine Description data has Incorrect properties!");
                        idamFree(handle);
                        break;
                    }

                    floop[i].r[j] = *((double*)getIdamData(handle));

                    sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/POSITION/%d/Z", i + 1, j + 1);

                    handle = idamGetAPI(signal, source);

                    if (handle < 0 || getIdamErrorCode(handle) > 0) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Unable to access Machine Description data!");
                        idamFree(handle);
                        break;
                    }

                    if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_DOUBLE ||
                        getIdamDataNum(handle) != 1) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Machine Description data has Incorrect properties!");
                        idamFree(handle);
                        break;
                    }

                    floop[i].z[j] = *((double*)getIdamData(handle));

                    sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/POSITION/%d/PHI", i + 1, j + 1);

                    handle = idamGetAPI(signal, source);

                    if (handle < 0 || getIdamErrorCode(handle) > 0) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Unable to access Machine Description data!");
                        idamFree(handle);
                        break;
                    }

                    if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_DOUBLE ||
                        getIdamDataNum(handle) != 1) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Machine Description data has Incorrect properties!");
                        idamFree(handle);
                        break;
                    }

                    floop[i].phi[j] = *((double*)getIdamData(handle));

                }

                if (err != 0) break;

/*
MAGNETICS/FLUX_LOOP/1/FLUX/DATA
MAGNETICS/FLUX_LOOP/1/FLUX/TIME
*/

                floop[i].data_count = 1;

                floop[i].data = (double*)malloc(floop[i].data_count * sizeof(double));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].data, floop[i].data_count,
                          sizeof(double), "double");
                floop[i].data[0] = 3.1415927;

                floop[i].time = (double*)malloc(floop[i].data_count * sizeof(double));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].time, floop[i].data_count,
                          sizeof(double), "double");
                floop[i].time[0] = 2.71828;
            }

// Pass Data

            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*)floop;

            strcpy(data_block->data_desc, "Proxy for a Magnetics IDS CODE structure");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "FLUX_LOOP", 0);

            break;

        } else if (!strcasecmp(request_block->function, "test8")) {    // FLUX_LOOP data structure with MAST data

            char signal[256], source[256];
            int stringLength;

// Create the Returned Structure Definitions

            defineIDSStructures();

// Access MAST machine description data: Flux Loops
// Use the IDAM client API with IMAS name abstraction

            sprintf(source, "MAST::%d", exp_number);

// Test the cache for Machine Description data - based on the SOURCE identifier
// Reuse if cached
// Create static arrays (with generous limits) rather than pointers for machine description data

//          FLUX_LOOP_CACHE flux_loop_cache;

// 1. Number of Flux Loops
            sprintf(signal, "MAGNETICS/FLUX_LOOP/SHAPE_OF");

            int handle = idamGetAPI(signal, source);

            if (handle < 0 || getIdamErrorCode(handle) > 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                             "Unable to access Machine Description data!");
                idamFree(handle);
                break;
            }

            if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_INT || getIdamDataNum(handle) != 1) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                             "Machine Description data has Incorrect properties!");
                idamFree(handle);
                break;
            }

            int flux_loop_count = *((int*)getIdamData(handle));

            //flux_loop_count = 1;

// Build the Returned Structures

            FLUX_LOOP* floop = (FLUX_LOOP*)malloc(flux_loop_count * sizeof(FLUX_LOOP));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)floop, flux_loop_count, sizeof(FLUX_LOOP),
                      "FLUX_LOOP");

// 2. Loop over all Flux Loops

            for (i = 0; i < flux_loop_count; i++) {

                //initFluxLoop(floop);

                sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/NAME", i + 1);

                int handle = idamGetAPI(signal, source);

                if (handle < 0 || getIdamErrorCode(handle) > 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Unable to access Machine Description data!");
                    idamFree(handle);
                    break;
                }

                if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_STRING) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Machine Description data has Incorrect properties!");
                    idamFree(handle);
                    break;
                }

                char* name = (char*)getIdamData(handle);
                stringLength = strlen(name) + 1;
                floop[i].name = (char*)malloc(stringLength * sizeof(char));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].name, 1, stringLength * sizeof(char),
                          "STRING");    // Scalar String
                strcpy(floop[i].name, name);

                sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/IDENTIFIER", i + 1);

                handle = idamGetAPI(signal, source);

                if (handle < 0 || getIdamErrorCode(handle) > 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Unable to access Machine Description data!");
                    idamFree(handle);
                    break;
                }

                if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_STRING) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Machine Description data has Incorrect properties!");
                    idamFree(handle);
                    break;
                }

                char* identifier = (char*)getIdamData(handle);
                stringLength = strlen(identifier) + 1;
                floop[i].identifier = (char*)malloc(stringLength * sizeof(char));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].identifier, 1,
                          stringLength * sizeof(char), "STRING");    // Scalar String
                strcpy(floop[i].identifier, identifier);

// 3. Number of Coordinates

                sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/POSITION/SHAPE_OF", i + 1);

                handle = idamGetAPI(signal, source);

                if (handle < 0 || getIdamErrorCode(handle) > 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Unable to access Machine Description data!");
                    idamFree(handle);
                    break;
                }

                if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_INT || getIdamDataNum(handle) != 1) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Machine Description data has Incorrect properties!");
                    idamFree(handle);
                    break;
                }

                floop[i].position_count = *((int*)getIdamData(handle));

                floop[i].position_count = 1;

                floop[i].r = (double*)malloc(floop[i].position_count * sizeof(double));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].r, floop[i].position_count,
                          sizeof(double), "double");
                floop[i].z = (double*)malloc(floop[i].position_count * sizeof(double));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].z, floop[i].position_count,
                          sizeof(double), "double");
                floop[i].phi = (double*)malloc(floop[i].position_count * sizeof(double));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].phi, floop[i].position_count,
                          sizeof(double), "double");

// 4. Loop over coordinates

                int j;
                for (j = 0; j < floop[i].position_count; j++) {

                    sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/POSITION/%d/R", i + 1, j + 1);

                    handle = idamGetAPI(signal, source);

                    if (handle < 0 || getIdamErrorCode(handle) > 0) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Unable to access Machine Description data!");
                        idamFree(handle);
                        break;
                    }

                    if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_DOUBLE ||
                        getIdamDataNum(handle) != 1) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Machine Description data has Incorrect properties!");
                        idamFree(handle);
                        break;
                    }

                    floop[i].r[j] = *((double*)getIdamData(handle));

                    sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/POSITION/%d/Z", i + 1, j + 1);

                    handle = idamGetAPI(signal, source);

                    if (handle < 0 || getIdamErrorCode(handle) > 0) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Unable to access Machine Description data!");
                        idamFree(handle);
                        break;
                    }

                    if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_DOUBLE ||
                        getIdamDataNum(handle) != 1) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Machine Description data has Incorrect properties!");
                        idamFree(handle);
                        break;
                    }

                    floop[i].z[j] = *((double*)getIdamData(handle));

                    sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/POSITION/%d/PHI", i + 1, j + 1);

                    handle = idamGetAPI(signal, source);

                    if (handle < 0 || getIdamErrorCode(handle) > 0) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Unable to access Machine Description data!");
                        idamFree(handle);
                        break;
                    }

                    if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_DOUBLE ||
                        getIdamDataNum(handle) != 1) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Machine Description data has Incorrect properties!");
                        idamFree(handle);
                        break;
                    }

                    floop[i].phi[j] = *((double*)getIdamData(handle));

                }

                if (err != 0) break;

// 5. Access measurement data and apply subsetting

                floop[i].data_count = 0;
                floop[i].data = NULL;
                floop[i].time = NULL;

                sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/FLUX/DATA", i + 1);
                handle = idamGetAPI(signal, source);

                if (handle < 0 || getIdamErrorCode(handle) >
                                  0) {        // Unable to access Machine Measurement data - set count to zero! Ignore Error
                    idamFree(handle);
                    continue;
                }

                sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/FLUX/TIME", i + 1);
                int handle2 = idamGetAPI(signal, source);

                if (handle2 < 0 || getIdamErrorCode(handle2) >
                                   0) {    // Unable to access Machine Measurement data - set count to zero! Ignore Error
                    idamFree(handle);
                    idamFree(handle2);
                    continue;
                }

                if (getIdamRank(handle2) != 0 || getIdamDataType(handle2) != TYPE_DOUBLE ||
                    getIdamDataNum(handle) != getIdamDataNum(handle2)) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Machine Measurement data has Incorrect properties!");
                    idamFree(handle);
                    idamFree(handle2);
                    break;
                }

                int data_count = getIdamDataNum(handle);

                double* fd = (double*)getIdamData(handle);
                double* ft = (double*)getIdamData(handle2);

                if (isStartTime) {
                    double sum1 = 0.0, sum2 = 0.0;
                    int index1 = -1;
                    int index2 = -1;
                    for (j = 0; j < data_count; j++) {
                        if (index1 < 0 && ft[j] >= startTime) {
                            index1 = j;
                            if (!isEndTime) break;
                        }
                        if (index1 >= 0 && index2 < 0 && ft[j] >= endTime) {
                            index2 = j - 1;
                            if (index2 < 0) index2 = 0;
                            if (ft[j] == endTime) index2 = j;
                            break;
                        }
                    }
                    if (index1 == -1) {
                        // ERROR
                    }

                    if (isEndTime) {    // Subset of the data
                        if (index2 == -1) index2 = data_count - 1;
                        data_count = index2 - index1 + 1;
                    } else {        // Single value
                        data_count = 1;
                        index2 = index1;
                    }

                    if (isFirst) {
                        data_count = 1;
                        index2 = index1;
                    } else if (isLast) {
                        data_count = 1;
                        index1 = index2;
                    } else if (isNearest) {
                        data_count = 1;
                        if (index1 > 0 && ((startTime - ft[index1 - 1]) > (ft[index1] - startTime))) {
                            index1 = index1 - 1;
                        }
                    } else if (isAverage) {
                        for (j = 0; j < data_count; j++) {
                            sum1 += fd[index1 + j];
                            sum2 += ft[index1 + j];
                        }
                        sum1 = sum1 / (double)data_count;
                        sum2 = sum2 / (double)data_count;
                        data_count = 1;
                    }

                    floop[i].data_count = data_count;
                    floop[i].data = (double*)malloc(floop[i].data_count * sizeof(double));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].data, floop[i].data_count,
                              sizeof(double), "double");
                    floop[i].time = (double*)malloc(floop[i].data_count * sizeof(double));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].time, floop[i].data_count,
                              sizeof(double), "double");

                    if (isAverage) {
                        floop[i].data[0] = sum1;
                        floop[i].time[0] = sum2;
                    } else {
                        for (j = 0; j < floop[i].data_count; j++) {
                            floop[i].data[j] = fd[index1 + j];
                            floop[i].time[j] = ft[index1 + j];
                        }
                    }
                } else {        // All data are returned

                    floop[i].data_count = data_count;

// Reuse the allocated data blocks - no need to copy!

                    floop[i].data = (double*)getIdamData(handle);
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].data, floop[i].data_count,
                              sizeof(double), "double");
                    floop[i].time = (double*)getIdamData(handle2);
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].time, floop[i].data_count,
                              sizeof(double), "double");
                    DATA_BLOCK* db = getIdamDataBlock(handle);
                    db->data = NULL;        // Prevent double free
                    db = getIdamDataBlock(handle2);
                    db->data = NULL;
                }

                idamFree(handle);        // Do not cache
                idamFree(handle2);

            }

            if (err != 0) break;

// 6. Cache the machine description data


// 7. Return the Data


            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*)floop;

            strcpy(data_block->data_desc, "Proxy for a Magnetics IDS CODE structure");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "FLUX_LOOP", 0);

            break;

        } else if (!strcasecmp(request_block->function, "test9")) {    // FLUX_LOOP data structure with MAST data

            char signal[256], source[256];
            int stringLength;

            FLUX_LOOP* floop = NULL;
            int flux_loop_count = 0;
            int cacheData = 0, cacheRetrieve = 0;


// Create the Returned Structure Definitions

            defineIDSStructures();

// Access MAST machine description data: Flux Loops
// Use the IDAM client API with IMAS name abstraction

            sprintf(source, "%s%s%d", request_block->device_name, request_block->api_delim, exp_number);

// Test the cache for Machine Description data - based on the SOURCE identifier
// Reuse if cached
// Create static arrays (with generous limits) rather than pointers for machine description data

            if (isCache && strcmp(source, flux_loop_source) != 0) {    // clear the cache - new data requested
                if (flux_loop_cache_count > 0 && flux_loop_cache != NULL) {
                    for (i = 0; i < flux_loop_cache_count; i++) freeFluxLoop(&flux_loop_cache[i]);
                    free((void*)flux_loop_cache);
                    flux_loop_cache = NULL;
                    flux_loop_cache_count = 0;
                }
                strcpy(flux_loop_source, source);
            }

            if (isCache && flux_loop_cache_count == 0) cacheData = 1;        // Cache the machine description data
            if (isCache && flux_loop_cache_count > 0) {
                cacheRetrieve = 1;
            }    // Retrieve the machine description data from the Cache


// Read Machine Description Data

            if (!isCache || cacheData) {

// 1. Number of Flux Loops

                sprintf(signal, "MAGNETICS/FLUX_LOOP/SHAPE_OF");

                int handle = idamGetAPI(signal, source);

                if (handle < 0 || getIdamErrorCode(handle) > 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Unable to access Machine Description data!");
                    idamFree(handle);
                    break;
                }

                if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_INT || getIdamDataNum(handle) != 1) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Machine Description data has Incorrect properties!");
                    idamFree(handle);
                    break;
                }

                flux_loop_count = *((int*)getIdamData(handle));

// Return if only the count is requested

                if (isCount) return (returnCount(flux_loop_count, data_block));

// Build the Returned Structures

                floop = (FLUX_LOOP*)malloc(flux_loop_count * sizeof(FLUX_LOOP));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop, flux_loop_count, sizeof(FLUX_LOOP),
                          "FLUX_LOOP");

// 2. Loop over all Flux Loops

                for (i = 0; i < flux_loop_count; i++) {

                    initFluxLoop(&floop[i]);

                    sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/NAME", i + 1);

                    handle = idamGetAPI(signal, source);

                    if (handle < 0 || getIdamErrorCode(handle) > 0) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Unable to access Machine Description data!");
                        idamFree(handle);
                        break;
                    }

                    if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_STRING) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Machine Description data has Incorrect properties!");
                        idamFree(handle);
                        break;
                    }

                    char* name = (char*)getIdamData(handle);
                    stringLength = strlen(name) + 1;
                    floop[i].name = (char*)malloc(stringLength * sizeof(char));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].name, 1,
                              stringLength * sizeof(char), "STRING");    // Scalar String
                    strcpy(floop[i].name, name);

                    sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/IDENTIFIER", i + 1);

                    handle = idamGetAPI(signal, source);

                    if (handle < 0 || getIdamErrorCode(handle) > 0) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Unable to access Machine Description data!");
                        idamFree(handle);
                        break;
                    }

                    if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_STRING) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Machine Description data has Incorrect properties!");
                        idamFree(handle);
                        break;
                    }

                    char* identifier = (char*)getIdamData(handle);
                    stringLength = strlen(identifier) + 1;
                    floop[i].identifier = (char*)malloc(stringLength * sizeof(char));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].identifier, 1,
                              stringLength * sizeof(char), "STRING");    // Scalar String
                    strcpy(floop[i].identifier, identifier);

// 3. Number of Coordinates

                    sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/POSITION/SHAPE_OF", i + 1);

                    handle = idamGetAPI(signal, source);

                    if (handle < 0 || getIdamErrorCode(handle) > 0) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Unable to access Machine Description data!");
                        idamFree(handle);
                        break;
                    }

                    if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_INT ||
                        getIdamDataNum(handle) != 1) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Machine Description data has Incorrect properties!");
                        idamFree(handle);
                        break;
                    }

                    floop[i].position_count = *((int*)getIdamData(handle));

                    floop[i].r = (double*)malloc(floop[i].position_count * sizeof(double));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].r, floop[i].position_count,
                              sizeof(double), "double");
                    floop[i].z = (double*)malloc(floop[i].position_count * sizeof(double));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].z, floop[i].position_count,
                              sizeof(double), "double");
                    floop[i].phi = (double*)malloc(floop[i].position_count * sizeof(double));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].phi, floop[i].position_count,
                              sizeof(double), "double");

// 4. Loop over coordinates

                    int j;
                    for (j = 0; j < floop[i].position_count; j++) {

                        sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/POSITION/%d/R", i + 1, j + 1);

                        handle = idamGetAPI(signal, source);

                        if (handle < 0 || getIdamErrorCode(handle) > 0) {
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                         "Unable to access Machine Description data!");
                            idamFree(handle);
                            break;
                        }

                        if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_DOUBLE ||
                            getIdamDataNum(handle) != 1) {
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                         "Machine Description data has Incorrect properties!");
                            idamFree(handle);
                            break;
                        }

                        floop[i].r[j] = *((double*)getIdamData(handle));

                        sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/POSITION/%d/Z", i + 1, j + 1);

                        handle = idamGetAPI(signal, source);

                        if (handle < 0 || getIdamErrorCode(handle) > 0) {
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                         "Unable to access Machine Description data!");
                            idamFree(handle);
                            break;
                        }

                        if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_DOUBLE ||
                            getIdamDataNum(handle) != 1) {
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                         "Machine Description data has Incorrect properties!");
                            idamFree(handle);
                            break;
                        }

                        floop[i].z[j] = *((double*)getIdamData(handle));

                        sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/POSITION/%d/PHI", i + 1, j + 1);

                        handle = idamGetAPI(signal, source);

                        if (handle < 0 || getIdamErrorCode(handle) > 0) {
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                         "Unable to access Machine Description data!");
                            idamFree(handle);
                            break;
                        }

                        if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_DOUBLE ||
                            getIdamDataNum(handle) != 1) {
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                         "Machine Description data has Incorrect properties!");
                            idamFree(handle);
                            break;
                        }

                        floop[i].phi[j] = *((double*)getIdamData(handle));

                    } // Loop over coordinates

                    if (err != 0) break;

                } // Loop over all Flux Loops
            } // Read Machine Description Data

// Save to cache after data access

            if (cacheData) {

                flux_loop_cache_count = flux_loop_count;
                flux_loop_cache = (FLUX_LOOP*)malloc(flux_loop_count * sizeof(FLUX_LOOP));

// 2. Loop over all Flux Loops

                for (i = 0; i < flux_loop_count; i++) {

                    initFluxLoop(&flux_loop_cache[i]);

                    stringLength = strlen(floop[i].name) + 1;
                    flux_loop_cache[i].name = (char*)malloc(stringLength * sizeof(char));
                    strcpy(flux_loop_cache[i].name, floop[i].name);

                    stringLength = strlen(floop[i].identifier) + 1;
                    flux_loop_cache[i].identifier = (char*)malloc(stringLength * sizeof(char));
                    strcpy(flux_loop_cache[i].identifier, floop[i].identifier);

// 3. Number of Coordinates

                    flux_loop_cache[i].position_count = floop[i].position_count;

                    flux_loop_cache[i].r = (double*)malloc(floop[i].position_count * sizeof(double));
                    flux_loop_cache[i].z = (double*)malloc(floop[i].position_count * sizeof(double));
                    flux_loop_cache[i].phi = (double*)malloc(floop[i].position_count * sizeof(double));

// 4. Loop over coordinates

                    int j;
                    for (j = 0; j < floop[i].position_count; j++) {
                        flux_loop_cache[i].r[j] = floop[i].r[j];
                        flux_loop_cache[i].z[j] = floop[i].z[j];
                        flux_loop_cache[i].phi[j] = floop[i].phi[j];
                    }
                } // Loop over all Flux Loops

            } else // cacheData

            if (cacheRetrieve) {        // Retrieve Machine Description Data from Cache

// 1. Number of Flux Loops

                flux_loop_count = flux_loop_cache_count;

// Return if only the count is requested

                if (isCount) return (returnCount(flux_loop_count, data_block));

// Build the Returned Structures

                floop = (FLUX_LOOP*)malloc(flux_loop_count * sizeof(FLUX_LOOP));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)floop, flux_loop_count, sizeof(FLUX_LOOP),
                          "FLUX_LOOP");

// 2. Loop over all Flux Loops

                for (i = 0; i < flux_loop_count; i++) {

                    initFluxLoop(&floop[i]);

                    stringLength = strlen(flux_loop_cache[i].name) + 1;
                    floop[i].name = (char*)malloc(stringLength * sizeof(char));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].name, 1,
                              stringLength * sizeof(char), "STRING");    // Scalar String
                    strcpy(floop[i].name, flux_loop_cache[i].name);

                    stringLength = strlen(flux_loop_cache[i].identifier) + 1;
                    floop[i].identifier = (char*)malloc(stringLength * sizeof(char));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].identifier, 1,
                              stringLength * sizeof(char), "STRING");    // Scalar String
                    strcpy(floop[i].identifier, flux_loop_cache[i].identifier);

// 3. Number of Coordinates

                    floop[i].position_count = flux_loop_cache[i].position_count;

                    floop[i].r = (double*)malloc(floop[i].position_count * sizeof(double));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].r, floop[i].position_count,
                              sizeof(double), "double");
                    floop[i].z = (double*)malloc(floop[i].position_count * sizeof(double));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].z, floop[i].position_count,
                              sizeof(double), "double");
                    floop[i].phi = (double*)malloc(floop[i].position_count * sizeof(double));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].phi, floop[i].position_count,
                              sizeof(double), "double");

// 4. Loop over coordinates

                    int j;
                    for (j = 0; j < floop[i].position_count; j++) {
                        floop[i].r[j] = flux_loop_cache[i].r[j];
                        floop[i].z[j] = flux_loop_cache[i].z[j];
                        floop[i].phi[j] = flux_loop_cache[i].phi[j];
                    }
                }    // Loop over all Flux Loops

            }    // end of cache retrieval



// 5. Access measurement data and apply subsetting

            for (i = 0; i < flux_loop_count; i++) {

                floop[i].data_count = 0;
                floop[i].data = NULL;
                floop[i].time = NULL;

                sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/FLUX/DATA", i + 1);
                int handle = idamGetAPI(signal, source);

                if (handle < 0 || getIdamErrorCode(handle) >
                                  0) {        // Unable to access Machine Measurement data - set count to zero! Ignore Error
                    idamFree(handle);
                    continue;
                }

                sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/FLUX/TIME", i + 1);
                int handle2 = idamGetAPI(signal, source);

                if (handle2 < 0 || getIdamErrorCode(handle2) >
                                   0) {    // Unable to access Machine Measurement data - set count to zero! Ignore Error
                    idamFree(handle);
                    idamFree(handle2);
                    continue;
                }

                if (getIdamRank(handle2) != 0 || getIdamDataType(handle2) != TYPE_DOUBLE ||
                    getIdamDataNum(handle) != getIdamDataNum(handle2)) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Machine Measurement data has Incorrect properties!");
                    idamFree(handle);
                    idamFree(handle2);
                    break;
                }

                int data_count = getIdamDataNum(handle);

                double* fd = (double*)getIdamData(handle);
                double* ft = (double*)getIdamData(handle2);

                if (isStartTime) {
                    double sum1 = 0.0, sum2 = 0.0;
                    int index1 = -1;
                    int index2 = -1;
                    int j;
                    for (j = 0; j < data_count; j++) {
                        if (index1 < 0 && ft[j] >= startTime) {
                            index1 = j;
                            if (!isEndTime) break;
                        }
                        if (index1 >= 0 && index2 < 0 && ft[j] >= endTime) {
                            index2 = j - 1;
                            if (index2 < 0) index2 = 0;
                            if (ft[j] == endTime) index2 = j;
                            break;
                        }
                    }
                    if (index1 == -1) {
                        // ERROR
                    }

                    if (isEndTime) {    // Subset of the data
                        if (index2 == -1) index2 = data_count - 1;
                        data_count = index2 - index1 + 1;
                    } else {        // Single value
                        data_count = 1;
                        index2 = index1;
                    }

                    if (isFirst) {
                        data_count = 1;
                        index2 = index1;
                    } else if (isLast) {
                        data_count = 1;
                        index1 = index2;
                    } else if (isNearest) {
                        data_count = 1;
                        if (index1 > 0 && ((startTime - ft[index1 - 1]) > (ft[index1] - startTime))) {
                            index1 = index1 - 1;
                        }
                    } else if (isAverage) {
                        for (j = 0; j < data_count; j++) {
                            sum1 += fd[index1 + j];
                            sum2 += ft[index1 + j];
                        }
                        sum1 = sum1 / (double)data_count;
                        sum2 = sum2 / (double)data_count;
                        data_count = 1;
                    }

                    floop[i].data_count = data_count;
                    floop[i].data = (double*)malloc(floop[i].data_count * sizeof(double));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].data, floop[i].data_count,
                              sizeof(double), "double");
                    floop[i].time = (double*)malloc(floop[i].data_count * sizeof(double));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].time, floop[i].data_count,
                              sizeof(double), "double");

                    if (isAverage) {
                        floop[i].data[0] = sum1;
                        floop[i].time[0] = sum2;
                    } else {
                        for (j = 0; j < floop[i].data_count; j++) {
                            floop[i].data[j] = fd[index1 + j];
                            floop[i].time[j] = ft[index1 + j];
                        }
                    }
                } else {        // All data are returned

                    floop[i].data_count = data_count;

// Reuse the allocated data blocks - no need to copy!

                    floop[i].data = (double*)getIdamData(handle);
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].data, floop[i].data_count,
                              sizeof(double), "double");
                    floop[i].time = (double*)getIdamData(handle2);
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].time, floop[i].data_count,
                              sizeof(double), "double");
                    DATA_BLOCK* db = getIdamDataBlock(handle);
                    db->data = NULL;        // Prevent double free
                    db = getIdamDataBlock(handle2);
                    db->data = NULL;
                }

                idamFree(handle);        // Do not cache
                idamFree(handle2);

            } // Access measurement data and apply subsetting

            if (err != 0) break;

// 6. Cache the machine description data


// 7. Return the Data


            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*)floop;

            strcpy(data_block->data_desc, "Proxy for a Magnetics IDS CODE structure");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "FLUX_LOOP", 0);

            break;

        } else if (!strcasecmp(request_block->function, "test9x")) {    // FLUX_LOOP Measurement Data Only
            char signal[256], source[256];
            int cacheData = 0, cacheRetrieve = 0;

            FLUX_LOOP_TEST1* floop = NULL;
            int flux_loop_count = 0;

// Create the Returned Structure Definitions

            defineIDSStructures();

// Access MAST machine description data: Flux Loops
// Use the IDAM client API with IMAS name abstraction

            sprintf(source, "%s%s%d", request_block->device_name, request_block->api_delim, exp_number);

// Read Machine Description Data

// Test the cache for Machine Description data - based on the SOURCE identifier
// Reuse if cached

            if (isCache && strcmp(source, flux_loop_source) != 0) {    // clear the cache - new data requested
                if (flux_loop_cache_count > 0 && flux_loop_cache != NULL) {
                    for (i = 0; i < flux_loop_cache_count; i++) freeFluxLoop(&flux_loop_cache[i]);
                    free((void*)flux_loop_cache);
                    flux_loop_cache = NULL;
                    flux_loop_cache_count = 0;
                }
                strcpy(flux_loop_source, source);
            }

            if (isCache && flux_loop_cache_count == 0) cacheData = 1;        // Cache the machine description data
            if (isCache && flux_loop_cache_count > 0) {
                cacheRetrieve = 1;
            }    // Retrieve the machine description data from the Cache

// Read Machine Description Data

            if (!isCache || cacheData) {

// 1. Number of Flux Loops

                sprintf(signal, "MAGNETICS/FLUX_LOOP/SHAPE_OF");

                int handle = idamGetAPI(signal, source);

                if (handle < 0 || getIdamErrorCode(handle) > 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Unable to access Machine Description data!");
                    idamFree(handle);
                    break;
                }

                if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_INT || getIdamDataNum(handle) != 1) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Machine Description data has Incorrect properties!");
                    idamFree(handle);
                    break;
                }

                flux_loop_count = *((int*)getIdamData(handle));

            } // Read Machine Description Data

            if (cacheData) {

                flux_loop_cache_count = flux_loop_count;

            } else if (cacheRetrieve) {        // Retrieve Machine Description Data from Cache

// 1. Number of Flux Loops

                flux_loop_count = flux_loop_cache_count;

            }    // end of cache retrieval



// Build the Returned Structures

            floop = (FLUX_LOOP_TEST1*)malloc(flux_loop_count * sizeof(FLUX_LOOP_TEST1));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)floop, flux_loop_count, sizeof(FLUX_LOOP_TEST1),
                      "FLUX_LOOP_TEST1");

// 5. Access measurement data and apply subsetting

            for (i = 0; i < flux_loop_count; i++) {

                floop[i].data_count = 0;
                floop[i].data = NULL;
                floop[i].time = NULL;

                sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/FLUX/DATA", i + 1);
                int handle = idamGetAPI(signal, source);

                if (handle < 0 || getIdamErrorCode(handle) >
                                  0) {        // Unable to access Machine Measurement data - set count to zero! Ignore Error
                    idamFree(handle);
                    continue;
                }

                sprintf(signal, "MAGNETICS/FLUX_LOOP/%d/FLUX/TIME", i + 1);
                int handle2 = idamGetAPI(signal, source);

                if (handle2 < 0 || getIdamErrorCode(handle2) >
                                   0) {    // Unable to access Machine Measurement data - set count to zero! Ignore Error
                    idamFree(handle);
                    idamFree(handle2);
                    continue;
                }

                if (getIdamRank(handle2) != 0 || getIdamDataType(handle2) != TYPE_DOUBLE ||
                    getIdamDataNum(handle) != getIdamDataNum(handle2)) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Machine Measurement data has Incorrect properties!");
                    idamFree(handle);
                    idamFree(handle2);
                    break;
                }

                int data_count = getIdamDataNum(handle);

                double* fd = (double*)getIdamData(handle);
                double* ft = (double*)getIdamData(handle2);

                if (isStartTime) {
                    double sum1 = 0.0, sum2 = 0.0;
                    int index1 = -1;
                    int index2 = -1;

                    int j;
                    for (j = 0; j < data_count; j++) {
                        if (index1 < 0 && ft[j] >= startTime) {
                            index1 = j;
                            if (!isEndTime) break;
                        }
                        if (index1 >= 0 && index2 < 0 && ft[j] >= endTime) {
                            index2 = j - 1;
                            if (index2 < 0) index2 = 0;
                            if (ft[j] == endTime) index2 = j;
                            break;
                        }
                    }
                    if (index1 == -1) {
                        // ERROR
                    }

                    if (isEndTime) {    // Subset of the data
                        if (index2 == -1) index2 = data_count - 1;
                        data_count = index2 - index1 + 1;
                    } else {        // Single value
                        data_count = 1;
                        index2 = index1;
                    }

                    if (isFirst) {
                        data_count = 1;
                        index2 = index1;
                    } else if (isLast) {
                        data_count = 1;
                        index1 = index2;
                    } else if (isNearest) {
                        data_count = 1;
                        if (index1 > 0 && ((startTime - ft[index1 - 1]) > (ft[index1] - startTime))) {
                            index1 = index1 - 1;
                        }
                    } else if (isAverage) {
                        for (j = 0; j < data_count; j++) {
                            sum1 += fd[index1 + j];
                            sum2 += ft[index1 + j];
                        }
                        sum1 = sum1 / (double)data_count;
                        sum2 = sum2 / (double)data_count;
                        data_count = 1;
                    }
                    floop[i].data_count = data_count;
                    floop[i].data = (double*)malloc(floop[i].data_count * sizeof(double));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].data, floop[i].data_count,
                              sizeof(double), "double");
                    floop[i].time = (double*)malloc(floop[i].data_count * sizeof(double));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].time, floop[i].data_count,
                              sizeof(double), "double");

                    if (isAverage) {
                        floop[i].data[0] = sum1;
                        floop[i].time[0] = sum2;
                    } else {
                        for (j = 0; j < floop[i].data_count; j++) {
                            floop[i].data[j] = fd[index1 + j];
                            floop[i].time[j] = ft[index1 + j];
                        }
                    }
                } else {        // All data are returned

                    floop[i].data_count = data_count;

// Reuse the allocated data blocks - no need to copy!

                    floop[i].data = (double*)getIdamData(handle);
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].data, floop[i].data_count,
                              sizeof(double), "double");
                    floop[i].time = (double*)getIdamData(handle2);
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)floop[i].time, floop[i].data_count,
                              sizeof(double), "double");
                    DATA_BLOCK* db = getIdamDataBlock(handle);
                    db->data = NULL;        // Prevent double free
                    db = getIdamDataBlock(handle2);
                    db->data = NULL;
                }

                idamFree(handle);        // Do not cache
                idamFree(handle2);

            } // Access measurement data and apply subsetting

            if (err != 0) break;

// 7. Return the Data

            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*)floop;

            strcpy(data_block->data_desc, "Proxy for a Magnetics IDS CODE structure");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "FLUX_LOOP_TEST1", 0);

            break;

        } else if (!strcasecmp(request_block->function, "test10")) {    // BPOL_PROBE data structure with MAST data

            char signal[256], source[256];
            int stringLength;

            BPOL_PROBE* bprobe = NULL;
            int bpol_probe_count = 0;
            int cacheData = 0, cacheRetrieve = 0;

// Create the Returned Structure Definitions

            defineIDSStructures();

// Access MAST machine description data
// Use the IDAM client API with IMAS name abstraction

            sprintf(source, "%s%s%d", request_block->device_name, request_block->api_delim, exp_number);

// Test the cache for Machine Description data - based on the SOURCE identifier
// Reuse if cached
// Create static arrays (with generous limits) rather than pointers for machine description data

            if (isCache && strcmp(source, bpol_probe_source) != 0) {    // clear the cache - new data requested
                if (bpol_probe_cache_count > 0 && bpol_probe_cache != NULL) {
                    for (i = 0; i < bpol_probe_cache_count; i++) freeBpolProbe(&bpol_probe_cache[i]);
                    free((void*)bpol_probe_cache);
                    bpol_probe_cache = NULL;
                    bpol_probe_cache_count = 0;
                }
                strcpy(bpol_probe_source, source);
            }

            if (isCache && bpol_probe_cache_count == 0) cacheData = 1;        // Cache the machine description data
            if (isCache && bpol_probe_cache_count > 0) {
                cacheRetrieve = 1;
            }    // Retrieve the machine description data from the Cache

// Read Machine Description Data

            if (!isCache || cacheData) {

// 1. Number of Magnetic Probes

                sprintf(signal, "MAGNETICS/BPOL_PROBE/SHAPE_OF");

                int handle = idamGetAPI(signal, source);

                if (handle < 0 || getIdamErrorCode(handle) > 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Unable to access Machine Description data!");
                    idamFree(handle);
                    break;
                }

                if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_INT || getIdamDataNum(handle) != 1) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Machine Description data has Incorrect properties!");
                    idamFree(handle);
                    break;
                }

                bpol_probe_count = *((int*)getIdamData(handle));

//bpol_probe_count = 1;

// Return if only the count is requested

                if (isCount) return (returnCount(bpol_probe_count, data_block));

// Build the Returned Structures

                bprobe = (BPOL_PROBE*)malloc(bpol_probe_count * sizeof(BPOL_PROBE));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)bprobe, bpol_probe_count, sizeof(BPOL_PROBE),
                          "BPOL_PROBE");

// 2. Loop over all Bpol Probes

                for (i = 0; i < bpol_probe_count; i++) {

                    initBpolProbe(&bprobe[i]);

                    sprintf(signal, "MAGNETICS/BPOL_PROBE/%d/NAME", i + 1);

                    handle = idamGetAPI(signal, source);

                    if (handle < 0 || getIdamErrorCode(handle) > 0) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Unable to access Machine Description data!");
                        idamFree(handle);
                        break;
                    }

                    if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_STRING) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Machine Description data has Incorrect properties!");
                        idamFree(handle);
                        break;
                    }

                    char* name = (char*)getIdamData(handle);
                    stringLength = strlen(name) + 1;
                    bprobe[i].name = (char*)malloc(stringLength * sizeof(char));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)bprobe[i].name, 1,
                              stringLength * sizeof(char), "STRING");    // Scalar String
                    strcpy(bprobe[i].name, name);

                    sprintf(signal, "MAGNETICS/BPOL_PROBE/%d/IDENTIFIER", i + 1);

                    handle = idamGetAPI(signal, source);

                    if (handle < 0 || getIdamErrorCode(handle) > 0) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Unable to access Machine Description data!");
                        idamFree(handle);
                        break;
                    }

                    if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_STRING) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Machine Description data has Incorrect properties!");
                        idamFree(handle);
                        break;
                    }

                    char* identifier = (char*)getIdamData(handle);
                    stringLength = strlen(identifier) + 1;
                    bprobe[i].identifier = (char*)malloc(stringLength * sizeof(char));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)bprobe[i].identifier, 1,
                              stringLength * sizeof(char),
                              "STRING");    // Scalar String
                    strcpy(bprobe[i].identifier, identifier);

// 3. Coordinates

                    sprintf(signal, "MAGNETICS/BPOL_PROBE/%d/POSITION/R", i + 1);

                    handle = idamGetAPI(signal, source);

                    if (handle < 0 || getIdamErrorCode(handle) > 0) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Unable to access Machine Description data!");
                        idamFree(handle);
                        break;
                    }

                    if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_DOUBLE ||
                        getIdamDataNum(handle) != 1) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Machine Description data has Incorrect properties!");
                        idamFree(handle);
                        break;
                    }

                    bprobe[i].r = *((double*)getIdamData(handle));

                    sprintf(signal, "MAGNETICS/BPOL_PROBE/%d/POSITION/Z", i + 1);

                    handle = idamGetAPI(signal, source);

                    if (handle < 0 || getIdamErrorCode(handle) > 0) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Unable to access Machine Description data!");
                        idamFree(handle);
                        break;
                    }

                    if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_DOUBLE ||
                        getIdamDataNum(handle) != 1) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Machine Description data has Incorrect properties!");
                        idamFree(handle);
                        break;
                    }

                    bprobe[i].z = *((double*)getIdamData(handle));

                    sprintf(signal, "MAGNETICS/BPOL_PROBE/%d/POSITION/PHI", i + 1);

                    handle = idamGetAPI(signal, source);

                    if (handle < 0 || getIdamErrorCode(handle) > 0) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Unable to access Machine Description data!");
                        idamFree(handle);
                        break;
                    }

                    if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_DOUBLE ||
                        getIdamDataNum(handle) != 1) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Machine Description data has Incorrect properties!");
                        idamFree(handle);
                        break;
                    }

                    bprobe[i].phi = *((double*)getIdamData(handle));

                } //  Loop over all Bpol Probes
            } // Read Machine Description Data

// Save to cache after data access

            if (cacheData) {

                bpol_probe_cache_count = bpol_probe_count;
                bpol_probe_cache = (BPOL_PROBE*)malloc(bpol_probe_count * sizeof(BPOL_PROBE));

// 2. Loop over all Bpol Probes

                for (i = 0; i < bpol_probe_count; i++) {

                    initBpolProbe(&bpol_probe_cache[i]);

                    stringLength = strlen(bprobe[i].name) + 1;
                    bpol_probe_cache[i].name = (char*)malloc(stringLength * sizeof(char));
                    strcpy(bpol_probe_cache[i].name, bprobe[i].name);

                    stringLength = strlen(bprobe[i].identifier) + 1;
                    bpol_probe_cache[i].identifier = (char*)malloc(stringLength * sizeof(char));
                    strcpy(bpol_probe_cache[i].identifier, bprobe[i].identifier);

// 3. Coordinates
                    bpol_probe_cache[i].r = bprobe[i].r;
                    bpol_probe_cache[i].z = bprobe[i].z;
                    bpol_probe_cache[i].phi = bprobe[i].phi;
                } // Loop over all Bpol Probes

            } else // cacheData

            if (cacheRetrieve) {        // Retrieve Machine Description Data from Cache

// 1. Number of Flux Loops

                bpol_probe_count = bpol_probe_cache_count;

// Return if only the count is requested

                if (isCount) return (returnCount(bpol_probe_count, data_block));

// Build the Returned Structures

                bprobe = (BPOL_PROBE*)malloc(bpol_probe_count * sizeof(BPOL_PROBE));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)bprobe, bpol_probe_count, sizeof(BPOL_PROBE),
                          "BPOL_PROBE");

// 2. Loop over all Flux Loops

                for (i = 0; i < bpol_probe_count; i++) {

                    initBpolProbe(&bprobe[i]);

                    stringLength = strlen(bpol_probe_cache[i].name) + 1;
                    bprobe[i].name = (char*)malloc(stringLength * sizeof(char));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)bprobe[i].name, 1,
                              stringLength * sizeof(char), "STRING");    // Scalar String
                    strcpy(bprobe[i].name, bpol_probe_cache[i].name);

                    stringLength = strlen(bpol_probe_cache[i].identifier) + 1;
                    bprobe[i].identifier = (char*)malloc(stringLength * sizeof(char));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)bprobe[i].identifier, 1,
                              stringLength * sizeof(char),
                              "STRING");    // Scalar String
                    strcpy(bprobe[i].identifier, bpol_probe_cache[i].identifier);

// 3. Coordinates
                    bprobe[i].r = bpol_probe_cache[i].r;
                    bprobe[i].z = bpol_probe_cache[i].z;
                    bprobe[i].phi = bpol_probe_cache[i].phi;
                }    // Loop over all Bpol Probes

            }    // end of cache retrieval



// 5. Access measurement data and apply subsetting

            for (i = 0; i < bpol_probe_count; i++) {

                bprobe[i].data_count = 0;
                bprobe[i].data = NULL;
                bprobe[i].time = NULL;

                sprintf(signal, "MAGNETICS/BPOL_PROBE/%d/FIELD/DATA", i + 1);
                int handle = idamGetAPI(signal, source);

                if (handle < 0 || getIdamErrorCode(handle) >
                                  0) {        // Unable to access Machine Measurement data - set count to zero! Ignore Error
                    idamFree(handle);
                    continue;
                }

                sprintf(signal, "MAGNETICS/BPOL_PROBE/%d/FIELD/TIME", i + 1);
                int handle2 = idamGetAPI(signal, source);

                if (handle2 < 0 || getIdamErrorCode(handle2) >
                                   0) {    // Unable to access Machine Measurement data - set count to zero!
                    idamFree(handle);
                    idamFree(handle2);
                    continue;
                }

                if (getIdamRank(handle2) != 0 || getIdamDataType(handle2) != TYPE_DOUBLE ||
                    getIdamDataNum(handle) != getIdamDataNum(handle2)) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Machine Measurement data has Incorrect properties!");
                    idamFree(handle);
                    idamFree(handle2);
                    break;
                }

                int data_count = getIdamDataNum(handle);

                double* fd = (double*)getIdamData(handle);
                double* ft = (double*)getIdamData(handle2);

                if (isStartTime) {
                    double sum1 = 0.0, sum2 = 0.0;
                    int index1 = -1;
                    int index2 = -1;

                    int j;
                    for (j = 0; j < data_count; j++) {
                        if (index1 < 0 && ft[j] >= startTime) {
                            index1 = j;
                            if (!isEndTime) break;
                        }
                        if (index1 >= 0 && index2 < 0 && ft[j] >= endTime) {
                            index2 = j - 1;
                            if (index2 < 0) index2 = 0;
                            if (ft[j] == endTime) index2 = j;
                            break;
                        }
                    }
                    if (index1 == -1) {
                        // ERROR
                    }

                    if (isEndTime) {    // Subset of the data
                        if (index2 == -1) index2 = data_count - 1;
                        data_count = index2 - index1 + 1;
                    } else {        // Single value
                        data_count = 1;
                        index2 = index1;
                    }

                    if (isFirst) {
                        data_count = 1;
                        index2 = index1;
                    } else if (isLast) {
                        data_count = 1;
                        index1 = index2;
                    } else if (isNearest) {
                        data_count = 1;
                        if (index1 > 0 && ((startTime - ft[index1 - 1]) > (ft[index1] - startTime))) {
                            index1 = index1 - 1;
                        }
                    } else if (isAverage) {
                        for (j = 0; j < data_count; j++) {
                            sum1 += fd[index1 + j];
                            sum2 += ft[index1 + j];
                        }
                        sum1 = sum1 / (double)data_count;
                        sum2 = sum2 / (double)data_count;
                        data_count = 1;
                    }

                    bprobe[i].data_count = data_count;
                    bprobe[i].data = (double*)malloc(bprobe[i].data_count * sizeof(double));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)bprobe[i].data, bprobe[i].data_count,
                              sizeof(double), "double");
                    bprobe[i].time = (double*)malloc(bprobe[i].data_count * sizeof(double));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)bprobe[i].time, bprobe[i].data_count,
                              sizeof(double), "double");

                    if (isAverage) {
                        bprobe[i].data[0] = sum1;
                        bprobe[i].time[0] = sum2;
                    } else {
                        for (j = 0; j < bprobe[i].data_count; j++) {
                            bprobe[i].data[j] = fd[index1 + j];
                            bprobe[i].time[j] = ft[index1 + j];
                        }
                    }

                } else {        // All data are returned

                    bprobe[i].data_count = data_count;

// Reuse the allocated data blocks - no need to copy!

                    bprobe[i].data = (double*)getIdamData(handle);
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)bprobe[i].data, bprobe[i].data_count,
                              sizeof(double), "double");
                    bprobe[i].time = (double*)getIdamData(handle2);
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)bprobe[i].time, bprobe[i].data_count,
                              sizeof(double), "double");
                    DATA_BLOCK* db = getIdamDataBlock(handle);
                    db->data = NULL;        // Prevent double free
                    db = getIdamDataBlock(handle2);
                    db->data = NULL;
                }

                idamFree(handle);        // Do not cache
                idamFree(handle2);

            } // Access measurement data and apply subsetting

            if (err != 0) break;

// 7. Return the Data

            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*)bprobe;

            strcpy(data_block->data_desc, "Proxy for a Magnetics IDS CODE structure");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "BPOL_PROBE", 0);

            break;

        } else if (!strcasecmp(request_block->function, "test10x")) {    // BPOL_PROBE data structure with MAST data

            char signal[256], source[256];

            BPOL_PROBE_TEST1* bprobe = NULL;
            int bpol_probe_count = 0;

// Create the Returned Structure Definitions

            initUserDefinedType(&usertype);   // New structure definition

            strcpy(usertype.name, "BPOL_PROBE_TEST1");
            usertype.size = sizeof(BPOL_PROBE_TEST1);
            strcpy(usertype.source, "LiveDisplay");
            usertype.ref_id = 0;
            usertype.imagecount = 0;       // No Structure Image data
            usertype.image = NULL;
            usertype.idamclass = TYPE_COMPOUND;

            int offset = 0;

            defineField(&field, "xdata_count", "Measurement data count", &offset, SCALARINT);
            addCompoundField(&usertype, field);
            defineField(&field, "xdata", "Measurement data", &offset, ARRAYDOUBLE);        // Doesn't like 'data' !!!
            addCompoundField(&usertype, field);
            defineField(&field, "xtime", "Measurement time", &offset, ARRAYDOUBLE);
            addCompoundField(&usertype, field);

            addUserDefinedType(userdefinedtypelist, usertype);    // BPOL_PROBE_TEST1


// Access MAST machine description data
// Use the IDAM client API with IMAS name abstraction

            sprintf(source, "MAST::%d", exp_number);

// 1. Number of Magnetic Probes

            sprintf(signal, "MAGNETICS/BPOL_PROBE/SHAPE_OF");

            int handle = idamGetAPI(signal, source);

            if (handle < 0 || getIdamErrorCode(handle) > 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                             "Unable to access Machine Description data!");
                idamFree(handle);
                break;
            }

            if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_INT || getIdamDataNum(handle) != 1) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                             "Machine Description data has Incorrect properties!");
                idamFree(handle);
                break;
            }

            bpol_probe_count = *((int*)getIdamData(handle));

//bpol_probe_count = 1;

// Build the Returned Structures

            bprobe = (BPOL_PROBE_TEST1*)malloc(bpol_probe_count * sizeof(BPOL_PROBE_TEST1));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)bprobe, bpol_probe_count, sizeof(BPOL_PROBE),
                      "BPOL_PROBE_TEST1");



// 5. Access measurement data and apply subsetting

            for (i = 0; i < bpol_probe_count; i++) {

                bprobe[i].data_count = 1;
                bprobe[i].data = (double*)malloc(bprobe[i].data_count * sizeof(double));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)bprobe[i].data, bprobe[i].data_count,
                          sizeof(double), "double");
                bprobe[i].time = (double*)malloc(bprobe[i].data_count * sizeof(double));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)bprobe[i].time, bprobe[i].data_count,
                          sizeof(double), "double");
                bprobe[i].data[0] = 10 + 3.1415927;
                bprobe[i].time[0] = 100 + 2.71828;

                bprobe[i].data_count = 0;
                bprobe[i].data = NULL;
                bprobe[i].time = NULL;

                sprintf(signal, "MAGNETICS/BPOL_PROBE/%d/FIELD/DATA", i + 1);
                handle = idamGetAPI(signal, source);

                if (handle < 0 || getIdamErrorCode(handle) >
                                  0) {        // Unable to access Machine Measurement data - set count to zero! Ignore Error
                    idamFree(handle);
                    continue;
                }

                sprintf(signal, "MAGNETICS/BPOL_PROBE/%d/FIELD/TIME", i + 1);
                int handle2 = idamGetAPI(signal, source);

                if (handle2 < 0 || getIdamErrorCode(handle2) >
                                   0) {    // Unable to access Machine Measurement data - set count to zero!
                    idamFree(handle);
                    idamFree(handle2);
                    continue;
                }

                if (getIdamRank(handle2) != 0 || getIdamDataType(handle2) != TYPE_DOUBLE ||
                    getIdamDataNum(handle) != getIdamDataNum(handle2)) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Machine Measurement data has Incorrect properties!");
                    idamFree(handle);
                    idamFree(handle2);
                    break;
                }

                int data_count = getIdamDataNum(handle);

                double* fd = (double*)getIdamData(handle);
                double* ft = (double*)getIdamData(handle2);

                if (isStartTime) {
                    double sum1 = 0.0, sum2 = 0.0;
                    int index1 = -1;
                    int index2 = -1;

                    int j;
                    for (j = 0; j < data_count; j++) {
                        if (index1 < 0 && ft[j] >= startTime) {
                            index1 = j;
                            if (!isEndTime) break;
                        }
                        if (index1 >= 0 && index2 < 0 && ft[j] >= endTime) {
                            index2 = j - 1;
                            if (index2 < 0) index2 = 0;
                            if (ft[j] == endTime) index2 = j;
                            break;
                        }
                    }
                    if (index1 == -1) {
                        // ERROR
                    }

                    if (isEndTime) {    // Subset of the data
                        if (index2 == -1) index2 = data_count - 1;
                        data_count = index2 - index1 + 1;
                    } else {        // Single value
                        data_count = 1;
                        index2 = index1;
                    }

                    if (isFirst) {
                        data_count = 1;
                        index2 = index1;
                    } else if (isLast) {
                        data_count = 1;
                        index1 = index2;
                    } else if (isNearest) {
                        data_count = 1;
                        if (index1 > 0 && ((startTime - ft[index1 - 1]) > (ft[index1] - startTime))) {
                            index1 = index1 - 1;
                        }
                    } else if (isAverage) {
                        for (j = 0; j < data_count; j++) {
                            sum1 += fd[index1 + j];
                            sum2 += ft[index1 + j];
                        }
                        sum1 = sum1 / (double)data_count;
                        sum2 = sum2 / (double)data_count;
                        data_count = 1;
                    }

                    bprobe[i].data_count = data_count;
                    bprobe[i].data = (double*)malloc(bprobe[i].data_count * sizeof(double));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)bprobe[i].data, bprobe[i].data_count,
                              sizeof(double), "double");

//for(j=0;j<bprobe[i].data_count;j++) bprobe[i].data[j] = 2*3.1415927;

                    bprobe[i].time = (double*)malloc(bprobe[i].data_count * sizeof(double));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)bprobe[i].time, bprobe[i].data_count,
                              sizeof(double), "double");

                    if (isAverage) {
                        bprobe[i].data[0] = sum1;
                        bprobe[i].time[0] = sum2;
                    } else {
                        for (j = 0; j < bprobe[i].data_count; j++) {
                            bprobe[i].data[j] = fd[index1 + j];
                            bprobe[i].time[j] = ft[index1 + j];
                        }
                    }

                } else {        // All data are returned
                    bprobe[i].data_count = 1;
                    bprobe[i].data = (double*)malloc(bprobe[i].data_count * sizeof(double));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)bprobe[i].data, bprobe[i].data_count,
                              sizeof(double), "double");
                    int j;
                    for (j = 0; j < bprobe[i].data_count; j++) bprobe[i].data[j] = 0.5 * 3.1415927;
                    bprobe[i].time = (double*)malloc(bprobe[i].data_count * sizeof(double));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)bprobe[i].time, bprobe[i].data_count,
                              sizeof(double), "double");
                    for (j = 0; j < bprobe[i].data_count; j++) bprobe[i].time[j] = 1.2345;
                }

                idamFree(handle);        // Do not cache
                idamFree(handle2);

            } // Access measurement data and apply subsetting



            if (err != 0) break;

// 7. Return the Data


            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*)bprobe;

            strcpy(data_block->data_desc, "Proxy for a Magnetics IDS CODE structure");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "BPOL_PROBE_TEST1", 0);

            break;

        } else if (!strcasecmp(request_block->function, "test10y")) {    // BPOL_PROBE data structure with MAST data

            static BPOL_PROBE_TEST2* bpol_probe_cache = NULL;
            static unsigned short bpol_probe_cache_count = 0;
            static char bpol_probe_source[256];

            char signal[256], source[256];
            int stringLength;

            BPOL_PROBE_TEST2* bprobe = NULL;
            int bpol_probe_count = 0;
            int cacheData = 0, cacheRetrieve = 0;

// Create the Returned Structure Definitions

            initUserDefinedType(&usertype);   // New structure definition

            strcpy(usertype.name, "BPOL_PROBE_TEST2");
            usertype.size = sizeof(BPOL_PROBE_TEST2);
            strcpy(usertype.source, "LiveDisplay");
            usertype.ref_id = 0;
            usertype.imagecount = 0;       // No Structure Image data
            usertype.image = NULL;
            usertype.idamclass = TYPE_COMPOUND;

            int offset = 0;

            defineField(&field, "identifier", "Identifier", &offset, SCALARSTRING);
            addCompoundField(&usertype, field);
            defineField(&field, "name", "name", &offset, SCALARSTRING);
            addCompoundField(&usertype, field);
            defineField(&field, "r", "Major Radius", &offset, SCALARDOUBLE);
            addCompoundField(&usertype, field);
            defineField(&field, "z", "Height", &offset, SCALARDOUBLE);
            addCompoundField(&usertype, field);
            defineField(&field, "phi", "Toroidal Angle", &offset, SCALARDOUBLE);
            addCompoundField(&usertype, field);
            addUserDefinedType(userdefinedtypelist, usertype);    // BPOL_PROBE_TEST2

// Access MAST machine description data
// Use the IDAM client API with IMAS name abstraction

            sprintf(source, "MAST::%d", exp_number);

// Test the cache for Machine Description data - based on the SOURCE identifier
// Reuse if cached
// Create static arrays (with generous limits) rather than pointers for machine description data

            if (isCache && strcmp(source, bpol_probe_source) != 0) {    // clear the cache - new data requested
                if (bpol_probe_cache_count > 0 && bpol_probe_cache != NULL) {
                    for (i = 0; i < bpol_probe_cache_count; i++) freeBpolProbe2(&bpol_probe_cache[i]);
                    free((void*)bpol_probe_cache);
                    bpol_probe_cache = NULL;
                    bpol_probe_cache_count = 0;
                }
                strcpy(bpol_probe_source, source);
            }

            if (isCache && bpol_probe_cache_count == 0) cacheData = 1;        // Cache the machine description data
            if (isCache && bpol_probe_cache_count > 0) {
                cacheRetrieve = 1;
            }    // Retrieve the machine description data from the Cache

// Read Machine Description Data

            if (!isCache || cacheData) {

// 1. Number of Magnetic Probes

                sprintf(signal, "MAGNETICS/BPOL_PROBE/SHAPE_OF");

                int handle = idamGetAPI(signal, source);

                if (handle < 0 || getIdamErrorCode(handle) > 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Unable to access Machine Description data!");
                    idamFree(handle);
                    break;
                }

                if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_INT || getIdamDataNum(handle) != 1) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Machine Description data has Incorrect properties!");
                    idamFree(handle);
                    break;
                }

                bpol_probe_count = *((int*)getIdamData(handle));

//bpol_probe_count = 1;

// Build the Returned Structures

                bprobe = (BPOL_PROBE_TEST2*)malloc(bpol_probe_count * sizeof(BPOL_PROBE_TEST2));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)bprobe, bpol_probe_count,
                          sizeof(BPOL_PROBE_TEST2), "BPOL_PROBE_TEST2");

// 2. Loop over all Bpol Probes

                for (i = 0; i < bpol_probe_count; i++) {

                    initBpolProbe2(&bprobe[i]);

                    sprintf(signal, "MAGNETICS/BPOL_PROBE/%d/NAME", i + 1);

                    handle = idamGetAPI(signal, source);

                    if (handle < 0 || getIdamErrorCode(handle) > 0) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Unable to access Machine Description data!");
                        idamFree(handle);
                        break;
                    }

                    if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_STRING) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Machine Description data has Incorrect properties!");
                        idamFree(handle);
                        break;
                    }

                    char* name = (char*)getIdamData(handle);
                    stringLength = strlen(name) + 1;
                    bprobe[i].name = (char*)malloc(stringLength * sizeof(char));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)bprobe[i].name, 1,
                              stringLength * sizeof(char), "STRING");    // Scalar String
                    strcpy(bprobe[i].name, name);

                    sprintf(signal, "MAGNETICS/BPOL_PROBE/%d/IDENTIFIER", i + 1);

                    handle = idamGetAPI(signal, source);

                    if (handle < 0 || getIdamErrorCode(handle) > 0) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Unable to access Machine Description data!");
                        idamFree(handle);
                        break;
                    }

                    if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_STRING) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Machine Description data has Incorrect properties!");
                        idamFree(handle);
                        break;
                    }

                    char* identifier = (char*)getIdamData(handle);
                    stringLength = strlen(identifier) + 1;
                    bprobe[i].identifier = (char*)malloc(stringLength * sizeof(char));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)bprobe[i].identifier, 1,
                              stringLength * sizeof(char),
                              "STRING");    // Scalar String
                    strcpy(bprobe[i].identifier, identifier);

// 3. Coordinates

                    sprintf(signal, "MAGNETICS/BPOL_PROBE/%d/POSITION/R", i + 1);

                    handle = idamGetAPI(signal, source);

                    if (handle < 0 || getIdamErrorCode(handle) > 0) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Unable to access Machine Description data!");
                        idamFree(handle);
                        break;
                    }

                    if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_DOUBLE ||
                        getIdamDataNum(handle) != 1) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Machine Description data has Incorrect properties!");
                        idamFree(handle);
                        break;
                    }

                    bprobe[i].r = *((double*)getIdamData(handle));

                    sprintf(signal, "MAGNETICS/BPOL_PROBE/%d/POSITION/Z", i + 1);

                    handle = idamGetAPI(signal, source);

                    if (handle < 0 || getIdamErrorCode(handle) > 0) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Unable to access Machine Description data!");
                        idamFree(handle);
                        break;
                    }

                    if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_DOUBLE ||
                        getIdamDataNum(handle) != 1) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Machine Description data has Incorrect properties!");
                        idamFree(handle);
                        break;
                    }

                    bprobe[i].z = *((double*)getIdamData(handle));

                    sprintf(signal, "MAGNETICS/BPOL_PROBE/%d/POSITION/PHI", i + 1);

                    handle = idamGetAPI(signal, source);

                    if (handle < 0 || getIdamErrorCode(handle) > 0) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Unable to access Machine Description data!");
                        idamFree(handle);
                        break;
                    }

                    if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_DOUBLE ||
                        getIdamDataNum(handle) != 1) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Machine Description data has Incorrect properties!");
                        idamFree(handle);
                        break;
                    }

                    bprobe[i].phi = *((double*)getIdamData(handle));

                } //  Loop over all Bpol Probes
            } // Read Machine Description Data

// Save to cache after data access

            if (cacheData) {

                bpol_probe_cache_count = bpol_probe_count;
                bpol_probe_cache = (BPOL_PROBE_TEST2*)malloc(bpol_probe_count * sizeof(BPOL_PROBE_TEST2));

// 2. Loop over all Bpol Probes

                for (i = 0; i < bpol_probe_count; i++) {

                    initBpolProbe2(&bpol_probe_cache[i]);

                    stringLength = strlen(bprobe[i].name) + 1;
                    bpol_probe_cache[i].name = (char*)malloc(stringLength * sizeof(char));
                    strcpy(bpol_probe_cache[i].name, bprobe[i].name);

                    stringLength = strlen(bprobe[i].identifier) + 1;
                    bpol_probe_cache[i].identifier = (char*)malloc(stringLength * sizeof(char));
                    strcpy(bpol_probe_cache[i].identifier, bprobe[i].identifier);

// 3. Coordinates
                    bpol_probe_cache[i].r = bprobe[i].r;
                    bpol_probe_cache[i].z = bprobe[i].z;
                    bpol_probe_cache[i].phi = bprobe[i].phi;
                } // Loop over all Bpol Probes

            } else // cacheData

            if (cacheRetrieve) {        // Retrieve Machine Description Data from Cache

// 1. Number of Flux Loops

                bpol_probe_count = bpol_probe_cache_count;

// Build the Returned Structures

                bprobe = (BPOL_PROBE_TEST2*)malloc(bpol_probe_count * sizeof(BPOL_PROBE_TEST2));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)bprobe, bpol_probe_count,
                          sizeof(BPOL_PROBE_TEST2), "BPOL_PROBE_TEST2");

// 2. Loop over all Flux Loops

                for (i = 0; i < bpol_probe_count; i++) {

                    initBpolProbe2(&bprobe[i]);

                    stringLength = strlen(bpol_probe_cache[i].name) + 1;
                    bprobe[i].name = (char*)malloc(stringLength * sizeof(char));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)bprobe[i].name, 1,
                              stringLength * sizeof(char), "STRING");    // Scalar String
                    strcpy(bprobe[i].name, bpol_probe_cache[i].name);

                    stringLength = strlen(bpol_probe_cache[i].identifier) + 1;
                    bprobe[i].identifier = (char*)malloc(stringLength * sizeof(char));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)bprobe[i].identifier, 1,
                              stringLength * sizeof(char),
                              "STRING");    // Scalar String
                    strcpy(bprobe[i].identifier, bpol_probe_cache[i].identifier);

// 3. Coordinates
                    bprobe[i].r = bpol_probe_cache[i].r;
                    bprobe[i].z = bpol_probe_cache[i].z;
                    bprobe[i].phi = bpol_probe_cache[i].phi;
                }    // Loop over all Bpol Probes

            }    // end of cache retrieval



// 5. Access measurement data and apply subsetting

            if (err != 0) break;

// 7. Return the Data


            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*)bprobe;

            strcpy(data_block->data_desc, "Proxy for a Magnetics IDS CODE structure");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "BPOL_PROBE_TEST2", 0);

            break;

        } else


// -------------------------------------------------------------------------------------------------------------------------------------------------
/*
MAGNETICS/METHOD/COUNT_OF
MAGNETICS/METHOD/%d/IP/DATA
MAGNETICS/METHOD/%d/IP/TIME
MAGNETICS/METHOD/%d/DIAMAGNETIC_FLUX/DATA
MAGNETICS/METHOD/%d/DIAMAGNETIC_FLUX/TIME
*/
        if (!strcasecmp(request_block->function, "magnetics") ||
            !strcasecmp(request_block->function, "test11")) {    // MAGNETICS data structure with MAST data
            char* p = NULL;
            int rc, size = 0;
            char* type = NULL;
            char signal[256], source[256];
            DATA_BLOCK plugin_data_block;
            REQUEST_BLOCK plugin_request_block;
            IDAM_PLUGIN_INTERFACE idam_plugin_interface2;

// Create the Returned Structure Definitions
// ToDo ... pass in the structureVersion

            defineIDSStructures();

// Build the Returned Structures

            MAGNETICS_PROXY* magnetics = (MAGNETICS_PROXY*)malloc(1 * sizeof(MAGNETICS_PROXY));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)magnetics, 1, sizeof(MAGNETICS_PROXY),
                      "MAGNETICS_PROXY");

// Access MAST machine description data
// Use the IDAM client API with IMAS name abstraction

            sprintf(source, "%s%s%d", request_block->device_name, request_block->api_delim, exp_number);

// Read Flux Loop Data (Recursive call to this plugin)

            initDataBlock(&plugin_data_block);
            initRequestBlock(&plugin_request_block);

            idam_plugin_interface2 = *idam_plugin_interface;
            idam_plugin_interface2.data_block = &plugin_data_block;
            idam_plugin_interface2.request_block = &plugin_request_block;

            strcpy(plugin_request_block.source, source);

            strcpy(signal, request_block->signal);
            p = strchr(request_block->signal, '(');
            sprintf(idam_plugin_interface2.request_block->signal, "%s%stest9%s", request_block->format,
                    request_block->api_delim, p);    // Request Flux_Loop Data

            makeServerRequestBlock(&plugin_request_block, *pluginList);

            rc = livedisplay(&idam_plugin_interface2);

            magnetics->flux_loop = (FLUX_LOOP*)(plugin_data_block.data);

            plugin_data_block.data = NULL;    // Prevent Double Free
            freeDataBlock(&plugin_data_block);
            freeNameValueList(&plugin_request_block.nameValueList);

            size = 0;
            type = NULL;
            findMalloc(idam_plugin_interface->logmalloclist, magnetics->flux_loop, &magnetics->flux_loop_count, &size, &type);

            initDataBlock(&plugin_data_block);
            initRequestBlock(&plugin_request_block);

            idam_plugin_interface2 = *idam_plugin_interface;
            idam_plugin_interface2.data_block = &plugin_data_block;
            idam_plugin_interface2.request_block = &plugin_request_block;

            strcpy(plugin_request_block.source, source);

            strcpy(signal, request_block->signal);
            p = strrchr(signal, ')');
            p[0] = '\0';
            strcat(signal, ", /Count)");
            p = strchr(signal, '(');
            sprintf(idam_plugin_interface2.request_block->signal, "%s%stest9%s", request_block->format,
                    request_block->api_delim, p);    // Request Flux_Loop Data Count

            makeServerRequestBlock(&plugin_request_block, *pluginList);

            rc = livedisplay(&idam_plugin_interface2);

            magnetics->flux_loop_count = *((int*)plugin_data_block.data);

            freeDataBlock(&plugin_data_block);
            freeNameValueList(&plugin_request_block.nameValueList);


// Read Bpol Probe Data (Recursive call to this plugin)

            initDataBlock(&plugin_data_block);
            initRequestBlock(&plugin_request_block);

            idam_plugin_interface2 = *idam_plugin_interface;
            idam_plugin_interface2.data_block = &plugin_data_block;
            idam_plugin_interface2.request_block = &plugin_request_block;

            strcpy(plugin_request_block.source, source);

            strcpy(signal, request_block->signal);
            p = strchr(request_block->signal, '(');
            sprintf(idam_plugin_interface2.request_block->signal, "%s%stest10%s", request_block->format,
                    request_block->api_delim, p);    // Request Bpol Probe Data

            makeServerRequestBlock(&plugin_request_block, *pluginList);

            rc = livedisplay(&idam_plugin_interface2);

            magnetics->bpol_probe = (BPOL_PROBE*)(plugin_data_block.data);

            plugin_data_block.data = NULL;    // Prevent Double Free
            freeDataBlock(&plugin_data_block);
            freeNameValueList(&plugin_request_block.nameValueList);

            initDataBlock(&plugin_data_block);
            initRequestBlock(&plugin_request_block);

            idam_plugin_interface2 = *idam_plugin_interface;
            idam_plugin_interface2.data_block = &plugin_data_block;
            idam_plugin_interface2.request_block = &plugin_request_block;

            strcpy(plugin_request_block.source, source);

            strcpy(signal, request_block->signal);
            p = strrchr(signal, ')');
            p[0] = '\0';
            strcat(signal, ", /Count)");
            p = strchr(signal, '(');
            sprintf(idam_plugin_interface2.request_block->signal, "%s%stest10%s", request_block->format,
                    request_block->api_delim, p);    // Request Bpol Probe Data Count

            makeServerRequestBlock(&plugin_request_block, *pluginList);

            rc = livedisplay(&idam_plugin_interface2);

            magnetics->bpol_probe_count = *((int*)plugin_data_block.data);

            freeDataBlock(&plugin_data_block);
            freeNameValueList(&plugin_request_block.nameValueList);

// Build the Returned METHOD Structure

            magnetics->method_count = 1;
            METHOD* method = (METHOD*)malloc(magnetics->method_count * sizeof(METHOD));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)method, magnetics->method_count, sizeof(METHOD_DATA),
                      "METHOD");

            int j;
            for (j = 0; j < magnetics->method_count; j++) initMethod(&method[j]);

            magnetics->method = method;

            int stringLength = 56;
            method[0].name = (char*)malloc(stringLength * sizeof(char));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)method[0].name, 1, stringLength * sizeof(char),
                      "STRING");    // Scalar String
            strcpy(method[0].name, "Method #1");

// Access the Plasma Current

            strcpy(signal, "amc_plasma current");
            sprintf(source, "%s%s%d", request_block->device_name, request_block->api_delim, exp_number);

            int handle = idamGetAPI(signal, source);

            if (handle < 0 || getIdamErrorCode(handle) > 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                             "Unable to access Plasma Current data!");
                idamFree(handle);
                break;
            }

            if (getIdamRank(handle) != 1) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                             "Plasma Current data has Incorrect properties!");
                idamFree(handle);
                break;
            }

// Scan the data for the requested time slice and window

            METHOD_DATA* method_data = (METHOD_DATA*)malloc(sizeof(METHOD_DATA));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)method_data, 1, sizeof(METHOD_DATA), "METHOD_DATA");
            initMethodData(method_data);

            magnetics->method[0].ip = method_data;

            stringLength = 56;
            method_data->name = (char*)malloc(stringLength * sizeof(char));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)method_data->name, 1, stringLength * sizeof(char),
                      "STRING");    // Scalar String
            strcpy(method_data->name, "Plasma Current");
            method_data->identifier = (char*)malloc(stringLength * sizeof(char));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)method_data->identifier, 1,
                      stringLength * sizeof(char), "STRING");    // Scalar String
            strcpy(method_data->identifier, "#1");

            int data_count = getIdamDataNum(handle);

            double* fd = (double*)malloc(data_count * sizeof(double));
            double* ft = (double*)malloc(data_count * sizeof(double));

            getIdamDoubleData(handle, fd);
            getIdamDoubleDimData(handle, getIdamOrder(handle), ft);

            idamFree(handle);

// data conversion factors

            double dataScaling = 1000.0;

            if (isStartTime) {
                double sum1 = 0.0, sum2 = 0.0;
                int index1 = -1;
                int index2 = -1;
                int subset_count = 0;

                err = dataSubset(ft, data_count, startTime, isEndTime, endTime, isFirst, isLast, isNearest,
                                 &subset_count, &index1, &index2);

                if (err != 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Unable to Subset Plasma Current!");
                    idamFree(handle);
                    if (fd != NULL) free((void*)fd);
                    if (ft != NULL) free((void*)ft);
                    break;
                }

                if (isAverage) {
                    for (j = 0; j < subset_count; j++) {
                        sum1 += fd[index1 + j];
                        sum2 += ft[index1 + j];
                    }

                    if (fd != NULL) free((void*)fd);
                    if (ft != NULL) free((void*)ft);
                    sum1 = sum1 / (double)subset_count;
                    sum2 = sum2 / (double)subset_count;
                    data_count = 1;

                    method_data->count = data_count;
                    method_data->data = (double*)malloc(data_count * sizeof(double));
                    method_data->time = (double*)malloc(data_count * sizeof(double));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)method_data->data, data_count,
                              sizeof(double), "double");
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)method_data->time, data_count,
                              sizeof(double), "double");

                    method_data->data[j] = dataScaling * sum1;
                    method_data->time[j] = sum2;

                } else {

                    method_data->count = subset_count;
                    method_data->data = (double*)malloc(subset_count * sizeof(double));
                    method_data->time = (double*)malloc(subset_count * sizeof(double));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)method_data->data, subset_count,
                              sizeof(double), "double");
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)method_data->time, subset_count,
                              sizeof(double), "double");

                    for (j = 0; j < subset_count; j++) {
                        method_data->data[j] = dataScaling * fd[index1 + j];
                        method_data->time[j] = ft[index1 + j];
                    }

                    if (fd != NULL) free((void*)fd);
                    if (ft != NULL) free((void*)ft);

                }

            } else {        // All data are returned

                method_data->count = data_count;

                for (j = 0; j < data_count; j++) fd[j] = dataScaling * fd[j];

                method_data->data = fd;
                method_data->time = ft;
                addMalloc(idam_plugin_interface->logmalloclist, (void*)method_data->data, data_count, sizeof(double),
                          "double");
                addMalloc(idam_plugin_interface->logmalloclist, (void*)method_data->time, data_count, sizeof(double),
                          "double");
            }


// Access the Diamagnetic Flux

            strcpy(signal, "amd_dia flux");
            sprintf(source, "%s%s%d", request_block->device_name, request_block->api_delim, exp_number);

            handle = idamGetAPI(signal, source);

            if (handle < 0 || getIdamErrorCode(handle) > 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                             "Unable to access Diamagnetic Flux data!");
                idamFree(handle);
                break;
            }

            if (getIdamRank(handle) != 1) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                             "Diamagnetic Flux data has Incorrect properties!");
                idamFree(handle);
                break;
            }

// Scan the data for the requested time slice and window

            method_data = (METHOD_DATA*)malloc(sizeof(METHOD_DATA));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)method_data, 1, sizeof(METHOD_DATA), "METHOD_DATA");
            initMethodData(method_data);

            magnetics->method[0].diamagnetic_flux = method_data;

            stringLength = 56;
            method_data->name = (char*)malloc(stringLength * sizeof(char));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)method_data->name, 1, stringLength * sizeof(char),
                      "STRING");    // Scalar String
            strcpy(method_data->name, "Diamagnetic Flux");
            method_data->identifier = (char*)malloc(stringLength * sizeof(char));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)method_data->identifier, 1,
                      stringLength * sizeof(char), "STRING");    // Scalar String
            strcpy(method_data->identifier, "#1");

            data_count = getIdamDataNum(handle);

            fd = (double*)malloc(data_count * sizeof(double));
            ft = (double*)malloc(data_count * sizeof(double));

            getIdamDoubleData(handle, fd);
            getIdamDoubleDimData(handle, getIdamOrder(handle), ft);

            idamFree(handle);

            if (isStartTime) {
                double sum1 = 0.0, sum2 = 0.0;
                int index1 = -1;
                int index2 = -1;
                int subset_count = 0;

                err = dataSubset(ft, data_count, startTime, isEndTime, endTime, isFirst, isLast, isNearest,
                                 &subset_count, &index1, &index2);

                if (err != 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Unable to Subset Diamagnetic Flux!");
                    idamFree(handle);
                    if (fd != NULL) free((void*)fd);
                    if (ft != NULL) free((void*)ft);
                    break;
                }

                if (isAverage) {
                    for (j = 0; j < subset_count; j++) {
                        sum1 += fd[index1 + j];
                        sum2 += ft[index1 + j];
                    }

                    if (fd != NULL) free((void*)fd);
                    if (ft != NULL) free((void*)ft);
                    sum1 = sum1 / (double)subset_count;
                    sum2 = sum2 / (double)subset_count;
                    data_count = 1;

                    method_data->count = data_count;
                    method_data->data = (double*)malloc(data_count * sizeof(double));
                    method_data->time = (double*)malloc(data_count * sizeof(double));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)method_data->data, data_count,
                              sizeof(double), "double");
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)method_data->time, data_count,
                              sizeof(double), "double");

                    method_data->data[j] = sum1;
                    method_data->time[j] = sum2;

                } else {

                    method_data->count = subset_count;
                    method_data->data = (double*)malloc(subset_count * sizeof(double));
                    method_data->time = (double*)malloc(subset_count * sizeof(double));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)method_data->data, subset_count,
                              sizeof(double), "double");
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)method_data->time, subset_count,
                              sizeof(double), "double");

                    for (j = 0; j < subset_count; j++) {
                        method_data->data[j] = fd[index1 + j];
                        method_data->time[j] = ft[index1 + j];
                    }

                    if (fd != NULL) free((void*)fd);
                    if (ft != NULL) free((void*)ft);

                }

            } else {        // All data are returned

                method_data->count = data_count;

                method_data->data = fd;
                method_data->time = ft;
                addMalloc(idam_plugin_interface->logmalloclist, (void*)method_data->data, data_count, sizeof(double),
                          "double");
                addMalloc(idam_plugin_interface->logmalloclist, (void*)method_data->time, data_count, sizeof(double),
                          "double");
            }

// Return the Data

            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*)magnetics;

            strcpy(data_block->data_desc, "Proxy for a Magnetics IDS CODE structure");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "MAGNETICS_PROXY", 0);

            break;

        } else


// -------------------------------------------------------------------------------------------------------------------------------------------------
/*
*/

        if (!strcasecmp(request_block->function, "limiter")) {    // STATIC_LIMITER data structure

// Create the Returned Structure Definitions

            defineIDSStructures();

// Build the Returned Structures

            STATIC_LIMITER* limiter = (STATIC_LIMITER*)malloc(sizeof(STATIC_LIMITER));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)limiter, 1, sizeof(STATIC_LIMITER),
                      "STATIC_LIMITER");

            initStaticLimiter(limiter);

            limiter->count = 37;

            limiter->r = (double*)malloc(limiter->count * sizeof(STATIC_LIMITER));
            limiter->z = (double*)malloc(limiter->count * sizeof(STATIC_LIMITER));

            addMalloc(idam_plugin_interface->logmalloclist, (void*)limiter->r, limiter->count, sizeof(double),
                      "double");
            addMalloc(idam_plugin_interface->logmalloclist, (void*)limiter->z, limiter->count, sizeof(double),
                      "double");

            double rr[] = { 1.9000000, 1.5551043, 1.5551043, 1.4079306, 1.4079306, 1.0399311,
                            1.0399311, 1.9000000, 1.9000000, 0.56493068, 0.56493068, 0.78350002, 0.78350002,
                            0.58259028, 0.41650000, 0.28000000, 0.28000000, 0.19524440, 0.19524440, 0.28000000,
                            0.28000000, 0.41650000, 0.58259028, 0.78350002, 0.78350002, 0.56493068, 0.56493068,
                            1.9000000, 1.9000000, 1.0399311, 1.0399311, 1.4079306, 1.4079306, 1.5551043, 1.5551043,
                            1.9000000, 1.9000000 };

            double zz[] = { 0.40500000, 0.40500000, 0.82250023, 0.82250023, 1.0330003, 1.0330003,
                            1.1950001, 1.1950001, 1.8250000, 1.8250000, 1.7280816, 1.7280816, 1.7155817, 1.5470001,
                            1.5470001, 1.6835001, 1.2290885, 1.0835000, -1.0835000, -1.2290885, -1.6835001,
                            -1.5470001, -1.5470001, -1.7155817, -1.7280816, -1.7280816, -1.8250000, -1.8250000,
                            -1.1950001, -1.1950001, -1.0330003, -1.0330003, -0.82250023, -0.82250023, -0.40500000,
                            -0.40500000, 0.40500000 };

            for (i = 0; i < limiter->count; i++) {
                limiter->r[i] = rr[i];
                limiter->z[i] = zz[i];
            }

// Pass Data

            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*)limiter;

            strcpy(data_block->data_desc, "Passive Limiter Coordinates");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "STATIC_LIMITER", 0);

            break;

        } else

// -------------------------------------------------------------------------------------------------------------------------------------------------
// To Do
//
// plasma current: IDS core_profiles, equilibrium, MAGNETICS
// vacuum toroidal field: IDS core_profiles, core_sources, core_transport, EQUILIBRIUM, mhd_linear, ntms, sawteeth
// diamagnetic flux: MAGNETICS
// -------------------------------------------------------------------------------------------------------------------------------------------------
// Vacuum R*Bphi
// -ve sign means clockwise when viewed from above

/*
TF/B_TOR_VACUUM_R
TF/B_TOR_VACUUM_DATA
TF/B_TOR_VACUUM_TIME
*/

        if (!strcasecmp(request_block->function, "RB")) {
            char signal[256], source[256];

// Create the Returned Structure Definitions
// ToDo ... pass in the structureVersion

            defineIDSStructures();

// Access the Data

            strcpy(signal, "amc_tf current");
            sprintf(source, "%s%s%d", request_block->device_name, request_block->api_delim, exp_number);

            int handle = idamGetAPI(signal, source);

            if (handle < 0 || getIdamErrorCode(handle) > 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                             "Unable to access Vacuum Magnetic Field!");
                idamFree(handle);
                break;
            }

            if (getIdamRank(handle) != 1 &&
                !(getIdamDataType(handle) == TYPE_DOUBLE || getIdamDataType(handle) == TYPE_FLOAT)) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                             "Vacuum Magnetic Field data has Incorrect properties!");
                idamFree(handle);
                break;
            }

// Build the Returned Structure

            TF_PROXY* RB = (TF_PROXY*)malloc(1 * sizeof(TF_PROXY));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)RB, 1, sizeof(TF_PROXY), "TF_PROXY");

            initRB(RB);

// Scan the data for the requested time slice and window

            int data_count = getIdamDataNum(handle);

            double r0 = 0.7;        // MAST specific data
            double factor = 0.0048;        // Conversion factor

            double* b0 = (double*)malloc(data_count * sizeof(double));
            double* ft = (double*)malloc(data_count * sizeof(double));

            getIdamDoubleData(handle, b0);
            getIdamDoubleDimData(handle, getIdamOrder(handle), ft);

            if (data_count == 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err, "No Vacuum Magnetic Field data!");
                idamFree(handle);
                if (b0 != NULL) free((void*)b0);
                if (ft != NULL) free((void*)ft);
                break;
            }

            if (isStartTime) {
                double sum1b = 0.0, sum2 = 0.0;
                int index1 = -1;
                int index2 = -1;
                int subset_count = 0;

                err = dataSubset(ft, data_count, startTime, isEndTime, endTime, isFirst, isLast, isNearest,
                                 &subset_count, &index1, &index2);

                if (err != 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Unable to Subset Vacuum Magnetic Field data!");
                    idamFree(handle);
                    if (b0 != NULL) free((void*)b0);
                    if (ft != NULL) free((void*)ft);
                    break;
                }

                if (isAverage) {
                    int j;
                    for (j = 0; j < subset_count; j++) {
                        sum1b += b0[index1 + j];
                        sum2 += ft[index1 + j];
                    }
                    sum1b = sum1b / (double)(subset_count);
                    sum2 = sum2 / (double)(subset_count);
                    data_count = 1;

                    RB->r0 = (double*)malloc(sizeof(double));
                    RB->b0 = (double*)malloc(sizeof(double));
                    RB->rb0 = (double*)malloc(sizeof(double));
                    RB->time = (double*)malloc(sizeof(double));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)RB->r0, 1, sizeof(double), "double");
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)RB->b0, 1, sizeof(double), "double");
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)RB->rb0, 1, sizeof(double), "double");
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)RB->time, 1, sizeof(double), "double");
                    RB->data_count = 1;
                    RB->r0[0] = r0;
                    RB->b0[0] = factor * sum1b;
                    RB->rb0[0] = r0 * factor * sum1b;
                    RB->time[0] = sum2;
                } else {
                    RB->r0 = (double*)malloc(subset_count * sizeof(double));
                    RB->b0 = (double*)malloc(subset_count * sizeof(double));
                    RB->rb0 = (double*)malloc(subset_count * sizeof(double));
                    RB->time = (double*)malloc(subset_count * sizeof(double));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)RB->r0, subset_count, sizeof(double),
                              "double");
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)RB->b0, subset_count, sizeof(double),
                              "double");
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)RB->rb0, subset_count, sizeof(double),
                              "double");
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)RB->time, subset_count, sizeof(double),
                              "double");
                    RB->data_count = subset_count;
                    int j;
                    for (j = 0; j < subset_count; j++) {
                        RB->r0[j] = r0;
                        RB->b0[j] = factor * b0[index1 + j];
                        RB->rb0[j] = r0 * factor * b0[index1 + j];
                        RB->time[j] = ft[index1 + j];
                    }
                }

            } else {        // All data are returned

                RB->data_count = data_count;
                RB->r0 = (double*)malloc(data_count * sizeof(double));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)RB->r0, RB->data_count, sizeof(double),
                          "double");
                addMalloc(idam_plugin_interface->logmalloclist, (void*)RB->b0, RB->data_count, sizeof(double),
                          "double");
                addMalloc(idam_plugin_interface->logmalloclist, (void*)RB->time, RB->data_count, sizeof(double),
                          "double");
                RB->rb0 = (double*)malloc(RB->data_count * sizeof(double));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)RB->rb0, RB->data_count, sizeof(double),
                          "double");
                int j;
                for (j = 0; j < RB->data_count; j++) {
                    RB->r0[j] = r0;
                    RB->b0[j] = factor * b0[j];
                    RB->rb0[j] = r0 * factor * b0[j];
                    RB->time[j] = ft[j];
                }
            }

            idamFree(handle);
            if (b0 != NULL) free((void*)b0);
            if (ft != NULL) free((void*)ft);

// Pass Data

            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*)RB;

            strcpy(data_block->data_desc, "Vacuum Toroidal Magnetic Field");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TF_PROXY", 0);

            break;

        } else if (!strcasecmp(request_block->function, "RB_EFIT")) {
            char signal[256], source[256];

// Create the Returned Structure Definitions
// ToDo ... pass in the structureVersion

            defineIDSStructures();

// Access the Data

            strcpy(signal, "efm_bvac_r");
            sprintf(source, "%s%s%d", request_block->device_name, request_block->api_delim, exp_number);

            int handle1 = idamGetAPI(signal, source);

            if (handle1 < 0 || getIdamErrorCode(handle1) > 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                             "Unable to access Vacuum Magnetic Field!");
                idamFree(handle1);
                break;
            }

            if (getIdamRank(handle1) != 1 &&
                !(getIdamDataType(handle1) == TYPE_DOUBLE || getIdamDataType(handle1) == TYPE_FLOAT)) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                             "Vacuum Magnetic Field data has Incorrect properties!");
                idamFree(handle1);
                break;
            }

            strcpy(signal, "efm_bvac_val");

            int handle2 = idamGetAPI(signal, source);

            if (handle2 < 0 || getIdamErrorCode(handle2) > 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                             "Unable to access Vacuum Magnetic Field!");
                idamFree(handle2);
                break;
            }

            if (getIdamRank(handle2) != 1 &&
                !(getIdamDataType(handle2) == TYPE_DOUBLE || getIdamDataType(handle2) == TYPE_FLOAT)) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                             "Vacuum Magnetic Field data has Incorrect properties!");
                idamFree(handle2);
                break;
            }

// Build the Returned Structure

            TF_PROXY* RB = (TF_PROXY*)malloc(1 * sizeof(TF_PROXY));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)RB, 1, sizeof(TF_PROXY), "TF_PROXY");

            initRB(RB);

// Scan the data for the requested time slice and window

            int data_count = getIdamDataNum(handle1);

            double* r0 = (double*)malloc(data_count * sizeof(double));
            double* b0 = (double*)malloc(data_count * sizeof(double));
            double* ft = (double*)malloc(data_count * sizeof(double));
            double* ft2 = (double*)malloc(data_count * sizeof(double));

            getIdamDoubleData(handle1, r0);
            getIdamDoubleData(handle2, b0);
            getIdamDoubleDimData(handle1, getIdamOrder(handle1), ft);
            getIdamDoubleDimData(handle2, getIdamOrder(handle2), ft2);

// Verify Time arrays are consistent

            if (data_count != getIdamDataNum(handle2)) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                             "Vacuum Magnetic Field data has Inconsistent properties!");
                idamFree(handle1);
                idamFree(handle2);
                if (r0 != NULL) free((void*)r0);
                if (b0 != NULL) free((void*)b0);
                if (ft != NULL) free((void*)ft);
                if (ft2 != NULL) free((void*)ft2);
                break;
            }
            if (data_count == 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err, "No Vacuum Magnetic Field data!");
                idamFree(handle1);
                idamFree(handle2);
                if (r0 != NULL) free((void*)r0);
                if (b0 != NULL) free((void*)b0);
                if (ft != NULL) free((void*)ft);
                if (ft2 != NULL) free((void*)ft2);
                break;
            }

            idamFree(handle1);
            idamFree(handle2);

            int j;
            for (j = 0; j < data_count; j++) {
                if (ft[j] != ft2[j]) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Vacuum Magnetic Field data has Inconsistent properties!");
                    if (r0 != NULL) free((void*)r0);
                    if (b0 != NULL) free((void*)b0);
                    if (ft != NULL) free((void*)ft);
                    if (ft2 != NULL) free((void*)ft2);
                    break;
                }
            }
            if (err != 0) break;

            if (ft2 != NULL) free((void*)ft2);

            if (isStartTime) {
                double sum1a = 0.0, sum1b = 0.0, sum2 = 0.0;
                int index1 = -1;
                int index2 = -1;
                int subset_count = 0;

                err = dataSubset(ft, data_count, startTime, isEndTime, endTime, isFirst, isLast, isNearest,
                                 &subset_count, &index1, &index2);

                if (err != 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Unable to Subset Vacuum Magnetic Field datas!");
                    if (r0 != NULL) free((void*)r0);
                    if (b0 != NULL) free((void*)b0);
                    if (ft != NULL) free((void*)ft);
                    break;
                }

                if (isAverage) {
                    for (j = 0; j < subset_count; j++) {
                        sum1a += r0[index1 + j];
                        sum1b += b0[index1 + j];
                        sum2 += ft[index1 + j];
                    }
                    sum1a = sum1a / (double)(subset_count);
                    sum1b = sum1b / (double)(subset_count);
                    sum2 = sum2 / (double)(subset_count);
                    data_count = 1;

                    RB->r0 = (double*)malloc(sizeof(double));
                    RB->b0 = (double*)malloc(sizeof(double));
                    RB->rb0 = (double*)malloc(sizeof(double));
                    RB->time = (double*)malloc(sizeof(double));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)RB->r0, 1, sizeof(double), "double");
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)RB->b0, 1, sizeof(double), "double");
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)RB->rb0, 1, sizeof(double), "double");
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)RB->time, 1, sizeof(double), "double");
                    RB->data_count = 1;
                    RB->r0[0] = sum1a;
                    RB->b0[0] = sum1b;
                    RB->rb0[0] = sum1a * sum1b;
                    RB->time[0] = sum2;
                } else {
                    RB->r0 = (double*)malloc(subset_count * sizeof(double));
                    RB->b0 = (double*)malloc(subset_count * sizeof(double));
                    RB->rb0 = (double*)malloc(subset_count * sizeof(double));
                    RB->time = (double*)malloc(subset_count * sizeof(double));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)RB->r0, subset_count, sizeof(double),
                              "double");
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)RB->b0, subset_count, sizeof(double),
                              "double");
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)RB->rb0, subset_count, sizeof(double),
                              "double");
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)RB->time, subset_count, sizeof(double),
                              "double");
                    RB->data_count = subset_count;
                    for (j = 0; j < subset_count; j++) {
                        RB->r0[j] = r0[index1 + j];
                        RB->b0[j] = b0[index1 + j];
                        RB->rb0[j] = r0[index1 + j] * b0[index1 + j];
                        RB->time[j] = ft[index1 + j];
                    }
                }

            } else {        // All data are returned

                RB->data_count = data_count;

// Reuse the allocated data blocks - no need to copy!

                RB->r0 = r0;
                addMalloc(idam_plugin_interface->logmalloclist, (void*)RB->r0, RB->data_count, sizeof(double),
                          "double");
                RB->b0 = b0;
                addMalloc(idam_plugin_interface->logmalloclist, (void*)RB->b0, RB->data_count, sizeof(double),
                          "double");
                RB->time = ft;
                addMalloc(idam_plugin_interface->logmalloclist, (void*)RB->time, RB->data_count, sizeof(double),
                          "double");
                RB->rb0 = (double*)malloc(RB->data_count * sizeof(double));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)RB->rb0, RB->data_count, sizeof(double),
                          "double");
                for (j = 0; j < RB->data_count; j++) RB->rb0[j] = r0[j] * b0[j];
            }

            if (r0 != NULL) free((void*)r0);
            if (b0 != NULL) free((void*)b0);
            if (ft != NULL) free((void*)ft);

// Pass Data

            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*)RB;

            strcpy(data_block->data_desc, "Vacuum Toroidal Magnetic Field");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TF_PROXY", 0);

            break;

        } else

// -------------------------------------------------------------------------------------------------------------------------------------------------
/*
PF_ACTIVE/COIL/SHAPE_OF
PF_ACTIVE/COIL/%d/NAME
PF_ACTIVE/COIL/%d/IDENTIFIER
PF_ACTIVE/COIL/%d/ELEMENT/SHAPE_OF
PF_ACTIVE/COIL/%d/ELEMENT/%d/GEOMETRY/RECTANGLE/R
PF_ACTIVE/COIL/%d/ELEMENT/%d/GEOMETRY/RECTANGLE/Z
PF_ACTIVE/COIL/%d/ELEMENT/%d/GEOMETRY/RECTANGLE/WIDTH
PF_ACTIVE/COIL/%d/ELEMENT/%d/GEOMETRY/RECTANGLE/HEIGHT
PF_ACTIVE/COIL/%d/CURRENT/DATA		# Apply scaling factor to data within data access plugin
PF_ACTIVE/COIL/%d/CURRENT/TIME
*/

        if (!strcasecmp(request_block->function, "PFActive")) {    // PF_ACTIVE data structure with MAST data

            char signal[256], source[256];
            int stringLength;
            PF_ACTIVE_PROXY* pfactive_proxy = NULL;
            PF_ACTIVE* pfactive = NULL;
            int pfactive_count = 0;
            int cacheData = 0, cacheRetrieve = 0;

// Create the Returned Structure Definitions

            defineIDSStructures();

// Access MAST machine description data: PF_Active
// Use the IDAM client API with IMAS name abstraction

            sprintf(source, "%s%s%d", request_block->device_name, request_block->api_delim, exp_number);

// Test the cache for Machine Description data - based on the SOURCE identifier
// Reuse if cached
// Create static arrays (with generous limits) rather than pointers for machine description data

            if (isCache && strcmp(source, pfactive_source) != 0) {    // clear the cache - new data requested
                if (pfactive_cache_count > 0 && pfactive_cache != NULL) {
                    for (i = 0; i < pfactive_cache_count; i++) freePFActive(&pfactive_cache[i]);
                    free((void*)pfactive_cache);
                    pfactive_cache = NULL;
                    pfactive_cache_count = 0;
                }
                strcpy(pfactive_source, source);
            }

            if (isCache && pfactive_cache_count == 0) cacheData = 1;        // Cache the machine description data
            if (isCache && pfactive_cache_count > 0) {
                cacheRetrieve = 1;
            }        // Retrieve the machine description data from the Cache


// Read Machine Description Data

            if (!isCache || cacheData) {

// 1. Number of PF Active Coils

                sprintf(signal, "PF_ACTIVE/COIL/SHAPE_OF");

                int handle = idamGetAPI(signal, source);

                if (handle < 0 || getIdamErrorCode(handle) > 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Unable to access Machine Description data!");
                    idamFree(handle);
                    break;
                }

                if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_INT || getIdamDataNum(handle) != 1) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Machine Description data has Incorrect properties!");
                    idamFree(handle);
                    break;
                }

                pfactive_count = *((int*)getIdamData(handle));

// Return if only the count is requested

                if (isCount) return (returnCount(pfactive_count, data_block));

// Build the Returned Data Structures

                pfactive_proxy = (PF_ACTIVE_PROXY*)malloc(sizeof(PF_ACTIVE_PROXY));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)pfactive_proxy, 1, sizeof(PF_ACTIVE_PROXY),
                          "PF_ACTIVE_PROXY");

                pfactive = (PF_ACTIVE*)malloc(pfactive_count * sizeof(PF_ACTIVE));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)pfactive, pfactive_count, sizeof(PF_ACTIVE),
                          "PF_ACTIVE");

                pfactive_proxy->pfactive_count = pfactive_count;
                pfactive_proxy->pfactive = pfactive;


// 2. Loop over all Coils

                for (i = 0; i < pfactive_count; i++) {

                    initPFActive(&pfactive[i]);

                    sprintf(signal, "PF_ACTIVE/COIL/%d/NAME", i + 1);

                    handle = idamGetAPI(signal, source);

                    if (handle < 0 || getIdamErrorCode(handle) > 0) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Unable to access Machine Description data!");
                        idamFree(handle);
                        break;
                    }

                    if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_STRING) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Machine Description data has Incorrect properties!");
                        idamFree(handle);
                        break;
                    }

                    char* name = (char*)getIdamData(handle);
                    stringLength = strlen(name) + 1;
                    pfactive[i].name = (char*)malloc(stringLength * sizeof(char));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)pfactive[i].name, 1,
                              stringLength * sizeof(char), "STRING");    // Scalar String
                    strcpy(pfactive[i].name, name);

                    sprintf(signal, "PF_ACTIVE/COIL/%d/IDENTIFIER", i + 1);

                    handle = idamGetAPI(signal, source);

                    if (handle < 0 || getIdamErrorCode(handle) > 0) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Unable to access Machine Description data!");
                        idamFree(handle);
                        break;
                    }

                    if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_STRING) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Machine Description data has Incorrect properties!");
                        idamFree(handle);
                        break;
                    }

                    char* identifier = (char*)getIdamData(handle);
                    stringLength = strlen(identifier) + 1;
                    pfactive[i].identifier = (char*)malloc(stringLength * sizeof(char));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)pfactive[i].identifier, 1,
                              stringLength * sizeof(char),
                              "STRING");    // Scalar String
                    strcpy(pfactive[i].identifier, identifier);

// 3. Number of Elements/Coordinates

                    sprintf(signal, "PF_ACTIVE/COIL/%d/ELEMENT/SHAPE_OF", i + 1);

                    handle = idamGetAPI(signal, source);

                    if (handle < 0 || getIdamErrorCode(handle) > 0) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Unable to access Machine Description data!");
                        idamFree(handle);
                        break;
                    }

                    if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_INT ||
                        getIdamDataNum(handle) != 1) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                     "Machine Description data has Incorrect properties!");
                        idamFree(handle);
                        break;
                    }

                    pfactive[i].element_count = *((int*)getIdamData(handle));

                    PF_ACTIVE_ELEMENT* element = (PF_ACTIVE_ELEMENT*)malloc(
                            pfactive[i].element_count * sizeof(PF_ACTIVE_ELEMENT));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)element, pfactive[i].element_count,
                              sizeof(PF_ACTIVE_ELEMENT),
                              "PF_ACTIVE_ELEMENT");

                    pfactive[i].element = element;

// 4. Loop over Elements/Coordinates

                    int j;
                    for (j = 0; j < pfactive[i].element_count; j++) {

                        sprintf(signal, "PF_ACTIVE/COIL/%d/ELEMENT/%d/GEOMETRY/RECTANGLE/R", i + 1, j + 1);

                        handle = idamGetAPI(signal, source);

                        if (handle < 0 || getIdamErrorCode(handle) > 0) {
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                         "Unable to access Machine Description data!");
                            idamFree(handle);
                            break;
                        }

                        if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_DOUBLE ||
                            getIdamDataNum(handle) != 1) {
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                         "Machine Description data has Incorrect properties!");
                            idamFree(handle);
                            break;
                        }

                        element[j].r = *((double*)getIdamData(handle));

                        sprintf(signal, "PF_ACTIVE/COIL/%d/ELEMENT/%d/GEOMETRY/RECTANGLE/Z", i + 1, j + 1);

                        handle = idamGetAPI(signal, source);

                        if (handle < 0 || getIdamErrorCode(handle) > 0) {
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                         "Unable to access Machine Description data!");
                            idamFree(handle);
                            break;
                        }

                        if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_DOUBLE ||
                            getIdamDataNum(handle) != 1) {
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                         "Machine Description data has Incorrect properties!");
                            idamFree(handle);
                            break;
                        }

                        element[j].z = *((double*)getIdamData(handle));

                        sprintf(signal, "PF_ACTIVE/COIL/%d/ELEMENT/%d/GEOMETRY/RECTANGLE/WIDTH", i + 1, j + 1);

                        handle = idamGetAPI(signal, source);

                        if (handle < 0 || getIdamErrorCode(handle) > 0) {
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                         "Unable to access Machine Description data!");
                            idamFree(handle);
                            break;
                        }

                        if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_DOUBLE ||
                            getIdamDataNum(handle) != 1) {
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                         "Machine Description data has Incorrect properties!");
                            idamFree(handle);
                            break;
                        }

                        element[j].width = *((double*)getIdamData(handle));

                        sprintf(signal, "PF_ACTIVE/COIL/%d/ELEMENT/%d/GEOMETRY/RECTANGLE/HEIGHT", i + 1, j + 1);

                        handle = idamGetAPI(signal, source);

                        if (handle < 0 || getIdamErrorCode(handle) > 0) {
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                         "Unable to access Machine Description data!");
                            idamFree(handle);
                            break;
                        }

                        if (getIdamRank(handle) != 0 || getIdamDataType(handle) != TYPE_DOUBLE ||
                            getIdamDataNum(handle) != 1) {
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                         "Machine Description data has Incorrect properties!");
                            idamFree(handle);
                            break;
                        }

                        element[j].height = *((double*)getIdamData(handle));

                    } // Loop over elements/coordinates

                    if (err != 0) break;

                } // Loop over all PF_Active Coils
            } // Read Machine Description Data

// Save to cache after data access

            if (cacheData) {

                pfactive_cache_count = pfactive_count;
                pfactive_cache = (PF_ACTIVE*)malloc(pfactive_count * sizeof(PF_ACTIVE));

// 2. Loop over all PF Active Coils

                for (i = 0; i < pfactive_count; i++) {

                    initPFActive(&pfactive_cache[i]);

                    stringLength = strlen(pfactive[i].name) + 1;
                    pfactive_cache[i].name = (char*)malloc(stringLength * sizeof(char));
                    strcpy(pfactive_cache[i].name, pfactive[i].name);

                    stringLength = strlen(pfactive[i].identifier) + 1;
                    pfactive_cache[i].identifier = (char*)malloc(stringLength * sizeof(char));
                    strcpy(pfactive_cache[i].identifier, pfactive[i].identifier);

// 3. Number of Elements/Coordinates

                    pfactive_cache[i].element_count = pfactive[i].element_count;
                    pfactive_cache[i].element = (PF_ACTIVE_ELEMENT*)malloc(
                            pfactive[i].element_count * sizeof(PF_ACTIVE_ELEMENT));

// 4. Loop over coordinates

                    int j;
                    for (j = 0; j < pfactive[i].element_count; j++) {
                        pfactive_cache[i].element[j].r = pfactive[i].element[j].r;
                        pfactive_cache[i].element[j].z = pfactive[i].element[j].z;
                        pfactive_cache[i].element[j].width = pfactive[i].element[j].width;
                        pfactive_cache[i].element[j].height = pfactive[i].element[j].height;
                    }
                } // Loop over all PF Coils

            } else // cacheData

            if (cacheRetrieve) {        // Retrieve Machine Description Data from Cache

// 1. Number of PF Active Coils

                pfactive_count = pfactive_cache_count;

// Return if only the count is requested

                if (isCount) return (returnCount(pfactive_count, data_block));

// Build the Returned Structures

                pfactive_proxy = (PF_ACTIVE_PROXY*)malloc(sizeof(PF_ACTIVE_PROXY));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)pfactive_proxy, 1, sizeof(PF_ACTIVE_PROXY),
                          "PF_ACTIVE_PROXY");

                pfactive = (PF_ACTIVE*)malloc(pfactive_count * sizeof(PF_ACTIVE));
                addMalloc(idam_plugin_interface->logmalloclist, (void*)pfactive, pfactive_count, sizeof(PF_ACTIVE),
                          "PF_ACTIVE");

                pfactive_proxy->pfactive_count = pfactive_count;
                pfactive_proxy->pfactive = pfactive;

// 2. Loop over all Flux Loops

                for (i = 0; i < pfactive_count; i++) {

                    initPFActive(&pfactive[i]);

                    stringLength = strlen(pfactive_cache[i].name) + 1;
                    pfactive[i].name = (char*)malloc(stringLength * sizeof(char));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)pfactive[i].name, 1,
                              stringLength * sizeof(char), "STRING");    // Scalar String
                    strcpy(pfactive[i].name, pfactive_cache[i].name);

                    stringLength = strlen(pfactive_cache[i].identifier) + 1;
                    pfactive[i].identifier = (char*)malloc(stringLength * sizeof(char));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)pfactive[i].identifier, 1,
                              stringLength * sizeof(char),
                              "STRING");    // Scalar String
                    strcpy(pfactive[i].identifier, pfactive_cache[i].identifier);

// 3. Number of Elements/Coordinates

                    pfactive[i].element_count = pfactive_cache[i].element_count;

                    PF_ACTIVE_ELEMENT* element = (PF_ACTIVE_ELEMENT*)malloc(
                            pfactive[i].element_count * sizeof(PF_ACTIVE_ELEMENT));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)element, pfactive[i].element_count,
                              sizeof(PF_ACTIVE_ELEMENT),
                              "PF_ACTIVE_ELEMENT");

                    pfactive[i].element = element;

// 4. Loop over Elements/coordinates

                    int j;
                    for (j = 0; j < pfactive[i].element_count; j++) {
                        pfactive[i].element[j].r = pfactive_cache[i].element[j].r;
                        pfactive[i].element[j].z = pfactive_cache[i].element[j].z;
                        pfactive[i].element[j].width = pfactive_cache[i].element[j].width;
                        pfactive[i].element[j].height = pfactive_cache[i].element[j].height;
                    }
                }    // Loop over all PF Active Coils

            }    // end of cache retrieval


// 5. Access measurement data and apply subsetting

            for (i = 0; i < pfactive_count; i++) {

                pfactive[i].data_count = 0;
                pfactive[i].data = NULL;
                pfactive[i].time = NULL;

                sprintf(signal, "PF_ACTIVE/COIL/%d/CURRENT/DATA", i + 1);
                int handle = idamGetAPI(signal, source);

                if (handle < 0 || getIdamErrorCode(handle) >
                                  0) {        // Unable to access Machine Measurement data - set count to zero! Ignore Error
                    idamFree(handle);
                    continue;
                }

                sprintf(signal, "PF_ACTIVE/COIL/%d/CURRENT/TIME", i + 1);
                int handle2 = idamGetAPI(signal, source);

                if (handle2 < 0 || getIdamErrorCode(handle2) >
                                   0) {    // Unable to access Machine Measurement data - set count to zero! Ignore Error
                    idamFree(handle);
                    idamFree(handle2);
                    continue;
                }

                if (getIdamRank(handle2) != 0 || getIdamDataType(handle2) != TYPE_DOUBLE ||
                    getIdamDataNum(handle) != getIdamDataNum(handle2)) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "LiveDisplay", err,
                                 "Machine Measurement data has Incorrect properties!");
                    idamFree(handle);
                    idamFree(handle2);
                    break;
                }

                int data_count = getIdamDataNum(handle);

                double* fd = (double*)getIdamData(handle);
                double* ft = (double*)getIdamData(handle2);

                if (isStartTime) {
                    double sum1 = 0.0, sum2 = 0.0;
                    int index1 = -1;
                    int index2 = -1;

                    int j;
                    for (j = 0; j < data_count; j++) {
                        if (index1 < 0 && ft[j] >= startTime) {
                            index1 = j;
                            if (!isEndTime) break;
                        }
                        if (index1 >= 0 && index2 < 0 && ft[j] >= endTime) {
                            index2 = j - 1;
                            if (index2 < 0) index2 = 0;
                            if (ft[j] == endTime) index2 = j;
                            break;
                        }
                    }
                    if (index1 == -1) {
                        // ERROR
                    }

                    if (isEndTime) {    // Subset of the data
                        if (index2 == -1) index2 = data_count - 1;
                        data_count = index2 - index1 + 1;
                    } else {        // Single value
                        data_count = 1;
                        index2 = index1;
                    }

                    if (isFirst) {
                        data_count = 1;
                        index2 = index1;
                    } else if (isLast) {
                        data_count = 1;
                        index1 = index2;
                    } else if (isNearest) {
                        data_count = 1;
                        if (index1 > 0 && ((startTime - ft[index1 - 1]) > (ft[index1] - startTime))) {
                            index1 = index1 - 1;
                        }
                    } else if (isAverage) {
                        for (j = 0; j < data_count; j++) {
                            sum1 += fd[index1 + j];
                            sum2 += ft[index1 + j];
                        }
                        sum1 = sum1 / (double)data_count;
                        sum2 = sum2 / (double)data_count;
                        data_count = 1;
                    }

                    pfactive[i].data_count = data_count;
                    pfactive[i].data = (double*)malloc(pfactive[i].data_count * sizeof(double));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)pfactive[i].data, pfactive[i].data_count,
                              sizeof(double), "double");
                    pfactive[i].time = (double*)malloc(pfactive[i].data_count * sizeof(double));
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)pfactive[i].time, pfactive[i].data_count,
                              sizeof(double), "double");

                    if (isAverage) {
                        pfactive[i].data[0] = sum1;
                        pfactive[i].time[0] = sum2;
                    } else {
                        for (j = 0; j < pfactive[i].data_count; j++) {
                            pfactive[i].data[j] = fd[index1 + j];
                            pfactive[i].time[j] = ft[index1 + j];
                        }
                    }
                } else {        // All data are returned

                    pfactive[i].data_count = data_count;

// Reuse the allocated data blocks - no need to copy!

                    pfactive[i].data = (double*)getIdamData(handle);
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)pfactive[i].data, pfactive[i].data_count,
                              sizeof(double), "double");
                    pfactive[i].time = (double*)getIdamData(handle2);
                    addMalloc(idam_plugin_interface->logmalloclist, (void*)pfactive[i].time, pfactive[i].data_count,
                              sizeof(double), "double");
                    DATA_BLOCK* db = getIdamDataBlock(handle);
                    db->data = NULL;        // Prevent double free
                    db = getIdamDataBlock(handle2);
                    db->data = NULL;
                }

                idamFree(handle);        // Do not cache
                idamFree(handle2);

            } // Access measurement data and apply subsetting

            if (err != 0) break;

// Return the Data

            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*)pfactive_proxy;

            strcpy(data_block->data_desc, "Proxy for a PF_ACTIVE IDS structure");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "PF_ACTIVE_PROXY", 0);

            break;

        } else

//----------------------------------------------------------------------------------------
// Principal methods
//----------------------------------------------------------------------------------------
// GET
/*
        if (!strcasecmp(request_block->function, "get")) {

            //idamLog(UDA_LOG_DEBUG, "LiveDisplay: GET entered\n");

// Create the Returned Structure Definitions

            initUserDefinedType(&usertype);            // New structure definition

            strcpy(usertype.name, "CODE");
            usertype.size = sizeof(CODE);
            strcpy(usertype.source, "LiveDisplay");
            usertype.ref_id = 0;
            usertype.imagecount = 0;                // No Structure Image data
            usertype.image = NULL;
            usertype.idamclass = TYPE_COMPOUND;

            offset = 0;

            defineField(&field, "name", "Code Name", &offset, SCALARSTRING);
            addCompoundField(&usertype, field);

            defineField(&field, "version", "Code Version", &offset, SCALARSTRING);
            addCompoundField(&usertype, field);
            defineField(&field, "parameters", "Code Parameters", &offset, SCALARSTRING);
            addCompoundField(&usertype, field);
            defineField(&field, "output_flag_count", "The array count", &offset, SCALARINT);
            addCompoundField(&usertype, field);
            defineField(&field, "output_flag", "The Code output flags", &offset, ARRAYINT);
            addCompoundField(&usertype, field);

            addUserDefinedType(userdefinedtypelist, usertype);


            initUserDefinedType(&usertype);            // New structure definition

            strcpy(usertype.name, "MAGNETICS_TEST");
            usertype.size = sizeof(MAGNETICS_TEST);
            strcpy(usertype.source, "LiveDisplay");
            usertype.ref_id = 0;
            usertype.imagecount = 0;                // No Structure Image data
            usertype.image = NULL;
            usertype.idamclass = TYPE_COMPOUND;

            offset = 0;

            defineField(&field, "comment", "Comment", &offset, SCALARSTRING);
            addCompoundField(&usertype, field);
            defineField(&field, "homogeneous_time", "homogeneous_time", &offset, SCALARINT);
            addCompoundField(&usertype, field);
            defineField(&field, "flux_loop_count", "flux_loop array count", &offset, SCALARINT);
            addCompoundField(&usertype, field);
            defineField(&field, "bpol_probe_count", "bpol_probe array count", &offset, SCALARINT);
            addCompoundField(&usertype, field);
            defineField(&field, "method_count", "method array count", &offset, SCALARINT);
            addCompoundField(&usertype, field);

            defineField(&field, "output_flag", "The Code output flags", &offset, ARRAYINT);
            addCompoundField(&usertype, field);

            initCompoundField(&field);
            strcpy(field.name, "code");
            field.atomictype = TYPE_UNKNOWN;
            strcpy(field.type, "CODE");
            strcpy(field.desc, "Code Information");
            field.pointer = 0;
            field.count = 1;
            field.rank = 0;
            field.shape = NULL;            		// Needed when rank >= 1
            field.size = field.count * sizeof(CODE);
            field.offset = newoffset(offset, field.type);
            field.offpad = padding(offset, field.type);
            field.alignment = getalignmentof(field.type);
            offset = field.offset + field.size;		// Next Offset
            addCompoundField(&usertype, field);		// Single Structure element

            addUserDefinedType(userdefinedtypelist, usertype);

// Build the Returned Structures

            CODE code;

	    stringLength = 56;

	    code.name = (char *)malloc(stringLength*sizeof(char));


            addMalloc(idam_plugin_interface->logmalloclist, (void*)code.name, 1, stringLength*sizeof(char), "STRING");	// Scalar String
	    strcpy(code.name, "IDAM LiveDisplay Plugin");

	    stringLength = 56;
	    code.version = (char *)malloc(stringLength*sizeof(char));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)code.version, 1, stringLength*sizeof(char), "STRING");	// Scalar String
	    sprintf(code.version, "LiveDisplay Plugin Version: %d", THISPLUGIN_VERSION);

	    stringLength = strlen(request_block->signal)+strlen(request_block->source)+7;
	    code.parameters = (char *)malloc(stringLength*sizeof(char));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)code.parameters, 1, stringLength*sizeof(char), "STRING");	// Scalar String
	    sprintf(code.parameters, "\"%s\", \"%s\"", request_block->signal, request_block->source);

            code.output_flag_count = 1;
	    code.output_flag = (int *)malloc(code.output_flag_count*sizeof(int));
	    addMalloc(idam_plugin_interface->logmalloclist, (void*)code.output_flag, code.output_flag_count, sizeof(int), "int");	// Integer Array
	    code.output_flag[0] = 0;

	    MAGNETICS_TEST *magnetics = (MAGNETICS_TEST *)malloc(sizeof(MAGNETICS_TEST));
	    addMalloc(idam_plugin_interface->logmalloclist, (void*)magnetics, 1, sizeof(MAGNETICS_TEST), "MAGNETICS_TEST");

	    stringLength = 128;
	    magnetics->comment = (char *)malloc(stringLength*sizeof(char));
	    addMalloc(idam_plugin_interface->logmalloclist, (void*)magnetics->comment, 1, stringLength*sizeof(char), "STRING");	// Scalar String
	    strcpy(magnetics->comment, "This is the LiveDisplay prototype plugin for Magnetics IDS data");

	    magnetics->homogeneous_time = 1;
	    magnetics->flux_loop_count = 2;
	    magnetics->bpol_probe_count = 3;
	    magnetics->method_count = 4;

	    magnetics->code = code;

// Pass Data

            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*) magnetics;

            strcpy(data_block->data_desc, "Proxy for a Magnetics IDS");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*) findUserDefinedType(userdefinedtypelist, "MAGNETICS_TEST", 0);

            //idamLog(UDA_LOG_DEBUG, "LiveDisplay: GET exited\n");

            break;

        } else
*/
//----------------------------------------------------------------------------------------
// Help: A Description of library functionality
//
// livedisplay::help()
// Information is returned as a single string containing all required format control characters.

        if (!strcasecmp(request_block->function, "help")) {
            const char* help = "\nLIVEDISPLAY: description and examples\n";
            const char* desc = "LiveDisplay Plugin help";

            err = setReturnDataString(idam_plugin_interface->data_block, help, desc);

            break;
        } else

//----------------------------------------------------------------------------------------
// Standard methods: version, builddate, defaultmethod, maxinterfaceversion

        if (!strcasecmp(request_block->function, "version")) {
            err = setReturnDataIntScalar(idam_plugin_interface->data_block, THISPLUGIN_VERSION,
                                         "Plugin version number");

            break;
        } else

// Plugin Build Date

        if (!strcasecmp(request_block->function, "builddate")) {
            err = setReturnDataString(idam_plugin_interface->data_block, __DATE__, "Plugin build date");

            break;
        } else

// Plugin Default Method

        if (!strcasecmp(request_block->function, "defaultmethod")) {
            err = setReturnDataString(idam_plugin_interface->data_block, THISPLUGIN_DEFAULT_METHOD,
                                      "Plugin default method");

            break;
        } else

// Plugin Maximum Interface Version

        if (!strcasecmp(request_block->function, "maxinterfaceversion")) {
            err = setReturnDataIntScalar(idam_plugin_interface->data_block, THISPLUGIN_MAX_INTERFACE_VERSION,
                                         "Maximum Interface Version");

            break;
        } else {
            RAISE_PLUGIN_ERROR("Unknown Function requested");
        }
    } while (0);

    return err;
}

