//
// Library of XDR (de)serialiser routines for passing Hierarchical Data structures
//
//-----------------------------------------------------------------------

#include "xdrHData.h"

#ifdef HIERARCHICAL_DATA

//-----------------------------------------------------------------------
// Signal Instance

bool_t xdr_instance(XDR *xdrs, INSTANCE *str) {
    return( 		WrapXDRString(xdrs, (char *)str->archive, XMLMAXSTRING)
                    &&	WrapXDRString(xdrs, (char *)str->file, XMLMAXSTRING)
                    &&	WrapXDRString(xdrs, (char *)str->signal, XMLMAXSTRING)
                    &&	WrapXDRString(xdrs, (char *)str->owner, XMLMAXSTRING)
                    &&	WrapXDRString(xdrs, (char *)str->format, XMLMAXSTRING)
                    &&	xdr_int(xdrs, &str->seq)
                    &&	xdr_int(xdrs, &str->status)
                    &&	xdr_float(xdrs, &str->factor)
          );
}

//-----------------------------------------------------------------------
// Toroidal Magnetic Field

bool_t xdr_toroidalfield(XDR *xdrs, TOROIDALFIELD *str) {
    return( 		WrapXDRString(xdrs, (char *)str->id, XMLMAXSTRING)
                    &&	xdr_instance(xdrs, &str->instance)
                    &&	xdr_float(xdrs, &str->aerr)
                    &&	xdr_float(xdrs, &str->rerr)
          );
}

//-----------------------------------------------------------------------
// Plasma Current

bool_t xdr_plasmacurrent(XDR *xdrs, PLASMACURRENT *str) {
    return( 		WrapXDRString(xdrs, (char *)str->id, XMLMAXSTRING)
                    &&	xdr_instance(xdrs, &str->instance)
                    &&	xdr_float(xdrs, &str->aerr)
                    &&	xdr_float(xdrs, &str->rerr)
          );
}

//-----------------------------------------------------------------------
// Diamagnetic Flux

bool_t xdr_diamagnetic(XDR *xdrs, DIAMAGNETIC *str) {
    return( 		WrapXDRString(xdrs, (char *)str->id, XMLMAXSTRING)
                    &&	xdr_instance(xdrs, &str->instance)
                    &&	xdr_float(xdrs, &str->aerr)
                    &&	xdr_float(xdrs, &str->rerr)
          );
}

//-----------------------------------------------------------------------
// PF Circuit

bool_t xdr_pfcircuit1(XDR *xdrs, PFCIRCUIT *str) {
    return( 		WrapXDRString(xdrs, (char *)str->id, XMLMAXSTRING)
                    &&	xdr_instance(xdrs, &str->instance)
                    &&	xdr_int(xdrs, &str->nco)
                    &&	xdr_int(xdrs, &str->supply)
          );
}

bool_t xdr_pfcircuit2(XDR *xdrs, PFCIRCUIT *str) {
    return(xdr_vector(xdrs, (char *)str->coil, (int)str->nco, sizeof(int), (xdrproc_t)xdr_int));
}

//-----------------------------------------------------------------------
// Magnetic Probe

bool_t xdr_magprobe(XDR *xdrs, MAGPROBE *str) {
    return( 		WrapXDRString(xdrs, (char *)str->id, XMLMAXSTRING)
                    &&	xdr_instance(xdrs, &str->instance)
                    &&	xdr_float(xdrs, &str->r)
                    &&	xdr_float(xdrs, &str->z)
                    &&	xdr_float(xdrs, &str->angle)
                    &&	xdr_float(xdrs, &str->aerr)
                    &&	xdr_float(xdrs, &str->rerr)
          );
}

//-----------------------------------------------------------------------
// PF Supplies

bool_t xdr_pfsupplies(XDR *xdrs, PFSUPPLIES *str) {
    return( 		WrapXDRString(xdrs, (char *)str->id, XMLMAXSTRING)
                    &&	xdr_instance(xdrs, &str->instance)
                    &&	xdr_float(xdrs, &str->aerr)
                    &&	xdr_float(xdrs, &str->rerr)
          );
}

//-----------------------------------------------------------------------
// Flux Loops

bool_t xdr_fluxloop1(XDR *xdrs, FLUXLOOP *str) {
    return( 		WrapXDRString(xdrs, (char *)str->id, XMLMAXSTRING)
                    &&	xdr_instance(xdrs, &str->instance)
                    &&	xdr_int(xdrs, &str->nco)
                    &&	xdr_float(xdrs, &str->aerr)
                    &&	xdr_float(xdrs, &str->rerr)
          );
}

bool_t xdr_fluxloop2(XDR *xdrs, FLUXLOOP *str) {
    return(		xdr_vector(xdrs, (char *)str->r, (int)str->nco, sizeof(float), (xdrproc_t)xdr_float)
                &&	xdr_vector(xdrs, (char *)str->z, (int)str->nco, sizeof(float), (xdrproc_t)xdr_float)
                &&	xdr_vector(xdrs, (char *)str->dphi, (int)str->nco, sizeof(float), (xdrproc_t)xdr_float)
          );
}

//-----------------------------------------------------------------------
// PF Passive

bool_t xdr_pfpassive1(XDR *xdrs, PFPASSIVE *str) {
    return( 		WrapXDRString(xdrs, (char *)str->id, XMLMAXSTRING)
                    &&	xdr_instance(xdrs, &str->instance)
                    &&	xdr_int(xdrs, &str->nco)
                    &&	xdr_int(xdrs, &str->modelnrnz[0])
                    &&	xdr_int(xdrs, &str->modelnrnz[1])
                    &&	xdr_float(xdrs, &str->aerr)
                    &&	xdr_float(xdrs, &str->rerr)
          );
}


bool_t xdr_pfpassive2(XDR *xdrs, PFPASSIVE *str) {
    return(		xdr_vector(xdrs, (char *)str->r, (int)str->nco, sizeof(float), (xdrproc_t)xdr_float)
                &&	xdr_vector(xdrs, (char *)str->z, (int)str->nco, sizeof(float), (xdrproc_t)xdr_float)
                &&	xdr_vector(xdrs, (char *)str->dr, (int)str->nco, sizeof(float), (xdrproc_t)xdr_float)
                &&	xdr_vector(xdrs, (char *)str->dz, (int)str->nco, sizeof(float), (xdrproc_t)xdr_float)
                &&	xdr_vector(xdrs, (char *)str->ang1, (int)str->nco, sizeof(float), (xdrproc_t)xdr_float)
                &&	xdr_vector(xdrs, (char *)str->ang2, (int)str->nco, sizeof(float), (xdrproc_t)xdr_float)
                &&	xdr_vector(xdrs, (char *)str->res, (int)str->nco, sizeof(float), (xdrproc_t)xdr_float)
          );
}

//-----------------------------------------------------------------------
// PF Coils

bool_t xdr_pfcoils1(XDR *xdrs, PFCOILS *str) {
    return( 		WrapXDRString(xdrs, (char *)str->id, XMLMAXSTRING)
                    &&	xdr_instance(xdrs, &str->instance)
                    &&	xdr_int(xdrs, &str->nco)
                    &&	xdr_int(xdrs, &str->turns)
                    &&	xdr_float(xdrs, &str->fturns)
                    &&	xdr_int(xdrs, &str->modelnrnz[0])
                    &&	xdr_int(xdrs, &str->modelnrnz[1])
                    &&	xdr_float(xdrs, &str->aerr)
                    &&	xdr_float(xdrs, &str->rerr)
          );
}

bool_t xdr_pfcoils2(XDR *xdrs, PFCOILS *str) {
    return(		xdr_vector(xdrs, (char *)str->r, (int)str->nco, sizeof(float), (xdrproc_t)xdr_float)
                &&	xdr_vector(xdrs, (char *)str->z, (int)str->nco, sizeof(float), (xdrproc_t)xdr_float)
                &&	xdr_vector(xdrs, (char *)str->dr, (int)str->nco, sizeof(float), (xdrproc_t)xdr_float)
                &&	xdr_vector(xdrs, (char *)str->dz, (int)str->nco, sizeof(float), (xdrproc_t)xdr_float)
          );
}


//-----------------------------------------------------------------------
// Limiter

bool_t xdr_limiter1(XDR *xdrs, LIMITER *str) {
    return( 		xdr_int(xdrs, &str->nco)
                    &&	xdr_float(xdrs, &str->factor)
          );
}

bool_t xdr_limiter2(XDR *xdrs, LIMITER *str) {
    return(		xdr_vector(xdrs, (char *)str->r, (int)str->nco, sizeof(float), (xdrproc_t)xdr_float)
                &&	xdr_vector(xdrs, (char *)str->z, (int)str->nco, sizeof(float), (xdrproc_t)xdr_float)
          );
}

//-----------------------------------------------------------------------
// EFIT

bool_t xdr_efit(XDR *xdrs, EFIT *str) {
    return( 		WrapXDRString(xdrs, (char *)str->device, XMLMAXSTRING)
                    &&	xdr_int(xdrs, &str->exp_number)
                    &&	xdr_int(xdrs, &str->nfluxloops)
                    &&	xdr_int(xdrs, &str->nmagprobes)
                    &&	xdr_int(xdrs, &str->npfcircuits)
                    &&	xdr_int(xdrs, &str->npfpassive)
                    &&	xdr_int(xdrs, &str->npfsupplies)
                    &&	xdr_int(xdrs, &str->nplasmacurrent)
                    &&	xdr_int(xdrs, &str->ndiamagnetic)
                    &&	xdr_int(xdrs, &str->ntoroidalfield)
                    &&	xdr_int(xdrs, &str->npfcoils)
                    &&	xdr_int(xdrs, &str->nlimiter)
          );
}

#endif
