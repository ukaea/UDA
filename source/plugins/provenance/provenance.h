#ifndef IDAM_PLUGINS_PROVENANCE_PROVENANCE_H
#define IDAM_PLUGINS_PROVENANCE_PROVENANCE_H

#include <libpq-fe.h>

#include <plugins/idamPlugin.h>

#ifdef __cplusplus
extern "C" {
#endif

#define API_DEVICE "MAST"

#define UUIDPREFIXLENGTH    256
#define MAXROOT            1024
#define BUFFERSIZE        10*1024
#define CMDSIZE            2*1024
#define LOGEXT            "log"
#define WEBSCP            "dgm@tm-icat1.fusion.ccfe.ac.uk"
#define WEBURL            "http://tm-icat1.fusion.ccfe.ac.uk/wordpressCCFE/wp-content/uploads"
#define WEBDIR            "/opt/lampp/htdocs/wordpressCCFE/wp-content/uploads"
#define HASHLENGTH        20

struct PROVENANCEHELP
{
    char * value;                // Scalar String with arbitrary length
};
typedef struct PROVENANCEHELP PROVENANCEHELP;

struct PROVENANCEUUID
{
    unsigned short structVersion;    // Structure Version number
    char * uuid;                // the registered UUID
    char * owner;                // Owner of the UUID (Scalar String with arbitrary length)
    char * class;
    char * title;
    char * description;
    char * icatRef;            // the ICAT Reference ID (foreign key)
    char status;                // Status of the UUID
    char * creation;            // Date of original registration
};
typedef struct PROVENANCEUUID PROVENANCEUUID;

struct PROVENANCESIGNAL
{
    char * uuid;                // the client UUID
    char * requestedSignal;        // the signal requested
    char * requestedSource;        // the source requested
    char * trueSignal;            // the true signal name
    char * trueSource;            // the true source
    char * trueSourceUUID;        // the true source file's registered UUID
    char * logRecord;            // the IDAM Server's log record
    char * creation;            // record creation date
};
typedef struct PROVENANCESIGNAL PROVENANCESIGNAL;

struct PROVENANCESIGNALLIST
{
    unsigned short structVersion;    // Structure Version number
    unsigned short count;        // number of records in the list
    char * uuid;                // the client's UUID
    PROVENANCESIGNAL * list;        // List of signals
};
typedef struct PROVENANCESIGNALLIST PROVENANCESIGNALLIST;

extern char * pghost;
extern char pgport[56];
extern char * dbname;
extern char * user;
extern char * pswrd;
extern int initTime;

int admin(IDAM_PLUGIN_INTERFACE * idam_plugin_interface);

int help(IDAM_PLUGIN_INTERFACE * idam_plugin_interface);

int get(IDAM_PLUGIN_INTERFACE * idam_plugin_interface);

int status(IDAM_PLUGIN_INTERFACE * idam_plugin_interface);

int putSignal(IDAM_PLUGIN_INTERFACE * idam_plugin_interface);

int listSignals(IDAM_PLUGIN_INTERFACE * idam_plugin_interface);

int put(IDAM_PLUGIN_INTERFACE * idam_plugin_interface);

int makeProvenanceDir(char ** newDir, char * root, char * UID, unsigned short * priorDir);

int copyProvenanceWebFile(char * oldFile, char * dir, char * newFileName, FILE * log);

int copyProvenanceFile(char * oldFile, char * dir, char * newFileName);

void getFileList(char * list, char *** fileList, char *** fileNames, unsigned short * fileListCount);

void freeFileNameList(char *** list, unsigned short listCount);

int preventSQLInjection(PGconn * DBConnect, char ** from, unsigned short freeHeap);

#ifdef __cplusplus
}
#endif

#endif // IDAM_PLUGINS_PROVENANCE_PROVENANCE_H