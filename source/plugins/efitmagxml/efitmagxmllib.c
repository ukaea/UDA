
//--------------------------------------------------------------- 
// Library of XML Accessor functions 
// 
// Change History: 
// 06Sept2016	dgm	Original Version
//-------------------------------------------------------------- 
#include "efitmagxml.h"

#include <clientserver/udaErrors.h>

INSTANCE* getinstmagprobe(EFIT* efit, const int index)
{
    return &efit->magprobe[index].instance;
}

INSTANCE* getinstfluxloop(EFIT* efit, const int index)
{
    return &efit->fluxloop[index].instance;
}

INSTANCE* getinstplasmacurrent(EFIT* efit, const int index)
{
    return &efit->plasmacurrent[index].instance;
}

INSTANCE* getinsttoroidalfield(EFIT* efit, const int index)
{
    return &efit->toroidalfield[index].instance;
}

INSTANCE* getinstpfcoils(EFIT* efit, const int index)
{
    return &efit->pfcoils[index].instance;
}

INSTANCE* getinstpfpassive(EFIT* efit, const int index)
{
    return &efit->pfpassive[index].instance;
}

INSTANCE* getinstpfsupplies(EFIT* efit, const int index)
{
    return &efit->pfsupplies[index].instance;
}

INSTANCE* getinstpfcircuit(EFIT* efit, const int index)
{
    return &efit->pfcircuit[index].instance;
}


//=========================================================================


char* getdevice(EFIT* efit)
{
    return efit->device;
}

int getexpnumber(EFIT* efit)
{
    return (efit->exp_number);
}

void getinstance(INSTANCE* str, int* seq, int* status, float* factor,
                 char** archive, char** file, char** signal, char** owner, char** format)
{

//fprintf(stdout,"Archive : %s\n", str->archive); 
//fprintf(stdout,"File    : %s\n", str->file); 
//fprintf(stdout,"Signal  : %s\n", str->signal); 
//fprintf(stdout,"Owner   : %s\n", str->owner); 
//fprintf(stdout,"Format  : %s\n", str->format); 

    *archive = str->archive;
    *file = str->file;
    *signal = str->signal;
    *owner = str->owner;
    *format = str->format;
    *seq = str->seq;
    *status = str->status;
    *factor = str->factor;

    return;
}

int getmagprobe(EFIT* efit, const int n, float* r, float* z, float* angle, float* aerr, float* rerr)
{
    if (n >= 0 && n <= efit->nmagprobes - 1) {
        *r = efit->magprobe[n].r;
        *z = efit->magprobe[n].z;
        *angle = efit->magprobe[n].angle;
        *aerr = efit->magprobe[n].aerr;
        *rerr = efit->magprobe[n].rerr;
        return 0;
    } else { return 1; }
}

/*
int getpolarimetry(EFIT *efit, const int n, float *r, float *z, float *angle, float *aerr, float *rerr){ 
   if(n >= 0 && n <= efit->npolarimetry-1){ 
      *r      = efit->polarimetry[n].r; 
      *z      = efit->polarimetry[n].z; 
      *angle  = efit->polarimetry[n].angle; 
      *aerr   = efit->polarimetry[n].aerr; 
      *rerr   = efit->polarimetry[n].rerr; 
      return 0; 
   } else return 1; 
}

int getinterferometry(EFIT *efit, const int n, float *r, float *z, float *angle, float *aerr, float *rerr){ 
   if(n >= 0 && n <= efit->ninterferometry-1){ 
      *r      = efit->interferometry[n].r; 
      *z      = efit->interferometry[n].z; 
      *angle  = efit->interferometry[n].angle; 
      *aerr   = efit->interferometry[n].aerr; 
      *rerr   = efit->interferometry[n].rerr; 
      return 0; 
   } else return 1; 
}      
*/
int getpfsupply(EFIT* efit, const int n, float* aerr, float* rerr)
{
    if (n >= 0 && n <= efit->npfsupplies - 1) {
        *aerr = efit->pfsupplies[n].aerr;
        *rerr = efit->pfsupplies[n].rerr;
        return 0;
    } else { return 1; }
}

int getfluxloop(EFIT* efit, const int n, float** r, float** z, float** dphi, float* aerr, float* rerr)
{
    if (n >= 0 && n <= efit->nfluxloops - 1) {
        *r = efit->fluxloop[n].r;
        *z = efit->fluxloop[n].z;
        *dphi = efit->fluxloop[n].dphi;
        *aerr = efit->fluxloop[n].aerr;
        *rerr = efit->fluxloop[n].rerr;
        return 0;
    } else { return 1; }
}


int getpfpassive(EFIT* efit, const int n, float** r, float** z, float** dr,
                 float** dz, float** ang1, float** ang2, float** res,
                 float* aerr, float* rerr)
{
    if (n >= 0 && n <= efit->npfpassive - 1) {
        *aerr = efit->pfpassive[n].aerr;
        *rerr = efit->pfpassive[n].rerr;
        *r = efit->pfpassive[n].r;
        *z = efit->pfpassive[n].z;
        *dr = efit->pfpassive[n].dr;
        *dz = efit->pfpassive[n].dz;
        *ang1 = efit->pfpassive[n].ang1;
        *ang2 = efit->pfpassive[n].ang2;
        *res = efit->pfpassive[n].res;
        return 0;
    } else { return 1; }
}

int getpfcoil(EFIT* efit, const int n, int* turns, float** r, float** z,
              float** dr, float** dz, float* aerr, float* rerr)
{
    if (n >= 0 && n <= efit->npfcoils - 1) {
        *turns = efit->pfcoils[n].turns;
        *aerr = efit->pfcoils[n].aerr;
        *rerr = efit->pfcoils[n].rerr;
        *r = efit->pfcoils[n].r;
        *z = efit->pfcoils[n].z;
        *dr = efit->pfcoils[n].dr;
        *dz = efit->pfcoils[n].dz;
        return 0;
    } else { return 1; }
}

int getpfcircuit(EFIT* efit, const int n, int* supply, int** coil)
{
    if (n >= 0 && n <= efit->npfcircuits - 1) {
        *supply = efit->pfcircuit[n].supply;
        *coil = efit->pfcircuit[n].coil;
        return 0;
    } else {
        return 1;
    }
}

int getplasmacurrent(EFIT* efit, float* aerr, float* rerr)
{
    *aerr = efit->plasmacurrent->aerr;
    *rerr = efit->plasmacurrent->rerr;
    return 0;
}

int gettoroidalfield(EFIT* efit, float* aerr, float* rerr)
{
    *aerr = efit->toroidalfield->aerr;
    *rerr = efit->toroidalfield->rerr;
    return 0;
}

int getlimitercoords(EFIT* efit, float** r, float** z)
{
    if (efit->nlimiter) {
        *r = efit->limiter->r;
        *z = efit->limiter->z;
        return 0;
    } else { return 1; }
}

int getdiamagnetic(EFIT* efit, float* aerr, float* rerr)
{
    *aerr = efit->diamagnetic->aerr;
    *rerr = efit->diamagnetic->rerr;
    return 0;
}  
