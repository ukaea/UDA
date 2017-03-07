//
// IDL DLM C Code Wrapper to netCDF4 API functions for writing netCDF4 data files
//

#include "putdata.h"		// IDL API Headers

#include <stdlib.h>
#include <hdf5.h>

#include <clientserver/udaTypes.h>
#include <clientserver/stringUtils.h>
#include <logging/accessLog.h>
#include <client/accAPI.h>
#include <client/udaPutAPI.h>

// Function Prototypes

void userhelp(FILE* fh, char* name);

static void putdata_exit_handler(void);

static int idamType(UCHAR idl_type);

static PUTDATA_BLOCK toPutData(IDL_VPTR data);

extern IDL_VPTR IDL_CDECL putdata(int argc, IDL_VPTR argv[], char* argk);

static IDL_SYSFUN_DEF2 putdata_functions[] = {
        {{(IDL_FUN_RET) putdata}, "PUTDATA", 0, 4, IDL_SYSFUN_DEF_F_KEYWORDS, 0}
};

int putdata_startup(void) {

    // TRUE for Functions, FALSE for Procedures

    if (!IDL_SysRtnAdd(putdata_functions, TRUE, ARRLEN(putdata_functions))) { return IDL_FALSE; }

    // Register the exit handler

    IDL_ExitRegister(putdata_exit_handler);

    return IDL_TRUE;
}

int IDL_Load(void) {
    if (!IDL_SysRtnAdd(putdata_functions, TRUE, ARRLEN(putdata_functions))) { return IDL_FALSE; }
    return IDL_TRUE;
}

// Called when IDL is shutdown

void putdata_exit_handler(void) {
    // Nothing to do!
}

void freeMem(UCHAR* memPtr) {
    free((void*) memPtr);
}

void initKW(KW_RESULT* kw) {
    kw->data = NULL;
    kw->create = 0;
    kw->close = 0;
    kw->update = 0;
    kw->delete = 0;

    kw->notstrict = 0;
    kw->nocompliance = 0;

    kw->verbose = 0;
    kw->debug = 0;
    kw->fileid = 0;
    kw->length = 0;
    kw->resolution = 0;
    kw->channels = 0;
    kw->channel = 0;
    kw->compression = 0;
    kw->chunksize = 0;
    kw->scale = 0.0;
    kw->offset = 0.0;
    kw->range[0] = 0.0;
    kw->range[1] = 0.0;

    kw->pass = 0;

    kw->is_date = 0;
    kw->is_dimensions = 0;
    kw->is_stepId = 0;
    kw->is_group = 0;
    kw->is_label = 0;
    kw->is_filename = 0;
    kw->is_errors = 0;
    kw->is_units = 0;
    kw->is_length = 0;
    kw->is_device = 0;
    kw->is_serial = 0;
    kw->is_resolution = 0;
    kw->is_range = 0;
    kw->is_channel = 0;
    kw->is_channels = 0;
    kw->is_scale = 0;
    kw->is_offset = 0;
    kw->is_name = 0;
    kw->is_link = 0;
    kw->is_compression = 0;
    kw->is_chunksize = 0;

    kw->is_conventions = 0;
    kw->is_code = 0;
    kw->is_date = 0;
    kw->is_exp_number = 0;
    kw->is_fileid = 0;
    kw->is_format = 0;
    kw->is_directory = 0;
    kw->is_pass = 0;
    kw->is_status = 0;
    kw->is_time = 0;
    kw->is_version = 0;
    kw->is_xml = 0;
    kw->is_class = 0;
    kw->is_title = 0;
    kw->is_comment = 0;

    return;
}

void printKW(FILE* fd, KW_RESULT kw) {

    fprintf(fd, "debug?   %d\n", (int) kw.debug);
    fprintf(fd, "verbose? %d\n", (int) kw.verbose);

    fprintf(fd, "create?  %d\n", (int) kw.create);
    fprintf(fd, "close?   %d\n", (int) kw.close);
    fprintf(fd, "update?  %d\n", (int) kw.update);
    fprintf(fd, "delete?  %d\n", (int) kw.delete);

    fprintf(fd, "Not Strict?             %d\n", (int) kw.notstrict);
    fprintf(fd, "No Compliance Testing?  %d\n", (int) kw.nocompliance);

    if (kw.is_exp_number) fprintf(fd, "exp_number  %d\n", (int) kw.exp_number);
    if (kw.is_pass) fprintf(fd, "pass        %d\n", (int) kw.pass);
    if (kw.is_status) fprintf(fd, "status      %d\n", (int) kw.status);
    if (kw.is_version) fprintf(fd, "version     %d\n", (int) kw.version);
    if (kw.is_length) fprintf(fd, "length      %d\n", (int) kw.length);
    if (kw.unlimited) fprintf(fd, "unlimited?  %d\n", (int) kw.unlimited);
    if (kw.is_channels) fprintf(fd, "channels    %d\n", (int) kw.channels);
    if (kw.is_channel) fprintf(fd, "channel     %d\n", (int) kw.channel);
    if (kw.is_resolution) fprintf(fd, "resolution  %d\n", (int) kw.resolution);
    if (kw.is_compression) fprintf(fd, "compression %d\n", (int) kw.compression);
    if (kw.is_chunksize) fprintf(fd, "chunksize   %d\n", (int) kw.chunksize);
    if (kw.is_scale) fprintf(fd, "scale       %f\n", (float) kw.scale);
    if (kw.is_offset) fprintf(fd, "offset      %f\n", (float) kw.offset);
    if (kw.is_range) fprintf(fd, "range       %f : %f \n", (float) kw.range[0], (float) kw.range[1]);
    if (kw.is_fileid) fprintf(fd, "fileId      %d\n", (int) IDL_LongScalar(kw.fileid));

    if (kw.is_conventions) fprintf(fd, "conventions %s\n", IDL_STRING_STR(&kw.conventions));
    if (kw.is_class) fprintf(fd, "class       %s\n", IDL_STRING_STR(&kw.class));
    if (kw.is_title) fprintf(fd, "title       %s\n", IDL_STRING_STR(&kw.title));
    if (kw.is_comment) fprintf(fd, "comment     %s\n", IDL_STRING_STR(&kw.comment));
    if (kw.is_code) fprintf(fd, "code        %s\n", IDL_STRING_STR(&kw.code));
    if (kw.is_format) fprintf(fd, "format      %s\n", IDL_STRING_STR(&kw.format));
    if (kw.is_directory) fprintf(fd, "directory   %s\n", IDL_STRING_STR(&kw.directory));
    if (kw.is_date) fprintf(fd, "date        %s\n", IDL_STRING_STR(&kw.date));
    if (kw.is_time) fprintf(fd, "time        %s\n", IDL_STRING_STR(&kw.time));
    if (kw.is_xml) fprintf(fd, "xml         %s\n", IDL_STRING_STR(&kw.xml));
    if (kw.is_stepId) fprintf(fd, "stepId      %s\n", IDL_STRING_STR(&kw.stepId));

    if (kw.is_group) fprintf(fd, "group       %s\n", IDL_STRING_STR(&kw.group));
    if (kw.is_filename) fprintf(fd, "filename        %s\n", IDL_STRING_STR(&kw.filename));
    if (kw.is_errors) fprintf(fd, "errors      %s\n", IDL_STRING_STR(&kw.errors));
    if (kw.is_dimensions) fprintf(fd, "dimensions  %s\n", IDL_STRING_STR(&kw.dimensions));
    if (kw.is_device) fprintf(fd, "device      %s\n", IDL_STRING_STR(&kw.device));
    if (kw.is_id) fprintf(fd, "id          %s\n", IDL_STRING_STR(&kw.id));
    if (kw.is_serial) fprintf(fd, "serial      %s\n", IDL_STRING_STR(&kw.serial));
    if (kw.is_name) fprintf(fd, "name     %s\n", IDL_STRING_STR(&kw.name));
    if (kw.is_units) fprintf(fd, "units       %s\n", IDL_STRING_STR(&kw.units));
    if (kw.is_label) fprintf(fd, "label       %s\n", IDL_STRING_STR(&kw.label));

    return;
}

//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
// API Function

IDL_VPTR IDL_CDECL putdata(int argc, IDL_VPTR argv[], char* argk) {
    //---------------------------------------------------------------------------
    // Maintain Alphabetical Order of Keywords
    // Keywords are IDL LONG, IDL DOUBLE and IDL_STRING types only.

    static IDL_KW_ARR_DESC_R rangeDesc = {IDL_KW_OFFSETOF(range), 2, 2, IDL_KW_OFFSETOF(rangeCount)};

    static IDL_KW_PAR kw_pars[] =
            {
                    IDL_KW_FAST_SCAN,
                    {"DATA", IDL_TYP_UNDEF, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_data), IDL_KW_OFFSETOF(data)},
                    {"CHANNELS", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_channels), IDL_KW_OFFSETOF(channels)},
                    {"CHUNKSIZE", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_chunksize),
                     IDL_KW_OFFSETOF(chunksize)},
                    {"CLASS", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_class), IDL_KW_OFFSETOF(class)},
                    {"CLOSE", IDL_TYP_LONG, 0, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(close)},
                    {"CODE", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_code), IDL_KW_OFFSETOF(code)},
                    {"COMMENT", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_comment), IDL_KW_OFFSETOF(comment)},
                    {"COMPRESSION", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_compression),
                     IDL_KW_OFFSETOF(compression)},
                    {"CONVENTIONS", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_conventions),
                     IDL_KW_OFFSETOF(conventions)},
                    {"CREATE", IDL_TYP_LONG, 0, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(create)},
                    {"DATE", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_date), IDL_KW_OFFSETOF(date)},
                    {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                    {"DELETE", IDL_TYP_LONG, 0, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(delete)},
                    {"DEVICE", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_device), IDL_KW_OFFSETOF(device)},
                    {"TYPE", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_type), IDL_KW_OFFSETOF(type)},
                    {"DIMENSIONS", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_dimensions),
                     IDL_KW_OFFSETOF(dimensions)},
                    {"DIRECTORY", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_directory),
                     IDL_KW_OFFSETOF(directory)},
                    {"ERRORS", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_errors), IDL_KW_OFFSETOF(errors)},
                    {"EXP_NUMBER", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_exp_number),
                     IDL_KW_OFFSETOF(exp_number)},
                    {"FILEID", IDL_TYP_UNDEF, 1, IDL_KW_OUT | IDL_KW_ZERO, IDL_KW_OFFSETOF(is_fileid),
                     IDL_KW_OFFSETOF(fileid)},
                    {"FORMAT", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_format), IDL_KW_OFFSETOF(format)},
                    {"GROUP", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_group), IDL_KW_OFFSETOF(group)},
                    {"ID", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_id), IDL_KW_OFFSETOF(id)},
                    {"LABEL", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_label), IDL_KW_OFFSETOF(label)},
                    {"LENGTH", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_length), IDL_KW_OFFSETOF(length)},
                    {"FILENAME", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_filename),
                     IDL_KW_OFFSETOF(filename)},
                    {"NOCOMPLIANCE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(nocompliance)},
                    {"NOTSTRICT", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(notstrict)},
                    {"OFFSET", IDL_TYP_DOUBLE, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_offset), IDL_KW_OFFSETOF(offset)},
                    {"PASS", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_pass), IDL_KW_OFFSETOF(pass)},
                    {"PULSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_exp_number),
                     IDL_KW_OFFSETOF(exp_number)},
                    {"RANGE", IDL_TYP_DOUBLE, 1, IDL_KW_ARRAY, IDL_KW_OFFSETOF(is_range), IDL_CHARA(rangeDesc)},
                    {"RESOLUTION", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_resolution),
                     IDL_KW_OFFSETOF(resolution)},
                    {"SCALE", IDL_TYP_DOUBLE, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_scale), IDL_KW_OFFSETOF(scale)},
                    {"SERIAL", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_serial), IDL_KW_OFFSETOF(serial)},
                    {"SHOT", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_exp_number), IDL_KW_OFFSETOF(exp_number)},
                    {"STATUS", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_status), IDL_KW_OFFSETOF(status)},
                    {"STEPID", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_stepId), IDL_KW_OFFSETOF(stepId)},
                    {"TIME", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_time), IDL_KW_OFFSETOF(time)},
                    {"TITLE", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_title), IDL_KW_OFFSETOF(title)},
                    {"UNITS", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_units), IDL_KW_OFFSETOF(units)},
                    {"UNLIMITED", IDL_TYP_STRING, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(unlimited)},
                    {"UPDATE", IDL_TYP_LONG, 0, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(update)},
                    {"VARNAME", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_name), IDL_KW_OFFSETOF(name)},
                    {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                    {"VERSION", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_version), IDL_KW_OFFSETOF(version)},
                    {"XML", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_xml), IDL_KW_OFFSETOF(xml)},
                    {"STARTS", IDL_TYP_DOUBLE, 1, IDL_KW_ARRAY, IDL_KW_OFFSETOF(is_starts), IDL_KW_OFFSETOF(xml)},
                    {NULL}
            };

    KW_RESULT kw;

    initKW(&kw);        // Initialise the Keyword Structure

    //---------------------------------------------------------------------------
    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    //---------------------------------------------------------------------------
    // Extract Keywords

    char* stepId = NULL;

    if (kw.is_stepId) {
        stepId = IDL_STRING_STR(&kw.stepId);

        do {
            if (STR_IEQUALS(stepId, "create")) {
                kw.create = 1;
                break;
            }
            if (STR_IEQUALS(stepId, "close")) {
                kw.close = 1;
                break;
            }
            if (STR_IEQUALS(stepId, "update")) {
                kw.update = 1;
                break;
            }
            if (STR_IEQUALS(stepId, "delete")) {
                kw.delete = 1;
                break;
            }
        } while (0);

    } else {
        if (kw.verbose) fprintf(stderr, "Please Identify the file operation step, e.g, dimension\n");
        IDL_KW_FREE;
        return IDL_GettmpLong(-1);
    }

    //---------------------------------------------------------------------------
    // Previous File ID? (i.e. the file is open)

    int fileid = 0;

    if (!kw.create && !kw.update) {
        if (kw.is_fileid) {
            fileid = (int) IDL_LongScalar(kw.fileid);
        } else {
            if (kw.verbose)
                fprintf(stderr, "FileId must be provided for all steps other than create, update and delete\n");
            IDL_KW_FREE;
            return IDL_GettmpLong(-1);
        }
    }

    //--------------------------------------------------------------------------
    // Create an Error Trap

    int err = 0;

    do {
        //---------------------------------------------------------------------------
        // Create or Close the File?

        if (kw.create || kw.close || kw.update || kw.delete) {
            if (!kw.close) {
                err = opennetcdf(&kw, &fileid);    // Open the file

                // Return the fileid if a keyword was passed

                if (kw.is_fileid && fileid != INT_MAX) {
                    IDL_StoreScalar(kw.fileid, IDL_TYP_LONG, (IDL_ALLTYPES*) &fileid);
                }
            } else {
                err = closenetcdf(&kw);        // Close the File
            }

            break;
        }

        //--------------------------------------------------------------------------
        // Devices: A Group Named after a Device

        if (STR_IEQUALS(stepId, "device")) {
            err = putDevice(&kw);
            break;
        }

        //--------------------------------------------------------------------------
        // Identify/Create a Group: Used by following steps

        if (!putGroup(&kw)) {
            break;
        }

        //--------------------------------------------------------------------------
        // Dimensions

        if (STR_IEQUALS(stepId, "dimension")) {
            err = putDimension(&kw);
            break;
        }

        //--------------------------------------------------------------------------
        // Coordinate Variables

        if (STR_IEQUALS(stepId, "coordinate")) {
            err = putCoordinate(&kw);
            break;
        }

        //--------------------------------------------------------------------------
        // Data Variables

        if (STR_IEQUALS(stepId, "variable")) {
            err = putVariable(&kw);
            break;
        }

        //--------------------------------------------------------------------------
        // Attributes: Additional attributes can be placed within any group level

        if (STR_IEQUALS(stepId, "attribute")) {
            err = putAttribute(&kw);
            break;
        }
        //--------------------------------------------------------------------------
        // End of Error Trap

    } while (0);

    //--------------------------------------------------------------------------
    // Cleanup Keywords

    IDL_KW_FREE;
    return IDL_GettmpLong(err);
}

static const char* plugin = "putdata";

#define CONCAT_(X, Y) X##Y
#define CONCAT(X, Y) CONCAT_(X, Y)
#define ADD_STR_ARG(ARGS, ARG) if (kw->CONCAT(is_,ARG)) { sprintf(ARGS, "%s, ARG=%s", ARGS, IDL_STRING_STR(&kw->ARG)); }
#define ADD_INT_ARG(ARGS, ARG) if (kw->CONCAT(is_,ARG)) { sprintf(ARGS, "%s, ARG=%ld", ARGS, (long)kw->ARG); }
#define ADD_FLT_ARG(ARGS, ARG) if (kw->CONCAT(is_,ARG)) { sprintf(ARGS, "%s, ARG=%g", ARGS, (double)kw->ARG); }
#define ADD_OPT_ARG(ARGS, ARG) if (kw->ARG) { sprintf(ARGS, "%s, ARG", ARGS); }

int opennetcdf(KW_RESULT* kw, int* fileid) {
    char args[2000];

    if (kw->create) {
        sprintf(args, "create");
    } else if (kw->update) {
        sprintf(args, "update");
    }

    ADD_STR_ARG(args, format)
    ADD_STR_ARG(args, filename)
    ADD_STR_ARG(args, directory)
    ADD_STR_ARG(args, conventions)
    ADD_STR_ARG(args, class)
    ADD_STR_ARG(args, title)
    ADD_STR_ARG(args, date)
    ADD_STR_ARG(args, time)
    ADD_INT_ARG(args, exp_number)
    ADD_INT_ARG(args, pass)
    ADD_INT_ARG(args, status)
    ADD_STR_ARG(args, comment)
    ADD_STR_ARG(args, code)
    ADD_INT_ARG(args, version)
    ADD_STR_ARG(args, xml)

    char request[2000];

    sprintf(request, "%s::open(%s)", plugin, args);

    if (kw->verbose) {
        fprintf(stderr, "IDAM request: %s\n", request);
    }
    int handle = idamPutAPI(request, NULL);

    if (handle < 0 || getIdamErrorCode(handle) != 0) {
        fprintf(stderr, "IDAM error: %s\n", getIdamError(handle));
        return -1;
    }

    DATA_BLOCK* data_block = getIdamDataBlock(handle);
    // TODO: Error checking

    *fileid = ((int*) data_block->data)[0];

    return 0;
}

int closenetcdf(KW_RESULT* kw) {
    char args[2000];
    sprintf(args, "fileid=%d", (int) IDL_LongScalar(kw->fileid));

    ADD_OPT_ARG(args, nocompliance)

    char request[2000];

    sprintf(request, "%s::close(%s)", plugin, args);

    if (kw->verbose) {
        fprintf(stderr, "IDAM request: %s\n", request);
    }
    int handle = idamPutAPI(request, NULL);

    if (handle < 0 || getIdamErrorCode(handle) != 0) {
        fprintf(stderr, "IDAM error: %s\n", getIdamError(handle));
        return -1;
    }

    return 0;
}

int putDevice(KW_RESULT* kw) {
    char args[2000];
    sprintf(args, "fileid=%d", (int) IDL_LongScalar(kw->fileid));

    ADD_STR_ARG(args, type)
    ADD_STR_ARG(args, id)
    ADD_STR_ARG(args, serial)
    ADD_STR_ARG(args, device)
    ADD_INT_ARG(args, resolution)
    ADD_INT_ARG(args, channels)

    sprintf(args, "%s, range=%f;%f", args, kw->range[0], kw->range[1]);

    char request[2000];

    sprintf(request, "%s::device(%s)", plugin, args);

    if (kw->verbose) {
        fprintf(stderr, "IDAM request: %s\n", request);
    }
    int handle = idamPutAPI(request, NULL);

    if (handle < 0 || getIdamErrorCode(handle) != 0) {
        fprintf(stderr, "IDAM error: %s\n", getIdamError(handle));
        return -1;
    }

    return 0;
}

int putGroup(KW_RESULT* kw) {
    char args[2000];
    sprintf(args, "fileid=%d", (int) IDL_LongScalar(kw->fileid));

    ADD_STR_ARG(args, name)

    char request[2000];

    sprintf(request, "%s::group(%s)", plugin, args);

    if (kw->verbose) {
        fprintf(stderr, "IDAM request: %s\n", request);
    }
    int handle = idamPutAPI(request, NULL);

    if (handle < 0 || getIdamErrorCode(handle) != 0) {
        fprintf(stderr, "IDAM error: %s\n", getIdamError(handle));
        return -1;
    }

    return 0;
}

int putAttribute(KW_RESULT* kw) {
    char args[2000];
    sprintf(args, "fileid=%d", (int) IDL_LongScalar(kw->fileid));

    ADD_STR_ARG(args, group)
    ADD_STR_ARG(args, name)

    char request[2000];

    sprintf(request, "%s::attribute(%s)", plugin, args);

    if (kw->verbose) {
        fprintf(stderr, "IDAM request: %s\n", request);
    }

    PUTDATA_BLOCK putdata = toPutData(kw->data);

    int handle = idamPutAPI(request, &putdata);

    if (handle < 0 || getIdamErrorCode(handle) != 0) {
        fprintf(stderr, "IDAM error: %s\n", getIdamError(handle));
        return -1;
    }

    return 0;
}

int putVariable(KW_RESULT* kw) {
    char args[2000];
    sprintf(args, "fileid=%d", (int) IDL_LongScalar(kw->fileid));

    ADD_STR_ARG(args, group)
    ADD_STR_ARG(args, name)
    ADD_STR_ARG(args, dimensions)
    ADD_INT_ARG(args, chunksize)
    ADD_INT_ARG(args, compression)
    ADD_FLT_ARG(args, scale)
    ADD_FLT_ARG(args, offset)
    ADD_STR_ARG(args, label)
    ADD_STR_ARG(args, title)
    ADD_STR_ARG(args, comment)
    ADD_STR_ARG(args, units)
    ADD_OPT_ARG(args, notstrict)
    ADD_STR_ARG(args, device)
    ADD_INT_ARG(args, channels)
    ADD_STR_ARG(args, errors)

    char request[2000];

    sprintf(request, "%s::variable(%s)", plugin, args);

    if (kw->verbose) {
        fprintf(stderr, "IDAM request: %s\n", request);
    }

    PUTDATA_BLOCK putdata = toPutData(kw->data);

    int handle = idamPutAPI(request, &putdata);

    if (handle < 0 || getIdamErrorCode(handle) != 0) {
        fprintf(stderr, "IDAM error: %s\n", getIdamError(handle));
        return -1;
    }

    return 0;
}

int putDimension(KW_RESULT* kw) {
    char args[2000];
    sprintf(args, "fileid=%d", (int) IDL_LongScalar(kw->fileid));

    ADD_STR_ARG(args, group)
    ADD_STR_ARG(args, name)
    ADD_INT_ARG(args, length)
    ADD_OPT_ARG(args, unlimited)

    char request[2000];

    sprintf(request, "%s::variable(%s)", plugin, args);

    if (kw->verbose) {
        fprintf(stderr, "IDAM request: %s\n", request);
    }

    int handle = idamPutAPI(request, NULL);

    if (handle < 0 || getIdamErrorCode(handle) != 0) {
        fprintf(stderr, "IDAM error: %s\n", getIdamError(handle));
        return -1;
    }

    return 0;
}

int putCoordinate(KW_RESULT* kw) {
    char args[2000];
    sprintf(args, "fileid=%d", (int) IDL_LongScalar(kw->fileid));

    ADD_STR_ARG(args, group)
    ADD_STR_ARG(args, name)
    ADD_INT_ARG(args, chunksize)
    ADD_INT_ARG(args, compression)
//    ADD_ARG(args, starts)
//    ADD_ARG(args, increments)
//    ADD_ARG(args, counts)
    ADD_STR_ARG(args, label)
    ADD_STR_ARG(args, class)
    ADD_STR_ARG(args, title)
    ADD_STR_ARG(args, errors)
    ADD_STR_ARG(args, units)

    char request[2000];

    sprintf(request, "%s::variable(%s)", plugin, args);

    if (kw->verbose) {
        fprintf(stderr, "IDAM request: %s\n", request);
    }

    PUTDATA_BLOCK putdata = toPutData(kw->data);

    int handle = idamPutAPI(request, &putdata);

    if (handle < 0 || getIdamErrorCode(handle) != 0) {
        fprintf(stderr, "IDAM error: %s\n", getIdamError(handle));
        return -1;
    }

    return 0;
}

int idamType(UCHAR idl_type) {
    // Translate IDL to IDAM Type

    switch (idl_type) {
        case IDL_TYP_FLOAT:
            return TYPE_FLOAT;
        case IDL_TYP_DOUBLE:
            return TYPE_DOUBLE;
        case IDL_TYP_LONG64:
            return TYPE_LONG64;
        case IDL_TYP_LONG:
            return TYPE_LONG;
        case IDL_TYP_INT:
            return TYPE_INT;
        case IDL_TYP_ULONG64:
            return TYPE_UNSIGNED_LONG64;
        case IDL_TYP_ULONG:
            return TYPE_UNSIGNED_LONG;
        case IDL_TYP_UINT:
            return TYPE_UNSIGNED_INT;
        case IDL_TYP_COMPLEX:
            return TYPE_COMPLEX;
        case IDL_TYP_DCOMPLEX:
            return TYPE_DCOMPLEX;
        case IDL_TYP_BYTE:
            return TYPE_UNSIGNED_CHAR;
        default:
            return TYPE_UNKNOWN;
    }
}

PUTDATA_BLOCK toPutData(IDL_VPTR data) {
    PUTDATA_BLOCK putdata;

    if (data->flags & IDL_V_ARR) {
        putdata.data = (char*) data->value.arr->data;
        putdata.count = (unsigned int) data->value.arr->n_elts;
        putdata.data_type = idamType(data->type);
        putdata.rank = data->value.arr->n_dim;
        putdata.shape = malloc(putdata.rank * sizeof(int));
        int i;
        for (i = 0; i < putdata.rank; ++i) {
            putdata.shape[i] = (int) data->value.arr->dim[i];
        }
    } else {
        size_t sz = (size_t) idamSizeOf(idamType(data->type));
        putdata.data = malloc(sz);
        memcpy(putdata.data, &data->value, sz);
        putdata.count = 1;
        putdata.data_type = idamType(data->type);
        putdata.rank = 0;
        putdata.shape = NULL;
    }

    return putdata;
}
