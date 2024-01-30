#ifndef UDA_CLIENTSERVER_INITXMLSTRUCTS_H
#define UDA_CLIENTSERVER_INITXMLSTRUCTS_H

#include "xmlStructs.h"

void initEfit(EFIT* str);
void initInstance(INSTANCE* str);
void initFluxLoop(FLUXLOOP* str);
void initPfPassive(PFPASSIVE* str);
void initPfCoils(PFCOILS* str);
void initMagProbe(MAGPROBE* str);
void initPfSupplies(PFSUPPLIES* str);
void initPfCircuits(PFCIRCUIT* str);
void initPlasmaCurrent(PLASMACURRENT* str);
void initDiaMagnetic(DIAMAGNETIC* str);
void initToroidalField(TOROIDALFIELD* str);
void initLimiter(LIMITER* str);

// Print Utilities

void printInstance(INSTANCE str);
void printMagProbe(MAGPROBE str);
void printPfSupplies(PFSUPPLIES str);
void printPfCircuits(PFCIRCUIT str);
void printFluxLoop(FLUXLOOP str);
void printPfCoils(PFCOILS str);
void printPfPassive(PFPASSIVE str);
void printPlasmaCurrent(PLASMACURRENT str);
void printDiaMagnetic(DIAMAGNETIC str);
void printToroidalField(TOROIDALFIELD str);
void printLimiter(LIMITER str);
void printEFIT(EFIT str);

#endif // UDA_CLIENTSERVER_INITXMLSTRUCTS_H
