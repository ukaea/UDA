#pragma once

#ifdef __GNUC__
#  include <sys/time.h>
#endif

#include <iostream>
#include <sstream>

#include <uda/types.h>

#include "udaDefines.h"
#include "name_value_list.hpp"

using UDA_ERROR_STACK = struct CUdaErrorStack {};
using PUTDATA_BLOCK_LIST = struct CPutDataBlockList {};
using PUTDATA_BLOCK = struct CPutDataBlock {};

namespace uda::client_server
{

//--------------------------------------------------------
// Structure Definitions

struct DataSubset {
    int subsetCount;      // Number of defined dimensions to subset
    int subset[MaxRank2]; // If 1 then subset to apply
    int start[MaxRank2];  // Starting Index of each dimension
    int stop[MaxRank2];   // Ending Index of each dimension
    int count[MaxRank2];  // The number of values (sub-samples) read from each dimension
    int stride[MaxRank2]; // The step stride along each dimension
};

struct VLen {
    int len;
    int type;
    void* data;
};

struct DComplex {
    double real;
    double imaginary;
};

struct Complex {
    float real;
    float imaginary;
};

struct Dims {
    int data_type;     // Type of data
    int error_type;    // Type of error data
    int error_model;   // Identify the Error Model
    int errasymmetry;  // Flags whether or not error data are asymmetrical
    int error_param_n; // the Number of Model Parameters
    int dim_n;         // Array lengths

    int compressed; // TRUE if data is regular and in compressed form
    double dim0;    // Starting Value for Regular Grid
    double diff;    // Step value for Regular Grid

    int method;         // Compression Method
    unsigned int udoms; // Number of Domains
    int* sams;          // Array of Domain Lengths         == sams[udoms]
    char* offs;         // Array of Domain Starting Values == offs[udoms]
    char* ints;         // Array of Domain Interval Values == ints[udoms]

    char* dim;       // Dimension Data Array
    char* synthetic; // Synthetic Data Array used in Client Side Error/Monte-Carlo Modelling

    char* errhi;                   // Dimension Error Array (Errors above the line: data + error)
    char* errlo;                   // Dimension Error Array (Errors below the line: data - error)
    float errparams[MaxErrParams]; // the array of model parameters

    char dim_units[StringLength];
    char dim_label[StringLength];
};

struct SecurityBlock {
    unsigned short structVersion;           // Structure Version number
    unsigned short encryptionMethod;        // How mutual authentication is executed
    unsigned short authenticationStep;      // Authentication step that created this data structure
    unsigned short client_ciphertextLength; // cipher lengths (not strings but unsigned byte arrays)
    unsigned short client2_ciphertextLength;
    unsigned short server_ciphertextLength;
    unsigned short client_X509Length;
    unsigned short client2_X509Length;
    unsigned char* client_ciphertext;  // client token encrypted with either the client's or 1st server's public key
    unsigned char* server_ciphertext;  // server token encrypted with the client's public key
    unsigned char* client2_ciphertext; // client2 token encrypted with either the client's or nth server's public key
    unsigned char* client_X509;
    unsigned char* client2_X509;
};

struct ClientBlock {
    int version;
    int pid;                 // Client Application process id
    char uid[StringLength]; // Who the Client is (claim of identity to the first server)

    // Server properties set by the client

    int timeout;     // Server Shutdown after this time (minutes) if no data request
    int compressDim; // Enable Compression of the Dimensional Data?

    unsigned int clientFlags; // client defined properties passed via bit flags
    int altRank;              // Specify the rank of the alternative signal/source to be used

    int get_nodimdata; // Don't send Dimensional Data: Send an index only.
    int get_timedble;  // Return Time Dimension Data in Double Precision if originally compressed
    int get_dimdble;   // Return all Dimensional Data in Double Precision
    int get_datadble;  // Return Data in Double Precision

    int get_bad;    // Return Only Data with Bad Status value
    int get_meta;   // Return Metadata associated with Signal
    int get_asis;   // Return data as Stored in data Archive
    int get_uncal;  // Disable Calibration Correction
    int get_notoff; // Disable Timing Offset Correction
    int get_scalar; // Reduce rank from 1 to 0 (Scalar) if dimensional data are all zero
    int get_bytes;  // Return Data as Bytes or Integers without applying the signal's ADC Calibration Data

    unsigned int privateFlags; // set of private flags used to communicate server to server

    char OSName[StringLength]; // Name of the Client side Operating System, e.g. OSX
    char DOI[StringLength];    // User's research DOI - to be logged with all data access requests

    char uid2[StringLength]; // Who the Client is (claim of identity to the last server)
    SecurityBlock
        securityBlock; // Contains encrypted tokens exchanged between client and server for mutual authentication
};

struct MetaDataField {
    std::array<char, StringLength> name;
    std::array<char, StringLength> value;
};

struct MetaData {
    std::vector<MetaDataField> fields = {};
    [[nodiscard]] std::string_view find(std::string_view name) const;
    [[nodiscard]] bool contains(std::string_view name) const;
    template <typename T>
    T find_as(const std::string_view name, T default_ = T{}) const {
        auto value = find(name);
        if (value.empty()) {
            return default_;
        }
        std::stringstream stream{value.data()};
        T result;
        stream >> result;
        return result;
    }
    template <typename T>
    void set(const std::string_view name, T value) {
        const auto capped_name = name.substr(0, StringLength - 1);
        const std::string capped_value = std::to_string(value).substr(0, StringLength - 1);
        MetaDataField field;
        std::copy(capped_name.begin(), capped_name.end(), field.name.begin());
        std::copy(capped_value.begin(), capped_value.end(), field.value.begin());
        fields.push_back(field);
    }
};

template <>
inline void MetaData::set<const char*>(const std::string_view name, const char* value) {
    const auto capped_name = name.substr(0, StringLength - 1);
    const std::string capped_value = std::string{value}.substr(0, StringLength - 1);
    MetaDataField field;
    std::copy(capped_name.begin(), capped_name.end(), field.name.begin());
    std::copy(capped_value.begin(), capped_value.end(), field.value.begin());
    fields.push_back(field);
}

template <>
inline void MetaData::set<std::string>(const std::string_view name, std::string value) {
    set(name, value.c_str());
}

template <>
inline void MetaData::set<char*>(const std::string_view name, char* value) {
    set(name, const_cast<const char*>(value));
}

struct DataBlock {
    int handle;
    int errcode;
    int source_status;
    int signal_status;
    unsigned int rank;
    int order;
    int data_type;

    int error_type;
    int error_model;   // Identify the Error Model
    int errasymmetry;  // Flags whether error data are asymmetrical
    int error_param_n; // the Number of Model Parameters

    int data_n;
    char* data;
    char* synthetic; // Synthetic Data Array used in Client Side Error/Monte-Carlo Modelling

    char* errhi;                   // Error Array (Errors above the line: data + error)
    char* errlo;                   // Error Array (Errors below the line: data - error)
    float errparams[MaxErrParams]; // the array of model parameters

    char data_units[StringLength];
    char data_label[StringLength];
    char data_desc[StringLength];

    char error_msg[StringLength];

    Dims* dims;
    MetaData meta_data;

    ClientBlock client_block; // Used to pass properties into data reader plugins

    int opaque_type;                 // Identifies the Data Structure Type;
    int opaque_count;                // Number of Instances of the Data Structure;
    void* opaque_block;              // Opaque pointer to Hierarchical Data Structures
    unsigned int totalDataBlockSize; // The amount of data within this structure.
    unsigned int cachePermission;    // Permission for the Client to cache this structure.
};

struct DataBlockList {
    int count;
    DataBlock* data;
};

struct DataObject {
    unsigned short objectType; // File or regular object
    unsigned int objectSize;
    unsigned short hashType;
    unsigned short hashLength;
    char* md;
    char* object;
};

struct PutDataBlock : PUTDATA_BLOCK {
    int data_type;
    unsigned int rank;
    unsigned int count;
    int* shape;
    const char* data;
    int opaque_type;              // Identifies the Data Structure Type;
    int opaque_count;             // Number of Instances of the Data Structure;
    void* opaque_block;           // Opaque pointer to Hierarchical Data Structures
    unsigned int blockNameLength; // Size of the Name character string
    const char* blockName;        // Name of the Data Block
};

struct PutDataBlockList : PUTDATA_BLOCK_LIST {
    unsigned int blockCount;    // Number of data blocks
    unsigned int blockListSize; // Number of data blocks allocated
    PutDataBlock* putDataBlock; // Array of data blocks
};

// forward declaration of enum in errorLog.h
enum class ErrorType;

struct UdaError {
    ErrorType type;               // Error Classification
    int code;                     // Error Code
    char location[StringLength]; // Where this Error is Located
    char msg[StringLength];      // Message
};

struct ErrorStack : UDA_ERROR_STACK {
    unsigned int nerrors; // Number of Errors
    UdaError* idamerror;  // Array of Errors
};

struct ServerBlock {
    int version;
    int error;
    char msg[StringLength];
    int pid; // Server Application process id
    ErrorStack idamerrorstack;
    char OSName[StringLength]; // Name of the Server's Operating System, e.g. OSX
    char DOI[StringLength];    // Server version/implementation DOI - to be logged with all data consumers
    SecurityBlock
    securityBlock; // Contains encrypted tokens exchanged between client and server for mutual authentication
};

enum class Request {
    Shutdown = -30,
    ReadGeneric, // Generic Signal via the UDA Database
    ReadIDA,     // an IDA File
    ReadMDS,     // an MDSPlus Server
    ReadUDA,    // a Remote UDA server
    ReadFormat,  // Server to Choose Plugin for Requested Format
    ReadCPF,     // netCDF File
    ReadHDF5,
    ReadXML,
    ReadUFile,   // TRANSP UFile
    ReadFile,
    ReadSQL,     // Read from an SQL Data Source
    ReadPPF,     // JET PPF
    ReadJPF,     // JET JPF
    ReadNewPlugin,
    ReadNothing,    // Immediate Return without Error: Client Server Timing Tests
    ReadHData,
    ReadServerside, // Server Side Functions
    ReadUnknown,    // Plugin Not Known
    ReadWeb,        // a Remote or Local web server
    Cached,
};

inline std::string format_as(Request request)
{
    switch (request) {
        case Request::Shutdown: return "Request::Shutdown";
        case Request::ReadGeneric: return "Request::ReadGeneric";
        case Request::ReadIDA: return "Request::ReadIDA";
        case Request::ReadMDS: return "Request::ReadMDS";
        case Request::ReadUDA: return "Request::ReadUDA";
        case Request::ReadFormat: return "Request::ReadFormat";
        case Request::ReadCPF: return "Request::ReadCPF";
        case Request::ReadHDF5: return "Request::ReadHDF5";
        case Request::ReadXML: return "Request::ReadXML";
        case Request::ReadUFile: return "Request::ReadUFile";
        case Request::ReadFile: return "Request::ReadFile";
        case Request::ReadSQL: return "Request::ReadSQL";
        case Request::ReadPPF: return "Request::ReadPPF";
        case Request::ReadJPF: return "Request::ReadJPF";
        case Request::ReadNewPlugin: return "Request::ReadNewPlugin";
        case Request::ReadNothing: return "Request::ReadNothing";
        case Request::ReadHData: return "Request::ReadHData";
        case Request::ReadServerside: return "Request::ReadServerside";
        case Request::ReadUnknown: return "Request::ReadUnknown";
        case Request::ReadWeb: return "Request::ReadWeb";
        case Request::Cached: return "Request::Cached";
    }
}

struct OptionalLong {
    bool init;
    long value;
};

struct Subset {
    int nbound;                                             // the Number of Subsetting Operations
    int reform;                                             // reduce Rank if any dimension has length 1
    int order;                                              // Time Dimension order
    double bound[MaxDataRank];                        // Array of Floating point Bounding values
    OptionalLong stride[MaxDataRank];                 // Array of Integer values: Striding values
    OptionalLong ubindex[MaxDataRank];                // Array of Integer values: Bounding or Upper Index
    OptionalLong lbindex[MaxDataRank];                // Array of Integer values: Lower Index
    char operation[MaxDataRank][SxmlMaxString]; // Array of Subsetting Operations
    int dimid[MaxDataRank];                           // Array of Dimension IDs to subset
    bool isindex[MaxDataRank];                        // Flag the Operation Bound is an Integer Type
    char data_signal[SxmlMaxString];                  // Name of Signal to subset
    char member[SxmlMaxString];                       // Name of Structure Member to extract and to subset
    char function[SxmlMaxString];                     // Apply this named function to the subsetted data
};

struct RequestData {
    int request;                     // Plugin or Shutdown Server
    int exp_number;                  // Pulse No.,Tree No., etc
    int pass;                        // Pass, Sequence, etc
    char tpass[StringLength];       //
    char path[StringLength];        // Path to File, Path to Node
    char file[StringLength];        // File name, Tree name
    char format[StringLength];      // File Format
    char signal[MaxMeta];            // Signal, Node etc
    char archive[StringLength];     // Archive: pr98, transp, etc
    char device_name[StringLength]; // Device name: Mast, Jet, etc
    char server[StringLength];      // UDA, MDS+ Server
    char source[StringLength];      // Data Source, Server Host, URL etc
    char function[StringLength];    // Server-Side function or attached plugin function
    char api_delim[MaxName];         // Delimiter string to use decoding the signal and source arguments
    char subset[StringLength];      // Subset instructions
    Subset datasubset;               // Parsed subset instructions (Server Side)
    NameValueList name_value_list;     // Set of Name-Value pairs (Server Side Function)

    int put;                           // flag to set the server to a put state
    PutDataBlockList putDataBlockList; // Data to be put on the server
};

struct RequestBlock {
    int num_requests;
    RequestData* requests;
};

void free_client_put_data_block_list(PutDataBlockList* putDataBlockList);

void free_data_block(DataBlock* data_block);

void free_data_block_list(DataBlockList* data_block_list);

void free_reduced_data_block(DataBlock* data_block);

void free_request_block(RequestBlock* request_block);

// void freeRequestData(RequestData* request_data);

void free_put_data_block_list(PutDataBlockList* putDataBlockList);

} // namespace uda::client_server
