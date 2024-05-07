#pragma once

#define UDA_GET_MAJOR_VERSION(X) (int)((X >> 24) & 0x000F)
#define UDA_GET_MINOR_VERSION(X) (int)((X >> 16) & 0x000F)
#define UDA_GET_BUGFIX_VERSION(X) (int)((X >> 8) & 0x000F)
#define UDA_GET_DELTA_VERSION(X) (int)((X >> 0) & 0x000F)

#define UDA_GET_VERSION(MAJOR, MINOR, BUGFIX, DELTA) (MAJOR << 24) | (MINOR << 16) | (BUGFIX << 8) | (DELTA)

namespace uda::client_server {

int get_protocol_version(int version);

}
