//------------------------------------------------------------------------------------------------------------------
/*!
Interprets the API arguments and assembles a Request data structure.

returns An integer Error Code: If non zero, a problem occured.
*/
//------------------------------------------------------------------------------------------------------------------

#include "makeServerRequestBlock.h"

#include <clientserver/makeRequestBlock.h>
#include <clientserver/initStructs.h>

#include "getServerEnvironment.h"

#if defined(SERVERBUILD) || defined(FATCLIENT)

int makeServerRequestBlock(REQUEST_BLOCK* request_block, PLUGINLIST pluginList)
{
    return make_request_block(request_block, pluginList, getServerEnvironment());
}

int makeServerRequestData(REQUEST_DATA* request, PLUGINLIST pluginList)
{
    return makeRequestData(request, pluginList, getServerEnvironment());
}

#endif
