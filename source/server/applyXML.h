#ifndef IDAM_APPLYXML_H
#define IDAM_APPLYXML_H

#include <clientserver/parseXML.h>
#include <clientserver/idamStructs.h>

int idamserverParseSignalXML(DATA_SOURCE data_source, SIGNAL signal, SIGNAL_DESC signal_desc,
                             ACTIONS *actions_desc, ACTIONS *actions_sig);
void applyCalibration(int type, int ndata, double factor, double offset, int invert, char *array);
void idamserverApplySignalXML(CLIENT_BLOCK client_block, DATA_SOURCE *data_source, SIGNAL *signal, SIGNAL_DESC *signal_desc,
                              DATA_BLOCK *data_block, ACTIONS actions);
void idamserverDeselectSignalXML(ACTIONS *actions_desc, ACTIONS *actions_sig);

#endif // IDAM_APPLYXML_H


