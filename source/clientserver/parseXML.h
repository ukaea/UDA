#ifndef UDA_CLIENTSERVER_PARSEXML_H
#define UDA_CLIENTSERVER_PARSEXML_H

#ifndef NOXMLPARSER
#  include <libxml/parser.h>
#  include <libxml/xmlmemory.h>
#endif

#include <stdbool.h>

#include "udaDefines.h"
#include "udaStructs.h"

typedef struct Map {
    int nmap;                                             // the Number of Mapping Operations
    double value[UDA_MAX_DATA_RANK];                      // Array of values to Map to
    char mapping[UDA_MAX_DATA_RANK][UDA_SXML_MAX_STRING]; // Array of Mapping Operations
    int dimid[UDA_MAX_DATA_RANK];                         // Array of Dimension IDs to Map to
    char data_signal[UDA_SXML_MAX_STRING];                // Name of Signal
} MAP;

typedef struct DimCalibration {
    double factor;
    double offset;
    int invert;
    char units[UDA_SXML_MAX_STRING];
} DIMCALIBRATION;

typedef struct DimComposite {
    int to_dim;   // duplicated as dimid     // Swap to Dimension ID
    int from_dim; // Swap from Dimension ID
    char file[UDA_SXML_MAX_STRING];
    char format[UDA_SXML_MAX_STRING];
    char dim_signal[UDA_SXML_MAX_STRING];  // Dimension Source Signal Name
    char dim_error[UDA_SXML_MAX_STRING];   // Dimension Error Source Signal Name
    char dim_aserror[UDA_SXML_MAX_STRING]; // Dimension Asymmetric Error Source Signal Name
} DIMCOMPOSITE;

typedef struct DimDocumentation {
    char label[UDA_SXML_MAX_STRING];
    char units[UDA_SXML_MAX_STRING];
} DIMDOCUMENTATION;

typedef struct DimErrorModel {
    int model;   // Error Model Id
    int param_n; // The number of parameters
    // float *params;                            // Parameter Array
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
    char file[UDA_SXML_MAX_STRING];   // Complete file name
    char format[UDA_SXML_MAX_STRING]; // File Format
    char data_signal[UDA_SXML_MAX_STRING];
    char error_signal[UDA_SXML_MAX_STRING];
    char aserror_signal[UDA_SXML_MAX_STRING]; // Asymmetric Error Source Signal Name
    char map_to_signal[UDA_SXML_MAX_STRING]; // straight replacement of signals (useful only if pass range is necessary)
    int order;                               // Identify the Time Dimension
    int ndimensions;
    int nsubsets;
    int nmaps;
    DIMENSION* dimensions;
    SUBSET* subsets;
    MAP* maps;
} COMPOSITE;

typedef struct ErrorModel {
    int model;   // Error Model Id
    int param_n; // The number of parameters
    // float *params;                            // Parameter Array
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
    int nactions;   // Number of Actions
    ACTION* action; // Array of Actions
} ACTIONS;

#ifndef NOXMLPARSER
int parseDoc(char* docname, ACTIONS* actions);
#endif
void printAction(ACTION action);
void printActions(ACTIONS actions);
void initAction(ACTION* act);
void initActions(ACTIONS* act);
void freeActions(ACTIONS* actions);
void copyActions(ACTIONS* actions_out, ACTIONS* actions_in);
void initServerside(SERVERSIDE* act);
void initSubset(SUBSET* act);

#endif // UDA_CLIENTSERVER_PARSEXML_H
