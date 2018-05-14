#ifndef IDAM_SERVER_APPLYXML_H
#define IDAM_SERVER_APPLYXML_H

#include <clientserver/parseXML.h>
#include <clientserver/udaStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

int idamserverParseSignalXML(DATA_SOURCE data_source, SIGNAL signal, SIGNAL_DESC signal_desc,
                             ACTIONS *actions_desc, ACTIONS *actions_sig);
void applyCalibration(int type, int ndata, double factor, double offset, int invert, char *array);
void idamserverApplySignalXML(CLIENT_BLOCK client_block, DATA_SOURCE *data_source, SIGNAL *signal, SIGNAL_DESC *signal_desc,
                              DATA_BLOCK *data_block, ACTIONS actions);
void idamserverDeselectSignalXML(ACTIONS *actions_desc, ACTIONS *actions_sig);

#ifdef __cplusplus
}
#endif

#endif // IDAM_SERVER_APPLYXML_H


