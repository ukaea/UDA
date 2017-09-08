#ifndef IDAM_CLIENTSERVER_IDAMSTRUCTS_H
#define IDAM_CLIENTSERVER_IDAMSTRUCTS_H

#include <sys/time.h>

#include "udaDefines.h"

//--------------------------------------------------------
// Structure Definitions

typedef struct DataSubset {
    int subsetCount;        // Number of defined dimensions to subset
    int subset[MAXRANK2];   // If 1 then subset to apply
    int start[MAXRANK2];    // Starting Index of each dimension
    int stop[MAXRANK2];     // Ending Index of each dimension
    int count[MAXRANK2];    // The number of values (sub-samples) read from each dimension
    int stride[MAXRANK2];   // The step stride along each dimension
} DATASUBSET;

typedef struct VLen {
    int len;
    int type;
    void* data;
} VLEN;

typedef struct DComplex {
    double real;
    double imaginary;
} DCOMPLEX;

typedef struct Complex {
    float real;
    float imaginary;
} COMPLEX;

typedef struct DataSystem {
    int system_id;
    int version;
    int meta_id;
    char type;
    char device_name[MAXNAME];
    char system_name[MAXNAME];
    char system_desc[MAXDESC];
    char creation[MAXDATE];
    char xml[MAXMETA];
    char xml_creation[MAXDATE];
    char _padding[3];
} DATA_SYSTEM;

typedef struct SystemConfig {
    int config_id;
    int system_id;
    int meta_id;
    char config_name[MAXNAME];
    char config_desc[MAXDESC];
    char creation[MAXDATE];
    char xml[MAXMETA];
    char xml_creation[MAXDATE];
} SYSTEM_CONFIG;

typedef struct DataSource {
    int source_id;
    int config_id;
    int reason_id;
    int run_id;
    int meta_id;
    int status_desc_id;
    int exp_number;
    int pass;
    int status;
    int status_reason_code;
    int status_impact_code;
    char access;
    char reprocess;
    char type;
    char source_alias[MAXNAME];
    char pass_date[MAXDATE];
    char archive[MAXNAME];
    char device_name[MAXNAME];
    char format[MAXFORMAT];
    char path[MAXPATH];
    char filename[MAXFILENAME];
    char server[MAXSERVER];
    char userid[MAXNAME];
    char reason_desc[MAXDESC];
    char run_desc[MAXMETA];
    char status_desc[MAXMETA];
    char creation[MAXDATE];
    char modified[MAXDATE];
    char xml[MAXMETA];
    char xml_creation[MAXDATE];
    char _padding[1];
} DATA_SOURCE;

typedef struct Signal {
    int source_id;
    int signal_desc_id;
    int meta_id;
    int status_desc_id;
    int status;
    int status_reason_code;
    int status_impact_code;
    char access;
    char reprocess;
    char status_desc[MAXMETA];
    char creation[MAXDATE];
    char modified[MAXDATE];
    char xml[MAXMETA];
    char xml_creation[MAXDATE];
    char _padding[2];
} SIGNAL;

typedef struct SignalDesc {
    int signal_desc_id;
    int meta_id;
    int rank;
    int range_start;
    int range_stop;
    char type;
    char source_alias[MAXNAME];
    char signal_alias[MAXNAME];
    char signal_name[MAXNAME];
    char generic_name[MAXNAME];
    char description[MAXDESC];
    char signal_class[MAXDESC];
    char signal_owner[MAXDESC];
    char creation[MAXDATE];
    char modified[MAXDATE];
    char xml[MAXMETA];
    char xml_creation[MAXDATE];
    char _padding[3];
    int signal_alias_type;
    int signal_map_id;
} SIGNAL_DESC;

typedef struct Dims {
    int data_type;                  // Type of data
    int error_type;                 // Type of error data
    int error_model;                // Identify the Error Model
    int errasymmetry;               // Flags whether or not error data are asymmetrical
    int error_param_n;              // the Number of Model Parameters
    int dim_n;                      // Array lengths

    int compressed;                 // TRUE if data is regular and in compressed form
    double dim0;                    // Starting Value for Regular Grid
    double diff;                    // Step value for Regular Grid

    int method;                     // Compression Method
    unsigned int udoms;             // Number of Domains
    int* sams;                      // Array of Domain Lengths         == sams[udoms]
    char* offs;                     // Array of Domain Starting Values == offs[udoms]
    char* ints;                     // Array of Domain Interval Values == ints[udoms]

    char* dim;                      // Dimension Data Array
    char* synthetic;                // Synthetic Data Array used in Client Side Error/Monte-Carlo Modelling

    char* errhi;                    // Dimension Error Array (Errors above the line: data + error)
    char* errlo;                    // Dimension Error Array (Errors below the line: data - error)
    float errparams[MAXERRPARAMS];  // the array of model parameters

    char dim_units[STRING_LENGTH];
    char dim_label[STRING_LENGTH];
} DIMS;

typedef struct SecurityBlock {
    unsigned short structVersion;               // Structure Version number
    unsigned short encryptionMethod;            // How mutual authentication is executed
    unsigned short authenticationStep;          // Authentication step that created this data structure
    unsigned short client_ciphertextLength;     // cipher lengths (not strings but unsigned byte arrays)
    unsigned short client2_ciphertextLength;
    unsigned short server_ciphertextLength;
    unsigned short client_X509Length;
    unsigned short client2_X509Length;
    unsigned char* client_ciphertext;          // client token encrypted with either the client's or 1st server's public key
    unsigned char* server_ciphertext;          // server token encrypted with the client's public key
    unsigned char* client2_ciphertext;         // client2 token encrypted with either the client's or nth server's public key
    unsigned char* client_X509;
    unsigned char* client2_X509;
} SECURITY_BLOCK;

typedef struct ClientBlock {
    int version;
    int pid;                        // Client Application process id
    char uid[STRING_LENGTH];        // Who the Client is (claim of identity to the first server)

    // Server properties set by the client

    int timeout;                    // Server Shutdown after this time (minutes) if no data request
    int compressDim;                // Enable Compression of the Dimensional Data?

    unsigned int clientFlags;       // client defined properties passed via bit flags
    int altRank;                    // Specify the rank of the alternative signal/source to be used

    int get_nodimdata;              // Don't send Dimensional Data: Send an index only.
    int get_timedble;               // Return Time Dimension Data in Double Precision if originally compressed
    int get_dimdble;                // Return all Dimensional Data in Double Precision
    int get_datadble;               // Return Data in Double Precision

    int get_bad;                    // Return Only Data with Bad Status value
    int get_meta;                   // Return Meta Data associated with Signal
    int get_asis;                   // Return data as Stored in data Archive
    int get_uncal;                  // Disable Calibration Correction
    int get_notoff;                 // Disable Timing Offset Correction
    int get_scalar;                 // Reduce rank from 1 to 0 (Scalar) if dimensional data are all zero
    int get_bytes;                  // Return Data as Bytes or Integers without applying the signal's ADC Calibration Data

    unsigned int privateFlags;      // set of private flags used to communicate server to server

    char OSName[STRING_LENGTH];     // Name of the Client side Operating System, e.g. OSX
    char DOI[STRING_LENGTH];        // User's research DOI - to be logged with all data access requests

    char uid2[STRING_LENGTH];       // Who the Client is (claim of identity to the last server)
    SECURITY_BLOCK securityBlock;   // Contains encrypted tokens exchanged between client and server for mutual authentication

} CLIENT_BLOCK;

typedef struct DataBlock {
    int handle;
    int errcode;
    int source_status;
    int signal_status;
    unsigned int rank;
    int order;
    int data_type;

    int error_type;
    int error_model;                // Identify the Error Model
    int errasymmetry;               // Flags whether or not error data are asymmetrical
    int error_param_n;              // the Number of Model Parameters

    int data_n;
    char* data;
    char* synthetic;                // Synthetic Data Array used in Client Side Error/Monte-Carlo Modelling

    char* errhi;                    // Error Array (Errors above the line: data + error)
    char* errlo;                    // Error Array (Errors below the line: data - error)
    float errparams[MAXERRPARAMS];  // the array of model parameters

    char data_units[STRING_LENGTH];
    char data_label[STRING_LENGTH];
    char data_desc[STRING_LENGTH];

    char error_msg[STRING_LENGTH];

    DIMS* dims;
    DATA_SYSTEM* data_system;
    SYSTEM_CONFIG* system_config;
    DATA_SOURCE* data_source;
    SIGNAL* signal_rec;
    SIGNAL_DESC* signal_desc;

    CLIENT_BLOCK client_block;      // Used to pass properties into data reader plugins

    int opaque_type;                // Identifies the Data Structure Type;
    int opaque_count;               // Number of Instances of the Data Structure;
    void* opaque_block;             // Opaque pointer to Hierarchical Data Structures
    unsigned int totalDataBlockSize;// The amount of data within this structure.
    unsigned int cachePermission;   // Permission for the Client to cache this structure.
} DATA_BLOCK;

typedef struct DataObject {
    unsigned short objectType;      // File or regular object
    unsigned int objectSize;
    unsigned short hashType;
    unsigned short hashLength;
    char* md;
    char* object;
} DATA_OBJECT;

typedef struct PutDataBlock {
    int data_type;
    unsigned int rank;
    unsigned int count;
    int* shape;
    const char* data;
    int opaque_type;                // Identifies the Data Structure Type;
    int opaque_count;               // Number of Instances of the Data Structure;
    void* opaque_block;             // Opaque pointer to Hierarchical Data Structures
    unsigned int blockNameLength;   // Size of the Name character string
    const char* blockName;          // Name of the Data Block
} PUTDATA_BLOCK;

typedef struct PutDataBlockList {
    unsigned int blockCount;        // Number of data blocks
    unsigned int blockListSize;     // Number of data blocks allocated
    PUTDATA_BLOCK* putDataBlock;    // Array of data blocks
} PUTDATA_BLOCK_LIST;

typedef struct IdamError {
    int type;                       // Error Classification
    int code;                       // Error Code
    char location[STRING_LENGTH];   // Where this Error is Located
    char msg[STRING_LENGTH];        // Message
} IDAMERROR;

typedef struct IdamErrorStack {
    unsigned int nerrors;           // Number of Errors
    IDAMERROR* idamerror;           // Array of Errors
} IDAMERRORSTACK;

typedef struct ServerBlock {
    int version;
    int error;
    char msg[STRING_LENGTH];
    int pid;                        // Server Application process id
    IDAMERRORSTACK idamerrorstack;
    char OSName[STRING_LENGTH];     // Name of the Server's Operating System, e.g. OSX
    char DOI[STRING_LENGTH];        // Server version/implementation DOI - to be logged with all data consumers
    SECURITY_BLOCK securityBlock;   // Contains encrypted tokens exchanged between client and server for mutual authentication
} SERVER_BLOCK;

typedef struct NameValuye {
    char* pair;    // The name value pair string
    char* name;    // The name
    char* value;   // The value
} NAMEVALUE;

typedef struct NameValueList {
    int pairCount;        // Number of name value pairs in list
    int listSize;         // Allocated Size of the List
    NAMEVALUE* nameValue;  // List of individual name value pairs in parse order
} NAMEVALUELIST;

typedef struct RequestBlock {
    int request;                       // Plugin or Shutdown Server
    int exp_number;                    // Pulse No.,Tree No., etc
    int pass;                          // Pass, Sequence, etc
    char tpass[STRING_LENGTH];          //
    char path[STRING_LENGTH];           // Path to File, Path to Node
    char file[STRING_LENGTH];           // File name, Tree name
    char format[STRING_LENGTH];         // File Format
    char signal[MAXMETA];               // Signal, Node etc
    char archive[STRING_LENGTH];        // Archive: pr98, transp, etc
    char device_name[STRING_LENGTH];    // Device name: Mast, Jet, etc
    char server[STRING_LENGTH];         // IDAM, MDS+ Server
    char source[STRING_LENGTH];         // Data Source, Server Host, URL etc
    char function[STRING_LENGTH];       // Server-Side function or attached plugin function
    char api_delim[MAXNAME];            // Delimiter string to use decoding the signal and source arguments
    char subset[STRING_LENGTH];         // Subset instructions
    DATASUBSET datasubset;               // Parsed subset instructions (Server Side)
    NAMEVALUELIST nameValueList;         // Set of Name-Value pairs (Server Side Function)

    int put;                             // flag to set the server to a put state
    PUTDATA_BLOCK_LIST putDataBlockList; // Data to be put on the server

} REQUEST_BLOCK;

//---------------------------------------------------------------------------------------------------
// Performance Measures

typedef struct Performance {
    int npoints;                    // Number of Measurement Points
    char label[20][256];            // Label the Measurement Point
    struct timeval tv_start[20];    // Start time at a Measurement Point
    struct timeval tv_end[20];      // Stop time
} PERFORMANCE;

//---------------------------------------------------------------------------------------------------
// System Environment Variables

typedef struct Environment {
    int server_port;                                // Principal IDAM server port
    int server_port2;                               // Backup IDAM server port
    int sql_port;
    int server_reconnect;                           // If the client changes to a different IDAM server then open a new socket
    int server_change_socket;                       // Connect to a Running Server
    int server_socket;                              // Clients must keep track of the sockets they open
    int data_path_id;                               // Identifies the algorithm that defines the default path to the standard data source.
    int external_user;                              // Flags this service as accessible by external users: Disable some access formats for security.
    unsigned int clientFlags;                       // Use legacy Name substitution
    int altRank;                                    // Use specific set of legacy name substitutes
    char logdir[MAXPATH];
    char logmode[2];
    int loglevel;
    char server_host[MAXNAME];                      // Principal IDAM server host
    char server_host2[MAXNAME];                     // Backup IDAM server host
    char server_proxy[MAXNAME];                     // host:port - Running as a Proxy IDAM server: Prefix 'IDAM::host:port/' to redirect request
    char server_this[MAXNAME];                      // host:port - The current server. Used to trap potential infinite redirects
    char sql_host[MAXNAME];
    char sql_dbname[MAXNAME];
    char sql_user[MAXNAME];
    char api_delim[MAXNAME];                        // Default API Signal and Source Delimiter Sub-String
    char api_device[STRING_LENGTH];                 // API Default Device name
    char api_archive[STRING_LENGTH];                // API Default Archive name
    char api_format[STRING_LENGTH];                 // API Default Client File Format
    char private_path_target[STRING_LENGTH];        // Target this path to private files
    char private_path_substitute[STRING_LENGTH];    // and substitute with this path (so the server can locate them!)
    char initialised;                               // Environment already initialised.
    char _padding[1];
} ENVIRONMENT;

#endif //IDAM_CLIENTSERVER_IDAMSTRUCTS_H
