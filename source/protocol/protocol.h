#pragma once

#include <rpc/types.h>
#include <rpc/xdr.h>
#include <string>
#include <fmt/format.h>

#include "structures/genStructs.h"
#include "xdr_lib.h"

//-------------------------------------------------------
// Client Server Conversation Protocols

namespace uda::protocol {

enum class ProtocolId : int {
    Start = 0,
    RequestBlock = 1,
    DataBlockList = 2,
    NextProtocol = 3,
    // DataSystem = 4,
    // SystemConfig = 5,
    // DataSource = 6,
    // Signal = 7,
    // SignalDesc = 8,
    Spare1 = 9,
    ClientBlock = 10,
    ServerBlock = 11,
    Spare2 = 12,
    CloseDown = 13,
    Sleep = 14,
    WakeUp = 15,
    PutdataBlockList = 16,
    // SecurityBlock = 17,
    Object = 18,
    SerialiseObject = 19,
    SerialiseFile = 20,
    DataObject = 21,
    DataObjectFile = 22,
    MetaData = 23,
    RegularStop = 99,

    OpaqueStart = 100,
    Structures = 101,
    Meta = 102, // XML metadata structure
    OpaqueStop = 200,
};

std::string format_as(ProtocolId protocol);

//---------------------------------------------------------------------------------------------------
// Client Server XDR data Streams (DON'T CHANGE ORDER or Legacy client won't work!)

void set_select_params(int fd, fd_set* rfds, timeval* tv, int* server_tot_block_time);

void update_select_params(int fd, fd_set* rfds, timeval* tv, int server_tot_block_time);

int protocol2(std::vector<client_server::UdaError>& error_stack, XDR* xdrs, ProtocolId protocol_id, XDRStreamDirection direction, ProtocolId* token, structures::LogMallocList* logmalloclist,
              structures::UserDefinedTypeList* userdefinedtypelist, void* str, int protocolVersion,
              structures::LogStructList* log_struct_list, unsigned int private_flags, int malloc_source);

} // namespace uda::client_server
