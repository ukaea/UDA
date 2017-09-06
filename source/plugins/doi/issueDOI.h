#ifndef IDAM_PLUGINS_DOI_ISSUEDOI_H
#define IDAM_PLUGINS_DOI_ISSUEDOI_H

#include <plugins/udaPlugin.h>

#ifdef __cplusplus
extern "C" {
#endif

//#define API_DEVICE "MAST"

#define DOIPREFIXLENGTH    256

struct DOIHELP
{
    char * value;         // Scalar String with arbitrary length
};
typedef struct DOIHELP DOIHELP;

struct ISSUEDOI
{
    unsigned short structVersion;    // Structure Version number
    char * doi;                // the issued DOI
    char * owner;                // Owner of the DOI (Scalar String with arbitrary length)
    char * icatRef;            // the ICAT Reference ID (foreign key)
    char status;                // Status of the DOI
};
typedef struct ISSUEDOI ISSUEDOI;

struct DOIRECORD
{
    char * doi;                // the client DOI
    char * requestedSignal;        // the signal requested
    char * requestedSource;        // the source requested
    char * trueSignal;            // the true signal name
    char * trueSource;            // the true source
    char * trueSourceDOI;            // the true source file's DOI
    char * logRecord;            // the IDAM Server's log record
    char * creation;            // record creation date
};
typedef struct DOIRECORD DOIRECORD;

struct LISTDOI
{
    unsigned short structVersion;    // Structure Version number
    unsigned short count;        // number of records in the list
    char * doi;                // the client DOI
    DOIRECORD * list;            // List of records
};
typedef struct LISTDOI LISTDOI;

int issueDOI(IDAM_PLUGIN_INTERFACE * idam_plugin_interface);

#ifdef __cplusplus
}
#endif

#endif // IDAM_PLUGINS_DOI_ISSUEDOI_H
