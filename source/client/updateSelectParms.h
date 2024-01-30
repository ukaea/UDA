#pragma once

#ifndef UDA_CLIENT_UPDATESELECTPARMS_H
#  define UDA_CLIENT_UPDATESELECTPARMS_H

#  include <ctime>

#  ifndef _WIN32
#    include <sys/select.h>
#  else
#    include <winsock.h>
#  endif

void udaUpdateSelectParms(int fd, fd_set* rfds, struct timeval* tv);

#endif // UDA_CLIENT_UPDATESELECTPARMS_H
