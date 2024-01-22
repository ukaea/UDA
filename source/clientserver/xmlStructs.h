#ifndef UDA_XMLSTRUCTS_H
#define UDA_XMLSTRUCTS_H

#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XMLMAXSTRING 56
#define XMLMAX 200 * 1024
#define UDA_XML_MAX_LOOP 1024 // Max Number of Array elements

//---------------------------------------------------------------------------
// Flux Loop Data Structures

typedef struct {
    char archive[XMLMAXSTRING]; // Data Archive Name
    char file[XMLMAXSTRING];    // Data File Name
    char signal[XMLMAXSTRING];  // Signal Name (Generic or Specific)
    char owner[XMLMAXSTRING];   // Owner Name
    char format[XMLMAXSTRING];  // Data Format
    int seq;                    // Data Sequence or Pass
    int status;                 // Signal Status
    float factor;               // Scaling Factor
} INSTANCE;

typedef struct {
    char id[XMLMAXSTRING]; // ID
    INSTANCE instance;
    float aerr; // Absolute Error
    float rerr; // Relative Error
} TOROIDALFIELD;

typedef struct {
    char id[XMLMAXSTRING]; // ID
    INSTANCE instance;
    float aerr; // Absolute Error
    float rerr; // Relative Error
} PLASMACURRENT;

typedef struct {
    char id[XMLMAXSTRING]; // ID
    INSTANCE instance;
    float aerr; // Absolute Error
    float rerr; // Relative Error
} DIAMAGNETIC;

typedef struct {
    char id[XMLMAXSTRING]; // ID
    INSTANCE instance;
    int nco;    // Number of Coils
    int* coil;  // List of Coil Connections
    int supply; // Supply Connections
} PFCIRCUIT;

typedef struct {
    char id[XMLMAXSTRING]; // ID
    INSTANCE instance;
    float r;     // Radial Position
    float z;     // Z Position
    float angle; // Angle
    float aerr;  // Absolute Error
    float rerr;  // Relative Error
} MAGPROBE;

typedef struct {
    char id[XMLMAXSTRING]; // ID
    INSTANCE instance;
    float aerr; // Absolute Error
    float rerr; // Relative Error
} PFSUPPLIES;

typedef struct {
    char id[XMLMAXSTRING]; // ID
    INSTANCE instance;
    int nco;     // Number of Coordinates
    float* r;    // Radial Position
    float* z;    // Z Position
    float* dphi; // Angle
    float aerr;  // Absolute Error
    float rerr;  // Relative Error
} FLUXLOOP;

typedef struct {
    char id[XMLMAXSTRING]; // ID
    INSTANCE instance;
    int nco;          // Number of Coordinates/Elements
    int modelnrnz[2]; // ?
    float* r;         // Radial Position
    float* z;         // Z Position
    float* dr;        // dRadial Position
    float* dz;        // dZ Position
    float* ang1;      // Angle #1
    float* ang2;      // Angle #2
    float* res;       // Resistance
    float aerr;       // Absolute Error
    float rerr;       // Relative Error
} PFPASSIVE;

typedef struct {
    char id[XMLMAXSTRING]; // ID
    INSTANCE instance;
    int nco;          // Number of Coordinates/Elements
    float* r;         // Radial Position
    float* z;         // Z Position
    float* dr;        // dRadial Position
    float* dz;        // dZ Position
    int turns;        // Turns per Element
    float fturns;     // Turns per Element if float! // Need to use Opaque types to avoid this mess!!
    int modelnrnz[2]; // ?
    float aerr;       // Absolute Error
    float rerr;       // Relative Error
} PFCOILS;

typedef struct {
    int nco;      // Number of Coordinates/Elements
    float* r;     // Radial Position
    float* z;     // Z Position
    float factor; // Correction factor
} LIMITER;

typedef struct {
    char device[XMLMAXSTRING];    // Device Name
    int exp_number;               // Experiment (Pulse) Number
    int nfluxloops;               // Number of Flux Loops
    int nmagprobes;               // Number of Magnetic Probes
    int npfcircuits;              // Number of PF Circuits
    int npfpassive;               // Number of PF Passive Components
    int npfsupplies;              // Number of PF Supplies
    int nplasmacurrent;           // Plasma Current Data
    int ndiamagnetic;             // Diamagnetic Data
    int ntoroidalfield;           // Toroidal Field Data
    int npfcoils;                 // Number of PF Coils
    int nlimiter;                 // Limiter Coordinates Available
    PFCOILS* pfcoils;             // PF Coils
    PFPASSIVE* pfpassive;         // PF Passive Components
    PFSUPPLIES* pfsupplies;       // PF Supplies
    FLUXLOOP* fluxloop;           // Flux Loops
    MAGPROBE* magprobe;           // Magnetic Probes
    PFCIRCUIT* pfcircuit;         // PF Circuits
    PLASMACURRENT* plasmacurrent; // Plasma Current
    DIAMAGNETIC* diamagnetic;     // Diamagnetic Flux
    TOROIDALFIELD* toroidalfield; // Toroidal Field
    LIMITER* limiter;             // Limiter Coordinates
} EFIT;

#ifdef __cplusplus
}
#endif

#endif // UDA_XMLSTRUCTS_H
