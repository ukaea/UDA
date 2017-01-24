#ifndef IDAM_PLUGINS_EQUIMAP_EQUIMAP_H
#define IDAM_PLUGINS_EQUIMAP_EQUIMAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <idamplugin.h>

#include "scruncher.h"

typedef struct EfitData
{
    float psi_bnd;
    float psi_mag;

    float rmag;
    float zmag;

    float rgeom;
    float zgeom;
    float aminor;
    float triangL;
    float triangU;
    float elong;

    float ip;     // plasma Current
    float bphi;     // toroidal magnetic field
    float bvac;     // vacuum toroidal magnetic field at R_Reference
    float rvac;     // vacuum toroidal magnetic field at rvac

    float rxpoint1;
    float zxpoint1;
    float rxpoint2;
    float zxpoint2;

    float Rcolumn;
    float Rmin;
    float Rmax;

    int nlcfs;    // Number of valid surface points
    float * rlcfs;    // LCFS Locus Radial positions
    float * zlcfs;    // LCFS Locus Z positions

    int nlim;     // Number of Limiter points
    float * rlim;     // Limiter Radial positions
    float * zlim;     // Limiter Z positions

    int psiCount[2];    // Number of Grid Points [R,Z]
    float ** psig;    // Psi data re-written to a rank 2 array
    float * rgrid;    // Radial Grid
    float * zgrid;    // Height Grid

    int psiCountSR[2];   // Number of Grid Points [R,Z] in Smoothed/Reduced Profile
    float ** psigSR;    // Smoothed/Reduced Psi data
    float * rgridSR;    // Smoothed/Reduced Radial Grid
    float * zgridSR;    // Smoothed/Reduced Height Grid

    int psiCountRZBox[2];   // Number of Grid Points [R,Z] in Reduced R-Z Box
    float ** psigRZBox;    // Psi data in reduced R-Z Box
    float * rgridRZBox;    // Radial Grid in reduced R-Z Box
    float * zgridRZBox;    // Height Grid in reduced R-Z Box

    float ** dpsidr;    // d/dr poloidal flux on R,Z grid
    float ** dpsidz;    // d/dz poloidal flux
    float ** Br;     // radial component of magnetic field
    float ** Bz;     // z component of magnetic field
    float ** Bphi;    // toroidal component of magnetic field
    float ** Jphi;    // toroidal component of current density

    int rz0Count;    // Number of Mid-Plane grid points
    float * psiz0;    // Psi on Mid-Plane (Z=0)
    float * rz0;     // Major Radii on Mid-Plane

    int qCount;    // Number of Points in Q, P, F profiles
    float * q;     // Q Profile
    float * p;     // P Profile
    float * f;     // F Profile
    float * rho;     // Efit normalised poloidal flux coordinate (flux surface label)
    float * psi;     // Poloidal flux (Rho, trho, rho_tor)
    float * phi;     // Toroidal flux (Rho, trho, rho_tor)
    float * trho;     // equivalent Efit normalised toroidal flux coordinate (flux surface label)
    float rho_torb;    // ITM Flux Label normalisation constant (Boundary Flux radius)
    float * rho_tor;    // equivalent Normalised ITM Flux radius

    float * pprime;     // P_Prime Profile
    float * ffprime;     // FF_Prime Profile
    float * elongp;     // Elongation Profile
    float * trianglp;    // Lower Triagularity profile
    float * triangup;    // Upper Triagularity profile
    float * volp;     // Volume profile
    float * areap;     // Area profile

    int nne;
    float * ne;     // Measured electron density profile
    float * rne;
    float * te;     // Measured electron temperature profile

    float * yagpsi;    // Poloidal Flux coordinates of YAG Measurement major radii
    float * yagphi;    // Toroidal Flux coordinates of YAG Measurement major radii

    float * yagprho;    // Normalised Poloidal Flux coordinates
    float * yagtrho;    // SQRT Normalised Toroidal Flux coordinates
    float * yagrhotor;    // Normalised ITM Flux coordinates

    float * mappsi;    // Psi Profile mapped to Rho
    float * mappsiB;    // Psi Profile mapped to RhoB
    float * mapq;     // Q Profile mapped to Rho
    float * mapqB;    // Q Profile mapped to RhoB
    float * mapp;
    float * mappB;
    float * mapf;
    float * mapfB;

    float * mapgm0;    // Flux Surface average of |grad(rho_tor)|
    float * mapgm1;    // Flux Surface average of |grad(rho_tor)|^2
    float * mapgm2;    // Flux Surface average of R
    float * mapgm3;    // Flux Surface average of |grad(rho_tor/R)|^2

    float * mappprime;
    float * mappprimeB;
    float * mapffprime;
    float * mapffprimeB;
    float * mapelongp;
    float * mapelongpB;
    float * maptrianglp;
    float * maptrianglpB;
    float * maptriangup;
    float * maptriangupB;
    float * mapvolp;
    float * mapvolpB;
    float * mapareap;
    float * mapareapB;

    float * mapyagne;
    float * mapyagte;
    float * mapyagpsi;
    float * mapyagphi;

    float * mapyagr1;
    float * mapyagne1;
    float * mapyagte1;
    float * mapyagpsi1;
    float * mapyagphi1;

    float * mapyagr2;
    float * mapyagne2;
    float * mapyagte2;
    float * mapyagpsi2;
    float * mapyagphi2;

    float * mapyagneB;
    float * mapyagteB;
    float * mapyagpsiB;
    float * mapyagphiB;

    float * mapyagr1B;
    float * mapyagne1B;
    float * mapyagte1B;
    float * mapyagpsi1B;
    float * mapyagphi1B;

    float * mapyagr2B;
    float * mapyagne2B;
    float * mapyagte2B;
    float * mapyagpsi2B;
    float * mapyagphi2B;

} EFITDATA;

typedef struct FluxAverages
{
    CONTOURS_OUT * contours;
    SCRUNCH_OUT * scrunch;
    METRIC_OUT metrics;
} FLUXAVERAGES;

typedef struct EquimapData
{
    int exp_number;
    int timeCount;
    float * times;
    int readITMData;
    int rhoType;     // Flux surface label definition
    int rhoCount;    // Number of (volume) flux surfaces excluding magnetic axis and LCFS
    int rhoBCount;   // Number of (bounding) flux surfaces including magnetic axis and LCFS
    float * rho;         // Centres between Flux Surfaces
    float * rhoB;        // Boundary of Flux Surfaces
    EFITDATA * efitdata;
    FLUXAVERAGES * fluxAverages;
} EQUIMAPDATA;

#define MAXHANDLES      100
#define MAXSIGNALNAME   256

#define ATMAYCSHOT      22831  // Shot number when ATM changes to AYC with data organisation changes

#define COORDINATECOUNT 51 // Default number of (bounding) flux surfaces including magnetic axis and LCFS

#define UNKNOWNCOORDINATETYPE       0
#define SQRTNORMALISEDTOROIDALFLUX  1
#define NORMALISEDPOLOIDALFLUX      2
#define NORMALISEDITMFLUXRADIUS     3

int equiMap_main(IDAM_PLUGIN_INTERFACE * idam_plugin_interface);
void initEquiMapData();
void freeEquiMapData();
void initEfitData(EFITDATA * efitdata);

#ifdef __cplusplus
}
#endif

#endif // IDAM_PLUGINS_EQUIMAP_EQUIMAP_H
