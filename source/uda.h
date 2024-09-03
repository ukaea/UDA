#ifndef UDA_H
#define UDA_H

#include <clientserver/udaTypes.h>
#include <clientserver/initStructs.h>

#ifdef UDA_CLIENT2
#  include <client2/udaGetAPI.h>
#  include <client2/udaPutAPI.h>
#  include <client2/accAPI.h>
#else
#  include <client/udaGetAPI.h>
#  include <client/udaPutAPI.h>
#  include <client/accAPI.h>
#  include <client/udaClient.h>
#  include <client/legacy_client.h>
#  include <client/legacy_accAPI.h>
#endif

#endif // UDA_H
