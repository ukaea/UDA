#ifndef IDAM_READCDF4_H
#define IDAM_READCDF4_H

#include <stddef.h>

#include <clientserver/udaStructs.h>
#include <structures/genStructs.h>

int readCDF(DATA_SOURCE data_source, SIGNAL_DESC signal_desc, REQUEST_BLOCK request_block, DATA_BLOCK* data_block,
            LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist);

#ifndef NONETCDFPLUGIN
#  include <netcdf.h>
#  include <stdio.h>

#  include <structures/genStructs.h>

#define NETCDF_ERROR_OPENING_FILE               200
#define NETCDF_ERROR_ALLOCATING_HEAP_1          201
#define NETCDF_ERROR_ALLOCATING_HEAP_2          202
#define NETCDF_ERROR_ALLOCATING_HEAP_3          203
#define NETCDF_ERROR_ALLOCATING_HEAP_4          204
#define NETCDF_ERROR_ALLOCATING_HEAP_5          205
#define NETCDF_ERROR_ALLOCATING_HEAP_6          206
#define NETCDF_ERROR_ALLOCATING_HEAP_7          207
#define NETCDF_ERROR_ALLOCATING_HEAP_8          208
#define NETCDF_ERROR_ALLOCATING_HEAP_9          209
#define NETCDF_ERROR_ALLOCATING_HEAP_10         210
#define NETCDF_ERROR_INQUIRING_VARIABLE_1       220
#define NETCDF_ERROR_INQUIRING_VARIABLE_2       221
#define NETCDF_ERROR_INQUIRING_VARIABLE_3       222
#define NETCDF_ERROR_INQUIRING_VARIABLE_4       223
#define NETCDF_ERROR_INQUIRING_VARIABLE_5       224
#define NETCDF_ERROR_INQUIRING_VARIABLE_6       225
#define NETCDF_ERROR_INQUIRING_VARIABLE_7       226
#define NETCDF_ERROR_INQUIRING_VARIABLE_8       227
#define NETCDF_ERROR_INQUIRING_VARIABLE_9       228
#define NETCDF_ERROR_INQUIRING_VARIABLE_10      229
#define NETCDF_ERROR_INQUIRING_VARIABLE_11      230
#define NETCDF_ERROR_INQUIRING_VARIABLE_12      231
#define NETCDF_ERROR_INQUIRING_VARIABLE_13      232
#define NETCDF_ERROR_INQUIRING_VARIABLE_14      233
#define NETCDF_ERROR_INQUIRING_DIM_1            240
#define NETCDF_ERROR_INQUIRING_DIM_2            241
#define NETCDF_ERROR_INQUIRING_DIM_3            242
#define NETCDF_ERROR_INQUIRING_DIM_4            243
#define NETCDF_ERROR_INQUIRING_DIM_5            244
#define NETCDF_ERROR_INQUIRING_ATT_1            250
#define NETCDF_ERROR_INQUIRING_ATT_2            251
#define NETCDF_ERROR_INQUIRING_ATT_3            252
#define NETCDF_ERROR_INQUIRING_ATT_4            253
#define NETCDF_ERROR_INQUIRING_ATT_5            254
#define NETCDF_ERROR_INQUIRING_ATT_6            255
#define NETCDF_ERROR_INQUIRING_ATT_7            256
#define NETCDF_ERROR_INQUIRING_ATT_8            257
#define NETCDF_ERROR_INQUIRING_ATT_9            258
#define NETCDF_ERROR_INQUIRING_ATT_10           259

#define TEST                        0

#define NOCLASS_DATA                0
#define RAW_DATA                    1
#define ANALYSED_DATA               2
#define MODELLED_DATA               3

#define    COMPLIANCE_PASS          31

#define NC_IGNOREHIDDENATTS         1
#define NC_IGNOREHIDDENVARS         2
#define NC_IGNOREHIDDENGROUPS       4
#define NC_IGNOREHIDDENDIMS         8
#define NC_NOTPOINTERTYPE           16
#define NC_NODIMENSIONDATA          32        // Don't add dimension documentation to a structure
#define NC_NOATTRIBUTEDATA          64        // Ignore all attribute data - group and variable
#define NC_NOVARATTRIBUTEDATA       128        // Ignore all Variable attribute data
#define NC_NOGROUPATTRIBUTEDATA     256        // Ignore all Group attribute data
#define NC_HIDDENPREFIX             '_'        // Objects with names prefixed with this character are to be optionaly ignored

#define NC_EMPTY_GROUP_VAR_NAME "ignore_this_variable"    // To prevent problems when groups are empty, add an empty 1 byte string

struct GROUPLIST {                // Pass the list of groups within scope
    int count;
    int grpid;
    int* grpids;
};
typedef struct GROUPLIST GROUPLIST;

struct ATTRIBUTE {
    int attid;
    char attname[NC_MAX_NAME + 1];
    nc_type atttype;
    int attlength;
    int udtIndex;
    USERDEFINEDTYPE* udt;
};
typedef struct ATTRIBUTE ATTRIBUTE;

struct VARIABLE {
    int varid;
    int numatts;
    ATTRIBUTE* attribute;
    char varname[NC_MAX_NAME + 1];
    nc_type vartype;
    int rank;
    int dimids[NC_MAX_VAR_DIMS];
    int* shape;                // Don't free
    int udtIndex;
    USERDEFINEDTYPE* udt;        // Don't free
};
typedef struct VARIABLE VARIABLE;

struct GROUP {
    int grpid;
    int parent;
    char grpname[NC_MAX_NAME + 1];
    int numgrps;
    int numatts;
    int numvars;
    int* grpids;
    int udtIndex;
    USERDEFINEDTYPE* udt;
    ATTRIBUTE* attribute;
    VARIABLE* variable;
};
typedef struct GROUP GROUP;

struct HGROUPS {
    int numgrps;
    GROUP* group;
};
typedef struct HGROUPS HGROUPS;

struct X1 {
    char* title;
};
typedef struct X1 X1;

struct X2 {
    X1* A;
    X1* B;
    X1* C;
    X1* D;
    X1* E;
};
typedef struct X2 X2;

struct CDFSUBSET {
    int subsetCount;                // Number of defined dimensions to subset
    int rank;                    // Data Rank (0 => dimids not known)
    int subset[NC_MAX_VAR_DIMS];            // If 1 then subset to apply
    int dimids[NC_MAX_VAR_DIMS];            // Dimension ID
    size_t start[NC_MAX_VAR_DIMS];        // Starting Index of each dimension
    size_t stop[NC_MAX_VAR_DIMS];        // Ending Index of each dimension
    size_t count[NC_MAX_VAR_DIMS];        // The number of values (sub-samples) read from each dimension
    ptrdiff_t stride[NC_MAX_VAR_DIMS];        // The step stride along each dimension
};
typedef struct CDFSUBSET CDFSUBSET;

int readCDF4Err(int grpid, int varid, int isCoordinate, int class, int rank, int* dimids, int* nevec,
                int* error_type, char** edata, LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist);

int readCDFAtts(int fd, int varid, char* units, char* longname);

int getGroupId(int ncgrpid, char* target, int* targetid);

int applyCDFCalibration(int grpid, int varid, int ndata, int* type, char** data);

void readCDF4CreateIndex(int ndata, void* dvec);

int readCDFCheckCoordinate(int grpid, int varid, int rank, int ncoords, char* coords, LOGMALLOCLIST* logmalloclist,
                           USERDEFINEDTYPELIST* userdefinedtypelist);

int isAtomicNCType(nc_type type);

int convertNCType(nc_type type);

void printNCType(FILE* fd, nc_type type);

int readCDFTypes(int grpid, USERDEFINEDTYPELIST* userdefinedtypelist);

int getCDF4SubTreeMeta(int grpid, int parent, USERDEFINEDTYPE* udt, LOGMALLOCLIST* logmalloclist,
                       USERDEFINEDTYPELIST* userdefinedtypelist, HGROUPS* hgroups);

int getCDF4SubTreeMetaX(int grpid, int parent, USERDEFINEDTYPE* udt, USERDEFINEDTYPELIST* userdefinedtypelist,
                        HGROUPS* hgroups);

void initHGroup(HGROUPS* hgroups);

int getCDF4SubTreeData(LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist, void** data, GROUP* group, HGROUPS* hgroups);

int getCDF4SubTreeUserDefinedTypes(int grpid, GROUPLIST* grouplist, USERDEFINEDTYPELIST* userdefinedtypelist);

int scopedUserDefinedTypes(LOGMALLOCLIST* logmalloclist, int grpid);

void replaceStrings(char** svec, int* ndata, char** dvec, int* ndims);

void replaceEmbeddedStrings(LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist, USERDEFINEDTYPE* udt, int ndata, char* dvec);

unsigned int readCDF4Properties();

extern CDFSUBSET cdfsubset;

extern int IMAS_HDF_READER;

extern nc_type ctype;

extern nc_type dctype;

#endif // !NONETCDFPLUGIN

#endif // IDAM_READCDF4_H
