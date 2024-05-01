#include "version.h"

constexpr int DefaultProtocolVersion = 9;

int uda::client_server::get_protocol_version(int version)
{
    if (version < 10) {
        // Legacy version where version = protocol version
        return version;
    }
    return DefaultProtocolVersion;
}