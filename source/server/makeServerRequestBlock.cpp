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

void initServerRequestBlock(REQUEST_BLOCK* str)
{
    str->request = 0;
    str->exp_number = 0;
    str->pass = -1;
    str->tpass[0] = '\0';
    str->path[0] = '\0';
    str->file[0] = '\0';
    str->format[0] = '\0';
    str->archive[0] = '\0';
    str->device_name[0] = '\0';
    str->server[0] = '\0';
    str->function[0] = '\0';
    str->api_delim[0] = '\0';
    str->signal[0] = '\0';
    str->source[0] = '\0';
    str->subset[0] = '\0';
    str->datasubset.subsetCount = 0;
    initNameValueList(&str->nameValueList);
}

int makeServerRequestBlock(REQUEST_BLOCK* request_block, PLUGINLIST pluginList)
{
    return make_request_block(request_block, pluginList, getServerEnvironment());
}

#endif
