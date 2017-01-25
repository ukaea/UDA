#ifndef IDAM_CLIENTSERVER_INITXMLSTRUCTS_H
#define IDAM_CLIENTSERVER_INITXMLSTRUCTS_H

#ifdef HIERARCHICAL_DATA

#include "idamclientserver.h"
#include "idamclientserverxml.h"

void initEfit(EFIT *str);
void initInstance(INSTANCE *str);
void initFluxLoop(FLUXLOOP *str);
void initPfPassive(PFPASSIVE *str);
void initPfCoils(PFCOILS *str);
void initMagProbe(MAGPROBE *str);
void initPfSupplies(PFSUPPLIES *str);
void initPfCircuits(PFCIRCUIT *str);
void initPlasmaCurrent(PLASMACURRENT *str);
void initDiaMagnetic(DIAMAGNETIC *str);
void initToroidalField(TOROIDALFIELD *str);
void initLimiter(LIMITER *str);

// Print Utilities

void printInstance(FILE *fh, INSTANCE str);
void printMagProbe(FILE *fh, MAGPROBE str);
void printPfSupplies(FILE *fh, PFSUPPLIES str);
void printPfCircuits(FILE *fh, PFCIRCUIT str);
void printFluxLoop(FILE *fh, FLUXLOOP str);
void printPfCoils(FILE *fh, PFCOILS str);
void printPfPassive(FILE *fh, PFPASSIVE str);
void printPlasmaCurrent(FILE *fh, PLASMACURRENT str);
void printDiaMagnetic(FILE *fh, DIAMAGNETIC str);
void printToroidalField(FILE *fh, TOROIDALFIELD str);
void printLimiter(FILE *fh, LIMITER str);
void printEFIT(FILE *fh, EFIT str);

#endif

#endif // IDAM_CLIENTSERVER_INITXMLSTRUCTS_H
