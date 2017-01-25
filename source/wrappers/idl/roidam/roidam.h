#ifndef IDAM_WRAPPERS_IDL_ROIDAM_H
#define IDAM_WRAPPERS_IDL_ROIDAM_H

#include "idl_export.h"     // IDL API Header 

// Returned Row limit

#define LISTROWLIMIT    100000
#define LISTRANGELIMIT  10

// Define Status, Reason and Impact Codes Defaults

#define DEFAULTSTATUSSTR        "1"
#define MINSTATUS           -1
#define MAXSTATUS           3

#define DEFAULTREASONCODE       0
#define DEFAULTIMPACTCODE       0
#define DEFAULTREASONIMPACTCODE     0
#define DEFAULTREASONIMPACTCODESTR  "0"

// Keyword Structure

typedef struct {
    IDL_KW_RESULT_FIRST_FIELD;

    IDL_LONG    exp_number;
    IDL_LONG    shot;
    IDL_LONG    pulno;
    IDL_LONG    pass;
    IDL_LONG        range;          // Exp_Number List is a Range
    IDL_LONG    is_exp_number;
    IDL_LONG    is_pass;

    IDL_MEMINT      npassrange;     // Number of Element in passrange array
    int             passrange[2];       // Pass Number Range
    IDL_LONG    is_passrange;

    IDL_MEMINT      nshotrange;     // Number of Element in shotrange array
    int             shotrange[2];       // Pass Number Range
    IDL_LONG    is_shotrange;

    IDL_LONG    alias;
    IDL_LONG    all;
    IDL_LONG    huge;
    IDL_LONG    distinct;

    IDL_STRING  source;
    IDL_LONG    is_source;

    IDL_STRING  signal;
    IDL_LONG    is_signal;

    IDL_LONG    status;
    IDL_LONG    reason;
    IDL_LONG    impact;
    IDL_LONG    code;
    IDL_LONG    is_status;
    IDL_LONG    is_reason;
    IDL_LONG    is_impact;
    IDL_LONG    is_code;

    IDL_STRING  description;        // Status Change Reason/Impact description
    IDL_LONG    is_description;

    IDL_STRING  search;
    IDL_LONG    is_search;

    IDL_STRING  class;
    IDL_LONG    is_class;

    IDL_STRING  owner;
    IDL_LONG    is_owner;

    IDL_LONG    match;
    IDL_LONG    multiple;
    IDL_LONG    latest;
    IDL_LONG    delete;
    IDL_LONG    replace;
    IDL_LONG    commit;

    IDL_LONG    test;
    IDL_LONG    verbose;
    IDL_LONG    debug;

    IDL_STRING  xml;
    IDL_STRING  query;
    IDL_STRING  type;
    IDL_LONG        is_xml;
    IDL_LONG        is_query;
    IDL_LONG        is_type;

    IDL_LONG        is_meta_id;
    IDL_LONG        meta_id;

    double      time_offset;
    double      time_start;
    double      time_interval;
    IDL_LONG    is_time_offset;
    IDL_LONG    is_time_start;
    IDL_LONG    is_time_interval;

    IDL_STRING  doc_label;
    IDL_STRING  doc_units;
    IDL_STRING  doc_desc;
    IDL_MEMINT      ndoc_dim_id;        // Number of elements in dimension id array
    IDL_MEMINT      ndoc_dim_label;
    IDL_MEMINT      ndoc_dim_units;
    int             doc_dim_id[8];      // Dimension id array
    IDL_STRING  doc_dim_label[8];
    IDL_STRING  doc_dim_units[8];
    IDL_LONG    is_doc_label;
    IDL_LONG    is_doc_units;
    IDL_LONG    is_doc_desc;
    IDL_LONG    is_doc_dim_id;
    IDL_LONG    is_doc_dim_label;
    IDL_LONG    is_doc_dim_units;

    double      cal_factor;
    double      cal_offset;
    IDL_STRING  cal_target;
    IDL_STRING  cal_units;
    IDL_MEMINT      ncal_dim_id;        // Number of elements in dimension id array
    IDL_MEMINT      ncal_dim_factor;
    IDL_MEMINT      ncal_dim_offset;
    IDL_MEMINT      ncal_dim_target;
    IDL_MEMINT      ncal_dim_units;
    int             cal_dim_id[32];     // 8 * 3 (= data, error, asymmetric error) Dimension id array
    double      cal_dim_factor[32];
    double      cal_dim_offset[32];
    IDL_STRING  cal_dim_target[32];
    IDL_STRING  cal_dim_units[32];
    IDL_LONG    is_cal_factor;
    IDL_LONG    is_cal_offset;
    IDL_LONG    is_cal_target;
    IDL_LONG    is_cal_units;
    IDL_LONG    is_cal_dim_id;
    IDL_LONG    is_cal_dim_factor;
    IDL_LONG    is_cal_dim_offset;
    IDL_LONG    is_cal_dim_target;
    IDL_LONG    is_cal_dim_units;

    IDL_LONG    err_model;
    IDL_MEMINT      nerr_params;
    IDL_MEMINT      nerr_dim_id;
    IDL_MEMINT      nerr_dim_model;
    float       err_params[4];
    int         err_dim_id[8];
    int         err_dim_model[8];
    IDL_VPTR    err_dim_params;     // Multi-Dimensional
    IDL_LONG    is_err_model;
    IDL_LONG    is_err_params;
    IDL_LONG    is_err_dim_id;
    IDL_LONG    is_err_dim_model;
    IDL_LONG    is_err_dim_params;

    IDL_STRING  comp_new_alias;
    IDL_STRING  comp_signal;
    IDL_STRING  comp_error;
    IDL_STRING  comp_aserror;
    IDL_MEMINT      ncomp_dim_id;
    IDL_MEMINT      ncomp_dim_signal;
    IDL_MEMINT      ncomp_dim_error;
    IDL_MEMINT      ncomp_dim_aserror;
    IDL_MEMINT      ncomp_dim_to;
    IDL_MEMINT      ncomp_dim_from;
    int     comp_dim_id[8];
    int     comp_dim_to[8];
    int     comp_dim_from[8];
    IDL_STRING  comp_dim_signal[8];
    IDL_STRING  comp_dim_error[8];
    IDL_STRING  comp_dim_aserror[8];

    IDL_LONG    is_comp_new_alias;
    IDL_LONG    is_comp_signal;
    IDL_LONG    is_comp_error;
    IDL_LONG    is_comp_aserror;
    IDL_LONG    is_comp_dim_id;
    IDL_LONG    is_comp_dim_signal;
    IDL_LONG    is_comp_dim_error;
    IDL_LONG    is_comp_dim_aserror;
    IDL_LONG    is_comp_dim_to;
    IDL_LONG    is_comp_dim_from;

} KW_RESULT;


// Useful macro

#define ARRLEN(arr) (sizeof(arr)/sizeof(arr[0]))

// Error management

extern IDL_MSG_BLOCK msg_block;

#define ERROR1  0
#define ERROR2  -1

#define     GDE_NO_ARGUMENTS    1
#define     GDE_NO_EXP_NUMBER   2
#define     GDE_NO_SIGNAL_ARGUMENT  3
#define     GDE_BAD_HANDLE      4
#define     GDE_NO_VALID_HANDLE 5
#define     GDE_DATA_HAS_ERROR  6
#define     GDE_UNKNOWN_DATA_TYPE   7
#define     GDE_NO_SUCH_DIMENSION   8
#define     GDE_NO_DATA_TO_RETURN   9
#define     GDE_RANK_TOO_HIGH   10
#define     GDE_HEAP_ALLOC_ERROR    11
#define     GDE_NO_API_IDENTIFIED   12
#define     GDE_NO_DATA_STRUCTURE_TO_RETURN 13
#define     GDE_NOT_IMPLEMENTED 20

//-----------------------------------------------------------------------------
// Define the startup function that adds C functions to IDL along with the exit handler

extern void roidam_exit_handler(void);
extern int  roidam_Startup(void);

extern int initEnvironment;     // Flag initilisation


//----------------------------------------------------------------------------
// Function Prototypes
/*
int queryIdamTable(char *sql, int verbose, FILE *fh, char *out);
int countIdamTable(char *sql, int verbose, FILE *fh);
int executeIdamSQL(char *sql, int verbose, FILE *fh);
void sqlIdamPatternMatch(char *in, char *out);
int checkSignalAuthorisation(int isKey, char *whr, char *user, int verbose, FILE *fh);
int checkSourceAuthorisation(char *source, char *user, int verbose, FILE *fh);
int checkAdminAuthorisation(char *user, int verbose, FILE *fh);

int createIdamKWXML(char *signal, KW_RESULT kw, char *xml);
int createIdamActionXML(ACTIONS actions, char *xml);
int combineIdamActions(int replace, ACTIONS newactions, ACTIONS *actions);
*/

#endif // IDAM_WRAPPERS_IDL_ROIDAM_H
