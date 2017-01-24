//! $LastChangedRevision: 87 $
//! $LastChangedDate: 2008-12-19 15:34:15 +0000 (Fri, 19 Dec 2008) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/client/accAPI_C.c $

//---------------------------------------------------------------
// C Legacy Accessor Functions - dependent on compiler option IDAM_LEGACY_NAMES
// All declared extern in idamclientpublic.h
//
// Change History
//
// 09Jul2009	dgm	Splitout from accAPI_C.c to isolate legacy extern functions and APIs
//--------------------------------------------------------------

#include "idamclientserverpublic.h"
#include "idamclientserverprivate.h"
#include "idamclientpublic.h"
#include "idamclientprivate.h"

#ifdef FATCLIENT
#include "idamserver.h"
#endif

#ifdef IDAM_LEGACY_NAMES

//--------------------------------------------------------------
// Legacy APIs

int ClientAPI(const char *file, const char *signal, int pass, int exp_number)
{
    return(idamClientAPI(file, signal, pass, exp_number));
}

int IdamClientAPI(const char *file, const char *signal, int pass, int exp_number)
{
    return(idamClientAPI(file, signal, pass, exp_number));
}

int ClientFileAPI(const char *file, const char *signal, const char *format)
{
    return(idamClientFileAPI(file, signal, format));
}

int IdamClientFileAPI(const char *file, const char *signal, const char *format)
{
    return(idamClientFileAPI(file, signal, format));
}

int ClientFileAPI2(const char *file, const char *format, const char *owner,
                   const char *signal, int exp_number, int pass)
{
    return(idamClientFileAPI2(file, format, owner, signal, exp_number, pass));
}

int IdamClientFileAPI2(const char *file, const char *format, const char *owner,
                       const char *signal, int exp_number, int pass)
{
    return(idamClientFileAPI2(file, format, owner, signal, exp_number, pass));
}

int ClientTestAPI(const char *file, const char *signal, int pass, int exp_number)
{
    return(idamClientTestAPI(file, signal, pass, exp_number));
}

int IdamClientTestAPI(const char *file, const char *signal, int pass, int exp_number)
{
    return(idamClientTestAPI(file, signal, pass, exp_number));
}

int ClientMDS(const char *server, const char *tree, const char *node, int *treenum)
{
    return(idamClientMDS(server, tree, node, *treenum));
}

int IdamClientMDS(const char *server, const char *tree, const char *node, int treenum)
{
    return(idamClientMDS(server, tree, node, treenum));
}

int IdamGetAPI(const char *data_object, const char *data_source) {
    return(idamGetAPI(data_object, data_source));
}

int IdamAPI(const char *signal, int exp_number) {
    return(idamAPI(signal, exp_number));
}

int IdamPassAPI(const char *signal, int exp_number, int pass)
{
    return(idamPassAPI(signal, exp_number, pass));
}

int IdamGenAPI(const char *archive, const char *device, const char *signal, int exp_number, int pass) {
    return(idamGenAPI(archive, device, signal, exp_number, pass));
}

void IdamFree(int handle) {
    idamFree(handle);
}
void IdamFreeAll() {
    idamFreeAll();
}

//--------------------------------------------------------------
// Set Properties

void setProperty(char *property) {
    setIdamProperty(property);
}

void resetProperty(char *property) {
    resetIdamProperty(property);
}

void resetProperties() {
    resetIdamProperties();
}

CLIENT_BLOCK saveProperties() {
    return(saveIdamProperties());
}

void restoreProperties(CLIENT_BLOCK cb) {
    restoreIdamProperties(cb);
}

//--------------------------------------------------------------
// Standard C PUT Accessor Routines

void putErrorModel(int handle, int model, int param_n, float *params) {
    putIdamErrorModel(handle, model, param_n, params);
}

void putDimErrorModel(int handle, int ndim, int model, int param_n, float *params) {
    putIdamDimErrorModel(handle, ndim, model, param_n, params);
}

//--------------------------------------------------------------
// Standard C GET Accessor Routines

int getErrorCode(int handle) {
    return(getIdamErrorCode(handle));
}

int GetErrorCode(int handle) {
    return (getIdamErrorCode(handle));
}

char *getErrorMsg(int handle) {
    return(getIdamErrorMsg(handle));
}

char *GetErrorMsg(int handle) {
    return(getIdamErrorMsg(handle));
}

int getSourceStatus(int handle) {
    return(getIdamSourceStatus(handle));
}

int GetSourceStatus(int handle) {
    return(getIdamSourceStatus(handle));
}

int getSignalStatus(int handle) {
    return(getIdamSignalStatus(handle));
}

int GetSignalStatus(int handle) {
    return(getIdamSignalStatus(handle));
}

int getDataStatus(int handle) {
    return(getIdamDataStatus(handle));
}

int GetDataStatus(int handle) {
    return(getIdamDataStatus(handle));
}

int getLastHandle() {
    return(getIdamLastHandle());
}

int GetLastHandle() {
    return(getIdamLastHandle());
}

int getDataNum(int handle) {
    return(getIdamDataNum(handle));
}

int GetDataNum(int handle) {
    return(getIdamDataNum(handle));
}

int getRank(int handle) {
    return(getIdamRank(handle));
}

int GetRank(int handle) {
    return(getIdamRank(handle));
}

int getOrder(int handle) {
    return(getIdamOrder(handle));
}

int GetOrder(int handle) {
    return(getIdamOrder(handle));
}

int getDataType(int handle) {
    return(getIdamDataType(handle));
}

int GetDataType(int handle) {
    return(getIdamDataType(handle));
}

int getErrorType(int handle) {
    return(getIdamErrorType(handle));
}

int GetErrorType(int handle) {
    return(getIdamErrorType(handle));
}

int getDataTypeId(const char *type) {
    return(getIdamDataTypeId(type));
}

void getErrorModel(int handle, int *model, int *param_n, float *params) {
    getIdamErrorModel(handle, model, param_n, params);
}

int getErrorAsymmetry(int handle) {
    return(getIdamErrorAsymmetry(handle));
}

// Return the Internal Code for a named Error Model
int getErrorModelId(const char *model) {
    return(getIdamErrorModelId(model));
}

char *getSyntheticData(int handle) {
    return(getIdamSyntheticData(handle));
}

char *getData(int handle) {
    return(getIdamData(handle));
}

char *GetData(int handle) {
    return(getIdamData(handle));
}

char *getAsymmetricError(int handle, int above) {
    return(getIdamAsymmetricError(handle, above));
}

char *getError(int handle) {
    return(getIdamError(handle));
}

char *GetError(int handle) {
    return(getIdamError(handle));
}

void getFloatData(int handle, float *fp) {
    getIdamFloatData(handle, fp);
}

void GetFloatData(int handle, float *fp) {
    getIdamFloatData(handle, fp);
}

void getFloatAsymmetricError(int handle, int above, float *fp) {
    getIdamFloatAsymmetricError(handle, above, fp);
}

void getFloatError(int handle, float *fp) {
    getIdamFloatError(handle,fp);
}

void GetFloatError(int handle, float *fp) {
    getIdamFloatError(handle, fp);
    return;
}

void getDBlock(int handle, DATA_BLOCK *db) {
    getIdamDBlock(handle, db);
}

void GetDBlock(int handle, DATA_BLOCK *db) {
    getDBlock(handle, db);
}

DATA_BLOCK *getDataBlock(int handle) {
    return(getIdamDataBlock(handle));
}

DATA_BLOCK *GetDataBlock(int handle) {
    return(getDataBlock(handle));
}



char *getDataLabel(int handle) {
    return(getIdamDataLabel(handle));
}

char *GetDataLabel(int handle) {
    return(getIdamDataLabel(handle));
}

char *getDataUnits(int handle) {
    return(getIdamDataUnits(handle));
}

char *GetDataUnits(int handle) {
    return(getIdamDataUnits(handle));
}

char *getDataDesc(int handle) {
    return(getIdamDataDesc(handle));
}

char *GetDataDesc(int handle) {
    return(getIdamDataDesc(handle));
}

int getDimNum(int handle, int ndim) {
    return(getIdamDimNum(handle, ndim));
}

int GetDimNum(int handle, int ndim) {
    return(getIdamDimNum(handle, ndim));
}

int getDimType(int handle, int ndim) {
    return(getIdamDimType(handle, ndim));
}

int GetDimType(int handle, int ndim) {
    return(getIdamDimType(handle, ndim));
}

int getDimErrorType(int handle, int ndim) {
    return(getIdamDimErrorType(handle, ndim));
}

int getDimErrorAsymmetry(int handle, int ndim) {
    return(getIdamDimErrorAsymmetry(handle, ndim));
}

void getDimErrorModel(int handle, int ndim, int *model, int *param_n, float *params) {
    getIdamDimErrorModel(handle, ndim, model, param_n, params);
}

char *getSyntheticDimData(int handle, int ndim) {
    return(getIdamSyntheticDimData(handle, ndim));
}

char *getDimData(int handle, int ndim) {
    return(getIdamDimData(handle, ndim));
}

char *GetDimData(int handle, int ndim) {
    return(getIdamDimData(handle, ndim));
}

char *getDimLabel(int handle, int ndim) {
    return(getIdamDimLabel(handle, ndim));
}

char *GetDimLabel(int handle, int ndim) {
    return(getIdamDimLabel(handle, ndim));
}

char *getDimUnits(int handle, int ndim) {
    return(getIdamDimUnits(handle, ndim));
}

char *GetDimUnits(int handle, int ndim) {
    return(getIdamDimUnits(handle, ndim));
}



void getFloatDimData(int handle, int ndim, float *fp) {
    getIdamFloatDimData(handle, ndim, fp);
}

void GetFloatDimData(int handle, int ndim, float *fp) {
    getIdamFloatDimData(handle, ndim, fp);
}

DIMS *getDimBlock(int handle, int ndim) {
    return(getIdamDimBlock(handle, ndim));
}

char *getDimAsymmetricError(int handle, int ndim, int above) {
    return(getIdamDimAsymmetricError(handle, ndim, above));
}

char *getDimError(int handle, int ndim) {
    return(getIdamDimError(handle, ndim));
}

void getFloatDimAsymmetricError(int handle, int ndim, int above, float *fp) {
    getIdamFloatDimAsymmetricError(handle, ndim, above, fp);
}

void GetFloatDimAsymmetricError(int handle, int ndim, int above, float *fp) {
    getIdamFloatDimAsymmetricError(handle, ndim, above, fp);
}

void getFloatDimError(int handle, int ndim, float *fp) {
    getIdamFloatDimError(handle, ndim, fp);
}

void GetFloatDimError(int handle, int ndim, float *fp) {
    getIdamFloatDimError(handle, ndim, fp);
}



DATA_SYSTEM *getDataSystem(int handle) {
    return(getIdamDataSystem(handle));
}

DATA_SYSTEM *GetDataSystem(int handle) {
    return(getIdamDataSystem(handle));
}

SYSTEM_CONFIG *getSystemConfig(int handle) {
    return(getIdamSystemConfig(handle));
}

SYSTEM_CONFIG *GetSystemConfig(int handle) {
    return(getIdamSystemConfig(handle));
}

DATA_SOURCE *getDataSource(int handle) {
    return(getIdamDataSource(handle));
}

DATA_SOURCE *GetDataSource(int handle) {
    return(getIdamDataSource(handle));
}

SIGNAL *getSignal(int handle) {
    return(getIdamSignal(handle));
}

SIGNAL *GetSignal(int handle) {
    return(getIdamSignal(handle));
}

SIGNAL_DESC *getSignalDesc(int handle) {
    return(getIdamSignalDesc(handle));
}

SIGNAL_DESC *GetSignalDesc(int handle) {
    return(getIdamSignalDesc(handle));
}


#endif

