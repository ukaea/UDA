#ifndef UDA_SERVER_APPLYXML_H
#define UDA_SERVER_APPLYXML_H

#include <clientserver/parseXML.h>
#include <clientserver/udaStructs.h>
#include <clientserver/export.h>

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int serverParseSignalXML(DATA_SOURCE data_source, SIGNAL signal, SIGNAL_DESC signal_desc,
                                     ACTIONS *actions_desc, ACTIONS *actions_sig);

LIBRARY_API void serverApplySignalXML(CLIENT_BLOCK client_block, DATA_SOURCE *data_source, SIGNAL *signal,
                                      SIGNAL_DESC *signal_desc, DATA_BLOCK *data_block, ACTIONS actions);

LIBRARY_API void serverDeselectSignalXML(ACTIONS *actions_desc, ACTIONS *actions_sig);

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_APPLYXML_H
