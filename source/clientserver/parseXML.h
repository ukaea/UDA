#ifndef UDA_CLIENTSERVER_PARSEXML_H
#define UDA_CLIENTSERVER_PARSEXML_H

#ifndef NOXMLPARSER
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#endif

#include "udaDefines.h"
#include "export.h"

#define UDA_SXML_MAX_STRING           1024
#define UDA_MAX_DATA_RANK             8
#define UDA_XML_MAX_DESC              1024
#define UDA_XML_MAX_RECURSIVE         10
#define UDA_XML_MAX_LOOP              1024

#define UDA_DIM_CALIBRATION_TYPE      1    // Identifies DIMENSION Union Structures
#define UDA_DIM_COMPOSITE_TYPE        2
#define UDA_DIM_DOCUMENTATION_TYPE    3
#define UDA_DIM_ERROR_MODEL_TYPE      4

#define UDA_TIME_OFFSET_TYPE    1    // Identifies ACTION Union Structures
#define UDA_DOCUMENTATION_TYPE  2
#define UDA_CALIBRATION_TYPE    3
#define UDA_COMPOSITE_TYPE      4
#define UDA_ERROR_MODEL_TYPE    5
#define UDA_SERVER_SIDE_TYPE    6
#define UDA_SUBSET_TYPE         7
#define UDA_MAP_TYPE            8

typedef struct OptionalLong {
    bool init;
    long value;
} OPTIONAL_LONG;

typedef struct Subset {
    int nbound;                                 // the Number of Subsetting Operations
    int reform;                                 // reduce Rank if any dimension has length 1
    int order;                                  // Time Dimension order
    double bound[UDA_MAX_DATA_RANK];                  // Array of Floating point Bounding values
    OPTIONAL_LONG stride[UDA_MAX_DATA_RANK];                   // Array of Integer values: Striding values
    OPTIONAL_LONG ubindex[UDA_MAX_DATA_RANK];                  // Array of Integer values: Bounding or Upper Index
    OPTIONAL_LONG lbindex[UDA_MAX_DATA_RANK];                  // Array of Integer values: Lower Index
    char operation[UDA_MAX_DATA_RANK][UDA_SXML_MAX_STRING];           // Array of Subsetting Operations
    int dimid[UDA_MAX_DATA_RANK];                     // Array of Dimension IDs to subset
    bool isindex[UDA_MAX_DATA_RANK];                   // Flag the Operation Bound is an Integer Type
    char data_signal[UDA_SXML_MAX_STRING];            // Name of Signal to subset
    char member[UDA_SXML_MAX_STRING];                 // Name of Structure Member to extract and to subset
    char function[UDA_SXML_MAX_STRING];               // Apply this named function to the subsetted data
} SUBSET;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Map {
    int nmap;                                   // the Number of Mapping Operations
    double value[UDA_MAX_DATA_RANK];                  // Array of values to Map to
    char mapping[UDA_MAX_DATA_RANK][UDA_SXML_MAX_STRING];   // Array of Mapping Operations
    int dimid[UDA_MAX_DATA_RANK];                     // Array of Dimension IDs to Map to
    char data_signal[UDA_SXML_MAX_STRING];            // Name of Signal
} MAP;

typedef struct DimCalibration {
    double factor;
    double offset;
    int invert;
    char units[UDA_SXML_MAX_STRING];
} DIMCALIBRATION;

typedef struct DimComposite {
    int to_dim;                                 // duplicated as dimid     // Swap to Dimension ID
    int from_dim;                               // Swap from Dimension ID
    char file[UDA_SXML_MAX_STRING];
    char format[UDA_SXML_MAX_STRING];
    char dim_signal[UDA_SXML_MAX_STRING];             // Dimension Source Signal Name
    char dim_error[UDA_SXML_MAX_STRING];              // Dimension Error Source Signal Name
    char dim_aserror[UDA_SXML_MAX_STRING];            // Dimension Asymmetric Error Source Signal Name
} DIMCOMPOSITE;

typedef struct DimDocumentation {
    char label[UDA_SXML_MAX_STRING];
    char units[UDA_SXML_MAX_STRING];
} DIMDOCUMENTATION;

typedef struct DimErrorModel {
    int model;                                  // Error Model Id
    int param_n;                                // The number of parameters
    //float *params;                            // Parameter Array
    float params[MAXERRPARAMS];
} DIMERRORMODEL;

typedef struct Dimension {
    int dimType;
    int dimid;
    union {
        DIMCALIBRATION dimcalibration;
        DIMCOMPOSITE dimcomposite;
        DIMDOCUMENTATION dimdocumentation;
        DIMERRORMODEL dimerrormodel;
    };
} DIMENSION;

typedef struct TimeOffset {
    double offset;
    double interval;
    int method;
} TIMEOFFSET;

typedef struct Calibration {
    double factor;
    double offset;
    int invert;
    char units[UDA_SXML_MAX_STRING];
    char target[UDA_SXML_MAX_STRING];
    int ndimensions;
    DIMENSION* dimensions;
} CALIBRATION;

typedef struct Documentation {
    char label[UDA_SXML_MAX_STRING];
    char units[UDA_SXML_MAX_STRING];
    char description[UDA_XML_MAX_DESC];
    int ndimensions;
    DIMENSION* dimensions;
} DOCUMENTATION;

typedef struct Composite {
    char file[UDA_SXML_MAX_STRING];                   // Complete file name
    char format[UDA_SXML_MAX_STRING];                 // File Format
    char data_signal[UDA_SXML_MAX_STRING];
    char error_signal[UDA_SXML_MAX_STRING];
    char aserror_signal[UDA_SXML_MAX_STRING];         // Asymmetric Error Source Signal Name
    char map_to_signal[UDA_SXML_MAX_STRING];          // straight replacement of signals (useful only if pass range is necessary)
    int order;                                  // Identify the Time Dimension
    int ndimensions;
    int nsubsets;
    int nmaps;
    DIMENSION* dimensions;
    SUBSET* subsets;
    MAP* maps;
} COMPOSITE;

typedef struct ErrorModel {
    int model;                                  // Error Model Id
    int param_n;                                // The number of parameters
    //float *params;                            // Parameter Array
    float params[MAXERRPARAMS];
    int ndimensions;
    DIMENSION* dimensions;
} ERRORMODEL;

typedef struct ServerSide {
    int nsubsets;
    int nmaps;
    SUBSET* subsets;
    MAP* maps;
} SERVERSIDE;

typedef struct Action {
    int actionType;
    int inRange;
    int actionId;
    int exp_range[2];
    int pass_range[2];

    union {
        TIMEOFFSET timeoffset;
        DOCUMENTATION documentation;
        CALIBRATION calibration;
        COMPOSITE composite;
        ERRORMODEL errormodel;
        SERVERSIDE serverside;
        SUBSET subset;
        MAP map;
    };

} ACTION;

typedef struct Actions {
    int nactions;                           // Number of Actions
    ACTION* action;                         // Array of Actions
} ACTIONS;

#ifndef NOXMLPARSER
LIBRARY_API int parseDoc(char* docname, ACTIONS* actions);
#endif
LIBRARY_API void printAction(const ACTION& action);
LIBRARY_API void printActions(ACTIONS actions);
LIBRARY_API void initAction(ACTION* act);
LIBRARY_API void initActions(ACTIONS* act);
LIBRARY_API void freeActions(ACTIONS* actions);
LIBRARY_API void copyActions(ACTIONS* actions_out, ACTIONS* actions_in);
LIBRARY_API void initServerside(SERVERSIDE* act);
LIBRARY_API void initSubset(SUBSET* act);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_PARSEXML_H
