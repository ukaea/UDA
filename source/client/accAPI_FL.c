//! $LastChangedRevision: 87 $
//! $LastChangedDate: 2008-12-19 15:34:15 +0000 (Fri, 19 Dec 2008) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/client/accAPI_FL.c $

//---------------------------------------------------------------
// Fortran Legacy Accessor Functions - dependent on compiler option IDAM_FORTRAN_LEGACY_NAMES
// All declared extern locally: there is No header file
//
// Change History
//
// 09Jul2009	dgm	Splitout from accAPI_F.c to isolate legacy extern functions and APIs
//--------------------------------------------------------------

#include "idamclientserverpublic.h"
#include "idamclientserverprivate.h"
#include "idamclientpublic.h"
#include "idamclientprivate.h"

#ifdef IDAM_FORTRAN_LEGACY_NAMES

//--------------------------------------------------------------------------------------------------
/* Notes:
 		All names are lower case
 		Function names are appended with an underscore _
 		All functions return VOID
 		Character Strings are Passed by Pointer with an Additional Integer (Passed by Value)
 		appended to the argument list. This is inserted automatically by the system and is not seen.
 		Character strings are passed back by writting direcly to the pointer address. The terminating
 		null character is not passed back.
		Floats are passed back by directly copying the structure contents to the address provided. No
		changes are made to array column/row ordering.
*/

//---------------------------------------------------------------------------------------------------------------------------------
// Fortran Accessors

extern void setproperty_(char *property, int lproperty) {
    setidamproperty_(property, lproperty);
}

extern void resetproperty_(char *property, int lproperty) {
    resetidamproperty_(property, lproperty);
}

extern void resetproperties_() {
    resetIdamProperties();
    return;
}

extern void puterrormodel_(int *handle, int *model, int *param_n, float *params) {
    putIdamErrorModel(*handle, *model, *param_n, params);
}
extern void putdimerrormodel_(int *handle, int *ndim, int *model, int *param_n, float *params) {
    putIdamDimErrorModel(*handle, *ndim, *model, *param_n, params);
}


extern void geterrorcode_(int *handle, int *errcode) {
    getidamerrorcode_(handle, errcode);
}

extern void geterrormsg_(int *hd, char *msg, int lngth) {
    getidamerrormsg_(hd, msg, lngth);
}

extern void getsourcestatus_(int *handle, int *status) {
    getidamsourcestatus_(handle, status);
}

extern void getsignalstatus_(int *handle, int *status) {
    getidamsignalstatus_(handle, status);
}

extern void getdatastatus_(int *handle, int *status) {
    getidamdatastatus_(handle, status);
}

extern void getlasthandle_(int *handle) {
    getidamlasthandle_(handle);
}

extern void getdatanum_(int *hd, int *datanum) {				// Number of Data Items
    getidamdatanum_(hd, datanum);
}

extern void  getrank_(int *hd, int *rank) {				// Data Array Rank
    getidamrank_(hd, rank);
}

extern void getorder_(int *hd, int *order) {				// Location of the Time Dimension
    getidamorder_(hd, order);
}

extern void getdatatype_(int *hd, int *data_type) {			// Type of Data Returned
    getidamdatatype_(hd, data_type);
}

extern void geterrortype_(int *hd, int *error_type) {			// Type of Data Error Returned
    getidamerrortype_(hd, error_type);
}

extern void getdatatypeid_(char *t, int *id, int lt) {
    getidamdatatypeid_(t, id, lt);
}

extern void geterrormodel_(int *handle, int *model, int *param_n, float *params) {
    getIdamErrorModel(*handle, model, param_n, params);
}

extern void geterrorasymmetry_(int *handle, int *asymmetry) {
    *asymmetry = getIdamErrorAsymmetry(*handle);
}

extern void geterrormodelid_(char *m, int *id, int lm) {
    getidamerrormodelid_(m, id, lm);
}

extern void getsyntheticdatablock_(int *handle, void *data) {
    getidamsyntheticdatablock_(handle, data);
}

extern void getfloatdatablock_(int *handle, float *data) {			// Return the Data Array cast to type Float
    getIdamFloatData(*handle, data);
}

extern void getdatablock_(int *hd, void *data) {					// Return the Data Array
    getidamdatablock_(hd, data);
}

extern void geterrorblock_(int *handle, void *errdata) {				// Return the Data Error Array
    getidamerrorblock_(handle, errdata);
}

extern void getfloaterrorblock_(int *handle, float *data) {			// Return the Data Array cast to type Float
    getIdamFloatError(*handle, data);
}

extern void getasymmetricerrorblock_(int *handle, int *above, void *errdata) {		// Return the Asymmetric Error Array Component
    getidamasymmetricerrorblock_(handle, above, errdata);
}

extern void getfloatasymmetricerrorblock_(int *handle, int *above, float *data) {	// Return the Asymmetric Error Array cast to type Float
    getIdamFloatAsymmetricError(*handle, *above, data);
}

extern void getdatalabellength_(int *hd, int *lngth) {			// Length of Data Units String
    getidamdatalabellength_(hd, lngth);
}

extern void getdatalabel_(int *hd, char *label, int lngth) {		// The Data Label
    getidamdatalabel_(hd, label, lngth);
}

extern void getdataunitslength_(int *hd, int *lngth) {			// Length of Data Units String
    getidamdataunitslength_(hd, lngth);
}

extern void getdataunits_(int *hd, char *units, int lngth) {		// Data Units
    getidamdataunits_(hd, units, lngth);
}

extern void getdatadesclength_(int *hd, int *lngth) {			// Length of Data Description String
    getidamdatadesclength_(hd, lngth);
}

extern void getdatadesc_(int *hd, char *desc, int lngth) {		// Data Description
    getidamdatadesc_(hd, desc, lngth);
}


extern void getdimnum_(int *hd, int *nd, int *num) {			// Length of the Dimension nd
    getidamdimnum_(hd, nd, num);
}

extern void getdimtype_(int *hd, int *nd, int *type) {			// Dimension nd Data Type
    getidamdimtype_(hd, nd, type);
}

extern void getdimerrortype_(int *handle, int *ndim, int *type) {	// Dimension Error Data Type
    *type  = getIdamDimErrorType(*handle, *ndim);
}
extern void getdimerrormodel_(int *handle, int *ndim, int *model, int *param_n, float *params) {
    getIdamDimErrorModel(*handle, *ndim, model, param_n, params);
}

extern void getdimerrorasymmetry_(int *handle, int *ndim, int *asymmetry) {
    *asymmetry = (int)getIdamDimErrorAsymmetry((int)*handle, (int)*ndim);
}

//==============================================================

extern void getsyntheticdimdatablock_(int *handle, int *ndim, void *data) {
    getidamsyntheticdimdatablock_(handle, ndim, data);
}

extern void getfloatdimdata_(int *handle, int *ndim, float *data) {		// Dimension nd Data Array cast to Float
    getIdamFloatDimData(*handle, *ndim, data);
}

extern void getdimdataXXX_(int *hd, int *nd, float *data) {				// Dimension nd Data Array
    float *fp=(float *)Data_Block[*hd].dims[*nd].dim;
    int   i, ndim=Data_Block[*hd].dims[*nd].dim_n;


    fprintf(stdout,"************************************ getdimdata_ *********************************\n");
    fprintf(stdout,"************************************ Returning X Floats Only !!! *****************\n");

    ndim = 1;
    //memcpy((void *)data, (void *)fp, (size_t)ndim*sizeof(float));

    for(i=0; i<ndim; i++) *(data+i) = *(fp+i);
}

extern void getdimdata_(int *hd, int *nd, void *data) {				// Dimension nd Data Array
    getidamdimdata_(hd, nd, data);
}

extern void getdimdatablock_(int *hd, int *nd, void *data) {			// Dimension nd Data Array
    getidamdimdata_(hd,nd,data);
}

extern void getdimasymmetricerrorblock_(int *handle, int *ndim, int *above, void *data) {
    getidamdimasymmetricerrorblock_(handle, ndim, above, data);
}

extern void getdimerrorblock_(int *handle, int *ndim, void *data) {
    getdimerrorblock_(handle, ndim, data);
}

extern void getfloatdimasymmetricerrorblock_(int *handle, int *ndim, int *above, float *data) {
    getIdamFloatDimAsymmetricError(*handle, *ndim, *above, data);
}

extern void getfloatdimerrorblock_(int *handle, int *ndim, float *data) {
    getIdamFloatDimError(*handle, *ndim, data);
}

//=============================================================

extern void getdimlabellength_(int *hd, int *nd, int *lngth) {	// Length of Dimension nd Label String
    getidamdimlabellength_(hd, nd, lngth);
}

extern void getdimlabel_(int *hd, int *nd, char *label, int lngth) {	// Dimension nd Label
    getidamdimlabel_(hd, nd, label, lngth);
}

extern void getdimunitslength_(int *hd, int *nd, int *lngth) {	// Length of Dimension nd Units String
    getidamdimunitslength_(hd, nd, lngth);
}

extern void getdimunits_(int *hd, int *nd, char *units, int lngth) {	// Dimension nd Units
    getidamdimunits_(hd, nd, units, lngth);
}

extern void clientfree_(int *hd) {
    int handle = *hd;
    idamFree(handle);
}
extern void ClientFree_(int *hd) {
    int handle = *hd;
    idamFree(handle);
}

extern void clientfreeall_() {
    idamFreeAll();
}

extern void get_env_(char *str, int *rc, char *env, int lstr, int lenv) {
    getidamenv_(str, rc, env, lstr, lenv);
}

extern void whereami_(void *var, char *loc, int lloc) {
    whereidamami_(var, loc, lloc);
}

#endif
