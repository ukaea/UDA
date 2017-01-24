#ifndef IDAM_IDAMSERVERGETDATA2_H
#define IDAM_IDAMSERVERGETDATA2_H

#include <clientserver/parseXML.h>
#include "sqllib.h"

int idamserverGetData(PGconn* DBConnect, int* depth, REQUEST_BLOCK request_block, CLIENT_BLOCK client_block,
                      DATA_BLOCK* data_block, DATA_SOURCE* data_source, SIGNAL* signal_rec, SIGNAL_DESC* signal_desc,
                      ACTIONS* actions_desc, ACTIONS* actions_sig);

int idamserverSwapSignalError(DATA_BLOCK* data_block, DATA_BLOCK* data_block2, int asymmetry);

int idamserverSwapSignalDim(DIMCOMPOSITE dimcomposite, DATA_BLOCK* data_block, DATA_BLOCK* data_block2);

int idamserverSwapSignalDimError(DIMCOMPOSITE dimcomposite, DATA_BLOCK* data_block, DATA_BLOCK* data_block2,
                                 int asymmetry);

int idamserverReadData(PGconn* DBConnect, REQUEST_BLOCK request_block, CLIENT_BLOCK client_block,
                       DATA_BLOCK* data_block, DATA_SOURCE* data_source, SIGNAL* signal_rec, SIGNAL_DESC* signal_desc);

#endif // IDAM_IDAMSERVERGETDATA2_H

