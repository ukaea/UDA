#ifndef UDA_CLIENTSERVER_PARSEXML_H
#define UDA_CLIENTSERVER_PARSEXML_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NOXMLPARSER
#  include <libxml/xmlmemory.h>
#  include <libxml/parser.h>
#endif

#include "udaDefines.h"

#define SXMLMAXSTRING           1024
#define MAXDATARANK             8
#define XMLMAXDESC              1024
#define XMLMAXRECURSIVE         10
#define XMLMAXLOOP              1024

#define DIMCALIBRATIONTYPE      1    // Identifies DIMENSION Union Structures
#define DIMCOMPOSITETYPE        2
#define DIMDOCUMENTATIONTYPE    3
#define DIMERRORMODELTYPE       4

#define TIMEOFFSETTYPE          1    // Identifies ACTION Union Structures
#define DOCUMENTATIONTYPE       2
#define CALIBRATIONTYPE         3
#define COMPOSITETYPE           4
#define ERRORMODELTYPE          5
#define SERVERSIDETYPE          6
#define SUBSETTYPE              7
#define MAPTYPE                 8

typedef struct Subset {
    int nbound;                                 // the Number of Subsetting Operations
    int reform;                                 // reduce Rank if any dimension has length 1
    int order;                                  // Time Dimension order
    double bound[MAXDATARANK];                  // Array of Floating point Bounding values
    long ubindex[MAXDATARANK];                  // Array of Integer values: Bounding or Upper Index
    long lbindex[MAXDATARANK];                  // Array of Integer values: Lower Index
    char operation[MAXDATARANK][SXMLMAXSTRING]; // Array of Subsetting Operations
    int dimid[MAXDATARANK];                     // Array of Dimension IDs to subset
    int isindex[MAXDATARANK];                   // Flag the Operation Bound is an Integer Type
    char data_signal[SXMLMAXSTRING];            // Name of Signal to subset
    char member[SXMLMAXSTRING];                 // Name of Structure Member to extract and to subset
    char function[SXMLMAXSTRING];               // Apply this named function to the subsetted data
} SUBSET;

typedef struct Map {
    int nmap;                                   // the Number of Mapping Operations
    double value[MAXDATARANK];                  // Array of values to Map to
    char mapping[MAXDATARANK][SXMLMAXSTRING];   // Array of Mapping Operations
    int dimid[MAXDATARANK];                     // Array of Dimension IDs to Map to
    char data_signal[SXMLMAXSTRING];            // Name of Signal
} MAP;

typedef struct DimCalibration {
    double factor;
    double offset;
    int invert;
    char units[SXMLMAXSTRING];
} DIMCALIBRATION;

typedef struct DimComposite {
    int to_dim;                                 // duplicated as dimid 	// Swap to Dimension ID
    int from_dim;                               // Swap from Dimension ID
    char file[SXMLMAXSTRING];
    char format[SXMLMAXSTRING];
    char dim_signal[SXMLMAXSTRING];             // Dimension Source Signal Name
    char dim_error[SXMLMAXSTRING];              // Dimension Error Source Signal Name
    char dim_aserror[SXMLMAXSTRING];            // Dimension Asymmetric Error Source Signal Name
} DIMCOMPOSITE;

typedef struct DimDocumentation {
    char label[SXMLMAXSTRING];
    char units[SXMLMAXSTRING];
} DIMDOCUMENTATION;

typedef struct DimErrorModel {
    int model;                                  // Error Model Id
    int param_n;                                // The number of parameters
    //float *params;			                // Parameter Array
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
    char units[SXMLMAXSTRING];
    char target[SXMLMAXSTRING];
    int ndimensions;
    DIMENSION* dimensions;
} CALIBRATION;

typedef struct Documentation {
    char label[SXMLMAXSTRING];
    char units[SXMLMAXSTRING];
    char description[XMLMAXDESC];
    int ndimensions;
    DIMENSION* dimensions;
} DOCUMENTATION;

typedef struct Composite {
    char file[SXMLMAXSTRING];                   // Complete file name
    char format[SXMLMAXSTRING];                 // File Format
    char data_signal[SXMLMAXSTRING];
    char error_signal[SXMLMAXSTRING];
    char aserror_signal[SXMLMAXSTRING];         // Asymmetric Error Source Signal Name
    char map_to_signal[SXMLMAXSTRING];          // straight replacement of signals (useful only if pass range is necessary)
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
    //float *params;			                // Parameter Array
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

double deScale(char * scale);

void parseTargetValue(xmlDocPtr doc, xmlNodePtr cur, char * target, double * value);

void parseTargetString(xmlDocPtr doc, xmlNodePtr cur, char * target, char * str);

void parseDimension(xmlDocPtr doc, xmlNodePtr cur, ACTION * action);

void parseSwapDim(xmlDocPtr doc, xmlNodePtr cur, ACTION * action);

void parseSwap(xmlDocPtr doc, xmlNodePtr cur, ACTION * action);

void parseActionRange(xmlDocPtr doc, xmlNodePtr cur, ACTIONS * actions);

void parseFixedLengthArray(xmlNodePtr cur, char * target, void * array, int arraytype, int * n);

void parseFixedLengthStrArray(xmlNodePtr cur, char * target, char array[MAXDATARANK][SXMLMAXSTRING], int * n);

int parseDoc(char * docname, ACTIONS * actions);

//void parseComposite(xmlDocPtr doc, xmlNodePtr cur, ACTIONS * actions);

void parseDocumentation(xmlDocPtr doc, xmlNodePtr cur, ACTIONS * actions);

void parseCalibration(xmlDocPtr doc, xmlNodePtr cur, ACTIONS * actions);

void parseTimeOffset(xmlDocPtr doc, xmlNodePtr cur, ACTIONS * actions);

void parseErrorModel(xmlDocPtr doc, xmlNodePtr cur, ACTIONS * actions);

void parseSubset(xmlDocPtr doc, xmlNodePtr cur, ACTIONS * actions);

void parseMap(xmlDocPtr doc, xmlNodePtr cur, ACTIONS * actions);

#endif

void printAction(ACTION action);

void printActions(ACTIONS actions);

void initAction(ACTION* act);

void initActions(ACTIONS* act);

void freeActions(ACTIONS* actions);

void copyActions(ACTIONS* actions_out, ACTIONS* actions_in);

void printDimensions(int ndim, DIMENSION* dims);

void initDimCalibration(DIMCALIBRATION* act);

void initDimComposite(DIMCOMPOSITE* act);

void initDimDocumentation(DIMDOCUMENTATION* act);

void initDimErrorModel(DIMERRORMODEL* act);

void initDimension(DIMENSION* act);

void initTimeOffset(TIMEOFFSET* act);

void initCalibration(CALIBRATION* act);

void initDocumentation(DOCUMENTATION* act);

void initComposite(COMPOSITE* act);

void initServerside(SERVERSIDE* act);

void initErrorModel(ERRORMODEL* act);

void initSubset(SUBSET* act);

void initMap(MAP* act);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_PARSEXML_H

