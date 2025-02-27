#pragma once

namespace uda::client_server {

//--------------------------------------------------------
// Size Definitions

constexpr unsigned int StringLength = 1024;
constexpr unsigned int MaxStringLength = StringLength; // Ensure same as StringLength
constexpr unsigned int DateLength = 1024;

constexpr int MaxDate = 12;
constexpr int MaxName = StringLength;
constexpr int MaxFilename = StringLength;
constexpr int MaxServer = StringLength;
constexpr int MaxPath = StringLength;
constexpr int MaxFormat = StringLength;
constexpr int MaxDesc = StringLength;
constexpr int MaxMeta = 10 * StringLength;

constexpr int MaxErrParams = 8;

constexpr int MaxRank2 = 10; // Number of sub-setting dimensions

constexpr int SxmlMaxString = 1024;
constexpr int MaxDataRank = 8;

constexpr int Md5Size = 16;

constexpr int MaxLoop = 10'000;
constexpr int MaxBlock = 1'000; // msecs

constexpr int DBReadBlockSize = 32 * 1024; // 16384
constexpr int DBWriteBlockSize = 32 * 1024; // 16384

constexpr int GrowPutdataBlockList = 10;

//--------------------------------------------------------
// Client Specified Properties

constexpr int TimeOut = 600; // Server Shutdown after this time (Secs)
constexpr bool CompressDim = true; // Compress regular dimensional data

//--------------------------------------------------------
// Private Flags: Properties passed to IDAM clients called by servers (32 bits)

namespace private_flags {

constexpr unsigned char FullReset = 0b1111'1101; // Reset flags except EXTERNAL
constexpr unsigned char XdrFile = 0b0000'0001; // Use an intermediate file containing the XDR data rather than a data stream
constexpr unsigned char External = 0b0000'0010; // The originating server is an External Facing server
constexpr unsigned char Cache = 0b0000'0100; // Cache all data
constexpr unsigned char XdrObject = 0b0000'1000; // Use an intermediate XDR data object rather than a data stream

}

//--------------------------------------------------------
// Client Flags: Client specified local properties (32 bit)

namespace client_flags {

constexpr unsigned char FullReset = 0b1111'1111; // Reset flags
constexpr unsigned char AltData = 0b0000'0001;
constexpr unsigned char XdrFile = 0b0000'0010; // Use an intermediate file with the XDR data rather than a data stream
constexpr unsigned char Cache = 0b0000'0100; // Access data from the local cache and write new data to cache
constexpr unsigned char CloseDown = 0b0000'1000; // Immediate Closedown
constexpr unsigned char XdrObject = 0b0001'0000; // Use a XDR object in memory
constexpr unsigned char ReuseLastHandle = 0b0010'0000; // Reuse the last issued handle value (for this thread) - assume application has freed heap
constexpr unsigned char FreeReuseLastHandle = 0b0100'0000; // Free the heap associated with the last issued handle and reuse the handle value
constexpr unsigned char FileCache = 0b1000'0000; // Access data from and save data to local cache files

}

//--------------------------------------------------------
// Error Models

constexpr unsigned long int ErrorModelSeed = 12345;

enum class ErrorModelType : int {
    Unknown = 0,
    Default = 1,
    DefaultAsymmetric = 2,
    Gaussian = 3,
    Reseed = 4,
    GaussianShift = 5,
    Poisson = 6,
    Undefined = 7,
};

//--------------------------------------------------------
// Caching

enum class PluginCachePermission {
    NotOkToCache = 0, // Plugin state management incompatible with client side cacheing
    OkToCache = 1, // Data are OK for the Client to Cache
};

// The cache permission to use as the default
constexpr PluginCachePermission CachePermissionDefault = PluginCachePermission::NotOkToCache;

//--------------------------------------------------------
// Character used to separate directory file path elements

#ifndef _WIN32
constexpr const char* PathSeparator = "/";
#else
constexpr const char* PathSeparator = r"\\";
#endif

//--------------------------------------------------------
// QA Status

constexpr int DefaultStatus = 1; // Default Signal and Data_Source Status value

}
