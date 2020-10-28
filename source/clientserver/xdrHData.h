#ifndef UDA_CLIENTSERVER_XDRHDATA_H
#define UDA_CLIENTSERVER_XDRHDATA_H

#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HIERARCHICAL_DATA

#include "idamclientserver.h"
#include "idamclientserverxml.h"

//-----------------------------------------------------------------------
// Signal Instance
LIBRARY_API bool_t xdr_instance(XDR *xdrs, INSTANCE *str);

//-----------------------------------------------------------------------
// Toroidal Magnetic Field
LIBRARY_API bool_t xdr_toroidalfield(XDR *xdrs, TOROIDALFIELD *str);

//-----------------------------------------------------------------------
// Plasma Current
LIBRARY_API bool_t xdr_plasmacurrent(XDR *xdrs, PLASMACURRENT *str);

//-----------------------------------------------------------------------
// Diamagnetic Flux
LIBRARY_API bool_t xdr_diamagnetic(XDR *xdrs, DIAMAGNETIC *str);

//-----------------------------------------------------------------------
// PF Circuit
LIBRARY_API bool_t xdr_pfcircuit1(XDR *xdrs, PFCIRCUIT *str);
LIBRARY_API bool_t xdr_pfcircuit2(XDR *xdrs, PFCIRCUIT *str);

//-----------------------------------------------------------------------
// Magnetic Probe
LIBRARY_API bool_t xdr_magprobe(XDR *xdrs, MAGPROBE *str);

//-----------------------------------------------------------------------
// PF Supplies
LIBRARY_API bool_t xdr_pfsupplies(XDR *xdrs, PFSUPPLIES *str);

//-----------------------------------------------------------------------
// Flux Loops
LIBRARY_API bool_t xdr_fluxloop1(XDR *xdrs, FLUXLOOP *str);
LIBRARY_API bool_t xdr_fluxloop2(XDR *xdrs, FLUXLOOP *str);

//-----------------------------------------------------------------------
// PF Passive
LIBRARY_API bool_t xdr_pfpassive1(XDR *xdrs, PFPASSIVE *str);
LIBRARY_API bool_t xdr_pfpassive2(XDR *xdrs, PFPASSIVE *str);

//-----------------------------------------------------------------------
// PF Coils
LIBRARY_API bool_t xdr_pfcoils1(XDR *xdrs, PFCOILS *str);
LIBRARY_API bool_t xdr_pfcoils2(XDR *xdrs, PFCOILS *str);

//-----------------------------------------------------------------------
// Limiter
LIBRARY_API bool_t xdr_limiter1(XDR *xdrs, LIMITER *str);
LIBRARY_API bool_t xdr_limiter2(XDR *xdrs, LIMITER *str);

//-----------------------------------------------------------------------
// EFIT
LIBRARY_API bool_t xdr_efit(XDR *xdrs, EFIT *str);

#endif // HIERARCHICAL_DATA

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_XDRHDATA_H

