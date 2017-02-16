/*---------------------------------------------------------------
* Allocate Memory for XML based Hierarchical Data Structures
*
* returns 0 if No Heap Allocation Error occurs
*
*--------------------------------------------------------------*/
#include "efitmagxml.h"

#include <clientserver/udaErrors.h>

int alloc_efit(EFIT* efit)
{
    void* ap = NULL;

    if (efit->npfcoils > 0) {
        if ((ap = (void*) malloc(efit->npfcoils * sizeof(PFCOILS))) == NULL) return (ERROR_ALLOCATING_HEAP);
        efit->pfcoils = (PFCOILS*) ap;
        ap = NULL;
    } else {
        efit->pfcoils = NULL;
    }

    if (efit->npfpassive > 0) {
        if ((ap = (void*) malloc(efit->npfpassive * sizeof(PFPASSIVE))) == NULL) return (ERROR_ALLOCATING_HEAP);
        efit->pfpassive = (PFPASSIVE*) ap;
        ap = NULL;
    } else {
        efit->pfpassive = NULL;
    }

    if (efit->npfsupplies > 0) {
        if ((ap = (void*) malloc(efit->npfsupplies * sizeof(PFSUPPLIES))) == NULL) return (ERROR_ALLOCATING_HEAP);
        efit->pfsupplies = (PFSUPPLIES*) ap;
        ap = NULL;
    } else {
        efit->pfsupplies = NULL;
    }

    if (efit->nfluxloops > 0) {
        if ((ap = (void*) malloc(efit->nfluxloops * sizeof(FLUXLOOP))) == NULL) return (ERROR_ALLOCATING_HEAP);
        efit->fluxloop = (FLUXLOOP*) ap;
        ap = NULL;
    } else {
        efit->fluxloop = NULL;
    }

    if (efit->nmagprobes > 0) {
        if ((ap = (void*) malloc(efit->nmagprobes * sizeof(MAGPROBE))) == NULL) return (ERROR_ALLOCATING_HEAP);
        efit->magprobe = (MAGPROBE*) ap;
        ap = NULL;
    } else {
        efit->magprobe = NULL;
    }

    if (efit->npfcircuits > 0) {
        if ((ap = (void*) malloc(efit->npfcircuits * sizeof(PFCIRCUIT))) == NULL) return (ERROR_ALLOCATING_HEAP);
        efit->pfcircuit = (PFCIRCUIT*) ap;
        ap = NULL;
    } else {
        efit->pfcircuit = NULL;
    }

    if (efit->nplasmacurrent > 0) {
        if ((ap = (void*) malloc(efit->nplasmacurrent * sizeof(PLASMACURRENT))) == NULL) return (ERROR_ALLOCATING_HEAP);
        efit->plasmacurrent = (PLASMACURRENT*) ap;
        ap = NULL;
    } else {
        efit->plasmacurrent = NULL;
    }

    if (efit->ndiamagnetic > 0) {
        if ((ap = (void*) malloc(efit->ndiamagnetic * sizeof(DIAMAGNETIC))) == NULL) return (ERROR_ALLOCATING_HEAP);
        efit->diamagnetic = (DIAMAGNETIC*) ap;
        ap = NULL;
    } else {
        efit->diamagnetic = NULL;
    }

    if (efit->ntoroidalfield > 0) {
        if ((ap = (void*) malloc(efit->ntoroidalfield * sizeof(TOROIDALFIELD))) == NULL) return (ERROR_ALLOCATING_HEAP);
        efit->toroidalfield = (TOROIDALFIELD*) ap;
        ap = NULL;
    } else {
        efit->toroidalfield = NULL;
    }

    if (efit->nlimiter > 0) {
        if ((ap = (void*) malloc(efit->nlimiter * sizeof(LIMITER))) == NULL) return (ERROR_ALLOCATING_HEAP);
        efit->limiter = (LIMITER*) ap;
        ap = NULL;
    } else {
        efit->limiter = NULL;
    }

    return 0;
}


int alloc_pfcircuit(PFCIRCUIT* str)
{
    void* ap = NULL;

    if (str->nco > 0) {
        if ((ap = (void*) malloc(str->nco * sizeof(int))) == NULL) return (ERROR_ALLOCATING_HEAP);
        str->coil = (int*) ap;
    } else {
        str->coil = NULL;
    }

    return 0;
}


int alloc_pfcoils(PFCOILS* str)
{
    float* ap = NULL;

    if (str->nco > 0) {
        if ((ap = (float*) malloc(str->nco * sizeof(float))) == NULL) return (ERROR_ALLOCATING_HEAP);
        str->r = ap;
        if ((ap = (float*) malloc(str->nco * sizeof(float))) == NULL) return (ERROR_ALLOCATING_HEAP);
        str->z = ap;
        if ((ap = (float*) malloc(str->nco * sizeof(float))) == NULL) return (ERROR_ALLOCATING_HEAP);
        str->dr = ap;
        if ((ap = (float*) malloc(str->nco * sizeof(float))) == NULL) return (ERROR_ALLOCATING_HEAP);
        str->dz = ap;
    } else {
        str->r = NULL;
        str->z = NULL;
        str->dr = NULL;
        str->dz = NULL;
    }
    return 0;
}


int alloc_pfpassive(PFPASSIVE* str)
{
    float* ap = NULL;

    if (str->nco > 0) {
        if ((ap = (float*) malloc(str->nco * sizeof(float))) == NULL) return (ERROR_ALLOCATING_HEAP);
        str->r = ap;
        if ((ap = (float*) malloc(str->nco * sizeof(float))) == NULL) return (ERROR_ALLOCATING_HEAP);
        str->z = ap;
        if ((ap = (float*) malloc(str->nco * sizeof(float))) == NULL) return (ERROR_ALLOCATING_HEAP);
        str->dr = ap;
        if ((ap = (float*) malloc(str->nco * sizeof(float))) == NULL) return (ERROR_ALLOCATING_HEAP);
        str->dz = ap;
        if ((ap = (float*) malloc(str->nco * sizeof(float))) == NULL) return (ERROR_ALLOCATING_HEAP);
        str->ang1 = ap;
        if ((ap = (float*) malloc(str->nco * sizeof(float))) == NULL) return (ERROR_ALLOCATING_HEAP);
        str->ang2 = ap;
        if ((ap = (float*) malloc(str->nco * sizeof(float))) == NULL) return (ERROR_ALLOCATING_HEAP);
        str->res = ap;
    } else {
        str->r = NULL;
        str->z = NULL;
        str->dr = NULL;
        str->dz = NULL;
        str->ang1 = NULL;
        str->ang2 = NULL;
        str->res = NULL;
    }
    return 0;
}


int alloc_fluxloop(FLUXLOOP* str)
{
    float* ap = NULL;

    if (str->nco > 0) {
        if ((ap = (float*) malloc(str->nco * sizeof(float))) == NULL) return (ERROR_ALLOCATING_HEAP);
        str->r = ap;
        if ((ap = (float*) malloc(str->nco * sizeof(float))) == NULL) return (ERROR_ALLOCATING_HEAP);
        str->z = ap;
        if ((ap = (float*) malloc(str->nco * sizeof(float))) == NULL) return (ERROR_ALLOCATING_HEAP);
        str->dphi = ap;
    } else {
        str->r = NULL;
        str->z = NULL;
        str->dphi = NULL;
    }
    return 0;
}


int alloc_limiter(LIMITER* str)
{
    float* ap = NULL;

    if (str->nco > 0) {
        if ((ap = (float*) malloc(str->nco * sizeof(float))) == NULL) return (ERROR_ALLOCATING_HEAP);
        str->r = ap;
        if ((ap = (float*) malloc(str->nco * sizeof(float))) == NULL) return (ERROR_ALLOCATING_HEAP);
        str->z = ap;
    } else {
        str->r = NULL;
        str->z = NULL;
    }
    return 0;
}                    
