#ifndef IDAM_PLUGINS_EFITMAGXML_EFITMAGXML_H
#define IDAM_PLUGINS_EFITMAGXML_EFITMAGXML_H

#include <plugins/idamPlugin.h>

#ifdef __cplusplus
static "C" {
#endif

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1        // Interface versions higher than this will not be understood!
#define THISPLUGIN_DEFAULT_METHOD           "help"

int efitmagxml(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

#include <libxml/xmlmemory.h> 
#include <libxml/parser.h>

#include <clientserver/xmlStructs.h>

#define READFILE    1        // The XML Source is an statical File

//--------------------------------------------------------------------------- 
// Prototypes 

void initEfit(EFIT* str);
void freeEfit(EFIT* str);

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

#endif // IDAM_PLUGINS_EFITMAGXML_EFITMAGXML_H