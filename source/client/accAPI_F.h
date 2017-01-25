#ifndef IDAM_CLIENT_ACCAPI_F_H
#define IDAM_CLIENT_ACCAPI_F_H

void idamgetapi_(char* data_object, char* data_source, int* handle, int ldata_object, int ldata_source);

void idamapi_(char* signal, int* pulno, int* handle, int lsignal);

void idampassapi_(char* signal, int* pulno, int* pass, int* handle, int lsignal);

void idamgenapi_(char* archive, char* device, char* signal, int* pulno, int* pass,
                 int* handle, int larchive, int ldevice, int lsignal);

void idamfileapi_(char* file, char* signal, char* format, int* handle,
                  int lfile, int lsignal, int lformat);

void idamida_(char* file, char* signal, int* pulno, int* pass, int* handle, int lfile, int lsignal);

void idammds_(char* server, char* tree, char* node, int* treenum, int* handle,
              int lserver, int ltree, int lnode);

void idamlocalapi_(char* archive, char* owner, char* file, char* format, char* signal, int* pulno, int* pass,
                   int* handle,
                   int larchive, int lowner, int lfile, int lformat, int lsignal);

void setidamproperty_(char* property, int lproperty);

void getidamproperty_(const char* property, int* value, int lproperty);

void resetidamproperty_(char* property, int lproperty);

void resetidamproperties_();

void putidamerrormodel_(int* handle, int* model, int* param_n, float* params);

void putidamdimerrormodel_(int* handle, int* ndim, int* model, int* param_n, float* params);

void putidamserver_(char* h, int* port, int lh);

void putidamserverhost_(char* h, int lh);

void putidamserverport_(int* port);

void putidamserversocket_(int* socket);

void getidamserver_(char* h, int* p, int* s, int lh);

void getidamserverhost_(char* h, int lh);

void getidamserverport_(int* port);

void getidamserversocket_(int* socket);

void getidamclientversion_(int* version);

void getidamserverversion_(int* version);

void getidamservererrorcode_(int* errcode);

void getidamservererrormsg_(char* s, int ls);

void getidamservererrorstacksize_(int* size);

void getidamservererrorstackrecordtype_(int* record, int* type);

void getidamservererrorstackrecordcode_(int* record, int* code);

void getidamservererrorstackrecordlocation_(int* record, char* s, int ls);

void getidamservererrorstackrecordmsg_(int* record, char* s, int ls);

void getidamerrorcode_(int* handle, int* errcode);

void getidamerrormsg_(int* handle, char* s, int ls);

void getidamsourcestatus_(int* handle, int* status);

void getidamsignalstatus_(int* handle, int* status);

void getidamdatastatus_(int* handle, int* status);

void getidamlasthandle_(int* handle);

void getidamdatanum_(int* hd, int* datanum);

void getidamrank_(int* hd, int* rank);

void getidamorder_(int* hd, int* order);

void getidamdatatype_(int* hd, int* data_type);

void getidamerrortype_(int* hd, int* error_type);

void getidamdatatypeid_(char* t, int* id, int lt);

void getidamerrormodel_(int* handle, int* model, int* param_n, float* params);

void getidamerrorasymmetry_(int* handle, int* asymmetry);

void getidamerrormodelid_(char* m, int* id, int lm);

void getidamsyntheticdatablock_(int* handle, void* data);

void getidamdoubledatablock_(int* handle, double* data);

void getidamfloatdatablock_(int* handle, float* data);

void getidamdatablock_(int* hd, void* data);

void getidamerrorblock_(int* handle, void* errdata);

void getidamfloaterrorblock_(int* handle, float* data);

void getidamasymmetricerrorblock_(int* handle, int* above, void* errdata);

void getidamfloatasymmetricerrorblock_(int* handle, int* above, float* data);

void getidamdatalabellength_(int* handle, int* lngth);

void getidamdatalabel_(int* handle, char* s, int ls);

void getidamdataunitslength_(int* handle, int* lngth);

void getidamdataunits_(int* handle, char* s, int ls);

void getidamdatadesclength_(int* handle, int* lngth);

void getidamdatadesc_(int* handle, char* s, int ls);

void getidamdimnum_(int* hd, int* nd, int* num);

void getidamdimtype_(int* hd, int* nd, int* type);

void getidamdimerrortype_(int* handle, int* ndim, int* type);

void getidamdimerrormodel_(int* handle, int* ndim, int* model, int* param_n, float* params);

void getidamdimerrorasymmetry_(int* handle, int* ndim, int* asymmetry);

void getidamsyntheticdimdatablock_(int* handle, int* ndim, void* data);

void getidamdoubledimdata_(int* handle, int* ndim, double* data);

void getidamfloatdimdata_(int* handle, int* ndim, float* data);

void getidamdimdata_(int* hd, int* nd, void* data);

void getidamdimdatablock_(int* hd, int* nd, void* data);

void getidamdimasymmetricerrorblock_(int* handle, int* ndim, int* above, void* data);

void getidamdimerrorblock_(int* handle, int* ndim, void* data);

void getidamfloatdimasymmetricerrorblock_(int* handle, int* ndim, int* above, float* data);

void getidamfloatdimerrorblock_(int* handle, int* ndim, float* data);

void getidamdimlabellength_(int* hd, int* nd, int* lngth);

void getidamdimlabel_(int* hd, int* nd, char* s, int ls);

void getidamdimunitslength_(int* hd, int* nd, int* lngth);

void getidamdimunits_(int* hd, int* nd, char* s, int ls);

void getidamfileformat_(int* hd, char* s, int ls);

void getidamdatachecksum_(int* handle, int* sum);

void getidamdimdatachecksum_(int* handle, int* ndim, int* sum);

void idamfree_(int* hd);

void idamfreeall_();

void getidamenv_(char* str, int* rc, char* env, int lstr, int lenv);

void whereidamami_(void* var, char* loc, int lloc);

#endif // IDAM_CLIENT_ACCAPI_F_H
