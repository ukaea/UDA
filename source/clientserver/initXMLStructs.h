#ifndef UDA_CLIENTSERVER_INITXMLSTRUCTS_H
#define UDA_CLIENTSERVER_INITXMLSTRUCTS_H

#include "export.h"
#include "xmlStructs.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API void initEfit(EFIT* str);
LIBRARY_API void initInstance(INSTANCE* str);
LIBRARY_API void initFluxLoop(FLUXLOOP* str);
LIBRARY_API void initPfPassive(PFPASSIVE* str);
LIBRARY_API void initPfCoils(PFCOILS* str);
LIBRARY_API void initMagProbe(MAGPROBE* str);
LIBRARY_API void initPfSupplies(PFSUPPLIES* str);
LIBRARY_API void initPfCircuits(PFCIRCUIT* str);
LIBRARY_API void initPlasmaCurrent(PLASMACURRENT* str);
LIBRARY_API void initDiaMagnetic(DIAMAGNETIC* str);
LIBRARY_API void initToroidalField(TOROIDALFIELD* str);
LIBRARY_API void initLimiter(LIMITER* str);

// Print Utilities

LIBRARY_API void printInstance(INSTANCE str);
LIBRARY_API void printMagProbe(MAGPROBE str);
LIBRARY_API void printPfSupplies(PFSUPPLIES str);
LIBRARY_API void printPfCircuits(PFCIRCUIT str);
LIBRARY_API void printFluxLoop(FLUXLOOP str);
LIBRARY_API void printPfCoils(PFCOILS str);
LIBRARY_API void printPfPassive(PFPASSIVE str);
LIBRARY_API void printPlasmaCurrent(PLASMACURRENT str);
LIBRARY_API void printDiaMagnetic(DIAMAGNETIC str);
LIBRARY_API void printToroidalField(TOROIDALFIELD str);
LIBRARY_API void printLimiter(LIMITER str);
LIBRARY_API void printEFIT(EFIT str);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_INITXMLSTRUCTS_H
