//! $LastChangedRevision: 226 $
//! $LastChangedDate: 2011-02-15 10:28:26 +0000 (Tue, 15 Feb 2011) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/include/parseEfitXMLPrivate.h $

// Change History
//
// 18Dec2008    dgm Holds all Private components of the library
// 21Jub2010    dgm MSE functions controlled using using JETMSEXML
//----------------------------------------------------------------------------

#ifndef parseEfitXMLPrivateInclude
#define parseEfitXMLPrivateInclude

//---------------------------------------------------------------------------
// Prototypes to Private Functions

static void initInstance(INSTANCE * str);
static void initFluxLoop(FLUXLOOP * str);
static void initPfPassive(PFPASSIVE * str);
static void initPfCoils(PFCOILS * str);
static void initMagProbe(MAGPROBE * str);
static void initPfSupplies(PFSUPPLIES * str);
static void initPfCircuits(PFCIRCUIT * str);
static void initPlasmaCurrent(PLASMACURRENT * str);
static void initDiaMagnetic(DIAMAGNETIC * str);
static void initToroidalField(TOROIDALFIELD * str);
static void initLimiter(LIMITER * str);
static void initPolarimetry(POLARIMETRY * str);
static void initInterferometry(INTERFEROMETRY * str);
#ifdef JETMSEXML
static void initMSE_Info(MSE_INFO * str);
static void initMSE(MSE * str);
#endif

static void printEFIT(FILE * fh, EFIT str);
static void printInstance(FILE * fh, INSTANCE str);
static void printMagProbe(FILE * fh, MAGPROBE str);
static void printPfSupplies(FILE * fh, PFSUPPLIES str);
static void printPfCircuits(FILE * fh, PFCIRCUIT str);
static void printFluxLoop(FILE * fh, FLUXLOOP str);
static void printPfCoils(FILE * fh, PFCOILS str);
static void printPfPassive(FILE * fh, PFPASSIVE str);
static void printPlasmaCurrent(FILE * fh, PLASMACURRENT str);
static void printDiaMagnetic(FILE * fh, DIAMAGNETIC str);
static void printToroidalField(FILE * fh, TOROIDALFIELD str);
static void printLimiter(FILE * fh, LIMITER str);
static void printPolarimetry(FILE * fh, POLARIMETRY str);
static void printInterferometry(FILE * fh, INTERFEROMETRY str);
#ifdef JETMSEXML
static void printMSE_Info(FILE * fh, MSE_INFO str);
static void printMSE(FILE * fh, MSE str);
#endif

static int alloc_efit(EFIT * efit);
static int alloc_pfcircuit(PFCIRCUIT * str);
static int alloc_pfcoils(PFCOILS * str);
static int alloc_pfpassive(PFPASSIVE * str);
static int alloc_fluxloop(FLUXLOOP * str);
static int alloc_limiter(LIMITER * str);

static float * parseFloatArray(xmlDocPtr doc, xmlNodePtr cur, char * target, int * n);
static void parseFloat(xmlDocPtr doc, xmlNodePtr cur, char * target, float * value);
static void parseInt(xmlDocPtr doc, xmlNodePtr cur, char * target, int * value);
static int * parseIntArray(xmlDocPtr doc, xmlNodePtr cur, char * target, int * n);
static float * parseFloatAngleArray(xmlDocPtr doc, xmlNodePtr cur, char * target, int * n);
static void parseFloatAngle(xmlDocPtr doc, xmlNodePtr cur, char * target, float * value);
static void parseInstance(xmlNodePtr cur, INSTANCE * str);
static char ** parseStringArray(xmlDocPtr doc, xmlNodePtr cur, char * target, int * n, char * comment);

static MAGPROBE * parseMagProbe(xmlDocPtr doc, xmlNodePtr cur, MAGPROBE * str, int * np);
static FLUXLOOP * parseFluxLoop(xmlDocPtr doc, xmlNodePtr cur, FLUXLOOP * str, int * np);
static PFCOILS * parsePfCoils(xmlDocPtr doc, xmlNodePtr cur, PFCOILS * str, int * np);
static PFPASSIVE * parsePfPassive(xmlDocPtr doc, xmlNodePtr cur, PFPASSIVE * str, int * np);
static PFSUPPLIES * parsePfSupplies(xmlDocPtr doc, xmlNodePtr cur, PFSUPPLIES * str, int * np);
static PFCIRCUIT * parsePfCircuits(xmlDocPtr doc, xmlNodePtr cur, PFCIRCUIT * str, int * np);
static PLASMACURRENT * parsePlasmaCurrent(xmlDocPtr doc,xmlNodePtr cur,PLASMACURRENT * str);
static DIAMAGNETIC * parseDiamagnetic(xmlDocPtr doc,xmlNodePtr cur,DIAMAGNETIC * str);
static TOROIDALFIELD * parseToroidalField(xmlDocPtr doc,xmlNodePtr cur,TOROIDALFIELD * str);
static LIMITER * parseLimiter(xmlDocPtr doc,xmlNodePtr cur,LIMITER * str);
static POLARIMETRY * parsePolarimetry(xmlDocPtr doc,xmlNodePtr cur,POLARIMETRY * str, int * np);
static INTERFEROMETRY * parseInterferometry(xmlDocPtr doc,xmlNodePtr cur,INTERFEROMETRY * str, int * np);
#ifdef JETMSEXML
static MSE * parseMSE(xmlDocPtr doc, xmlNodePtr cur, MSE_INFO * info, MSE * str, int * np);
#endif

#ifdef SQL_ENABLE
static PGconn * startItmSQL();
static void sqlItmXML(PGconn * DBConnect, char * device, char * xml);
#endif

static void StringtoFortran(char * srce, int lsrce);
static char * TrimString( char * szSource );
static char * convertNonPrintable(char * str);

static void getfinstance(INSTANCE * str, int * seq, int * status, float * factor,
                         char * archive, char * file, char * signal, char * owner, char * format,
                         int l3,        int l4,     int l5,       int l6,      int l7      );


#endif
