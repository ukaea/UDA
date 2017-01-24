//! $LastChangedRevision: 173 $
//! $LastChangedDate: 2010-02-26 15:27:47 +0000 (Fri, 26 Feb 2010) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/client/accAPI_XML.c $

//---------------------------------------------------------------
// Accessor Functions to IDAM XML Parsed Hierarchical Structured Data
//
// ToDo:
//
// Change History:
//
// 03Oct2007 dgm	Original Version based on apiItmParser.c
// 15Nov2007 dgm	Compliant with IDAM Structure Passing
//--------------------------------------------------------------
#ifdef HIERARCHICAL_DATA

#include "idamclientserver.h"
#include "idamclientserverxml.h"
#include "idamclient.h"

#define TEST 0

//EFIT efit;		// Global Data Repository - Single Instance

//--------------------------------------------------------------
// C Accessors

//! Free Heap Associated with the EFIT Hierarchical Data Structures

void freeIdamEfit(int handle) {

    EFIT *efit = NULL;
    int i, nel;

    if(handle < 0 || handle >= Data_Block_Count) return;			// Valid Handle?
    if(Data_Block[handle].opaque_type != OPAQUE_TYPE_EFIT) return;	// an EFIT Structure?

    efit = (EFIT *)Data_Block[handle].opaque_block;

    if(efit->magprobe != NULL) {
        free((void *)efit->magprobe);
        efit->magprobe   = NULL;
        efit->nmagprobes = 0;
    }

    nel = efit->nfluxloops;
    if(nel > 0) {
        for(i=0; i<nel; i++) {
            if(efit->fluxloop[i].r    != NULL) free((void *)efit->fluxloop[i].r);
            if(efit->fluxloop[i].z    != NULL) free((void *)efit->fluxloop[i].z);
            if(efit->fluxloop[i].dphi != NULL) free((void *)efit->fluxloop[i].dphi);
        }
    }
    if(efit->fluxloop != NULL) {
        free((void *)efit->fluxloop);
        efit->fluxloop   = NULL;
        efit->nfluxloops = 0;
    }

    nel = efit->npfpassive;
    if(nel > 0) {
        for(i=0; i<nel; i++) {
            if(efit->pfpassive[i].r != NULL)    free((void *)efit->pfpassive[i].r);
            if(efit->pfpassive[i].z != NULL)    free((void *)efit->pfpassive[i].z);
            if(efit->pfpassive[i].dr != NULL)   free((void *)efit->pfpassive[i].dr);
            if(efit->pfpassive[i].dz != NULL)   free((void *)efit->pfpassive[i].dz);
            if(efit->pfpassive[i].ang1 != NULL) free((void *)efit->pfpassive[i].ang1);
            if(efit->pfpassive[i].ang2 != NULL) free((void *)efit->pfpassive[i].ang2);
            if(efit->pfpassive[i].res != NULL)  free((void *)efit->pfpassive[i].res);
        }
    }
    if(efit->pfpassive != NULL) {
        free((void *)efit->pfpassive);
        efit->pfpassive  = NULL;
        efit->npfpassive = 0;
    }

    nel = efit->npfcoils;
    if(nel > 0) {
        for(i=0; i<nel; i++) {
            if(efit->pfcoils[i].r != NULL)  free((void *)efit->pfcoils[i].r);
            if(efit->pfcoils[i].z != NULL)  free((void *)efit->pfcoils[i].z);
            if(efit->pfcoils[i].dr != NULL) free((void *)efit->pfcoils[i].dr);
            if(efit->pfcoils[i].dz != NULL) free((void *)efit->pfcoils[i].dz);
        }
    }
    if(efit->pfcoils != NULL) {
        free((void *)efit->pfcoils);
        efit->pfcoils  = NULL;
        efit->npfcoils = 0;
    }

    nel = efit->npfcircuits;
    if(nel > 0) {
        for(i=0; i<nel; i++) {
            if(efit->pfcircuit[i].coil != NULL) free((void *)efit->pfcircuit[i].coil);
        }
    }
    if(efit->pfcircuit != NULL) {
        free((void *)efit->pfcircuit);
        efit->pfcircuit   = NULL;
        efit->npfcircuits = 0;
    }

    if(efit->pfsupplies != NULL) {
        free((void *)efit->pfsupplies);
        efit->pfsupplies  = NULL;
        efit->npfsupplies = 0;
    }

    if(efit->plasmacurrent != NULL) {
        free((void *)efit->plasmacurrent);
        efit->plasmacurrent  = NULL;
        efit->nplasmacurrent = 0;
    }

    if(efit->toroidalfield != NULL) {
        free((void *)efit->toroidalfield);
        efit->toroidalfield  = NULL;
        efit->ntoroidalfield = 0;
    }

    if(efit->diamagnetic != NULL) {
        free((void *)efit->diamagnetic);
        efit->diamagnetic  = NULL;
        efit->ndiamagnetic = 0;
    }

    nel = efit->nlimiter;
    if(nel > 0) {
        for(i=0; i<nel; i++) {
            if(efit->limiter[i].r != NULL) free((void *)efit->limiter[i].r);
            if(efit->limiter[i].z != NULL) free((void *)efit->limiter[i].z);
        }
    }
    if(efit->limiter != NULL) {
        free((void *)efit->limiter);
        efit->limiter  = NULL;
        efit->nlimiter = 0;
    }

    free(Data_Block[handle].opaque_block);			// Free the structure itself
    Data_Block[handle].opaque_block = NULL;
    Data_Block[handle].opaque_type = OPAQUE_TYPE_UNKNOWN;

    return;
}

//! Return the EFIT Data Structure

EFIT *getIdamEfit(int handle) {
    if(handle < 0 || handle >= Data_Block_Count) return NULL;			// Valid Handle?
    if(Data_Block[handle].opaque_type != OPAQUE_TYPE_EFIT) return NULL;		// an EFIT Structure?
    return((EFIT *)Data_Block[handle].opaque_block);
}


//! Return the number of Flux Loops

int getIdamFluxLoopCount(int handle) {
    EFIT *efit = NULL;
    if(handle < 0 || handle >= Data_Block_Count) return 0;			// Valid Handle?
    if(Data_Block[handle].opaque_type != OPAQUE_TYPE_EFIT) return 0;		// an EFIT Structure?
    efit = (EFIT *)Data_Block[handle].opaque_block;
    return(efit->nfluxloops);
}

//! Return the number of Flux Loops Coordinates

int getIdamFluxLoopCoordCount(int handle, int n) {
    EFIT *efit = NULL;
    if(handle < 0 || handle >= Data_Block_Count) return 0;			// Valid Handle?
    if(Data_Block[handle].opaque_type != OPAQUE_TYPE_EFIT) return 0;		// an EFIT Structure?
    efit = (EFIT *)Data_Block[handle].opaque_block;
    if(n >= 0 && n <= efit->nfluxloops-1)
        return(efit->fluxloop[n].nco);
    else return 0;
}

//! Return the number of Limiters

int getIdamLimiterCount(int handle) {
    EFIT *efit = NULL;
    if(handle < 0 || handle >= Data_Block_Count) return 0;			// Valid Handle?
    if(Data_Block[handle].opaque_type != OPAQUE_TYPE_EFIT) return 0;		// an EFIT Structure?
    efit = (EFIT *)Data_Block[handle].opaque_block;
    return(efit->nlimiter);
}

//! Return the number of Limiter Coordinates

int getIdamLimiterCoordCount(int handle, int n) {
    EFIT *efit = NULL;
    if(handle < 0 || handle >= Data_Block_Count) return 0;			// Valid Handle?
    if(Data_Block[handle].opaque_type != OPAQUE_TYPE_EFIT) return 0;		// an EFIT Structure?
    efit = (EFIT *)Data_Block[handle].opaque_block;
    if(n >= 0 && n <= efit->nlimiter-1)
        return(efit->limiter[n].nco);
    else return 0;
}

//! Return the number of Magnetic Probes

int getIdamMagProbeCount(int handle) {
    EFIT *efit = NULL;
    if(handle < 0 || handle >= Data_Block_Count) return 0;			// Valid Handle?
    if(Data_Block[handle].opaque_type != OPAQUE_TYPE_EFIT) return 0;		// an EFIT Structure?
    efit = (EFIT *)Data_Block[handle].opaque_block;
    return(efit->nmagprobes);
}

//! Return the PF Supplies

int getIdamPFSupplyCount(int handle) {
    EFIT *efit = NULL;
    if(handle < 0 || handle >= Data_Block_Count) return 0;			// Valid Handle?
    if(Data_Block[handle].opaque_type != OPAQUE_TYPE_EFIT) return 0;		// an EFIT Structure?
    efit = (EFIT *)Data_Block[handle].opaque_block;
    return(efit->npfsupplies);
}

//! Return the number of PF Coils

int getIdamPFCoilCount(int handle) {
    EFIT *efit = NULL;
    if(handle < 0 || handle >= Data_Block_Count) return 0;			// Valid Handle?
    if(Data_Block[handle].opaque_type != OPAQUE_TYPE_EFIT) return 0;		// an EFIT Structure?
    efit = (EFIT *)Data_Block[handle].opaque_block;
    return(efit->npfcoils);
}

//! Return the number of PF Coil Coordinates

int getIdamPFCoilCoordCount(int handle, int n) {
    EFIT *efit = NULL;
    if(handle < 0 || handle >= Data_Block_Count) return 0;			// Valid Handle?
    if(Data_Block[handle].opaque_type != OPAQUE_TYPE_EFIT) return 0;		// an EFIT Structure?
    efit = (EFIT *)Data_Block[handle].opaque_block;
    if(n >= 0 && n <= efit->npfcoils-1)
        return(efit->pfcoils[n].nco);
    else return 0;
}

//! Return the number of PF Circuits

int getIdamPFCircuitCount(int handle) {
    EFIT *efit = NULL;
    if(handle < 0 || handle >= Data_Block_Count) return 0;			// Valid Handle?
    if(Data_Block[handle].opaque_type != OPAQUE_TYPE_EFIT) return 0;		// an EFIT Structure?
    efit = (EFIT *)Data_Block[handle].opaque_block;
    return(efit->npfcircuits);
}

//! Return the number of PF Circuit Connections

int getIdamPFCircuitConnectionCount(int handle, int n) {
    EFIT *efit = NULL;
    if(handle < 0 || handle >= Data_Block_Count) return 0;			// Valid Handle?
    if(Data_Block[handle].opaque_type != OPAQUE_TYPE_EFIT) return 0;		// an EFIT Structure?
    efit = (EFIT *)Data_Block[handle].opaque_block;
    if(n >= 0 && n <= efit->npfcircuits-1)
        return(efit->pfcircuit[n].nco);
    else return 0;
}

//! Return the number of PF Passive elements

int getIdamPFPassiveCount(int handle) {
    EFIT *efit = NULL;
    if(handle < 0 || handle >= Data_Block_Count) return 0;			// Valid Handle?
    if(Data_Block[handle].opaque_type != OPAQUE_TYPE_EFIT) return 0;		// an EFIT Structure?
    efit = (EFIT *)Data_Block[handle].opaque_block;
    return(efit->npfpassive);
}

//! Return the number of PF Passive elements Coordinates

int getIdamPFPassiveCoordCount(int handle, int n) {
    EFIT *efit = NULL;
    if(handle < 0 || handle >= Data_Block_Count) return 0;			// Valid Handle?
    if(Data_Block[handle].opaque_type != OPAQUE_TYPE_EFIT) return 0;		// an EFIT Structure?
    efit = (EFIT *)Data_Block[handle].opaque_block;
    if(n >= 0 && n <= efit->npfpassive-1)
        return(efit->pfpassive[n].nco);
    else return 0;
}

//! Return the number of Plasma Currents

int getIdamPlasmaCurrentCount(int handle) {
    EFIT *efit = NULL;
    if(handle < 0 || handle >= Data_Block_Count) return 0;			// Valid Handle?
    if(Data_Block[handle].opaque_type != OPAQUE_TYPE_EFIT) return 0;		// an EFIT Structure?
    efit = (EFIT *)Data_Block[handle].opaque_block;
    return(efit->nplasmacurrent);
}

//! Return the number of Diamagnetic

int getIdamDiaMagneticCount(int handle) {
    EFIT *efit = NULL;
    if(handle < 0 || handle >= Data_Block_Count) return 0;			// Valid Handle?
    if(Data_Block[handle].opaque_type != OPAQUE_TYPE_EFIT) return 0;		// an EFIT Structure?
    efit = (EFIT *)Data_Block[handle].opaque_block;
    return(efit->ndiamagnetic);
}

//! Return the number of Toroidal Fields

int getIdamToroidalFieldCount(int handle) {
    EFIT *efit = NULL;
    if(handle < 0 || handle >= Data_Block_Count) return 0;			// Valid Handle?
    if(Data_Block[handle].opaque_type != OPAQUE_TYPE_EFIT) return 0;		// an EFIT Structure?
    efit = (EFIT *)Data_Block[handle].opaque_block;
    return(efit->ntoroidalfield);
}

//! Return a Pointer to the Device name

char *getIdamDeviceName(int handle) {
    EFIT *efit = NULL;
    if(handle < 0 || handle >= Data_Block_Count) return 0;			// Valid Handle?
    if(Data_Block[handle].opaque_type != OPAQUE_TYPE_EFIT) return 0;		// an EFIT Structure?
    efit = (EFIT *)Data_Block[handle].opaque_block;
    return efit->device;
}

//! Return the Specified Experiment Number

int getIdamExpNumber(int handle) {
    EFIT *efit = NULL;
    if(handle < 0 || handle >= Data_Block_Count) return 0;			// Valid Handle?
    if(Data_Block[handle].opaque_type != OPAQUE_TYPE_EFIT) return 0;		// an EFIT Structure?
    efit = (EFIT *)Data_Block[handle].opaque_block;
    return(efit->exp_number);
}

//! Return Details of the Data Location and Format

void getIdamDataProperty(INSTANCE *str, int *seq, int *status, float *factor,
                         char **archive, char **file, char **signal, char **owner, char **format) {
    if(str == NULL) return;
    *archive = str->archive;
    *file    = str->file;
    *signal  = str->signal;
    *owner   = str->owner;
    *format  = str->format;
    *seq     = str->seq;
    *status  = str->status;
    *factor  = str->factor;

    return;
}

//! Return the Magnetic Probe Data

int getIdamMagProbe(int handle, int n, float *r, float *z, float *angle, float *aerr, float *rerr) {
    EFIT *efit = NULL;
    if(handle < 0 || handle >= Data_Block_Count) return 0;			// Valid Handle?
    if(Data_Block[handle].opaque_type != OPAQUE_TYPE_EFIT) return 0;		// an EFIT Structure?
    efit = (EFIT *)Data_Block[handle].opaque_block;
    if(n >= 0 && n <= efit->nmagprobes-1) {
        *r      = efit->magprobe[n].r;
        *z      = efit->magprobe[n].z;
        *angle  = efit->magprobe[n].angle;
        *aerr   = efit->magprobe[n].aerr;
        *rerr   = efit->magprobe[n].rerr;
        return 0;
    } else return 1;
}

//! Return the PF Supply Data

int getIdamPFSupply(int handle, int n, float *aerr, float *rerr) {
    EFIT *efit = NULL;
    if(handle < 0 || handle >= Data_Block_Count) return 0;			// Valid Handle?
    if(Data_Block[handle].opaque_type != OPAQUE_TYPE_EFIT) return 0;		// an EFIT Structure?
    efit = (EFIT *)Data_Block[handle].opaque_block;
    if(n >= 0 && n <= efit->npfsupplies-1) {
        *aerr = efit->pfsupplies[n].aerr;
        *rerr = efit->pfsupplies[n].rerr;
        return 0;
    } else return 1;
}

//! Return the FLux Loop Data

int getIdamFluxLoop(int handle, int n, float **r, float **z, float **dphi) {
    EFIT *efit = NULL;
    if(handle < 0 || handle >= Data_Block_Count) return 0;			// Valid Handle?
    if(Data_Block[handle].opaque_type != OPAQUE_TYPE_EFIT) return 0;		// an EFIT Structure?
    efit = (EFIT *)Data_Block[handle].opaque_block;
    if(n >= 0 && n <= efit->nfluxloops-1) {
        *r    = efit->fluxloop[n].r;
        *z    = efit->fluxloop[n].z;
        *dphi = efit->fluxloop[n].dphi;
        return 0;
    } else return 1;
}

//! Return the Passive Element Data

int getIdamPFPassive(int handle, int n, float **r, float **z, float **dr,
                     float **dz, float **ang1, float **ang2, float **res,
                     float *aerr, float *rerr) {
    EFIT *efit = NULL;
    if(handle < 0 || handle >= Data_Block_Count) return 0;			// Valid Handle?
    if(Data_Block[handle].opaque_type != OPAQUE_TYPE_EFIT) return 0;		// an EFIT Structure?
    efit = (EFIT *)Data_Block[handle].opaque_block;
    if(n >= 0 && n <= efit->npfpassive-1) {
        *aerr   = efit->pfpassive[n].aerr;
        *rerr   = efit->pfpassive[n].rerr;
        *r   = efit->pfpassive[n].r;
        *z   = efit->pfpassive[n].z;
        *dr  = efit->pfpassive[n].dr;
        *dz  = efit->pfpassive[n].dz;
        *ang1= efit->pfpassive[n].ang1;
        *ang2= efit->pfpassive[n].ang2;
        *res = efit->pfpassive[n].res;
        return 0;
    } else return 1;
}

//! Return the PF Coil Data

int getIdamPFCoil(int handle, int n, int *turns, float **r, float **z,
                  float **dr, float **dz, float *aerr, float *rerr) {
    EFIT *efit = NULL;
    if(handle < 0 || handle >= Data_Block_Count) return 0;			// Valid Handle?
    if(Data_Block[handle].opaque_type != OPAQUE_TYPE_EFIT) return 0;		// an EFIT Structure?
    efit = (EFIT *)Data_Block[handle].opaque_block;
    if(n >= 0 && n <= efit->npfcoils-1) {
        *turns  = efit->pfcoils[n].turns;
        *aerr   = efit->pfcoils[n].aerr;
        *rerr   = efit->pfcoils[n].rerr;
        *r  = efit->pfcoils[n].r;
        *z  = efit->pfcoils[n].z;
        *dr = efit->pfcoils[n].dr;
        *dz = efit->pfcoils[n].dz;
        return 0;
    } else return 1;
}

//! Return the PF Circuit Data

int getIdamPFCircuit(int handle, int n, int *supply, int **coil) {
    EFIT *efit = NULL;
    if(handle < 0 || handle >= Data_Block_Count) return 0;			// Valid Handle?
    if(Data_Block[handle].opaque_type != OPAQUE_TYPE_EFIT) return 0;		// an EFIT Structure?
    efit = (EFIT *)Data_Block[handle].opaque_block;
    if(n >= 0 && n <= efit->npfcircuits-1) {
        *supply = efit->pfcircuit[n].supply;
        *coil   = efit->pfcircuit[n].coil;
        return 0;
    } else
        return 1;
}

//! Return the Plasma Current Data

int getIdamPlasmaCurrent(int handle, float *aerr, float *rerr) {
    EFIT *efit = NULL;
    if(handle < 0 || handle >= Data_Block_Count) return 0;			// Valid Handle?
    if(Data_Block[handle].opaque_type != OPAQUE_TYPE_EFIT) return 0;		// an EFIT Structure?
    efit = (EFIT *)Data_Block[handle].opaque_block;
    *aerr   = efit->plasmacurrent->aerr;
    *rerr   = efit->plasmacurrent->rerr;
    return 0;
}

//! Return the Toroidal Field Data

int getIdamToroidalField(int handle, float *aerr, float *rerr) {
    EFIT *efit = NULL;
    if(handle < 0 || handle >= Data_Block_Count) return 0;			// Valid Handle?
    if(Data_Block[handle].opaque_type != OPAQUE_TYPE_EFIT) return 0;		// an EFIT Structure?
    efit = (EFIT *)Data_Block[handle].opaque_block;
    *aerr   = efit->toroidalfield->aerr;
    *rerr   = efit->toroidalfield->rerr;
    return 0;
}

//! Return the Limiter Data

int getIdamLimiterCoords(int handle, int n, float **r, float **z) {
    EFIT *efit = NULL;
    if(handle < 0 || handle >= Data_Block_Count) return 0;			// Valid Handle?
    if(Data_Block[handle].opaque_type != OPAQUE_TYPE_EFIT) return 0;		// an EFIT Structure?
    efit = (EFIT *)Data_Block[handle].opaque_block;
    if(n >= 0 && n <= efit->nlimiter-1) {
        *r = efit->limiter[n].r;
        *z = efit->limiter[n].z;
        return 0;
    } else return 1;
}

//! Return the Diamagnetic Data

int getIdamDiaMagnetic(int handle, float *aerr, float *rerr) {
    EFIT *efit = NULL;
    if(handle < 0 || handle >= Data_Block_Count) return 0;			// Valid Handle?
    if(Data_Block[handle].opaque_type != OPAQUE_TYPE_EFIT) return 0;		// an EFIT Structure?
    efit = (EFIT *)Data_Block[handle].opaque_block;
    *aerr   = efit->diamagnetic->aerr;
    *rerr   = efit->diamagnetic->rerr;
    return 0;
}

#endif
