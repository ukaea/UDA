/*---------------------------------------------------------------
* Allocate Memory for XML based Hierarchical Data Structures
*
* returns 0 if No Heap Allocation Error occurs
*
*--------------------------------------------------------------*/

#include "allocXMLData.h"

#ifdef HIERARCHICAL_DATA

int alloc_efit(EFIT* efit)
{
    void* ap = nullptr;

    if (efit->npfcoils > 0) {
        if ((ap = (void*)malloc(efit->npfcoils * sizeof(PFCOILS))) == nullptr) return ERROR_ALLOCATING_HEAP;
        efit->pfcoils = (PFCOILS*)ap;
        ap = nullptr;
    } else {
        efit->pfcoils = nullptr;
    }

    if (efit->npfpassive > 0) {
        if ((ap = (void*)malloc(efit->npfpassive * sizeof(PFPASSIVE))) == nullptr) return ERROR_ALLOCATING_HEAP;
        efit->pfpassive = (PFPASSIVE*)ap;
        ap = nullptr;
    } else {
        efit->pfpassive = nullptr;
    }

    if (efit->npfsupplies > 0) {
        if ((ap = (void*)malloc(efit->npfsupplies * sizeof(PFSUPPLIES))) == nullptr) return ERROR_ALLOCATING_HEAP;
        efit->pfsupplies = (PFSUPPLIES*)ap;
        ap = nullptr;
    } else {
        efit->pfsupplies = nullptr;
    }

    if (efit->nfluxloops > 0) {
        if ((ap = (void*)malloc(efit->nfluxloops * sizeof(FLUXLOOP))) == nullptr) return ERROR_ALLOCATING_HEAP;
        efit->fluxloop = (FLUXLOOP*)ap;
        ap = nullptr;
    } else {
        efit->fluxloop = nullptr;
    }

    if (efit->nmagprobes > 0) {
        if ((ap = (void*)malloc(efit->nmagprobes * sizeof(MAGPROBE))) == nullptr) return ERROR_ALLOCATING_HEAP;
        efit->magprobe = (MAGPROBE*)ap;
        ap = nullptr;
    } else {
        efit->magprobe = nullptr;
    }

    if (efit->npfcircuits > 0) {
        if ((ap = (void*)malloc(efit->npfcircuits * sizeof(PFCIRCUIT))) == nullptr) return ERROR_ALLOCATING_HEAP;
        efit->pfcircuit = (PFCIRCUIT*)ap;
        ap = nullptr;
    } else {
        efit->pfcircuit = nullptr;
    }

    if (efit->nplasmacurrent > 0) {
        if ((ap = (void*)malloc(efit->nplasmacurrent * sizeof(PLASMACURRENT))) == nullptr) {
            return ERROR_ALLOCATING_HEAP;
        }
        efit->plasmacurrent = (PLASMACURRENT*)ap;
        ap = nullptr;
    } else {
        efit->plasmacurrent = nullptr;
    }

    if (efit->ndiamagnetic > 0) {
        if ((ap = (void*)malloc(efit->ndiamagnetic * sizeof(DIAMAGNETIC))) == nullptr) return ERROR_ALLOCATING_HEAP;
        efit->diamagnetic = (DIAMAGNETIC*)ap;
        ap = nullptr;
    } else {
        efit->diamagnetic = nullptr;
    }

    if (efit->ntoroidalfield > 0) {
        if ((ap = (void*)malloc(efit->ntoroidalfield * sizeof(TOROIDALFIELD))) == nullptr) {
            return ERROR_ALLOCATING_HEAP;
        }
        efit->toroidalfield = (TOROIDALFIELD*)ap;
        ap = nullptr;
    } else {
        efit->toroidalfield = nullptr;
    }

    if (efit->nlimiter > 0) {
        if ((ap = (void*)malloc(efit->nlimiter * sizeof(LIMITER))) == nullptr) return ERROR_ALLOCATING_HEAP;
        efit->limiter = (LIMITER*)ap;
        ap = nullptr;
    } else {
        efit->limiter = nullptr;
    }

    return 0;
}


int alloc_pfcircuit(PFCIRCUIT* str)
{
    void* ap = nullptr;

    if (str->nco > 0) {
        if ((ap = (void*)malloc(str->nco * sizeof(int))) == nullptr) return ERROR_ALLOCATING_HEAP;
        str->coil = (int*)ap;
    } else {
        str->coil = nullptr;
    }

    return 0;
}


int alloc_pfcoils(PFCOILS* str)
{
    float* ap = nullptr;

    if (str->nco > 0) {
        if ((ap = (float*)malloc(str->nco * sizeof(float))) == nullptr) return ERROR_ALLOCATING_HEAP;
        str->r = ap;
        if ((ap = (float*)malloc(str->nco * sizeof(float))) == nullptr) return ERROR_ALLOCATING_HEAP;
        str->z = ap;
        if ((ap = (float*)malloc(str->nco * sizeof(float))) == nullptr) return ERROR_ALLOCATING_HEAP;
        str->dr = ap;
        if ((ap = (float*)malloc(str->nco * sizeof(float))) == nullptr) return ERROR_ALLOCATING_HEAP;
        str->dz = ap;
    } else {
        str->r = nullptr;
        str->z = nullptr;
        str->dr = nullptr;
        str->dz = nullptr;
    }
    return 0;
}


int alloc_pfpassive(PFPASSIVE* str)
{
    float* ap = nullptr;

    if (str->nco > 0) {
        if ((ap = (float*)malloc(str->nco * sizeof(float))) == nullptr) return ERROR_ALLOCATING_HEAP;
        str->r = ap;
        if ((ap = (float*)malloc(str->nco * sizeof(float))) == nullptr) return ERROR_ALLOCATING_HEAP;
        str->z = ap;
        if ((ap = (float*)malloc(str->nco * sizeof(float))) == nullptr) return ERROR_ALLOCATING_HEAP;
        str->dr = ap;
        if ((ap = (float*)malloc(str->nco * sizeof(float))) == nullptr) return ERROR_ALLOCATING_HEAP;
        str->dz = ap;
        if ((ap = (float*)malloc(str->nco * sizeof(float))) == nullptr) return ERROR_ALLOCATING_HEAP;
        str->ang1 = ap;
        if ((ap = (float*)malloc(str->nco * sizeof(float))) == nullptr) return ERROR_ALLOCATING_HEAP;
        str->ang2 = ap;
        if ((ap = (float*)malloc(str->nco * sizeof(float))) == nullptr) return ERROR_ALLOCATING_HEAP;
        str->res = ap;
    } else {
        str->r = nullptr;
        str->z = nullptr;
        str->dr = nullptr;
        str->dz = nullptr;
        str->ang1 = nullptr;
        str->ang2 = nullptr;
        str->res = nullptr;
    }
    return 0;
}


int alloc_fluxloop(FLUXLOOP* str)
{
    float* ap = nullptr;

    if (str->nco > 0) {
        if ((ap = (float*)malloc(str->nco * sizeof(float))) == nullptr) return ERROR_ALLOCATING_HEAP;
        str->r = ap;
        if ((ap = (float*)malloc(str->nco * sizeof(float))) == nullptr) return ERROR_ALLOCATING_HEAP;
        str->z = ap;
        if ((ap = (float*)malloc(str->nco * sizeof(float))) == nullptr) return ERROR_ALLOCATING_HEAP;
        str->dphi = ap;
    } else {
        str->r = nullptr;
        str->z = nullptr;
        str->dphi = nullptr;
    }
    return 0;
}


int alloc_limiter(LIMITER* str)
{
    float* ap = nullptr;

    if (str->nco > 0) {
        if ((ap = (float*)malloc(str->nco * sizeof(float))) == nullptr) return ERROR_ALLOCATING_HEAP;
        str->r = ap;
        if ((ap = (float*)malloc(str->nco * sizeof(float))) == nullptr) return ERROR_ALLOCATING_HEAP;
        str->z = ap;
    } else {
        str->r = nullptr;
        str->z = nullptr;
    }
    return 0;
}

#endif
