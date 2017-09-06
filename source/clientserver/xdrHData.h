#ifndef IDAM_CLIENTSERVER_XDRHDATA_H
#define IDAM_CLIENTSERVER_XDRHDATA_H

#ifdef HIERARCHICAL_DATA

#include "idamclientserver.h"
#include "idamclientserverxml.h"

//-----------------------------------------------------------------------
// Signal Instance
bool_t xdr_instance(XDR *xdrs, INSTANCE *str);

//-----------------------------------------------------------------------
// Toroidal Magnetic Field
bool_t xdr_toroidalfield(XDR *xdrs, TOROIDALFIELD *str);

//-----------------------------------------------------------------------
// Plasma Current
bool_t xdr_plasmacurrent(XDR *xdrs, PLASMACURRENT *str);

//-----------------------------------------------------------------------
// Diamagnetic Flux
bool_t xdr_diamagnetic(XDR *xdrs, DIAMAGNETIC *str);

//-----------------------------------------------------------------------
// PF Circuit
bool_t xdr_pfcircuit1(XDR *xdrs, PFCIRCUIT *str);
bool_t xdr_pfcircuit2(XDR *xdrs, PFCIRCUIT *str);

//-----------------------------------------------------------------------
// Magnetic Probe
bool_t xdr_magprobe(XDR *xdrs, MAGPROBE *str);

//-----------------------------------------------------------------------
// PF Supplies
bool_t xdr_pfsupplies(XDR *xdrs, PFSUPPLIES *str);

//-----------------------------------------------------------------------
// Flux Loops
bool_t xdr_fluxloop1(XDR *xdrs, FLUXLOOP *str);
bool_t xdr_fluxloop2(XDR *xdrs, FLUXLOOP *str);

//-----------------------------------------------------------------------
// PF Passive
bool_t xdr_pfpassive1(XDR *xdrs, PFPASSIVE *str);
bool_t xdr_pfpassive2(XDR *xdrs, PFPASSIVE *str);

//-----------------------------------------------------------------------
// PF Coils
bool_t xdr_pfcoils1(XDR *xdrs, PFCOILS *str);
bool_t xdr_pfcoils2(XDR *xdrs, PFCOILS *str);

//-----------------------------------------------------------------------
// Limiter
bool_t xdr_limiter1(XDR *xdrs, LIMITER *str);
bool_t xdr_limiter2(XDR *xdrs, LIMITER *str);

//-----------------------------------------------------------------------
// EFIT
bool_t xdr_efit(XDR *xdrs, EFIT *str);

#endif // HIERARCHICAL_DATA

#endif // IDAM_CLIENTSERVER_XDRHDATA_H

