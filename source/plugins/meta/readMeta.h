#ifndef IdamReadMetaInclude
#define IdamReadMetaInclude

#include <plugins/idamPlugin.h>

#include <libpq-fe.h>

#ifdef __cplusplus
extern "C" {
#endif

#define API_DEVICE "MAST"            // Must be consistent with the Data Server

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1
#define THISPLUGIN_DEFAULT_METHOD        "help"

#define CONTEXT_DATA        1        // Default
#define CONTEXT_META        2
#define CONTEXT_CPF         3

#define CASTROW            1        // Default
#define CASTCOLUMN          2

/*
#define SCALARSTRING		1		// Default
#define ARRAYSTRING		    2

#define SCALARINT		    1		// Default
#define ARRAYINT		    2
#define SCALARUINT		    3
#define ARRAYUINT		    4
*/

struct METAHELP
{
    char * value;         // Scalar String with arbitrary length
};
typedef struct METAHELP METAHELP;

struct METADEVICE_R
{
    char * name;         // Scalar String with arbitrary length
};
typedef struct METADEVICE_R METADEVICE_R;

struct METADEVICE_C
{
    unsigned int count;
    char ** name;     // String array with arbitrary length
};
typedef struct METADEVICE_C METADEVICE_C;

struct METALIST_R
{
    char * class;         // Scalar String with arbitrary length
    char * system;
    char * device;
    char * ro;
    char * description;
};
typedef struct METALIST_R METALIST_R;

struct METALIST_C
{
    unsigned int count;
    char ** class;     // String array with arbitrary length
    char ** system;
    char ** device;
    char ** ro;
    char ** description;
};
typedef struct METALIST_C METALIST_C;

struct METADATA_R
{
    char * class;         // Scalar String with arbitrary length
    char * system;
    char * device;
    char * configuration;
    unsigned int version;
    unsigned int revision;
    char * status;
    char * description;
    char * comment;
    unsigned int range_start;
    unsigned int range_stop;
};
typedef struct METADATA_R METADATA_R;

struct METADATA_RS
{
    char * class;         // Scalar String with arbitrary length
    char * system;
    char * device;
    char * configuration;
    unsigned int version;
    unsigned int revision;
    char * status;
    char * description;
    char * comment;
    unsigned int range_start;
    unsigned int range_stop;
    char * type_name;
    char * structure_description;
    char * definition;
};
typedef struct METADATA_RS METADATA_RS;

struct METADATA_C
{
    unsigned int count;
    char ** class;         // String array with arbitrary length
    char ** system;
    char ** device;
    char ** configuration;
    unsigned int * version;
    unsigned int * revision;
    char ** status;
    char ** description;
    char ** comment;
    unsigned int * range_start;
    unsigned int * range_stop;
};
typedef struct METADATA_C METADATA_C;

struct METADATA_CS
{
    unsigned int count;
    char ** class;         // String array with arbitrary length
    char ** system;
    char ** device;
    char ** configuration;
    unsigned int * version;
    unsigned int * revision;
    char ** status;
    char ** description;
    char ** comment;
    unsigned int * range_start;
    unsigned int * range_stop;
    char ** type_name;
    char ** structure_description;
    char ** definition;
};
typedef struct METADATA_CS METADATA_CS;

struct DATALASTSHOT
{
    unsigned int lastshot;         // Scalar integer
};
typedef struct DATALASTSHOT DATALASTSHOT;

struct DATALASTPASS
{
    unsigned int lastpass;         // Scalar integer
};
typedef struct DATALASTPASS DATALASTPASS;

struct DATASHOTDATETIME
{
    unsigned int shot;         // Scalar integer
    char * date;
    char * time;
};
typedef struct DATASHOTDATETIME DATASHOTDATETIME;

struct DATALISTSIGNALS_R
{
    char * signal_name;
    char * generic_name;
    char * source_alias;
    char * type;
    char * description;
};
typedef struct DATALISTSIGNALS_R DATALISTSIGNALS_R;

struct DATALISTSIGNALS_RR
{
    unsigned int count;
    unsigned int shot;
    int pass;
    DATALISTSIGNALS_R * list;
};
typedef struct DATALISTSIGNALS_RR DATALISTSIGNALS_RR;

struct DATALISTSIGNALS_C
{
    unsigned int count;
    unsigned int shot;        // Scalar integer
    int pass;            // the default is -1
    char ** signal_name;        // Array of Strings each with arbitrary length
    char ** generic_name;
    char ** source_alias;
    char ** type;
    char ** description;
};
typedef struct DATALISTSIGNALS_C DATALISTSIGNALS_C;

struct DATALISTSOURCES_R
{
    int pass;
    short status;
    char * source_alias;
    char * format;
    char * filename;
    char * type;
};
typedef struct DATALISTSOURCES_R DATALISTSOURCES_R;

struct DATALISTSOURCES_RR
{
    unsigned int count;
    unsigned int shot;
    DATALISTSOURCES_R * list;
};
typedef struct DATALISTSOURCES_RR DATALISTSOURCES_RR;

struct DATALISTSOURCES_C
{
    unsigned int count;
    unsigned int shot;
    int * pass;
    short * status;
    char ** source_alias;
    char ** format;
    char ** filename;
    char ** type;
};
typedef struct DATALISTSOURCES_C DATALISTSOURCES_C;

struct CPFLIST_R
{
    char * class;
};
typedef struct CPFLIST_R CPFLIST_R;

struct CPFLIST_RR
{
    unsigned int count;
    CPFLIST_R * list;
};
typedef struct CPFLIST_RR CPFLIST_RR;

struct CPFLIST_C
{
    unsigned int count;
    char ** class;
};
typedef struct CPFLIST_C CPFLIST_C;

struct CPFLISTNAMES_R
{
    char * name;
    char * table;
    char * class;
    char * source;
    char * description;
    char * label;
    char * units;
};
typedef struct CPFLISTNAMES_R CPFLISTNAMES_R;

struct CPFLISTNAMES_RR
{
    unsigned int count;
    CPFLISTNAMES_R * list;
};
typedef struct CPFLISTNAMES_RR CPFLISTNAMES_RR;

struct CPFLISTNAMES_C
{
    unsigned int count;
    char ** name;
    char ** table;
    char ** class;
    char ** source;
    char ** description;
    char ** label;
    char ** units;
};
typedef struct CPFLISTNAMES_C CPFLISTNAMES_C;

struct CPFDATA_RR
{
    unsigned int count;
    void * list;            // Structure has no predefined size - multiple string pointers
};
typedef struct CPFDATA_RR CPFDATA_RR;

struct CPFDATA_C
{
    unsigned int count;
    void ** list;        // Structure has no predefined size - multiple string pointer arrays
};
typedef struct CPFDATA_C CPFDATA_C;

struct CPFDATA_3C
{
    unsigned int count;
    char ** a;
    char ** b;
    char ** c;
};
typedef struct CPFDATA_3C CPFDATA_3C;

extern int readMeta(IDAM_PLUGIN_INTERFACE * idam_plugin_interface);

#ifdef __cplusplus
}
#endif

#endif
