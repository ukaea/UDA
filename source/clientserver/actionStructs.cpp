#include "parseXML.h"

#include <cstdlib>
#include <cstring>

#include <logging/logging.h>
#include <clientserver/udaDiagnostics.h>

#include "actionStructs.h"

// Initialisation, printing, freeing and copying of the ACTION structures. Unlike the XML parser itself this needs no
// libxml2, so it is also built into libuda_plugins for plugins to use.
//==================================================================================================

static void print_dimensions(const UDA_DIAGNOSTICS* diag, int ndim, DIMENSION* dims)
{
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "No. Dimensions     : %d\n", ndim);
    for (int i = 0; i < ndim; i++) {
        UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Dim id     : %d\n", dims[i].dimid);

        switch (dims[i].dimType) {

            case UDA_DIM_CALIBRATION_TYPE:
                UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "factor     : %.12f\n", dims[i].dimcalibration.factor);
                UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Offset     : %.12f\n", dims[i].dimcalibration.offset);
                UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Units      : %s\n", dims[i].dimcalibration.units);
                break;

            case UDA_DIM_COMPOSITE_TYPE:
                UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "to Dim       : %d\n", dims[i].dimcomposite.to_dim);
                UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "from Dim     : %d\n", dims[i].dimcomposite.from_dim);
                UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Dim signal   : %s\n", dims[i].dimcomposite.dim_signal);
                UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Dim Error    : %s\n", dims[i].dimcomposite.dim_error);
                UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Dim ASError  : %s\n", dims[i].dimcomposite.dim_aserror);
                UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Dim Source File  : %s\n", dims[i].dimcomposite.file);
                UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Dim Source Format: %s\n", dims[i].dimcomposite.format);

                break;

            case UDA_DIM_DOCUMENTATION_TYPE:
                UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Dim Label  : %s\n", dims[i].dimdocumentation.label);
                UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Dim Units  : %s\n", dims[i].dimdocumentation.units);
                break;

            case UDA_DIM_ERROR_MODEL_TYPE:
                UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Error Model Id            : %d\n", dims[i].dimerrormodel.model);
                UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Number of Model Parameters: %d\n", dims[i].dimerrormodel.param_n);
                for (int j = 0; j < dims[i].dimerrormodel.param_n; j++)
                    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Parameters[%d] = %.12f\n", j, dims[i].dimerrormodel.params[j]);
                break;

            default:
                break;
        }

    }
}

void udaPrintAction(const UDA_DIAGNOSTICS* diag, ACTION action)
{
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Action XML Id    : %d\n", action.actionId);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Action Type      : %d\n", action.actionType);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "In Range?        : %d\n", action.inRange);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Exp Number Range : %d -> %d\n", action.exp_range[0], action.exp_range[1]);
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Pass Number Range: %d -> %d\n", action.pass_range[0], action.pass_range[1]);

    switch (action.actionType) {
        case UDA_TIME_OFFSET_TYPE:
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "TIMEOFFSET xml\n");
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Method         : %d\n", action.timeoffset.method);
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Time Offset    : %.12f\n", action.timeoffset.offset);
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Time Interval  : %.12f\n", action.timeoffset.interval);
            break;
        case UDA_DOCUMENTATION_TYPE:
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "DOCUMENTATION xml\n");
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Description: %s\n", action.documentation.description);
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Data Label : %s\n", action.documentation.label);
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Data Units : %s\n", action.documentation.units);
            print_dimensions(diag, action.documentation.ndimensions, action.documentation.dimensions);
            break;
        case UDA_CALIBRATION_TYPE:
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "CALIBRATION xml\n");
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Target     : %s\n", action.calibration.target);
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Factor     : %f\n", action.calibration.factor);
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Offset     : %f\n", action.calibration.offset);
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Invert     : %d\n", action.calibration.invert);
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Data Units : %s\n", action.calibration.units);
            print_dimensions(diag, action.calibration.ndimensions, action.calibration.dimensions);
            break;
        case UDA_COMPOSITE_TYPE:
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "COMPOSITE xml\n");
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Composite Data Signal    : %s\n", action.composite.data_signal);
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Composite Error Signal   : %s\n", action.composite.error_signal);
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Composite Asymmetric Error Signal   : %s\n", action.composite.aserror_signal);
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Composite Map to Signal  : %s\n", action.composite.map_to_signal);
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Composite Source File    : %s\n", action.composite.file);
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Composite Source Format  : %s\n", action.composite.format);
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Composite Time Dimension : %d\n", action.composite.order);
            print_dimensions(diag, action.composite.ndimensions, action.composite.dimensions);
            break;
        case UDA_ERROR_MODEL_TYPE:
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "ERRORMODEL xml\n");
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Error Model Id            : %d\n", action.errormodel.model);
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Number of Model Parameters: %d\n", action.errormodel.param_n);
            for (int i = 0; i < action.errormodel.param_n; i++)
                UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Parameters[%d] = %.12f\n", i, action.errormodel.params[i]);
            print_dimensions(diag, action.errormodel.ndimensions, action.errormodel.dimensions);
            break;

        case UDA_SERVER_SIDE_TYPE:
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "SERVERSIDE Actions\n");
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Number of Serverside Subsets: %d\n", action.serverside.nsubsets);
            for (int i = 0; i < action.serverside.nsubsets; i++) {
                UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Number of Subsetting Operations: %d\n", action.serverside.subsets[i].nbound);
                UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Reform?                        : %d\n", action.serverside.subsets[i].reform);
                UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Member                         : %s\n", action.serverside.subsets[i].member);
                UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Function                       : %s\n", action.serverside.subsets[i].function);
                UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Order                          : %d\n", action.serverside.subsets[i].order);
                UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Signal                         : %s\n",
                        action.serverside.subsets[i].data_signal);
                for (int j = 0; j < action.serverside.subsets[i].nbound; j++) {
                    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Bounding Value: %e\n", action.serverside.subsets[i].bound[j]);
                    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Operation     : %s\n", action.serverside.subsets[i].operation[j]);
                    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Dimension ID  : %d\n", action.serverside.subsets[i].dimid[j]);
                }
            }
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Number of Serverside mappings: %d\n", action.serverside.nmaps);
            break;
        case UDA_SUBSET_TYPE:
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "SUBSET Actions\n");
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Number of Subsets: 1\n");
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Number of Subsetting Operations: %d\n", action.subset.nbound);
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Reform?                        : %d\n", action.subset.reform);
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Member                         : %s\n", action.subset.member);
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Function                       : %s\n", action.subset.function);
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Order                       : %d\n", action.subset.order);
            UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Signal                         : %s\n", action.subset.data_signal);
            for (int j = 0; j < action.subset.nbound; j++) {
                UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Bounding Value: %e\n", action.subset.bound[j]);
                UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Operation     : %s\n", action.subset.operation[j]);
                UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "Dimension ID  : %d\n", action.subset.dimid[j]);
            }
            break;
        default:
            break;
    }

}

void udaPrintActions(const UDA_DIAGNOSTICS* diag, ACTIONS actions)
{
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "No. Action Blocks: %d\n", actions.nactions);
    for (int i = 0; i < actions.nactions; i++) {
        UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "\n\n# %d\n", i);
        udaPrintAction(diag, actions.action[i]);
    }
    UDA_DIAG_LOG(diag, UDA_LOG_DEBUG, "\n\n");
}

// Initialise an Action Structure and Child Structures

void init_dim_calibration(DIMCALIBRATION* act)
{
    act->factor = (double)1.0E0;    // Data Calibration Correction/Scaling factor
    act->offset = (double)0.0E0;    // Data Calibration Correction/Scaling offset
    act->invert = 0;            // Don't Invert the data
    act->units[0] = '\0';
}

void init_dim_composite(DIMCOMPOSITE* act)
{
    act->to_dim = -1;                // Swap to Dimension ID
    act->from_dim = -1;                // Swap from Dimension ID
    act->file[0] = '\0';            // Data Source File (with Full Path)
    act->format[0] = '\0';            // Data Source File's Format
    act->dim_signal[0] = '\0';            // Source Signal
    act->dim_error[0] = '\0';            // Error Source Signal
    act->dim_aserror[0] = '\0';            // Asymmetric Error Source Signal
}

void init_dim_documentation(DIMDOCUMENTATION* act)
{
    act->label[0] = '\0';
    act->units[0] = '\0';            // Lower in priority than Calibration Units
}

void init_dim_error_model(DIMERRORMODEL* act)
{
    act->model = ERROR_MODEL_UNKNOWN;    // No Error Model
    act->param_n = 0;            // No. Model parameters
    for (int i = 0; i < MAXERRPARAMS; i++) {
        act->params[i] = 0.0;
    }
}

void init_dimension(DIMENSION* act)
{
    act->dimid = -1;        // Dimension Id
    act->dimType = 0;        // Structure Type
}

void init_time_offset(TIMEOFFSET* act)
{
    act->method = 0;            // Correction Method: Standard offset correction only
    act->offset = (double)0.0E0;    // Time Dimension offset correction or start time
    act->interval = (double)0.0E0;    // Time Dimension Interval correction
}

void init_calibration(CALIBRATION* act)
{
    act->factor = (double)1.0E0;    // Data Calibration Correction/Scaling factor
    act->offset = (double)0.0E0;    // Data Calibration Correction/Scaling offset
    act->units[0] = '\0';
    act->target[0] = '\0';        // Which data Component to apply calibration? (all, data, error, aserror)
    act->invert = 0;        // No Inversion
    act->ndimensions = 0;
    act->dimensions = nullptr;
}

void init_documentation(DOCUMENTATION* act)
{
    act->label[0] = '\0';
    act->units[0] = '\0';        // Lower in priority than Calibration Units
    act->description[0] = '\0';
    act->ndimensions = 0;
    act->dimensions = nullptr;
}

void init_composite(COMPOSITE* act)
{
    act->data_signal[0] = '\0';            // Derived Data using this Data Source
    act->error_signal[0] = '\0';            // Use Errors from this Source
    act->aserror_signal[0] = '\0';            // Use Asymmetric Errors from this Source
    act->map_to_signal[0] = '\0';            // Straight swap of data: map to this signal
    act->file[0] = '\0';            // Data Source File (with Full Path)
    act->format[0] = '\0';            // Data Source File's Format
    act->order = -1;            // Identify the Time Dimension
    act->ndimensions = 0;
    act->nsubsets = 0;
    act->nmaps = 0;
    act->dimensions = nullptr;
    act->subsets = nullptr;
    act->maps = nullptr;
}

void initServerside(SERVERSIDE* act)
{
    act->nsubsets = 0;
    act->nmaps = 0;
    act->subsets = nullptr;
    act->maps = nullptr;
}

void init_error_model(ERRORMODEL* act)
{
    act->model = ERROR_MODEL_UNKNOWN;    // No Error Model
    act->param_n = 0;            // No. Model parameters
    for (int i = 0; i < MAXERRPARAMS; i++) {
        act->params[i] = 0.0;
    }
    act->ndimensions = 0;
    act->dimensions = nullptr;
}

void initSubset(SUBSET* act)
{
    for (int i = 0; i < UDA_MAX_DATA_RANK; i++) {
        act->bound[i] = 0.0;                // Subsetting Float Bounds
        act->ubindex[i] = { .init = false, .value = 0 };                // Subsetting Integer Bounds (Upper Index)
        act->lbindex[i] = { .init = false, .value = 0 };                // Lower Index
        act->stride[i] = { .init = false, .value = 0 };               // Stride
        act->isindex[i] = false;                // Flag the Bound is an Integer Type
        act->dimid[i] = -1;                // Dimension IDs
        act->operation[i][0] = '\0';            // Subsetting Operations
    }
    act->data_signal[0] = '\0';                // Data to Read
    act->member[0] = '\0';                // Structure Member to target
    act->function[0] = '\0';                // Name of simple function to apply
    act->nbound = 0;                // The number of Subsetting Operations
    act->reform = 0;                // reduce the Rank if a subsetted dimension has length 1
    act->order = -1;                // Explicitly set the order of the time dimension if >= 0
}

// Initialise an Action Structure

void initAction(ACTION* act)
{
    act->actionType = 0;        // Action Range Type
    act->inRange = 0;        // Is this Action Record Applicable to the Current data Request?
    act->actionId = 0;        // Action XML Tag Id
    act->exp_range[0] = 0;        // Applicable over this Exp. Number Range
    act->exp_range[1] = 0;
    act->pass_range[0] = -1;        // Applicable over this Pass Number Range
    act->pass_range[1] = -1;
}

// Initialise an Action Array Structure

void initActions(ACTIONS* act)
{
    act->nactions = 0;        // Number of Action blocks
    act->action = nullptr;    // Array of Action blocks
}

void freeActions(ACTIONS* actions)
{
    // Free Heap Memory From ACTION Structures
    void* cptr;

    UDA_LOG(UDA_LOG_DEBUG, "freeActions: Enter\n");
    UDA_LOG(UDA_LOG_DEBUG, "freeDataBlock: Number of Actions = %d \n", actions->nactions);

    if (actions->nactions == 0) return;

    for (int i = 0; i < actions->nactions; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "freeDataBlock: freeing action Type = %d \n", actions->action[i].actionType);

        switch (actions->action[i].actionType) {

            case UDA_COMPOSITE_TYPE:
                if ((cptr = (void*)actions->action[i].composite.dimensions) != nullptr) {
                    free(cptr);
                    actions->action[i].composite.dimensions = nullptr;
                    actions->action[i].composite.ndimensions = 0;
                }
                if (actions->action[i].composite.nsubsets > 0) {
                    if ((cptr = (void*)actions->action[i].composite.subsets) != nullptr) free(cptr);
                    actions->action[i].composite.subsets = nullptr;
                    actions->action[i].composite.nsubsets = 0;
                }
                if (actions->action[i].composite.nmaps > 0) {
                    if ((cptr = (void*)actions->action[i].composite.maps) != nullptr) free(cptr);
                    actions->action[i].composite.maps = nullptr;
                    actions->action[i].composite.nmaps = 0;
                }
                break;

            case UDA_ERROR_MODEL_TYPE:
                actions->action[i].errormodel.param_n = 0;

                for (int j = 0; j < actions->action[i].errormodel.ndimensions; j++)
                    if ((cptr = (void*)actions->action[i].errormodel.dimensions) != nullptr) {
                        free(cptr);
                        actions->action[i].errormodel.dimensions = nullptr;
                        actions->action[i].errormodel.ndimensions = 0;
                    }

                break;

            case UDA_CALIBRATION_TYPE:
                if ((cptr = (void*)actions->action[i].calibration.dimensions) != nullptr) {
                    free(cptr);
                    actions->action[i].calibration.dimensions = nullptr;
                    actions->action[i].calibration.ndimensions = 0;
                }
                break;

            case UDA_DOCUMENTATION_TYPE:
                if ((cptr = (void*)actions->action[i].documentation.dimensions) != nullptr) {
                    free(cptr);
                    actions->action[i].documentation.dimensions = nullptr;
                    actions->action[i].documentation.ndimensions = 0;
                }
                break;

            case UDA_SERVER_SIDE_TYPE:
                if (actions->action[i].serverside.nsubsets > 0) {
                    if ((cptr = (void*)actions->action[i].serverside.subsets) != nullptr) free(cptr);
                    actions->action[i].serverside.subsets = nullptr;
                    actions->action[i].serverside.nsubsets = 0;
                }
                if (actions->action[i].serverside.nmaps > 0) {
                    if ((cptr = (void*)actions->action[i].serverside.maps) != nullptr) free(cptr);
                    actions->action[i].serverside.maps = nullptr;
                    actions->action[i].serverside.nmaps = 0;
                }
                break;

            default:
                break;
        }
    }

    if ((cptr = (void*)actions->action) != nullptr) free(cptr);
    actions->nactions = 0;
    actions->action = nullptr;

    UDA_LOG(UDA_LOG_DEBUG, "freeActions: Exit\n");
}

// Copy an Action Structure and Drop Pointers to ACTION & DIMENSION Structures (ensures a single Heap free later)

void copyActions(ACTIONS* actions_out, ACTIONS* actions_in)
{
    *actions_out = *actions_in;
    actions_in->action = nullptr;
    actions_in->nactions = 0;
}

#ifndef UDA_COMMON_BUILD

#include <clientserver/errorLog.h>

void printAction(ACTION action)
{
    UDA_DIAGNOSTICS diag = udaGlobalDiagnostics();
    udaPrintAction(&diag, action);
}

void printActions(ACTIONS actions)
{
    UDA_DIAGNOSTICS diag = udaGlobalDiagnostics();
    udaPrintActions(&diag, actions);
}

#endif // UDA_COMMON_BUILD
