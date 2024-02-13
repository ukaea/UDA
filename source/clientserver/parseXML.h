#pragma once

#ifndef NOXMLPARSER
#  include <libxml/parser.h>
#  include <libxml/xmlmemory.h>
#endif

#include <stdbool.h>

#include "udaDefines.h"
#include "udaStructs.h"

namespace uda::client_server
{

struct Map {
    int nmap;                                             // the Number of Mapping Operations
    double value[UDA_MAX_DATA_RANK];                      // Array of values to Map to
    char mapping[UDA_MAX_DATA_RANK][UDA_SXML_MAX_STRING]; // Array of Mapping Operations
    int dimid[UDA_MAX_DATA_RANK];                         // Array of Dimension IDs to Map to
    char data_signal[UDA_SXML_MAX_STRING];                // Name of Signal
};

struct DimCalibration {
    double factor;
    double offset;
    int invert;
    char units[UDA_SXML_MAX_STRING];
};

struct DimComposite {
    int to_dim;   // duplicated as dimid     // Swap to Dimension ID
    int from_dim; // Swap from Dimension ID
    char file[UDA_SXML_MAX_STRING];
    char format[UDA_SXML_MAX_STRING];
    char dim_signal[UDA_SXML_MAX_STRING];  // Dimension Source Signal Name
    char dim_error[UDA_SXML_MAX_STRING];   // Dimension Error Source Signal Name
    char dim_aserror[UDA_SXML_MAX_STRING]; // Dimension Asymmetric Error Source Signal Name
};

struct DimDocumentation {
    char label[UDA_SXML_MAX_STRING];
    char units[UDA_SXML_MAX_STRING];
};

struct DimErrorModel {
    int model;   // Error Model Id
    int param_n; // The number of parameters
    // float *params;                            // Parameter Array
    float params[MAXERRPARAMS];
};

struct Dimension {
    int dimType;
    int dimid;
    union {
        DimCalibration dimcalibration;
        DimComposite dimcomposite;
        DimDocumentation dimdocumentation;
        DimErrorModel dimerrormodel;
    };
};

struct TimeOffset {
    double offset;
    double interval;
    int method;
};

struct Calibration {
    double factor;
    double offset;
    int invert;
    char units[UDA_SXML_MAX_STRING];
    char target[UDA_SXML_MAX_STRING];
    int ndimensions;
    Dimension* dimensions;
};

struct Documentation {
    char label[UDA_SXML_MAX_STRING];
    char units[UDA_SXML_MAX_STRING];
    char description[UDA_XML_MAX_DESC];
    int ndimensions;
    Dimension* dimensions;
};

struct Composite {
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
    Dimension* dimensions;
    Subset* subsets;
    Map* maps;
};

struct ErrorModel {
    int model;   // Error Model Id
    int param_n; // The number of parameters
    // float *params;                            // Parameter Array
    float params[MAXERRPARAMS];
    int ndimensions;
    Dimension* dimensions;
};

struct ServerSide {
    int nsubsets;
    int nmaps;
    Subset* subsets;
    Map* maps;
};

struct Action {
    int actionType;
    int inRange;
    int actionId;
    int exp_range[2];
    int pass_range[2];

    union {
        TimeOffset timeoffset;
        Documentation documentation;
        Calibration calibration;
        Composite composite;
        ErrorModel errormodel;
        ServerSide serverside;
        Subset subset;
        Map map;
    };

};

struct Actions {
    int nactions;   // Number of Actions
    Action* action; // Array of Actions
};

#ifndef NOXMLPARSER

int parse_doc(char* docname, Actions* actions);

#endif

void print_action(Action action);

void print_actions(Actions actions);

void init_action(Action* act);

void init_actions(Actions* act);

void free_actions(Actions* actions);

void copy_actions(Actions* actions_out, Actions* actions_in);

void init_server_side(ServerSide* act);

void init_subset(Subset* act);

} // namespace uda::client_server
