//
// Include header for putData functions
//`

#ifndef putDataInclude
#define putDataInclude

#include <stdio.h>
#include <limits.h>
#include <idl_export.h>     // IDL API Header

// Useful macro

#define ARRLEN(arr) (sizeof(arr)/sizeof(arr[0]))

//-------------------------------------------------------------------------------------------------------
// Standard Keyword Structure

typedef struct {
    IDL_KW_RESULT_FIRST_FIELD;

    IDL_VPTR data;

    IDL_STRING conventions;
    IDL_STRING class;
    IDL_STRING title;
    IDL_STRING comment;
    IDL_STRING code;
    IDL_STRING date;
    IDL_STRING format;
    IDL_STRING directory;
    IDL_STRING time;
    IDL_STRING xml;
    IDL_STRING stepId;
    IDL_STRING group;
    IDL_STRING filename;
    IDL_STRING errors;
    IDL_STRING dimensions;
    IDL_STRING device;
    IDL_STRING id;
    IDL_STRING serial;
    IDL_STRING name;
    IDL_STRING units;
    IDL_STRING label;
    IDL_STRING type;

    IDL_LONG is_data;
    IDL_LONG is_exp_number;
    IDL_LONG is_pass;
    IDL_LONG is_status;
    IDL_LONG is_version;
    IDL_LONG is_conventions;
    IDL_LONG is_class;
    IDL_LONG is_title;
    IDL_LONG is_comment;
    IDL_LONG is_code;
    IDL_LONG is_date;
    IDL_LONG is_format;
    IDL_LONG is_directory;
    IDL_LONG is_time;
    IDL_LONG is_xml;
    IDL_LONG is_type;

    IDL_LONG is_stepId;
    IDL_LONG is_group;
    IDL_LONG is_filename;
    IDL_LONG is_errors;
    IDL_LONG is_dimensions;
    IDL_LONG is_length;
    IDL_LONG is_device;
    IDL_LONG is_id;
    IDL_LONG is_serial;
    IDL_LONG is_channel;
    IDL_LONG is_channels;
    IDL_LONG is_resolution;
    IDL_LONG is_scale;
    IDL_LONG is_offset;
    IDL_LONG is_name;
    IDL_LONG is_units;
    IDL_LONG is_label;
    IDL_LONG is_link;
    IDL_LONG is_chunksize;
    IDL_LONG is_compression;

    IDL_LONG exp_number;
    IDL_LONG pass;
    IDL_LONG status;
    IDL_LONG version;
    IDL_LONG length;         // unused
    IDL_LONG unlimited;
    IDL_LONG chunksize;      // NEEDS to be an ARRAY!!!!!!!!
    IDL_LONG compression;

    IDL_LONG channels;
    IDL_LONG channel;
    IDL_LONG resolution;

    double scale;
    double offset;

    IDL_LONG is_range;
    IDL_MEMINT rangeCount;
    double range[2];

    IDL_LONG is_starts;
    IDL_LONG is_increments;
    IDL_LONG is_counts;

    IDL_VPTR starts;
    IDL_VPTR increments;
    IDL_VPTR counts;

    IDL_LONG is_fileid;
    IDL_VPTR fileid;         // Passed back to IDL

    IDL_LONG create;
    IDL_LONG close;
    IDL_LONG update;
    IDL_LONG delete;

    IDL_LONG notstrict;
    IDL_LONG nocompliance;

    IDL_LONG debug;
    IDL_LONG verbose;

} KW_RESULT;

//-----------------------------------------------------------------------------------------------------
// Function Prototypes

void initKW(KW_RESULT* kw);

void printKW(FILE* fd, KW_RESULT kw);

void reverseString(const char* in, char* out);

int opennetcdf(KW_RESULT* kw, int* fileid);

int closenetcdf(KW_RESULT* kw);

int putDevice(KW_RESULT* kw);

int putGroup(KW_RESULT* kw);

int putAttribute(KW_RESULT* kw);

int putVariable(KW_RESULT* kw);

int putDimension(KW_RESULT* kw);

int putCoordinate(KW_RESULT* kw);

#endif
