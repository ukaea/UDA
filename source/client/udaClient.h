#pragma once

#include "clientserver/uda_structs.h"
#include "structures/genStructs.h"

namespace uda::client
{

typedef struct ClientFlags {
    int get_dimdble;  // (Server Side) Return Dimensional Data in Double Precision
    int get_timedble; // (Server Side) Server Side cast of time dimension to double precision if in compresed format
    int get_scalar;   // (Server Side) Reduce Rank from 1 to 0 (Scalar) if time data are all zero
    int get_bytes;    // (Server Side) Return IDA Data in native byte or integer array without IDA signal's
    // calibration factor applied
    int get_meta;   // (Server Side) return All Meta Data
    int get_asis;   // (Server Side) Apply no XML based corrections to Data or Dimensions
    int get_uncal;  // (Server Side) Apply no XML based Calibrations to Data
    int get_notoff; // (Server Side) Apply no XML based Timing Corrections to Data
    int get_nodimdata;

    int get_datadble;  // (Client Side) Return Data in Double Precision
    int get_bad;       // (Client Side) return data with BAD Status value
    int get_synthetic; // (Client Side) Return Synthetic Data if available instead of Original data

    uint32_t flags;

    int user_timeout;
    int alt_rank;
} CLIENT_FLAGS;

uda::client_server::DataBlock* getDataBlock(int handle);

void udaPutThreadServerBlock(uda::client_server::ServerBlock* str);

void udaPutThreadClientBlock(uda::client_server::ClientBlock* str);

void udaPutServerSocket(int socket);

int udaGetServerSocket();

uda::client_server::ServerBlock udaGetThreadServerBlock();

uda::client_server::ClientBlock udaGetThreadClientBlock();

CLIENT_FLAGS* udaClientFlags();

unsigned int* udaPrivateFlags();

int udaClient(uda::client_server::RequestBlock* request_block, int* indices);

void updateClientBlock(uda::client_server::ClientBlock* str, const CLIENT_FLAGS* client_flags,
                       unsigned int private_flags);

void setUserDefinedTypeList(uda::structures::UserDefinedTypeList* userdefinedtypelist);

void setLogMallocList(uda::structures::LogMallocList* logmalloclist_in);

} // namespace uda::client
