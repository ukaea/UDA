#ifndef IDAM_PARSEEFITXMLPRIVATE_H
#define IDAM_PARSEEFITXMLPRIVATE_H

//---------------------------------------------------------------------------
// Prototypes to Private Functions

void initInstance(INSTANCE * str);
void initFluxLoop(FLUXLOOP * str);
void initPfPassive(PFPASSIVE * str);
void initPfCoils(PFCOILS * str);
void initMagProbe(MAGPROBE * str);
void initPfSupplies(PFSUPPLIES * str);
void initPfCircuits(PFCIRCUIT * str);
void initPlasmaCurrent(PLASMACURRENT * str);
void initDiaMagnetic(DIAMAGNETIC * str);
void initToroidalField(TOROIDALFIELD * str);
void initLimiter(LIMITER * str);
//void initPolarimetry(POLARIMETRY * str);
//void initInterferometry(INTERFEROMETRY * str);
#ifdef JETMSEXML
void initMSE_Info(MSE_INFO * str);
void initMSE(MSE * str);
#endif

void printEFIT(EFIT str);
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
//void printPolarimetry(POLARIMETRY str);
//void printInterferometry(INTERFEROMETRY str);
#ifdef JETMSEXML
void printMSE_Info(MSE_INFO str);
void printMSE(MSE str);
#endif

int alloc_efit(EFIT * efit);
int alloc_pfcircuit(PFCIRCUIT * str);
int alloc_pfcoils(PFCOILS * str);
int alloc_pfpassive(PFPASSIVE * str);
int alloc_fluxloop(FLUXLOOP * str);
int alloc_limiter(LIMITER * str);

float * parseFloatArray(xmlDocPtr doc, xmlNodePtr cur, char * target, int * n);
void parseFloat(xmlDocPtr doc, xmlNodePtr cur, char * target, float * value);
void parseInt(xmlDocPtr doc, xmlNodePtr cur, char * target, int * value);
int * parseIntArray(xmlDocPtr doc, xmlNodePtr cur, char * target, int * n);
float * parseFloatAngleArray(xmlDocPtr doc, xmlNodePtr cur, char * target, int * n);
void parseFloatAngle(xmlDocPtr doc, xmlNodePtr cur, char * target, float * value);
void parseInstance(xmlNodePtr cur, INSTANCE * str);
char ** parseStringArray(xmlDocPtr doc, xmlNodePtr cur, char * target, int * n, char * comment);

MAGPROBE * parseMagProbe(xmlDocPtr doc, xmlNodePtr cur, MAGPROBE * str, int * np);
FLUXLOOP * parseFluxLoop(xmlDocPtr doc, xmlNodePtr cur, FLUXLOOP * str, int * np);
PFCOILS * parsePfCoils(xmlDocPtr doc, xmlNodePtr cur, PFCOILS * str, int * np);
PFPASSIVE * parsePfPassive(xmlDocPtr doc, xmlNodePtr cur, PFPASSIVE * str, int * np);
PFSUPPLIES * parsePfSupplies(xmlDocPtr doc, xmlNodePtr cur, PFSUPPLIES * str, int * np);
PFCIRCUIT * parsePfCircuits(xmlDocPtr doc, xmlNodePtr cur, PFCIRCUIT * str, int * np);
PLASMACURRENT * parsePlasmaCurrent(xmlDocPtr doc,xmlNodePtr cur,PLASMACURRENT * str);
DIAMAGNETIC * parseDiamagnetic(xmlDocPtr doc,xmlNodePtr cur,DIAMAGNETIC * str);
TOROIDALFIELD * parseToroidalField(xmlDocPtr doc,xmlNodePtr cur,TOROIDALFIELD * str);
LIMITER * parseLimiter(xmlDocPtr doc,xmlNodePtr cur,LIMITER * str);
//POLARIMETRY * parsePolarimetry(xmlDocPtr doc,xmlNodePtr cur,POLARIMETRY * str, int * np);
//INTERFEROMETRY * parseInterferometry(xmlDocPtr doc,xmlNodePtr cur,INTERFEROMETRY * str, int * np);
#ifdef JETMSEXML
MSE * parseMSE(xmlDocPtr doc, xmlNodePtr cur, MSE_INFO * info, MSE * str, int * np);
#endif

#ifdef SQL_ENABLE
PGconn * startItmSQL();
void sqlItmXML(PGconn * DBConnect, char * device, char * xml);
#endif

void StringtoFortran(char * srce, int lsrce);
char * TrimString( char * szSource );
char * convertNonPrintable(char * str);

void getfinstance(INSTANCE * str, int * seq, int * status, float * factor,
                         char * archive, char * file, char * signal, char * owner, char * format,
                         int l3,        int l4,     int l5,       int l6,      int l7      );


#endif // IDAM_PARSEEFITXMLPRIVATE_H
