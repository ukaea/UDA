#ifndef IdamClientServerXmlInclude
#define IdamClientServerXmlInclude

#ifdef HIERARCHICAL_DATA

#include "idamclientserver.h"

#ifdef XML_ENABLE
#  include <libxml/xmlmemory.h>
#  include <libxml/parser.h>
#endif

#ifdef SQL_ENABLE
#  include <libpq-fe.h>
#endif

#define READFILE    1       // The XML Source is an External File 

#define XMLMAXSTRING    56
#define XMLMAX          200*1024
#define XMLMAXLOOP      1024        // Max Number of Array elements

#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------------
// Flux Loop Data Structures

typedef struct {
    char archive[XMLMAXSTRING];     // Data Archive Name
    char file[XMLMAXSTRING];        // Data File Name
    char signal[XMLMAXSTRING];      // Signal Name (Generic or Specific)
    char owner[XMLMAXSTRING];       // Owner Name
    char format[XMLMAXSTRING];      // Data Format
    int  seq;                       // Data Sequence or Pass
    int  status;                    // Signal Status
    float factor;                   // Scaling Factor
} INSTANCE;

typedef struct {
    char id[XMLMAXSTRING];          // ID
    INSTANCE instance;

    float aerr;                     // Absolute Error
    float rerr;                     // Relative Error
} TOROIDALFIELD;

typedef struct {
    char id[XMLMAXSTRING];          // ID
    INSTANCE instance;

    float aerr;                     // Absolute Error
    float rerr;                     // Relative Error
} PLASMACURRENT;

typedef struct {
    char id[XMLMAXSTRING];          // ID
    INSTANCE instance;

    float aerr;                     // Absolute Error
    float rerr;                     // Relative Error
} DIAMAGNETIC;

typedef struct {
    char id[XMLMAXSTRING];          // ID
    INSTANCE instance;

    int  nco;                       // Number of Coils
    int * coil;                     // List of Coil Connections
    int  supply;                    // Supply Connections
} PFCIRCUIT;

typedef struct {
    char id[XMLMAXSTRING];          // ID
    INSTANCE instance;

    float r;                        // Radial Position
    float z;                        // Z Position
    float angle;                    // Angle
    float aerr;                     // Absolute Error
    float rerr;                     // Relative Error
} MAGPROBE;

typedef struct {
    char id[XMLMAXSTRING];          // ID
    INSTANCE instance;

    float aerr;                     // Absolute Error
    float rerr;                     // Relative Error
} PFSUPPLIES;

typedef struct {
    char id[XMLMAXSTRING];          // ID
    INSTANCE instance;

    int  nco;                       // Number of Coordinates
    float * r;                      // Radial Position
    float * z;                      // Z Position
    float * dphi;                   // Angle
    float aerr;                     // Absolute Error
    float rerr;                     // Relative Error
} FLUXLOOP;

typedef struct {
    char id[XMLMAXSTRING];          // ID
    INSTANCE instance;

    int  nco;                       // Number of Coordinates/Elements
    int  modelnrnz[2];              // ?
    float * r;                      // Radial Position
    float * z;                      // Z Position
    float * dr;                     // dRadial Position
    float * dz;                     // dZ Position
    float * ang1;                   // Angle #1
    float * ang2;                   // Angle #2
    float * res;                    // Resistance
    float aerr;                     // Absolute Error
    float rerr;                     // Relative Error
} PFPASSIVE;

typedef struct {
    char id[XMLMAXSTRING];          // ID
    INSTANCE instance;
    int  nco;                       // Number of Coordinates/Elements
    float * r;                      // Radial Position
    float * z;                      // Z Position
    float * dr;                     // dRadial Position
    float * dz;                     // dZ Position
    int   turns;                    // Turns per Element
    float fturns;                   // Turns per Element if float! // Need to use Opaque types to avoid this mess!!
    int   modelnrnz[2];             // ?
    float aerr;                     // Absolute Error
    float rerr;                     // Relative Error
} PFCOILS;

typedef struct {
    int  nco;                       // Number of Coordinates/Elements
    float * r;                      // Radial Position
    float * z;                      // Z Position
    float factor;                   // Correction factor
} LIMITER;

typedef struct {
    char   device[XMLMAXSTRING];    // Device Name
    int    exp_number;              // Experiment (Pulse) Number
    int    nfluxloops;              // Number of Flux Loops
    int    nmagprobes;              // Number of Magnetic Probes
    int    npfcircuits;             // Number of PF Circuits
    int    npfpassive;              // Number of PF Passive Components
    int    npfsupplies;             // Number of PF Supplies
    int    nplasmacurrent;          // Plasma Current Data
    int    ndiamagnetic;            // Diamagnetic Data
    int    ntoroidalfield;          // Toroidal Field Data
    int    npfcoils;                // Number of PF Coils
    int    nlimiter;                // Limiter Coordinates Available
    PFCOILS * pfcoils;              // PF Coils
    PFPASSIVE * pfpassive;          // PF Passive Components
    PFSUPPLIES * pfsupplies;        // PF Supplies
    FLUXLOOP * fluxloop;            // Flux Loops
    MAGPROBE * magprobe;            // Magnetic Probes
    PFCIRCUIT * pfcircuit;          // PF Circuits
    PLASMACURRENT * plasmacurrent;  // Plasma Current
    DIAMAGNETIC * diamagnetic;      // Diamagnetic Flux
    TOROIDALFIELD * toroidalfield;  // Toroidal Field
    LIMITER * limiter;              // Limiter Coordinates
} EFIT;

extern EFIT efit;                   // Collection of XML Parsed Data

//---------------------------------------------------------------------------
// Data Aquisition Opaque Structures

#define DAQMAXSTRING 256

typedef struct {
    int    shot;                    // Shot Number
    char   datetime[DAQMAXSTRING];  // Date and Time of File Creation
    int    nxml;                    // Size of XML string
    char * xml;                     // Other Meta Data
} DAQ_HEADER;

typedef struct {
    int    ndaq;                    // Number of DAQ Structures
    int    nxml;                    // Size of XML string
    char * xml;                     // Meta Data
} DATAQ;

typedef struct {
    char   device[DAQMAXSTRING];    // Device type
    char   serial[DAQMAXSTRING];    // Device Serial Number
    int   devres;                   // Device Resolution in Bits
    float  devrange;                // Device input range: max - min
    float  devoffset;               // Device Offset: Zero value position in range
    int    nxml;                    // Size of XML string
    char * xml;                     // Other Meta Data
} DAQ_GROUP;

typedef struct {
    double offset;                  // Data Offset
    double scale;                   // Data Scale
    char   unit[DAQMAXSTRING];      // Date and Time of File Creation
    int    nxml;                    // Size of XML string
    char * xml;                     // Other Meta Data
} DAQ_DATA;

typedef struct {
    double * offset;                // First point segment positions
    double * scale;                 // Data interval within a segment
    int  *  samples;                // Array of segment lengths
    int     nxml;                   // Size of XML string
    char  * xml;                    // Other Meta Data
} DAQ_DIMENSION;

#endif

#ifdef __cplusplus
}
#endif

#endif
