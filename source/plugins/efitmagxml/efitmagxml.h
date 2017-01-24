
// Plugin EFITMAGXML

#ifndef IdamEfitMagXMLPluginInclude
#define IdamEfitMagXMLPluginInclude

// Change History:
//
// 06Sept2016	dgm	Original Version


#include <idamplugin.h>

#ifdef __cplusplus
static "C" {
#endif

#define THISPLUGIN_VERSION            1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1        // Interface versions higher than this will not be understood!
#define THISPLUGIN_DEFAULT_METHOD        "help"

extern int efitmagxml(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

#include <libxml/xmlmemory.h> 
#include <libxml/parser.h> 

#define READFILE    1        // The XML Source is an statical File

#define XMLMAXSTRING    56
#define XMLMAX    200*1024
#define XMLMAXLOOP    1024        // Max Number of Array elements

//--------------------------------------------------------------------------- 
// Flux Loop Data Structures 

typedef struct {
    char archive[XMLMAXSTRING];         // Data Archive Name
    char file[XMLMAXSTRING];            // Data File Name
    char signal[XMLMAXSTRING];          // Signal Name (Generic or Specific)
    char owner[XMLMAXSTRING];           // Owner Name
    char format[XMLMAXSTRING];          // Data Format
    int seq;                            // Data Sequence or Pass
    int status;                         // Signal Status
    float factor;                       // Scaling Factor
// int  handle;			                // Clientside IDAM API Data Handle
} INSTANCE;

typedef struct {
    char id[XMLMAXSTRING];        // ID
    INSTANCE instance;

    float aerr;                // Absolute Error
    float rerr;                // Relative Error
} TOROIDALFIELD;


typedef struct {
    char id[XMLMAXSTRING];        // ID
    INSTANCE instance;

    float aerr;                // Absolute Error
    float rerr;                // Relative Error
} PLASMACURRENT;


typedef struct {
    char id[XMLMAXSTRING];        // ID
    INSTANCE instance;

    float aerr;                // Absolute Error
    float rerr;                // Relative Error
} DIAMAGNETIC;

typedef struct {
    char id[XMLMAXSTRING];        // ID
    INSTANCE instance;

    int nco;                // Number of Coils
    int* coil;                // List of Coil Connections
    int supply;                // Supply Connections
} PFCIRCUIT;

typedef struct {
    char id[XMLMAXSTRING];        // ID
    INSTANCE instance;

    float r;                // Radial Position
    float z;                // Z Position
    float angle;                // Angle
    float aerr;                // Absolute Error
    float rerr;                // Relative Error
} MAGPROBE;


typedef struct {
    char id[XMLMAXSTRING];        // ID
    INSTANCE instance;

    float aerr;                // Absolute Error
    float rerr;                // Relative Error
} PFSUPPLIES;

typedef struct {
    char id[XMLMAXSTRING];        // ID
    INSTANCE instance;

    int nco;                // Number of Coordinates
    float* r;                // Radial Position
    float* z;                // Z Position
    float* dphi;                // Angle
    float aerr;                // Absolute Error
    float rerr;                // Relative Error
} FLUXLOOP;

typedef struct {
    char id[XMLMAXSTRING];        // ID
    INSTANCE instance;

    int nco;                // Number of Coordinates/Elements
    int modelnrnz[2];            // ?
    float* r;                // Radial Position
    float* z;                // Z Position
    float* dr;                // dRadial Position
    float* dz;                // dZ Position
    float* ang1;                // Angle #1
    float* ang2;                // Angle #2
    float* res;                // Resistance
    float aerr;                // Absolute Error
    float rerr;                // Relative Error
} PFPASSIVE;

typedef struct {
    char id[XMLMAXSTRING];        // ID
    INSTANCE instance;
    int nco;                // Number of Coordinates/Elements
    float* r;                // Radial Position
    float* z;                // Z Position
    float* dr;                // dRadial Position
    float* dz;                // dZ Position
    int turns;                // Turns per Element
    float fturns;            // Turns per Element if float! // Need to use Opaque types to avoid this mess!!
    int modelnrnz[2];            // ?
    float aerr;                // Absolute Error
    float rerr;                // Relative Error
} PFCOILS;

typedef struct {
    int nco;                // Number of Coordinates/Elements
    float* r;                // Radial Position
    float* z;                // Z Position
    float factor;            // Correction factor
} LIMITER;


typedef struct {
    char device[XMLMAXSTRING];        // Device Name
    int exp_number;            // Experiment (Pulse) Number
    int nfluxloops;            // Number of Flux Loops
    int nmagprobes;            // Number of Magnetic Probes
    int npfcircuits;            // Number of PF Circuits
    int npfpassive;            // Number of PF Passive Components
    int npfsupplies;            // Number of PF Supplies
    int nplasmacurrent;        // Plasma Current Data
    int ndiamagnetic;            // Diamagnetic Data
    int ntoroidalfield;        // Toroidal Field Data
    int npfcoils;            // Number of PF Coils
    int nlimiter;            // Limiter Coordinates Available
    PFCOILS* pfcoils;            // PF Coils
    PFPASSIVE* pfpassive;        // PF Passive Components
    PFSUPPLIES* pfsupplies;        // PF Supplies
    FLUXLOOP* fluxloop;            // Flux Loops
    MAGPROBE* magprobe;            // Magnetic Probes
    PFCIRCUIT* pfcircuit;        // PF Circuits
    PLASMACURRENT* plasmacurrent;    // Plasma Current
    DIAMAGNETIC* diamagnetic;        // Diamagnetic Flux
    TOROIDALFIELD* toroidalfield;    // Toroidal Field
    LIMITER* limiter;            // Limiter Coordinates
} EFIT;

// static EFIT efit;			// Collection of XML Parsed Data 



//--------------------------------------------------------------------------- 
// Prototypes 

//int parseIdamXML(char *xmlfile); 
/* 
void freeIdamEfit(int handle); 
static EFIT *getIdamEfit(int handle); 

int getIdamFluxLoopCount(int handle); 
int getIdamFluxLoopCoordCount(int handle, int n); 
int getIdamLimiterCount(int handle); 
int getIdamLimiterCoordCount(int handle, int n); 
int getIdamMagProbeCount(int handle); 
int getIdamPFSupplyCount(int handle); 
int getIdamPFCoilCount(int handle); 
int getIdamPFCoilCoordCount(int handle, int n); 
int getIdamPFCircuitCount(int handle); 
int getIdamPFCircuitConnectionCount(int handle, int n); 
int getIdamPFPassiveCount(int handle); 
int getIdamPFPassiveCoordCount(int handle, int n); 
int getIdamPlasmaCurrentCount(int handle); 
int getIdamDiaMagneticCount(int handle); 
int getIdamToroidalFieldCount(int handle); 
char *getIdamDeviceName(int handle); 
int getIdamExpNumber(int handle); 
void getIdamDataProperty(INSTANCE *str, int *seq, int *status, float *factor,  
                                char **archive, char **file, char **signal, char **owner, char **format); 
int getIdamMagProbe(int handle, int n, float *r, float *z, float *angle, float *aerr, float *rerr); 
int getIdamPFSupply(int handle, int n, float *aerr, float *rerr); 
int getIdamFluxLoop(int handle, int n, float **r, float **z, float **dphi); 
int getIdamPFPassive(int handle, int n, float **r, float **z, float **dr, float **dz, float **ang1, float **ang2,  
                            float **res, float *aerr, float *rerr); 
int getIdamPFCoil(int handle, int n, int *turns, float **r, float **z, float **dr, float **dz, float *aerr, float *rerr); 
int getIdamPFCircuit(int handle, int n, int *supply, int **coil); 
int getIdamPlasmaCurrent(int handle, float *aerr, float *rerr); 
int getIdamToroidalField(int handle, float *aerr, float *rerr); 
int getIdamLimiterCoords(int handle, int n, float **r, float **z); 
int getIdamDiaMagnetic(int handle, float *aerr, float *rerr); 
*/
//--------------------------------------------------------------------------- 
// Prototypes 

void initEfit(EFIT* str);
void freeEfit(EFIT* str);

void initInstance(INSTANCE* str);
void initFluxLoop(FLUXLOOP* str);
void initPfPassive(PFPASSIVE* str);
void initPfCoils(PFCOILS* str);
void initMagProbe(MAGPROBE* str);
void initPfSupplies(PFSUPPLIES* str);
void initPfCircuits(PFCIRCUIT* str);
void initPlasmaCurrent(PLASMACURRENT* str);
void initDiamagnetic(DIAMAGNETIC* str);
void initToroidalField(TOROIDALFIELD* str);
void initLimiter(LIMITER* str);

void printInstance(FILE* fh, INSTANCE str);
void printMagProbe(FILE* fh, MAGPROBE str);
void printPfSupplies(FILE* fh, PFSUPPLIES str);
void printPfCircuits(FILE* fh, PFCIRCUIT str);
void printFluxLoop(FILE* fh, FLUXLOOP str);
void printPfCoils(FILE* fh, PFCOILS str);
void printPfPassive(FILE* fh, PFPASSIVE str);
void printPlasmaCurrent(FILE* fh, PLASMACURRENT str);
void printDiamagnetic(FILE* fh, DIAMAGNETIC str);
void printToroidalField(FILE* fh, TOROIDALFIELD str);
void printLimiter(FILE* fh, LIMITER str);
char* convertNonPrintable(char* str);

int alloc_efit(EFIT* efit);
int alloc_pfcircuit(PFCIRCUIT* str);
int alloc_pfcoils(PFCOILS* str);
int alloc_pfpassive(PFPASSIVE* str);
int alloc_fluxloop(FLUXLOOP* str);
int alloc_limiter(LIMITER* str);

float* parseFloatArray(xmlDocPtr doc, xmlNodePtr cur, char* target, int* n);
void parseFloat(xmlDocPtr doc, xmlNodePtr cur, char* target, float* value);
void parseInt(xmlDocPtr doc, xmlNodePtr cur, char* target, int* value);
int* parseIntArray(xmlDocPtr doc, xmlNodePtr cur, char* target, int* n);
float* parseFloatAngleArray(xmlDocPtr doc, xmlNodePtr cur, char* target, int* n);
void parseFloatAngle(xmlDocPtr doc, xmlNodePtr cur, char* target, float* value);
void parseInstance(xmlNodePtr cur, INSTANCE* str);

MAGPROBE* parseMagProbe(xmlDocPtr doc, xmlNodePtr cur, MAGPROBE* str, int* np);
FLUXLOOP* parseFluxLoop(xmlDocPtr doc, xmlNodePtr cur, FLUXLOOP* str, int* np);
PFCOILS* parsePfCoils(xmlDocPtr doc, xmlNodePtr cur, PFCOILS* str, int* np);
PFPASSIVE* parsePfPassive(xmlDocPtr doc, xmlNodePtr cur, PFPASSIVE* str, int* np);
PFSUPPLIES* parsePfSupplies(xmlDocPtr doc, xmlNodePtr cur, PFSUPPLIES* str, int* np);
PFCIRCUIT* parsePfCircuits(xmlDocPtr doc, xmlNodePtr cur, PFCIRCUIT* str, int* np);
PLASMACURRENT* parsePlasmaCurrent(xmlDocPtr doc, xmlNodePtr cur, PLASMACURRENT* str);
DIAMAGNETIC* parseDiamagnetic(xmlDocPtr doc, xmlNodePtr cur, DIAMAGNETIC* str);
TOROIDALFIELD* parseToroidalField(xmlDocPtr doc, xmlNodePtr cur, TOROIDALFIELD* str);
LIMITER* parseLimiter(xmlDocPtr doc, xmlNodePtr cur, LIMITER* str);

int parseEfitXML(char* xmlfile, EFIT* efit);

void StringtoFortran(char* srce, int lsrce);

int getnfluxloops(EFIT* efit);
int getnfluxloopcoords(EFIT* efit, const int n);
int getnlimiter(EFIT* efit);
int getnlimitercoords(EFIT* efit);
int getnpfsupplies(EFIT* efit);
int getnpfcoils(EFIT* efit);
int getnpfcoilcoords(EFIT* efit, const int n);
int getnpfcircuits(EFIT* efit);
int getnpfcircuitconnections(EFIT* efit, const int n);
int getnpfpassive(EFIT* efit);
int getnpfpassivecoords(EFIT* efit, const int n);
int getnplasmacurrent(EFIT* efit);
int getndiamagnetic(EFIT* efit);
int getntoroidalfield(EFIT* efit);
int getnmagprobes(EFIT* efit);

char* getdevice(EFIT* efit);

void getinstance(INSTANCE* str, int* seq, int* status, float* factor,
                 char** archive, char** file, char** signal, char** owner, char** format);
int getmagprobe(EFIT* efit, const int n, float* r, float* z, float* angle, float* aerr, float* rerr);
int getpfsupplies(EFIT* efit, const int n, float* aerr, float* rerr);
int getfluxloop(EFIT* efit, const int n, float** r, float** z, float** dphi, float* aerr, float* rerr);
int getpfpassive(EFIT* efit, const int n, float** r, float** z, float** dr,
                 float** dz, float** ang1, float** ang2, float** res,
                 float* aerr, float* rerr);
int getpfcoil(EFIT* efit, const int n, int* turns, float** r, float** z,
              float** dr, float** dz, float* aerr, float* rerr);
int getpfcircuit(EFIT* efit, const int n, int* supply, int** coil);
int getplasmacurrent(EFIT* efit, float* aerr, float* rerr);
int getdiamagnetic(EFIT* efit, float* aerr, float* rerr);
int gettoroidalfield(EFIT* efit, float* aerr, float* rerr);
int getlimitercoords(EFIT* efit, float** r, float** z);
int getpfsupply(EFIT* efit, const int n, float* aerr, float* rerr);


char* getidmagprobe(EFIT* efit, const int index);
char* getidfluxloop(EFIT* efit, const int index);
char* getidplasmacurrent(EFIT* efit, const int index);
char* getidtoroidalfield(EFIT* efit, const int index);
char* getidpfcoils(EFIT* efit, const int index);
char* getidpfpassive(EFIT* efit, const int index);
char* getidpfsupplies(EFIT* efit, const int index);
char* getidpfcircuit(EFIT* efit, const int index);

#ifdef __cplusplus
}
#endif

#endif
