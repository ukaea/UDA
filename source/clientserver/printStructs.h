#ifndef IDAM_PRINTSTRUCTS_H
#define IDAM_PRINTSTRUCTS_H

#include "idamStructs.h"

void printRequestBlock(REQUEST_BLOCK str);

void printClientBlock(CLIENT_BLOCK str);

void printServerBlock(SERVER_BLOCK str);

void printDataBlock(DATA_BLOCK str);

void printSystemConfig(SYSTEM_CONFIG str);

void printDataSystem(DATA_SYSTEM str);

void printDataSource(DATA_SOURCE str);

void printSignal(SIGNAL str);

void printSignalDesc(SIGNAL_DESC str);

void printPerformance(PERFORMANCE str);

//-------------------------------------------------------------------------------------------------------------------

#ifdef IDAM_IDAMCLIENTPUBLIC_H    // Client Side only

void printIdamRequestBlock(FILE *fh, REQUEST_BLOCK str);
void printIdamClientBlock(FILE *fh, CLIENT_BLOCK str);
void printIdamServerBlock(FILE *fh, SERVER_BLOCK str);
void printIdamDataBlock(FILE *fh, DATA_BLOCK str);
void printIdamSystemConfig(FILE *fh, SYSTEM_CONFIG str);
void printIdamDataSystem(FILE *fh, DATA_SYSTEM str);
void printIdamDataSource(FILE *fh, DATA_SOURCE str);
void printIdamSignal(FILE *fh, SIGNAL str);
void printIdamSignalDesc(FILE *fh, SIGNAL_DESC str);
void printIdamPerformance(FILE *fh, PERFORMANCE str);

#endif

#endif // IDAM_PRINTSTRUCTS_H

