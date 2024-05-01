#pragma once

#define UDA_MAJOR_VERSION(X) (int)((X >> 24) & 0x000F)
#define UDA_MINOR_VERSION(X) (int)((X >> 16) & 0x000F)
#define UDA_BUGFIX_VERSION(X) (int)((X >> 8) & 0x000F)
#define UDA_DELTA_VERSION(X) (int)((X >> 0) & 0x000F)

#define UDA_VERSION(MAJOR, MINOR, BUGFIX, DELTA) (MAJOR << 24) | (MINOR << 16) | (BUGFIX << 8) | (DELTA)

namespace uda::client_server {

int get_protocol_version(int version);

}
