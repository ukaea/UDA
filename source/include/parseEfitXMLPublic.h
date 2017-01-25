#ifndef parseEfitXMLPublicInclude
#define parseEfitXMLPublicInclude

#ifdef __cplusplus
extern "C" {
#endif

//#define SQL_ENABLE

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#ifdef SQL_ENABLE
#include <libpq-fe.h>
#endif

#define READFILE    1

#define XMLMAXCOMMENT   512
#define XMLMAXSTRING    56
#define XMLMAX      200*1024
#define XMLMAXLOOP  1024        // Max Number of Array elements 

//---------------------------------------------------------------------------
// Data Structures


typedef struct {
    char archive[XMLMAXSTRING];     // Data Archive Name
    char file[XMLMAXSTRING];        // Data File Name
    char signal[XMLMAXSTRING];      // Signal Name (Generic or Specific)
    char owner[XMLMAXSTRING];       // Owner Name
    char format[XMLMAXSTRING];      // Data Format
    int  seq;               // Data Sequence or Pass
    int  status;                // Signal Status
    float factor;           // Scaling Factor
} INSTANCE;

typedef struct {
    int  pini_number;
    char pini_comment[XMLMAXCOMMENT];           // Comment
    int  signal_number;                 // Number of Signals
    char signal_comment[XMLMAXCOMMENT];         // Comment
    char ** signal_list;                // List of Signals
} MSE_INFO;

typedef struct {
    char id[XMLMAXSTRING];      // ID
    INSTANCE instance;
    int nco;
    float * weight;
    float * r;              // Radial Position
    float * z;              // Z Position
    float * hxvr;
    float * hxvf;
    float * hxvz;
    float * vxvr;
    float * vxvf;
    float * vxvz;
    float aerr;             // Absolute Error
    float rerr;             // Relative Error
    char    w_comment[XMLMAXCOMMENT];
    char    r_comment[XMLMAXCOMMENT];
    char    z_comment[XMLMAXCOMMENT];
    char hxvr_comment[XMLMAXCOMMENT];
    char hxvf_comment[XMLMAXCOMMENT];
    char hxvz_comment[XMLMAXCOMMENT];
    char vxvr_comment[XMLMAXCOMMENT];
    char vxvf_comment[XMLMAXCOMMENT];
    char vxvz_comment[XMLMAXCOMMENT];
} MSE;

typedef struct {
    char id[XMLMAXSTRING];      // ID
    INSTANCE instance;
    float r;                // Radial Position
    float z;                // Z Position
    float angle;                // Angle
    float aerr;             // Absolute Error
    float rerr;             // Relative Error
} POLARIMETRY;

typedef struct {
    char id[XMLMAXSTRING];      // ID
    INSTANCE instance;
    float r;                // Radial Position
    float z;                // Z Position
    float angle;                // Angle
    float aerr;             // Absolute Error
    float rerr;             // Relative Error
} INTERFEROMETRY;

typedef struct {
    char id[XMLMAXSTRING];      // ID
    INSTANCE instance;

    float aerr;             // Absolute Error
    float rerr;             // Relative Error
} TOROIDALFIELD;


typedef struct {
    char id[XMLMAXSTRING];      // ID
    INSTANCE instance;

    float aerr;             // Absolute Error
    float rerr;             // Relative Error
} PLASMACURRENT;


typedef struct {
    char id[XMLMAXSTRING];      // ID
    INSTANCE instance;

    float aerr;             // Absolute Error
    float rerr;             // Relative Error
} DIAMAGNETIC;

typedef struct {
    char id[XMLMAXSTRING];      // ID
    INSTANCE instance;

    int  nco;               // Number of Coils
    int * coil;             // List of Coil Connections
    int  supply;                // Supply Connections
} PFCIRCUIT;

typedef struct {
    char id[XMLMAXSTRING];      // ID
    INSTANCE instance;

    float r;                // Radial Position
    float z;                // Z Position
    float angle;                // Angle
    float aerr;             // Absolute Error
    float rerr;             // Relative Error
} MAGPROBE;

typedef struct {
    char id[XMLMAXSTRING];      // ID
    INSTANCE instance;

    float aerr;             // Absolute Error
    float rerr;             // Relative Error
} PFSUPPLIES;

typedef struct {
    char id[XMLMAXSTRING];      // ID
    INSTANCE instance;

    int  nco;               // Number of Coordinates
    float * r;              // Radial Position
    float * z;              // Z Position
    float * dphi;               // Angle
    float aerr;             // Absolute Error
    float rerr;             // Relative Error
} FLUXLOOP;

typedef struct {
    char id[XMLMAXSTRING];      // ID
    INSTANCE instance;

    int  nco;               // Number of Coordinates/Elements
    int  modelnrnz[2];          // ?
    float * r;              // Radial Position
    float * z;              // Z Position
    float * dr;             // dRadial Position
    float * dz;             // dZ Position
    float * ang1;               // Angle #1
    float * ang2;               // Angle #2
    float * res;            // Resistance
    float aerr;             // Absolute Error
    float rerr;             // Relative Error
} PFPASSIVE;

typedef struct {
    char id[XMLMAXSTRING];      // ID
    INSTANCE instance;
    int  nco;               // Number of Coordinates/Elements
    float * r;              // Radial Position
    float * z;              // Z Position
    float * dr;             // dRadial Position
    float * dz;             // dZ Position
    int   turns;                // Turns per Element
    float fturns;           // Turns per Element if float! // Need to use Opaque types to avoid this mess!!
    int   modelnrnz[2];         // ?
    float aerr;             // Absolute Error
    float rerr;             // Relative Error
} PFCOILS;

typedef struct {
    int  nco;               // Number of Coordinates/Elements
    float * r;              // Radial Position
    float * z;              // Z Position
    float factor;           // Correction factor
} LIMITER;

typedef struct {
    char   device[XMLMAXSTRING];        // Device Name
    int    exp_number;          // Experiment (Pulse) Number
    int    nfluxloops;          // Number of Flux Loops
    int    nmagprobes;          // Number of Magnetic Probes
    int    npfcircuits;         // Number of PF Circuits
    int    npfpassive;          // Number of PF Passive Components
    int    npfsupplies;         // Number of PF Supplies
    int    nplasmacurrent;      // Plasma Current Data
    int    ndiamagnetic;            // Diamagnetic Data
    int    ntoroidalfield;      // Toroidal Field Data
    int    npfcoils;            // Number of PF Coils
    int    nlimiter;            // Limiter Coordinates Available
    int    npolarimetry;            // Number of Polarimetry Measurements
    int    ninterferometry;     // Number of Interfereometry measurements
#ifdef JETMSEXML
    int    nmse;                // Number of MSE measurements
#endif
    PFCOILS * pfcoils;          // PF Coils
    PFPASSIVE * pfpassive;      // PF Passive Components
    PFSUPPLIES * pfsupplies;    // PF Supplies
    FLUXLOOP * fluxloop;            // Flux Loops
    MAGPROBE * magprobe;            // Magnetic Probes
    PFCIRCUIT * pfcircuit;      // PF Circuits
    PLASMACURRENT * plasmacurrent;  // Plasma Current
    DIAMAGNETIC * diamagnetic;      // Diamagnetic Flux
    TOROIDALFIELD * toroidalfield;  // Toroidal Field
    LIMITER * limiter;          // Limiter Coordinates
    POLARIMETRY * polarimetry;      // Polarimetry
    INTERFEROMETRY * interferometry; // Interferometry
#ifdef JETMSEXML
    MSE_INFO mse_info;          // MSE
    MSE * mse;              // MSE
#endif
} EFIT;

//---------------------------------------------------------------------------
// Prototypes for Public Functions

extern void parseItmXML(char * xmlfile, EFIT * efit0);
extern int parseEfitXML(char * xmlfile);

extern void copyEfitXML(EFIT * efit0);
extern void setEfitXMLVerbose();
extern void setEfitXMLDebug();


extern int apiitm_(char * xmlfile, unsigned lxmlfile);

extern void initEfit(EFIT * str);

extern int freeitm();
extern int freeitm_();

extern int getnfluxloops_();
extern int getnfluxloopcoords_(int * n);
extern int getnlimiter_();
extern int getnlimitercoords_();
extern int getnmagprobes_();
extern int getnpfsupplies_();
extern int getnpfcoils_();
extern int getnpfcoilcoords_(int * n);
extern int getnpfcircuits_();
extern int getnpfcircuitconnections_(int * n);
extern int getnpfpassive_();
extern int getnpfpassivecoords_(int * n);
extern int getnplasmacurrent_();
extern int getndiamagnetic_();
extern int getntoroidalfield_();
extern int getnpolarimetry_();
extern int getninterferometry_();
extern int getdevice_(char * str, unsigned l1 );
extern int getexpnumber_();
#ifdef JETMSEXML
extern int getnmse_pinis_();
extern int getnmse_signals_();
extern int getmse_signal_(int * n, char * signal_item, unsigned int lbuf);
extern int getnmsechannels_(int * n);
extern int getnmse_();
#endif
/*
#ifdef OLDEFITVERSION
   //extern void getinstance(INSTANCE *str, int *seq, int *status, float *factor,
   //                        char *archive, char *file, char *signal, char *owner, char *format);

   extern void getinstance(INSTANCE *str, int *seq, int *status, float *factor,
                           char *archive, char *file, char *signal, char *owner, char *format,
                           int l3,        int l4,     int l5,       int l6,      int l7      );

#else
*/
extern void getinstance(INSTANCE * str, int * seq, int * status, float * factor,
                        char ** archive, char ** file, char ** signal, char ** owner, char ** format);
//#endif


extern int getinstance_(int * n, char * str, int * seq, int * status, float * factor,
                        char * id, char * archive, char * file, char * signal, char * owner, char * format,
                        unsigned l1, unsigned l2, unsigned l3, unsigned l4, unsigned l5, unsigned l6, unsigned l7 );

extern int getmagprobe_(int * n, float * r, float * z, float * angle, float * aerr, float * rerr);
extern int getpfsupplies_(int * n, float * aerr, float * rerr);


#ifdef OLDEFITVERSION
extern int getfluxloop_(int * n, float * r, float * z, float * dphi, float * aerr, float * rerr);
#else
extern int getfluxloop_(int * n, float * r, float * z, float * dphi);
#endif

extern int getpfpassive_(int * n, float * r, float * z, float * dr,
                         float * dz, float * ang1, float * ang2, float * res,
                         float * aerr, float * rerr, int * modelnrnz);
extern int getpfcoil_(int * n, int * turns, float * r, float * z,
                      float * dr, float * dz, float * aerr, float * rerr, int * modelnrnz);
extern int getpfcircuit_(int * n, int * supply, int * coil);
extern int getplasmacurrent_(float * aerr, float * rerr);
extern int getdiamagnetic_(float * aerr, float * rerr);
extern int gettoroidalfield_(float * aerr, float * rerr);
extern int getlimitercoords_(float * r, float * z);
#ifdef JETMSEXML
extern int getmse_(int * n, float * r, float * z, float * hxvr, float * hxvf, float * hxvz,
                   float * vxvr, float * vxvf, float * vxvz, float * weight, float * aerr, float * rerr);
#endif

extern int getpolarimetry_(int * n, float * r, float * z, float * angle, float * aerr, float * rerr);
extern int getinterferometry_(int * n, float * r, float * z, float * angle, float * aerr, float * rerr);


extern int getnfluxloops();
extern int getnfluxloopcoords(const int n);
extern int getnlimiter();
extern int getnlimitercoords();
extern int getnmagprobes();
extern int getnpfsupplies();
extern int getnpfcoils();
extern int getnpfcoilcoords(const int n);
extern int getnpfcircuits();
extern int getnpfcircuitconnections(const int n);
extern int getnpfpassive();
extern int getnpfpassivecoords(const int n);
extern int getnplasmacurrent();
extern int getndiamagnetic();
extern int getntoroidalfield();
extern int getnpolarimetry();
extern int getninterferometry();
extern char * getdevice();
extern int getexpnumber();
#ifdef JETMSEXML
extern int getnmse_pinis();
extern int getnmse_signals();
extern char ** getmse_signals();
extern int getnmsechannels(const int n);
extern int getnmse();
#endif
extern int getmagprobe(const int n, float * r, float * z, float * angle, float * aerr, float * rerr);
extern int getpfsupply(const int n, float * aerr, float * rerr);
extern int getpfsupplies(const int n, float * aerr, float * rerr);
extern int getfluxloop(const int n, float ** r, float ** z, float ** dphi, float * aerr, float * rerr);
extern int getpfpassive(const int n, float ** r, float ** z, float ** dr,
                        float ** dz, float ** ang1, float ** ang2, float ** res,
                        float * aerr, float * rerr);
extern int getpfcoil(const int n, int * turns, float ** r, float ** z,
                     float ** dr, float ** dz, float * aerr, float * rerr);
extern int getpfcircuit(const int n, int * supply, int ** coil);
extern int getplasmacurrent(float * aerr, float * rerr);
extern int getdiamagnetic(float * aerr, float * rerr);
extern int gettoroidalfield(float * aerr, float * rerr);
extern int getlimitercoords(float ** r, float ** z);
#ifdef JETMSEXML
extern int getmse(const int n, float ** r, float ** z, float ** hxvr, float ** hxvf, float ** hxvz,
                  float ** vxvr, float ** vxvf, float ** vxvz, float ** weight, float * aerr, float * rerr);
#endif
extern int getpolarimetry(const int n, float * r, float * z, float * angle, float * aerr, float * rerr);
extern int getinterferometry(const int n, float * r, float * z, float * angle, float * aerr, float * rerr);

extern int getidmagprobe(const int index);
extern int getidfluxloop(const int index);
extern int getidplasmacurrent(const int index);
extern int getidtoroidalfield(const int index);
extern int getidpfcoils(const int index);
extern int getidpfpassive(const int index);
extern int getidpfsupplies(const int index);
extern int getidpfcircuit(const int index);

extern INSTANCE * getinstmagprobe(const int index);
extern INSTANCE * getinstfluxloop(const int index);
extern INSTANCE * getinstplasmacurrent(const int index);
extern INSTANCE * getinsttoroidalfield(const int index);
extern INSTANCE * getinstpfcoils(const int index);
extern INSTANCE * getinstpfpassive(const int index);
extern INSTANCE * getinstpfsupplies(const int index);
extern INSTANCE * getinstpfcircuit(const int index);


#endif

#ifdef __cplusplus
}
#endif
