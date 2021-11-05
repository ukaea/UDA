#pragma once

#ifndef UDA_SOURCE_CLIENT2_CLIENT_ENVIRONMENT_H
#define UDA_SOURCE_CLIENT2_CLIENT_ENVIRONMENT_H

#include <clientserver/udaStructs.h>

namespace uda {
namespace client {

Environment load_environment(bool* env_host, bool* env_port);
void print_client_environment(const Environment& environment);

}
}

#endif //UDA_SOURCE_CLIENT2_CLIENT_ENVIRONMENT_H
