
#ifndef IDAM_READIDAM_H
#define IDAM_READIDAM_H

#include "idamclientserverpublic.h"
#include "idamclientpublic.h"

int readIdam(DATA_SOURCE data_source, SIGNAL_DESC signal_desc, REQUEST_BLOCK request_block, DATA_BLOCK *data_block);

#ifndef NOIDAMPLUGIN
#ifndef FATCLIENT

struct OLD_CLIENT_BLOCK {

    int   version;
    int   pid;			// Client Application process id
    char  uid[STRING_LENGTH];	// Who the Client is

    // Server properties set by the client

    int   timeout;		// Server Shutdown after this time (minutes) if no data request
    int   compressDim;	// Enable Compression of the Dimensional Data?

    unsigned int   clientFlags;	// client defined properties passed via bit flags
    int   altRank;			// Specify the rank of the alternative signal/source to be used

    int   get_nodimdata;	// Don't send Dimensional Data: Send an index only.
    int   get_timedble;	// Return Time Dimension Data in Double Precision if originally compressed
    int   get_dimdble;	// Return all Dimensional Data in Double Precision
    int   get_datadble;	// Return Data in Double Precision

    int   get_bad;		// Return Only Data with Bad Status value
    int   get_meta;		// Return Meta Data associated with Signal
    int   get_asis;		// Return data as Stored in data Archive
    int   get_uncal;		// Disable Calibration Correction
    int   get_notoff;	// Disable Timing Offset Correction
    int   get_scalar;	// Reduce rank from 1 to 0 (Scalar) if dimensional data are all zero
    int   get_bytes;		// Return Data as Bytes or Integers without applying the signal's ADC Calibration Data

    unsigned int privateFlags;	// set of private flags used to communicate server to server

} ;
typedef struct OLD_CLIENT_BLOCK OLD_CLIENT_BLOCK ;

struct OLD_DATA_BLOCK {
    int	handle;
    int	errcode;
    int	source_status;
    int	signal_status;
    int	rank;
    int	order;
    int	data_type;

    int	error_type;
    int	error_model;		// Identify the Error Model
    int	errasymmetry;		// Flags whether or not error data are asymmetrical
    int      error_param_n;		// the Number of Model Parameters

    int 	data_n;
    char 	*data;
    char 	*synthetic;		// Synthetic Data Array used in Client Side Error/Monte-Carlo Modelling

    char 	*errhi;			// Error Array (Errors above the line: data + error)
    char 	*errlo;			// Error Array (Errors below the line: data - error)
    float 	errparams[MAXERRPARAMS];		// the array of model parameters

    char data_units[STRING_LENGTH];
    char data_label[STRING_LENGTH];
    char data_desc[STRING_LENGTH];

    char error_msg[STRING_LENGTH];

    DIMS          *dims;
    DATA_SYSTEM   *data_system;
    SYSTEM_CONFIG *system_config;
    DATA_SOURCE   *data_source;
    SIGNAL        *signal_rec;
    SIGNAL_DESC   *signal_desc;

    OLD_CLIENT_BLOCK  client_block;	// Used to pass properties into data reader plugins

    int   opaque_type;		// Identifies the Data Structure Type;
    int   opaque_count;		// Number of Instances of the Data Structure;
    void *opaque_block;		// Opaque pointer to Hierarchical Data Structures
} ;
typedef struct OLD_DATA_BLOCK OLD_DATA_BLOCK ;

#endif
#endif

#endif // IDAM_READIDAM_H

