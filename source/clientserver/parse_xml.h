#pragma once

#include "uda_defines.h"
#include "uda_structs.h"

namespace uda::client_server
{

struct Map {
    int nmap;                                             // the Number of Mapping Operations
    double value[MaxDataRank];                      // Array of values to Map to
    char mapping[MaxDataRank][SxmlMaxString]; // Array of Mapping Operations
    int dimid[MaxDataRank];                         // Array of Dimension IDs to Map to
    char data_signal[SxmlMaxString];                // Name of Signal
};

struct DimCalibration {
    double factor;
    double offset;
    int invert;
    char units[SxmlMaxString];
};

struct DimComposite {
    int to_dim;   // duplicated as dimid     // Swap to Dimension ID
    int from_dim; // Swap from Dimension ID
    char file[SxmlMaxString];
    char format[SxmlMaxString];
    char dim_signal[SxmlMaxString];  // Dimension Source Signal Name
    char dim_error[SxmlMaxString];   // Dimension Error Source Signal Name
    char dim_aserror[SxmlMaxString]; // Dimension Asymmetric Error Source Signal Name
};

struct DimDocumentation {
    char label[SxmlMaxString];
    char units[SxmlMaxString];
};

struct DimErrorModel {
    int model;   // Error Model Id
    int param_n; // The number of parameters
    // float *params;                            // Parameter Array
    float params[MaxErrParams];
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
    char units[SxmlMaxString];
    char target[SxmlMaxString];
    int ndimensions;
    Dimension* dimensions;
};

struct Documentation {
    char label[SxmlMaxString];
    char units[SxmlMaxString];
    char description[XmlMaxDesc];
    int ndimensions;
    Dimension* dimensions;
};

struct Composite {
    char file[SxmlMaxString];   // Complete file name
    char format[SxmlMaxString]; // File Format
    char data_signal[SxmlMaxString];
    char error_signal[SxmlMaxString];
    char aserror_signal[SxmlMaxString]; // Asymmetric Error Source Signal Name
    char map_to_signal[SxmlMaxString]; // straight replacement of signals (useful only if pass range is necessary)
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
    float params[MaxErrParams];
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

int parse_doc(std::vector<UdaError>& error_stack, const char* doc_name, Actions* actions);

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
