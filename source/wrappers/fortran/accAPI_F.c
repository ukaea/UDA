/*---------------------------------------------------------------
*
* ToDo:
*
*    Check all accessors are available for fortran
*
*
* Fortran APIs and Accessor Functions
*
*--------------------------------------------------------------*/
#include "accAPI_F.h"

#include <stdlib.h>
#include <errno.h>

#include "logging/logging.h"
#include "clientserver/udaTypes.h"
#include "clientserver/stringUtils.h"

#include "accAPI.h"
#include "idamClient.h"
#include "idamGetAPI.h"
#include "clientAPI.h"
#include "clientMDS.h"

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

//--------------------------------------------------------------------------------------------------
// General API

extern void idamgetapi_(char* data_object, char* data_source, int* handle, int ldata_object, int ldata_source)
{
    char* object = (char*) malloc((size_t) (ldata_object + 1));
    char* source = (char*) malloc((size_t) (ldata_source + 1));
    *handle = -1;
    if (object == NULL || source == NULL) {
        free(object);
        free(source);
        return;
    }
    strncpy(object, data_object, ldata_object);
    object[ldata_object] = '\0';
    TrimString(object);
    strncpy(source, data_source, ldata_source);
    source[ldata_source] = '\0';
    TrimString(source);
    *handle = idamGetAPI(object, source);
    free( object);
    free( source);
    return;
}

//--------------------------------------------------------------------------------------------------
// Call for Latest data

extern void idamapi_(char* signal, int* pulno, int* handle, int lsignal)
{
    char source[1024];
    sprintf(source, "%d", *pulno);

    char* sig = (char*) malloc((size_t) (lsignal + 1));

    *handle = -1;

    strncpy(sig, signal, lsignal);
    sig[lsignal] = '\0';

    sig = TrimString(sig);

    if (idamGetLogLevel() == UDA_LOG_DEBUG) {
        errno = 0;
        FILE* ftnout = fopen("./ftn.log", "w");
        if (errno != 0) {
            if (ftnout != NULL) {
                fclose(ftnout);
            }
            free( sig);
            return;
        }
        fprintf(ftnout, "Routine: idamAPI\n");
        fprintf(ftnout, "Signal        %s\n", sig);
        fprintf(ftnout, "Source        %s\n", source);
        fprintf(ftnout, "Length Signal %d (%d)\n", (int) strlen(sig), lsignal);
        fclose(ftnout);
    }

    *handle = idamGetAPI(sig, source);

    free( sig);

    return;
}

// Call for Passed data

extern void idampassapi_(char* signal, int* pulno, int* pass, int* handle, int lsignal)
{
    char source[1024];
    sprintf(source, "%d/%d", *pulno, *pass);

    char* sig = (char*) malloc((size_t) (lsignal + 1));

    FILE* ftnout;

    *handle = -1;

    strncpy(sig, signal, lsignal);
    sig[lsignal] = '\0';

    sig = TrimString(sig);

    if (idamGetLogLevel() == UDA_LOG_DEBUG) {
        errno = 0;
        ftnout = fopen("./ftn.log", "w");
        if (errno != 0) {
            if (ftnout != NULL) {
                fclose(ftnout);
            }
            free( sig);
            return;
        }
        fprintf(ftnout, "Routine: idamPassAPI\n");
        fprintf(ftnout, "Signal        %s\n", sig);
        fprintf(ftnout, "Source        %s\n", source);
        fprintf(ftnout, "Length Signal %d (%d)\n", (int) strlen(sig), lsignal);
        fclose(ftnout);
    }

    *handle = idamGetAPI(sig, source);

    free( sig);

    return;
}

// Call for General/External Data

extern void idamgenapi_(char* archive, char* device, char* signal, int* pulno, int* pass,
                        int* handle, int larchive, int ldevice, int lsignal)
{
    char source[1024];
    sprintf(source, "%s::%d/%d", archive, *pulno, *pass);

    char* sig = (char*) malloc((size_t) (lsignal + 1));

    FILE* ftnout;

    *handle = -1;

    strncpy(sig, signal, lsignal);
    sig[lsignal] = '\0';

    sig = TrimString(sig);

    if (idamGetLogLevel() == UDA_LOG_DEBUG) {
        errno = 0;
        ftnout = fopen("./ftn.log", "w");
        if (errno != 0) {
            if (ftnout != NULL) {
                fclose(ftnout);
            }
            free( sig);
            return;
        }
        fprintf(ftnout, "Routine: idamGenAPI\n");
        fprintf(ftnout, "Signal         %s\n", sig);
        fprintf(ftnout, "Source         %s\n", source);
        fclose(ftnout);
    }

    *handle = idamGetAPI(sig, source);

    free( sig);

    return;
}

extern void idamfileapi_(char* file, char* signal, char* format, int* handle,
                         int lfile, int lsignal, int lformat)
{
    char* p = (char*) malloc((size_t) (lfile + 1));
    char* s = (char*) malloc((size_t) (lsignal + 1));
    char* f = (char*) malloc((size_t) (lformat + 1));

    *handle = -1;

    strncpy(p, file, lfile);
    strncpy(s, signal, lsignal);
    strncpy(f, format, lformat);

    p[lfile] = '\0';
    s[lsignal] = '\0';
    f[lformat] = '\0';

    p = TrimString(p);
    s = TrimString(s);
    f = TrimString(f);

    *handle = idamClientFileAPI(p, s, f);

    free( p);
    free( s);
    free( f);

    return;
}

// Call for IDA data

extern void idamida_(char* file, char* signal, int* pulno, int* pass, int* handle, int lfile, int lsignal)
{
    int ps = *pass;
    int pno = *pulno;

    char* f = (char*) malloc((size_t) (lfile + 1));
    char* s = (char*) malloc((size_t) (lsignal + 1));

    FILE* ftnout;

    *handle = -1;

    strncpy(f, file, lfile);
    strncpy(s, signal, lsignal);
    f[lfile] = '\0';
    s[lsignal] = '\0';

    f = TrimString(f);
    s = TrimString(s);

    if (idamGetLogLevel() == UDA_LOG_DEBUG) {
        errno = 0;
        ftnout = fopen("./ftn.log", "w");
        if (errno != 0) {
            if (ftnout != NULL) {
                fclose(ftnout);
            }
            free( f);
            free( s);
            return;
        }
        fprintf(ftnout, "Routine: idamIDA\n");
        fprintf(ftnout, "File             %s\n", f);
        fprintf(ftnout, "Signal        %s\n", s);
        fprintf(ftnout, "Pass             %d\n", ps);
        fprintf(ftnout, "Pulno         %d\n", pno);
        fprintf(ftnout, "Length File   %d (%d)\n", (int) strlen(f), lfile);
        fprintf(ftnout, "Length Signal %d (%d)\n", (int) strlen(s), lsignal);
        fclose(ftnout);
    }

    *handle = idamClientAPI(f, s, ps, pno);

    free( f);
    free( s);

    return;
}

// Call for MDS+ data

extern void idammds_(char* server, char* tree, char* node, int* treenum, int* handle,
                     int lserver, int ltree, int lnode)
{
    int tnum = *treenum;

    char* s = (char*) malloc((size_t) (lserver + 1));
    char* t = (char*) malloc((size_t) (ltree + 1));
    char* n = (char*) malloc((size_t) (lnode + 1));

    FILE* ftnout;

    *handle = -1;

    strncpy(s, server, lserver);
    strncpy(t, tree, ltree);
    strncpy(n, node, lnode);
    s[lserver] = '\0';
    t[ltree] = '\0';
    n[lnode] = '\0';

    s = TrimString(s);
    t = TrimString(t);
    n = TrimString(n);

    if (idamGetLogLevel() == UDA_LOG_DEBUG) {
        errno = 0;
        ftnout = fopen("./ftn.log", "w");

        if (errno != 0) {
            if (ftnout != NULL) {
                fflush(ftnout);
                fclose(ftnout);
            }
            free( s);
            free( t);
            free( n);
            return;
        }
        fprintf(ftnout, "Routine: idamMDS\n");
        fprintf(ftnout, "Server        %s\n", s);
        fprintf(ftnout, "Tree             %s\n", t);
        fprintf(ftnout, "Node             %s\n", n);
        fprintf(ftnout, "Tree Number   %d\n", tnum);
        fprintf(ftnout, "Length Server %d (%d)\n", (int) strlen(s), lserver);
        fprintf(ftnout, "Length Tree   %d (%d)\n", (int) strlen(t), ltree);
        fprintf(ftnout, "Length Node   %d (%d)\n", (int) strlen(n), lnode);
    }

    *handle = idamClientMDS(s, t, n, tnum);

    free( s);
    free( t);
    free( n);

    return;
}

// Call for LOCAL data (e.g., PPF or JPF Data Readers)

extern void idamlocalapi_(char* archive, char* owner, char* file, char* format, char* signal, int* pulno, int* pass,
                          int* handle,
                          int larchive, int lowner, int lfile, int lformat, int lsignal)
{
    int l, lo;

    char* a = (char*) malloc((size_t) (larchive + 1));
    char* o = (char*) malloc((size_t) (lowner + 1));
    char* p = (char*) malloc((size_t) (lfile + 1));
    char* f = (char*) malloc((size_t) (lformat + 1));
    char* s = (char*) malloc((size_t) (lsignal + 1));

    char* api_signal = (char*) malloc((size_t) (larchive + lsignal + 3));
    char* api_source = (char*) malloc((size_t) (lformat + lfile + lowner + 7 + 56));

    *handle = -1;

    strncpy(a, archive, larchive);
    strncpy(p, file, lfile);
    strncpy(s, signal, lsignal);
    strncpy(f, format, lformat);
    strncpy(o, owner, lowner);

    a[larchive] = '\0';
    p[lfile] = '\0';
    s[lsignal] = '\0';
    f[lformat] = '\0';
    o[lowner] = '\0';

    a = TrimString(a);
    p = TrimString(p);
    s = TrimString(s);
    f = TrimString(f);
    o = TrimString(o);

    if ((l = (int) strlen(a)) > 0) {
        sprintf(api_signal, "%s::%s", a, s);
    } else {
        strcpy(api_signal, s);
    }

    lo = (int) strlen(o);
    if ((l = (int) strlen(f)) > 0) {
        if (lo > 0) {
            sprintf(api_source, "%s::/%s/%d/%d/%s", f, p, *pulno, *pass, o);
        } else {
            sprintf(api_source, "%s::/%s/%d/%d", f, p, *pulno, *pass);
        }
    } else {
        if (lo > 0) {
            sprintf(api_source, "%s::/%s/%d/%d/%s", a, p, *pulno, *pass, o);
        } else {
            sprintf(api_source, "%s::/%s/%d/%d ", a, p, *pulno, *pass);
        }
    }

    *handle = idamGetAPI(api_signal, api_source);

    free(a);
    free(p);
    free(s);
    free(f);
    free(o);

    free(api_signal);
    free(api_source);

    return;
}

//---------------------------------------------------------------------------------------------------------------------------------
// Fortran Accessors

// Properties

extern void setidamproperty_(char* property, int lproperty)
{
    char* s = (char*) malloc((size_t) (lproperty + 1));
    strncpy(s, property, lproperty);
    s[lproperty] = '\0';
    s = TrimString(s);
    setIdamProperty(s);
    free( s);
}

extern void getidamproperty_(const char* property, int* value, int lproperty)
{
    char* s = (char*) malloc((size_t) (lproperty + 1));
    strncpy(s, property, lproperty);
    s[lproperty] = '\0';
    s = TrimString(s);
    *value = getIdamProperty(s);
}

extern void resetidamproperty_(char* property, int lproperty)
{
    char* s = (char*) malloc((size_t) (lproperty + 1));
    strncpy(s, property, lproperty);
    s[lproperty] = '\0';
    s = TrimString(s);
    resetIdamProperty(s);
    free( s);
}

extern void resetidamproperties_()
{
    resetIdamProperties();
    return;
}

// Error Model

extern void putidamerrormodel_(int* handle, int* model, int* param_n, float* params)
{
    putIdamErrorModel(*handle, *model, *param_n, params);
}

extern void putidamdimerrormodel_(int* handle, int* ndim, int* model, int* param_n, float* params)
{
    putIdamDimErrorModel(*handle, *ndim, *model, *param_n, params);
}

// IDAM Server Host controls

extern void putidamserver_(char* h, int* port, int lh)
{
    char* host = (char*) malloc((size_t) (lh + 1));
    strncpy(host, h, lh);
    host[lh] = '\0';
    TrimString(host);
    putIdamServer(host, *port);
    free( host);
}

extern void putidamserverhost_(char* h, int lh)
{
    char* host = (char*) malloc((size_t) (lh + 1));
    strncpy(host, h, lh);
    host[lh] = '\0';
    TrimString(host);
    putIdamServerHost(host);
    free( host);
}

extern void putidamserverport_(int* port)
{
    putIdamServerPort(*port);
}

extern void putidamserversocket_(int* socket)
{
    putIdamServerSocket(*socket);
}

extern void getidamserver_(char* h, int* p, int* s, int lh)
{
    int port, socket, lhost;
    char* host = NULL;
    getIdamServer(&host, &port, &socket);

    lhost = (int) strlen(host);
    if (lhost <= lh)
        strncpy(h, host, lhost);
    else
        strncpy(h, host, lh);
    *p = port;
    *s = socket;
}

extern void getidamserverhost_(char* h, int lh)
{
    char* host = getIdamServerHost();
    int lhost = (int) strlen(host);
    if (lhost <= lh)
        strncpy(h, host, lhost);
    else
        strncpy(h, host, lh);
}

extern void getidamserverport_(int* port)
{
    *port = getIdamServerPort();
}

extern void getidamserversocket_(int* socket)
{
    *socket = getIdamServerSocket();
}

// IDAM versions

extern void getidamclientversion_(int* version)
{
    *version = getIdamClientVersion();
}

extern void getidamserverversion_(int* version)
{
    *version = getIdamServerVersion();
}

extern void getidamservererrorcode_(int* errcode)
{
    *errcode = getIdamServerErrorCode();
}

extern void getidamservererrormsg_(char* s, int ls)
{
    char* msg = getIdamServerErrorMsg();
    int lmsg = (int) strlen(msg);
    if (lmsg > ls)
        strncpy(s, msg, ls);
    else
        strncpy(s, msg, lmsg);
}

extern void getidamservererrorstacksize_(int* size)
{
    *size = getIdamServerErrorStackSize();
}

extern void getidamservererrorstackrecordtype_(int* record, int* type)
{
    *type = getIdamServerErrorStackRecordType(*record);
}

extern void getidamservererrorstackrecordcode_(int* record, int* code)
{
    *code = getIdamServerErrorStackRecordCode(*record);
}

extern void getidamservererrorstackrecordlocation_(int* record, char* s, int ls)
{
    char* location = getIdamServerErrorStackRecordLocation(*record);
    int lloc = (int) strlen(location);
    if (lloc > ls)
        strncpy(s, location, ls);
    else
        strncpy(s, location, lloc);
}

extern void getidamservererrorstackrecordmsg_(int* record, char* s, int ls)
{
    char* msg = getIdamServerErrorStackRecordMsg(*record);
    int lmsg = (int) strlen(msg);
    if (lmsg > ls)
        strncpy(s, msg, ls);
    else
        strncpy(s, msg, lmsg);
}


extern void getidamerrorcode_(int* handle, int* errcode)
{
    *errcode = getIdamErrorCode(*handle);
}

extern void getidamerrormsg_(int* handle, char* s, int ls)
{        //Error Message
    char* msg = getIdamErrorMsg(*handle);
    int lmsg = (int) strlen(msg);
    if (lmsg > ls)
        strncpy(s, msg, ls);
    else
        strncpy(s, msg, lmsg);
}

extern void getidamsourcestatus_(int* handle, int* status)
{
    *status = getIdamSourceStatus(*handle);
}

extern void getidamsignalstatus_(int* handle, int* status)
{
    *status = getIdamSignalStatus(*handle);
}

extern void getidamdatastatus_(int* handle, int* status)
{
    *status = getIdamDataStatus(*handle);
}

extern void getidamlasthandle_(int* handle)
{
    *handle = getIdamLastHandle();
}

extern void getidamdatanum_(int* hd, int* datanum)
{                // Number of Data Items
    int handle;
    handle = *hd;
    *datanum = getIdamDataNum(handle);
    return;
}

extern void getidamrank_(int* hd, int* rank)
{                // Data Array Rank
    int handle;
    handle = *hd;
    *rank = getIdamRank(handle);
}

extern void getidamorder_(int* hd, int* order)
{                // Location of the Time Dimension
    int handle;
    handle = *hd;
    *order = getIdamOrder(handle);
}

extern void getidamdatatype_(int* hd, int* data_type)
{            // Type of Data Returned
    int handle;
    handle = *hd;
    *data_type = getIdamDataType(handle);
}

extern void getidamerrortype_(int* hd, int* error_type)
{            // Type of Data Error Returned
    int handle;
    handle = *hd;
    *error_type = getIdamErrorType(handle);
}

extern void getidamdatatypeid_(char* t, int* id, int lt)
{
    char* type = (char*) malloc((size_t) (lt + 1));
    strncpy(type, t, lt);
    type[lt] = '\0';
    TrimString(type);
    *id = getIdamDataTypeId(type);
    free( type);
}

extern void getidamerrormodel_(int* handle, int* model, int* param_n, float* params)
{
    getIdamErrorModel(*handle, model, param_n, params);
}

extern void getidamerrorasymmetry_(int* handle, int* asymmetry)
{
    *asymmetry = getIdamErrorAsymmetry(*handle);
}

extern void getidamerrormodelid_(char* m, int* id, int lm)
{
    char* model = (char*) malloc((size_t) (lm + 1));
    strncpy(model, m, lm);
    model[lm] = '\0';
    TrimString(model);
    *id = getIdamErrorModelId(model);
    free( model);
}

extern void getidamsyntheticdatablock_(int* handle, void* data)
{
    void* synth = (void*) getIdamSyntheticData(*handle);
    size_t ndata = (size_t) getIdamDataNum(*handle);
    switch (getIdamDataType(*handle)) {
        case UDA_TYPE_FLOAT:
            memcpy(data, synth, ndata * sizeof(float));
            break;
        case UDA_TYPE_DOUBLE:
            memcpy(data, synth, ndata * sizeof(double));
            break;
        case UDA_TYPE_INT:
            memcpy(data, synth, ndata * sizeof(int));
            break;
        case UDA_TYPE_UNSIGNED_INT:
            memcpy(data, synth, ndata * sizeof(unsigned int));
            break;
        case UDA_TYPE_LONG:
            memcpy(data, synth, ndata * sizeof(long));
            break;
        case UDA_TYPE_UNSIGNED_LONG:
            memcpy(data, synth, ndata * sizeof(unsigned long));
            break;
        case UDA_TYPE_LONG64:
            memcpy(data, synth, ndata * sizeof(long long int));
            break;
        case UDA_TYPE_UNSIGNED_LONG64:
            memcpy(data, synth, ndata * sizeof(unsigned long long int));
            break;
        case UDA_TYPE_SHORT:
            memcpy(data, synth, ndata * sizeof(short));
            break;
        case UDA_TYPE_UNSIGNED_SHORT:
            memcpy(data, synth, ndata * sizeof(unsigned short));
            break;
        case UDA_TYPE_CHAR:
            memcpy(data, synth, ndata * sizeof(char));
            break;
        case UDA_TYPE_UNSIGNED_CHAR:
            memcpy(data, synth, ndata * sizeof(unsigned char));
            break;
        case UDA_TYPE_DCOMPLEX:
            memcpy(data, synth, ndata * sizeof(DCOMPLEX));
            break;
        case UDA_TYPE_COMPLEX:
            memcpy(data, synth, ndata * sizeof(COMPLEX));
            break;
    }
}

extern void getidamdoubledatablock_(int* handle, double* data)
{            // Return the Data Array cast to type double
    getIdamDoubleData(*handle, data);
}

extern void getidamfloatdatablock_(int* handle, float* data)
{            // Return the Data Array cast to type float
    getIdamFloatData(*handle, data);
}

extern void getidamdatablock_(int* hd, void* data)
{                    // Return the Data Array
    getIdamGenericData(*hd, data);
}

extern void getidamerrorblock_(int* handle, void* errdata)
{                // Return the Data Error Array
    void* errb = (void*) getIdamError(*handle);
    size_t ndata = (size_t) getIdamDataNum(*handle);
    switch (getIdamErrorType(*handle)) {
        case UDA_TYPE_FLOAT:
            memcpy(errdata, errb, ndata * sizeof(float));
            break;
        case UDA_TYPE_DOUBLE:
            memcpy(errdata, errb, ndata * sizeof(double));
            break;
        case UDA_TYPE_INT:
            memcpy(errdata, errb, ndata * sizeof(int));
            break;
        case UDA_TYPE_LONG:
            memcpy(errdata, errb, ndata * sizeof(long));
            break;
        case UDA_TYPE_LONG64:
            memcpy(errdata, errb, ndata * sizeof(long long int));
            break;
        case UDA_TYPE_SHORT:
            memcpy(errdata, errb, ndata * sizeof(short));
            break;
        case UDA_TYPE_CHAR:
            memcpy(errdata, errb, ndata * sizeof(char));
            break;
        case UDA_TYPE_UNSIGNED_INT:
            memcpy(errdata, errb, ndata * sizeof(unsigned int));
            break;
        case UDA_TYPE_UNSIGNED_LONG:
            memcpy(errdata, errb, ndata * sizeof(unsigned long));
            break;
        case UDA_TYPE_UNSIGNED_LONG64:
            memcpy(errdata, errb, ndata * sizeof(unsigned long long int));
            break;
        case UDA_TYPE_UNSIGNED_SHORT:
            memcpy(errdata, errb, ndata * sizeof(unsigned short));
            break;
        case UDA_TYPE_UNSIGNED_CHAR:
            memcpy(errdata, errb, ndata * sizeof(unsigned char));
            break;
        case UDA_TYPE_DCOMPLEX:
            memcpy(errdata, errb, ndata * sizeof(DCOMPLEX));
            break;
        case UDA_TYPE_COMPLEX:
            memcpy(errdata, errb, ndata * sizeof(COMPLEX));
            break;
    }
    return;
}

extern void getidamfloaterrorblock_(int* handle, float* data)
{            // Return the Data Array cast to type Float
    getIdamFloatError(*handle, data);
}

extern void getidamasymmetricerrorblock_(int* handle, int* above, void* errdata)
{        // Return the Asymmetric Error Array Component
    void* errb = (void*) getIdamAsymmetricError(*handle, *above);
    size_t ndata = (size_t) getIdamDataNum(*handle);
    switch ((int) getIdamErrorType(*handle)) {
        case UDA_TYPE_FLOAT:
            memcpy(errdata, errb, ndata * sizeof(float));
            break;
        case UDA_TYPE_DOUBLE:
            memcpy(errdata, errb, ndata * sizeof(double));
            break;
        case UDA_TYPE_INT:
            memcpy(errdata, errb, ndata * sizeof(int));
            break;
        case UDA_TYPE_LONG:
            memcpy(errdata, errb, ndata * sizeof(long));
            break;
        case UDA_TYPE_LONG64:
            memcpy(errdata, errb, ndata * sizeof(long long int));
            break;
        case UDA_TYPE_SHORT:
            memcpy(errdata, errb, ndata * sizeof(short));
            break;
        case UDA_TYPE_CHAR:
            memcpy(errdata, errb, ndata * sizeof(char));
            break;
        case UDA_TYPE_UNSIGNED_INT:
            memcpy(errdata, errb, ndata * sizeof(unsigned int));
            break;
        case UDA_TYPE_UNSIGNED_LONG:
            memcpy(errdata, errb, ndata * sizeof(unsigned long));
            break;
        case UDA_TYPE_UNSIGNED_LONG64:
            memcpy(errdata, errb, ndata * sizeof(unsigned long long int));
            break;
        case UDA_TYPE_UNSIGNED_SHORT:
            memcpy(errdata, errb, ndata * sizeof(unsigned short));
            break;
        case UDA_TYPE_UNSIGNED_CHAR:
            memcpy(errdata, errb, ndata * sizeof(unsigned char));
            break;
        case UDA_TYPE_DCOMPLEX:
            memcpy(errdata, errb, ndata * sizeof(DCOMPLEX));
            break;
        case UDA_TYPE_COMPLEX:
            memcpy(errdata, errb, ndata * sizeof(COMPLEX));
            break;
    }
    return;
}

extern void getidamfloatasymmetricerrorblock_(int* handle, int* above, float* data)
{    // Return the Asymmetric Error Array cast to type Float
    getIdamFloatAsymmetricError(*handle, *above, data);
}

extern void getidamdatalabellength_(int* handle, int* lngth)
{            // Length of Data Units String
    *lngth = (int) strlen(getIdamDataLabel(*handle));
}

extern void getidamdatalabel_(int* handle, char* s, int ls)
{            // The Data Label
    char* msg = getIdamDataLabel(*handle);
    int lmsg = (int) strlen(msg);
    if (lmsg > ls)
        strncpy(s, msg, ls);
    else
        strncpy(s, msg, lmsg);
}

extern void getidamdataunitslength_(int* handle, int* lngth)
{            // Length of Data Units String
    *lngth = (int) strlen(getIdamDataUnits(*handle));
}

extern void getidamdataunits_(int* handle, char* s, int ls)
{            // Data Units
    char* msg = getIdamDataUnits(*handle);
    int lmsg = (int) strlen(msg);
    if (lmsg > ls)
        strncpy(s, msg, ls);
    else
        strncpy(s, msg, lmsg);
}

extern void getidamdatadesclength_(int* handle, int* lngth)
{            // Length of Data Description String
    *lngth = (int) strlen(getIdamDataDesc(*handle));
}

extern void getidamdatadesc_(int* handle, char* s, int ls)
{            // Data Description
    char* msg = getIdamDataDesc(*handle);
    int lmsg = (int) strlen(msg);
    if (lmsg > ls)
        strncpy(s, msg, ls);
    else
        strncpy(s, msg, lmsg);
}

extern void getidamdimnum_(int* hd, int* nd, int* num)
{            // Length of the Dimension nd
    int handle, ndim;
    handle = *hd;
    ndim = *nd;
    *num = getIdamDimNum(handle, ndim);
}

extern void getidamdimtype_(int* hd, int* nd, int* type)
{            // Dimension nd Data Type
    int handle, ndim;
    handle = *hd;
    ndim = *nd;
    *type = getIdamDimType(handle, ndim);
}

extern void getidamdimerrortype_(int* handle, int* ndim, int* type)
{    // Dimension Error Data Type
    *type = getIdamDimErrorType(*handle, *ndim);
}

extern void getidamdimerrormodel_(int* handle, int* ndim, int* model, int* param_n, float* params)
{
    getIdamDimErrorModel(*handle, *ndim, model, param_n, params);
}

extern void getidamdimerrorasymmetry_(int* handle, int* ndim, int* asymmetry)
{
    *asymmetry = (int) getIdamDimErrorAsymmetry((int) *handle, (int) *ndim);
}

//==============================================================

extern void getidamsyntheticdimdatablock_(int* handle, int* ndim, void* data)
{
    void* synth = (void*) getIdamSyntheticDimData(*handle, *ndim);
    size_t ndata = (size_t) getIdamDimNum(*handle, *ndim);
    switch (getIdamDimType(*handle, *ndim)) {
        case UDA_TYPE_FLOAT:
            memcpy(data, synth, ndata * sizeof(float));
            break;
        case UDA_TYPE_DOUBLE:
            memcpy(data, synth, ndata * sizeof(double));
            break;
        case UDA_TYPE_INT:
            memcpy(data, synth, ndata * sizeof(int));
            break;
        case UDA_TYPE_LONG:
            memcpy(data, synth, ndata * sizeof(long));
            break;
        case UDA_TYPE_LONG64:
            memcpy(data, synth, ndata * sizeof(long long int));
            break;
        case UDA_TYPE_SHORT:
            memcpy(data, synth, ndata * sizeof(short));
            break;
        case UDA_TYPE_CHAR:
            memcpy(data, synth, ndata * sizeof(char));
            break;
        case UDA_TYPE_UNSIGNED_INT:
            memcpy(data, synth, ndata * sizeof(unsigned int));
            break;
        case UDA_TYPE_UNSIGNED_LONG:
            memcpy(data, synth, ndata * sizeof(unsigned long));
            break;
        case UDA_TYPE_UNSIGNED_LONG64:
            memcpy(data, synth, ndata * sizeof(unsigned long long int));
            break;
        case UDA_TYPE_UNSIGNED_SHORT:
            memcpy(data, synth, ndata * sizeof(unsigned short));
            break;
        case UDA_TYPE_UNSIGNED_CHAR:
            memcpy(data, synth, ndata * sizeof(unsigned char));
            break;
        case UDA_TYPE_DCOMPLEX:
            memcpy(data, synth, ndata * sizeof(DCOMPLEX));
            break;
        case UDA_TYPE_COMPLEX:
            memcpy(data, synth, ndata * sizeof(COMPLEX));
            break;
    }
}

extern void getidamdoubledimdata_(int* handle, int* ndim, double* data)
{        // Dimension nd Data Array cast to double
    getIdamDoubleDimData(*handle, *ndim, data);
}

extern void getidamfloatdimdata_(int* handle, int* ndim, float* data)
{        // Dimension nd Data Array cast to Float
    getIdamFloatDimData(*handle, *ndim, data);
}

extern void getidamdimdata_(int* hd, int* nd, void* data)
{                // Dimension nd Data Array
    getIdamGenericDimData(*hd, *nd, data);
}

extern void getidamdimdatablock_(int* hd, int* nd, void* data)
{            // Dimension nd Data Array
    getidamdimdata_(hd, nd, data);
}

extern void getidamdimasymmetricerrorblock_(int* handle, int* ndim, int* above, void* data)
{
    void* errb = (void*) getIdamDimAsymmetricError(*handle, *ndim, *above);
    size_t ndata = (size_t) getIdamDimNum(*handle, *ndim);
    switch (getIdamDimErrorType(*handle, *ndim)) {
        case UDA_TYPE_FLOAT:
            memcpy(data, errb, ndata * sizeof(float));
            break;
        case UDA_TYPE_DOUBLE:
            memcpy(data, errb, ndata * sizeof(double));
            break;
        case UDA_TYPE_INT:
            memcpy(data, errb, ndata * sizeof(int));
            break;
        case UDA_TYPE_LONG:
            memcpy(data, errb, ndata * sizeof(long));
            break;
        case UDA_TYPE_LONG64:
            memcpy(data, errb, ndata * sizeof(long long int));
            break;
        case UDA_TYPE_SHORT:
            memcpy(data, errb, ndata * sizeof(short));
            break;
        case UDA_TYPE_CHAR:
            memcpy(data, errb, ndata * sizeof(char));
            break;
        case UDA_TYPE_UNSIGNED_INT:
            memcpy(data, errb, ndata * sizeof(unsigned int));
            break;
        case UDA_TYPE_UNSIGNED_LONG:
            memcpy(data, errb, ndata * sizeof(unsigned long));
            break;
        case UDA_TYPE_UNSIGNED_LONG64:
            memcpy(data, errb, ndata * sizeof(unsigned long long int));
            break;
        case UDA_TYPE_UNSIGNED_SHORT:
            memcpy(data, errb, ndata * sizeof(unsigned short));
            break;
        case UDA_TYPE_UNSIGNED_CHAR:
            memcpy(data, errb, ndata * sizeof(unsigned char));
            break;
        case UDA_TYPE_DCOMPLEX:
            memcpy(data, errb, ndata * sizeof(DCOMPLEX));
            break;
        case UDA_TYPE_COMPLEX:
            memcpy(data, errb, ndata * sizeof(COMPLEX));
            break;
    }
}

extern void getidamdimerrorblock_(int* handle, int* ndim, void* data)
{
    void* errb = (void*) getIdamDimError(*handle, *ndim);
    size_t ndata = (size_t) getIdamDimNum(*handle, *ndim);
    switch (getIdamDimErrorType(*handle, *ndim)) {
        case UDA_TYPE_FLOAT:
            memcpy(data, errb, ndata * sizeof(float));
            break;
        case UDA_TYPE_DOUBLE:
            memcpy(data, errb, ndata * sizeof(double));
            break;
        case UDA_TYPE_INT:
            memcpy(data, errb, ndata * sizeof(int));
            break;
        case UDA_TYPE_LONG:
            memcpy(data, errb, ndata * sizeof(long));
            break;
        case UDA_TYPE_LONG64:
            memcpy(data, errb, ndata * sizeof(long long int));
            break;
        case UDA_TYPE_SHORT:
            memcpy(data, errb, ndata * sizeof(short));
            break;
        case UDA_TYPE_CHAR:
            memcpy(data, errb, ndata * sizeof(char));
            break;
        case UDA_TYPE_UNSIGNED_INT:
            memcpy(data, errb, ndata * sizeof(unsigned int));
            break;
        case UDA_TYPE_UNSIGNED_LONG:
            memcpy(data, errb, ndata * sizeof(unsigned long));
            break;
        case UDA_TYPE_UNSIGNED_LONG64:
            memcpy(data, errb, ndata * sizeof(unsigned long long int));
            break;
        case UDA_TYPE_UNSIGNED_SHORT:
            memcpy(data, errb, ndata * sizeof(unsigned short));
            break;
        case UDA_TYPE_UNSIGNED_CHAR:
            memcpy(data, errb, ndata * sizeof(unsigned char));
            break;
        case UDA_TYPE_DCOMPLEX:
            memcpy(data, errb, ndata * sizeof(DCOMPLEX));
            break;
        case UDA_TYPE_COMPLEX:
            memcpy(data, errb, ndata * sizeof(COMPLEX));
            break;
    }
}

extern void getidamfloatdimasymmetricerrorblock_(int* handle, int* ndim, int* above, float* data)
{
    getIdamFloatDimAsymmetricError(*handle, *ndim, *above, data);
}

extern void getidamfloatdimerrorblock_(int* handle, int* ndim, float* data)
{
    getIdamFloatDimError(*handle, *ndim, data);
}

//=============================================================

extern void getidamdimlabellength_(int* hd, int* nd, int* lngth)
{    // Length of Dimension nd Label String
    *lngth = (int) strlen(getIdamDimLabel(*hd, *nd));
}

extern void getidamdimlabel_(int* hd, int* nd, char* s, int ls)
{    // Dimension nd Label
    char* msg = getIdamDimLabel(*hd, *nd);
    int lmsg = (int) strlen(msg);
    if (lmsg > ls)
        strncpy(s, msg, ls);
    else
        strncpy(s, msg, lmsg);
}

extern void getidamdimunitslength_(int* hd, int* nd, int* lngth)
{    // Length of Dimension nd Units String
    *lngth = (int) strlen(getIdamDimUnits(*hd, *nd));
}

extern void getidamdimunits_(int* hd, int* nd, char* s, int ls)
{    // Dimension nd Units
    char* msg = getIdamDimUnits(*hd, *nd);
    int lmsg = (int) strlen(msg);
    if (lmsg > ls)
        strncpy(s, msg, ls);
    else
        strncpy(s, msg, lmsg);
}

extern void getidamfileformat_(int* hd, char* s, int ls)
{    // Data Source File Format
    char* format = getIdamFileFormat(*hd);
    if (format == NULL) return;
    int lstr = (int) strlen(format);
    if (lstr > ls)
        strncpy(s, format, ls);
    else
        strncpy(s, format, lstr);
}

extern void getidamdatachecksum_(int* handle, int* sum)
{
    *sum = getIdamDataCheckSum(*handle);
}

extern void getidamdimdatachecksum_(int* handle, int* ndim, int* sum)
{
    *sum = getIdamDimDataCheckSum(*handle, *ndim);
}


extern void idamfree_(int* hd)
{
    int handle = *hd;
    udaFree(handle);
}

extern void idamfreeall_()
{
    udaFreeAll();
}

extern void getidamenv_(char* str, int* rc, char* env, int lstr, int lenv)
{

    char* s = (char*) malloc((size_t) (lstr + 1));
    char* e = NULL;

    strncpy(s, str, lstr);
    s[lstr] = '\0';
    s = TrimString(s);

    e = getenv(s);
    *rc = 0;

    if (e == NULL) {
        *rc = 1;
        free( s);
        return;
    } else {
        strncpy(env, e, lenv - 1);
        env[lenv - 1] = '\0';
    }
    free( s);
}

extern void whereidamami_(void* var, char* loc, int lloc)
{

    char s[128];

#ifdef A64
    sprintf(s, "%llx", (unsigned long long) var);
#else
    sprintf(s,"%x",(unsigned int)var);
#endif
    strncpy(loc, s, (int) strlen(s));
}

