#pragma once

#ifndef UDA_CLIENT_UDACLIENT_H
#  define UDA_CLIENT_UDACLIENT_H

#  include "structures/genStructs.h"
#  include "clientserver/udaStructs.h"

typedef struct ClientFlags {
    int get_dimdble;  // (Server Side) Return Dimensional Data in Double Precision
    int get_timedble; // (Server Side) Server Side cast of time dimension to double precision if in compresed format
    int get_scalar;   // (Server Side) Reduce Rank from 1 to 0 (Scalar) if time data are all zero
    int get_bytes;    // (Server Side) Return IDA Data in native byte or integer array without IDA signal's
    // calibration factor applied
    int get_meta;     // (Server Side) return All Meta Data
    int get_asis;     // (Server Side) Apply no XML based corrections to Data or Dimensions
    int get_uncal;    // (Server Side) Apply no XML based Calibrations to Data
    int get_notoff;   // (Server Side) Apply no XML based Timing Corrections to Data
    int get_nodimdata;

    int get_datadble;  // (Client Side) Return Data in Double Precision
    int get_bad;       // (Client Side) return data with BAD Status value
    int get_synthetic; // (Client Side) Return Synthetic Data if available instead of Original data

    uint32_t flags;

    int user_timeout;
    int alt_rank;
} CLIENT_FLAGS;

DATA_BLOCK* getDataBlock(int handle);

void udaPutThreadServerBlock(SERVER_BLOCK* str);

void udaPutThreadClientBlock(CLIENT_BLOCK* str);

SERVER_BLOCK udaGetThreadServerBlock();

CLIENT_BLOCK udaGetThreadClientBlock();

CLIENT_FLAGS* udaClientFlags();

unsigned int* udaPrivateFlags();

int idamClient(REQUEST_BLOCK* request_block, int* indices);

void updateClientBlock(CLIENT_BLOCK* str, const CLIENT_FLAGS* client_flags, unsigned int private_flags);

void setUserDefinedTypeList(USERDEFINEDTYPELIST* userdefinedtypelist);

void setLogMallocList(LOGMALLOCLIST* logmalloclist_in);

#endif // UDA_CLIENT_UDACLIENT_H