#pragma once

#include <ctime>

#ifndef _WIN32
#  include <sys/select.h>
#else
#  include <winsock.h>
#endif

namespace uda::authentication
{

void update_select_params(int fd, fd_set* rfds, struct timeval* tv);

}
