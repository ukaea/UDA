// idam.c
//
// IDL DLM C Code Wrapper for IDAM Universal Data Access Layer
//
// Change Control
//
// To DO....
//
// Check for IDA API - lower case 'ida' only: Not Case Insensitive
// Status: No Accesssor Function; Pass Back?
//
//-------------------------------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "idl_export.h"                 // IDL API Header
#include "idamclientserverpublic.h"     // IDAM Header
#include "idamclientserverprivate.h"
#include "idamclientpublic.h"           // IDAM Header
#include "idamgenstructpublic.h"        // IDAM Header: Generalised Hierarchical data Structure accessors
#include "idamdlm.h"                    // DLM Header
#include "initStructs.h"
#include "idamPutAPI.h"
#include "ClientAPI.h"
#include "ClientMDS.h"
#include "printStructs.h"

#define NDEBUG  0

// Function Prototypes

void userhelp(FILE * fh, char * name);

extern void getdata_exit_handler(void);
extern int  getdata_Startup(void);

extern IDL_VPTR IDL_CDECL idamgetapi(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL idamputapi(int argc, IDL_VPTR argv[], char * argk);

extern IDL_VPTR IDL_CDECL callidam(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL callidam2(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getidamdata(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getidamdimdata(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getdataarray(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL geterrorarray(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getdimdataarray(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL freeidam(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL freeidamall(int argc, IDL_VPTR argv[], char * argk);

extern IDL_VPTR IDL_CDECL geterrorcode(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL geterrormsg(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL printerrormsgstack(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getsourcestatus(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getsignalstatus(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getdatastatus(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getdatanum(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getrank(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getorder(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getdatatype(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL geterrortype(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL geterrorasymmetry(int argc, IDL_VPTR argv[], char * argk);

extern IDL_VPTR IDL_CDECL getdatadata(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getsyntheticdata(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getdataerror(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getfloatdataerror(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getfloatasymmetricdataerror(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getasymmetricdataerror(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getfloatdata(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getdatalabel(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getdataunits(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getdatadesc(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getdimnum(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getdimtype(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getdimerrortype(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getdimerrorasymmetry(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getdimdata(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getsyntheticdimdata(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getdimerror(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getasymmetricdimerror(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getfloatdimdata(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getfloatdimerror(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getfloatasymmetricdimerror(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getdimlabel(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getdimunits(int argc, IDL_VPTR argv[], char * argk);

extern IDL_VPTR IDL_CDECL getdatasystemmeta(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getsystemconfigmeta(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getdatasourcemeta(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getsignalmeta(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getsignaldescmeta(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getxmldoc(int argc, IDL_VPTR argv[], char * argk);

extern IDL_VPTR IDL_CDECL setproperty(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL resetproperty(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL resetproperties(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getdatatypeid(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL geterrormodelid(int argc, IDL_VPTR argv[], char * argk);

extern IDL_VPTR IDL_CDECL setidamclientflag(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL resetidamclientflag(int argc, IDL_VPTR argv[], char * argk);

extern IDL_VPTR IDL_CDECL putidamserverhost(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL putidamserverport(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getidamserverhost(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getidamserverport(int argc, IDL_VPTR argv[], char * argk);

extern IDL_VPTR IDL_CDECL getidamclientversion(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getidamserverversion(int argc, IDL_VPTR argv[], char * argk);

extern IDL_VPTR IDL_CDECL getidamserversocket(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL putidamserversocket(int argc, IDL_VPTR argv[], char * argk);

extern IDL_VPTR IDL_CDECL puterrormodel(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL geterrormodel(int argc, IDL_VPTR argv[], char * argk);

extern IDL_VPTR IDL_CDECL putdimerrormodel(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getdimerrormodel(int argc, IDL_VPTR argv[], char * argk);

extern IDL_VPTR IDL_CDECL getlasthandle(int argc, IDL_VPTR argv[], char * argk);

extern IDL_VPTR IDL_CDECL getdomains(int argc, IDL_VPTR argv[], char * argk);

extern IDL_VPTR IDL_CDECL setidamdatatree(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL findidamtreestructurecomponent(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL findidamtreestructuredefinition(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL findidamtreestructure(int argc, IDL_VPTR argv[], char * argk);

extern IDL_VPTR IDL_CDECL getidamnodeatomiccount(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getidamnodeatomicrank(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getidamnodeatomicshape(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getidamnodeatomicnames(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getidamnodeatomictypes(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getidamnodeatomicpointers(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getidamnodeatomicdatacount(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getidamnodeatomicdata(int argc, IDL_VPTR argv[], char * argk);

extern IDL_VPTR IDL_CDECL getidamnodestructurecount(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getidamnodestructurerank(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getidamnodestructureshape(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getidamnodestructurenames(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getidamnodestructuretypes(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getidamnodestructurepointers(int argc, IDL_VPTR argv[], char * argk);

extern IDL_VPTR IDL_CDECL getidamnodestructuredatacount(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getidamnodestructuredatarank(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getidamnodestructuredatashape(int argc, IDL_VPTR argv[], char * argk);

extern IDL_VPTR IDL_CDECL getidamnodeparent(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getidamnodechild(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getidamnodechildrencount(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL getidamnodechildid(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL printidamtree(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL printidamtreestructurenames(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL printidamtreestructurecomponentnames(int argc, IDL_VPTR argv[], char * argk);
extern IDL_VPTR IDL_CDECL printidamnodestructure(int argc, IDL_VPTR argv[], char * argk);

extern IDL_VPTR IDL_CDECL regulariseidamvlenstructures(int argc, IDL_VPTR argv[], char * argk);

static IDL_SYSFUN_DEF2 getdata_functions[] = {
    {(IDL_FUN_RET) idamgetapi,            "IDAMGETAPI",       2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) idamputapi,            "IDAMPUTAPI",       2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) callidam,              "CALLIDAM",         1, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) callidam2,             "CALLIDAM2",        2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getidamdata,           "GETIDAMDATA",      1, 1, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getidamdimdata,        "GETIDAMDIMDATA",   2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getdataarray,          "GETDATAARRAY",     1, 1, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) geterrorarray,         "GETERRORARRAY",    1, 1, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getdimdataarray,       "GETDIMDATAARRAY",  2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) freeidam,              "FREEIDAM",         1, 1, 0, 0},
    {(IDL_FUN_RET) freeidamall,           "FREEIDAMALL",      0, 0, 0, 0},
    {(IDL_FUN_RET) geterrorcode,          "GETERRORCODE",     1, 1, 0, 0},
    {(IDL_FUN_RET) geterrormsg,           "GETERRORMSG",      2, 2, 0, 0},
    {(IDL_FUN_RET) printerrormsgstack,        "PRINTERRORMSGSTACK",   1, 1, 0, 0},
    {(IDL_FUN_RET) getsourcestatus,       "GETSOURCESTATUS",  1, 1, 0, 0},
    {(IDL_FUN_RET) getsignalstatus,       "GETSIGNALSTATUS",  1, 1, 0, 0},
    {(IDL_FUN_RET) getdatastatus,         "GETDATASTATUS",    1, 1, 0, 0},
    {(IDL_FUN_RET) getdatanum,            "GETDATANUM",       1, 1, 0, 0},
    {(IDL_FUN_RET) getrank,               "GETRANK",          1, 1, 0, 0},
    {(IDL_FUN_RET) getorder,              "GETORDER",         1, 1, 0, 0},
    {(IDL_FUN_RET) getdatatype,           "GETDATATYPE",      1, 1, 0, 0},
    {(IDL_FUN_RET) geterrortype,          "GETERRORTYPE",     1, 1, 0, 0},
    {(IDL_FUN_RET) geterrorasymmetry,     "GETERRORASYMMETRY",    1, 1, 0, 0},
    {(IDL_FUN_RET) getdatadata,           "GETDATADATA",      1, 1, 0, 0},
    {(IDL_FUN_RET) getdataerror,          "GETDATAERROR",     1, 1, 0, 0},
    {(IDL_FUN_RET) getasymmetricdataerror,    "GETASYMMETRICDATAERROR", 2, 2, 0, 0},
    {(IDL_FUN_RET) getfloatdata,          "GETFLOATDATA",     1, 1, 0, 0},
    {(IDL_FUN_RET) getsyntheticdata,      "GETSYNTHETICDATA",     1, 1, 0, 0},
    {(IDL_FUN_RET) getdatalabel,          "GETDATALABEL",     2, 2, 0, 0},
    {(IDL_FUN_RET) getdataunits,          "GETDATAUNITS",     2, 2, 0, 0},
    {(IDL_FUN_RET) getdatadesc,           "GETDATADESC",      2, 2, 0, 0},
    {(IDL_FUN_RET) getdimnum,             "GETDIMNUM",        2, 2, 0, 0},
    {(IDL_FUN_RET) getdimtype,            "GETDIMTYPE",       2, 2, 0, 0},
    {(IDL_FUN_RET) getdimerrortype,       "GETDIMERRORTYPE",  2, 2, 0, 0},
    {(IDL_FUN_RET) getdimerrorasymmetry,      "GETDIMERRORASYMMETRY", 2, 2, 0, 0},
    {(IDL_FUN_RET) getdimerror,           "GETDIMERROR",      2, 2, 0, 0},
    {(IDL_FUN_RET) getasymmetricdimerror,     "GETASYMMETRICDIMERROR", 3, 3, 0, 0},
    {(IDL_FUN_RET) getdimdata,            "GETDIMDATA",       2, 2, 0, 0},
    {(IDL_FUN_RET) getfloatdimdata,       "GETFLOATDIMDATA",      2, 2, 0, 0},
    {(IDL_FUN_RET) getsyntheticdimdata,       "GETSYNTHETICDIMDATA",  2, 2, 0, 0},
    {(IDL_FUN_RET) getdimlabel,           "GETDIMLABEL",      3, 3, 0, 0},
    {(IDL_FUN_RET) getdimunits,           "GETDIMUNITS",      3, 3, 0, 0},
    {(IDL_FUN_RET) getdatasystemmeta,         "GETDATASYSTEMMETA",    1, 1, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getsystemconfigmeta,       "GETSYSTEMCONFIGMETA",  1, 1, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getdatasourcemeta,         "GETDATASOURCEMETA",    1, 1, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getsignalmeta,             "GETSIGNALMETA",        1, 1, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getsignaldescmeta,         "GETSIGNALDESCMETA",    1, 1, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getxmldoc,             "GETXMLDOC",        2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) setproperty,           "SETPROPERTY",      1, 1, 0, 0},
    {(IDL_FUN_RET) resetproperty,         "RESETPROPERTY",    1, 1, 0, 0},
    {(IDL_FUN_RET) resetproperties,       "RESETPROPERTIES",      0, 0, 0, 0},
    {(IDL_FUN_RET) setidamclientflag,         "SETIDAMCLIENTFLAG",    1, 1, 0, 0},
    {(IDL_FUN_RET) resetidamclientflag,       "RESETIDAMCLIENTFLAG",  1, 1, 0, 0},
    {(IDL_FUN_RET) getdatatypeid,         "GETDATATYPEID",    1, 1, 0, 0},
    {(IDL_FUN_RET) geterrormodelid,       "GETERRORMODELID",  1, 1, 0, 0},
    {(IDL_FUN_RET) putidamserverhost,         "PUTIDAMSERVERHOST",    1, 1, 0, 0},
    {(IDL_FUN_RET) putidamserverport,         "PUTIDAMSERVERPORT",    1, 1, 0, 0},
    {(IDL_FUN_RET) getidamserverhost,         "GETIDAMSERVERHOST",    1, 1, 0, 0},
    {(IDL_FUN_RET) getidamserverport,         "GETIDAMSERVERPORT",    0, 0, 0, 0},
    {(IDL_FUN_RET) getidamclientversion,      "GETIDAMCLIENTVERSION", 0, 0, 0, 0},
    {(IDL_FUN_RET) getidamserverversion,      "GETIDAMSERVERVERSION", 0, 0, 0, 0},
    {(IDL_FUN_RET) getidamserversocket,       "GETIDAMSERVERSOCKET",  0, 0, 0, 0},
    {(IDL_FUN_RET) putidamserversocket,       "PUTIDAMSERVERSOCKET",  1, 1, 0, 0},
    {(IDL_FUN_RET) geterrormodel,         "GETERRORMODEL",    1, 1, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) puterrormodel,         "PUTERRORMODEL",    3, 3, 0, 0},
    {(IDL_FUN_RET) getdimerrormodel,          "GETDIMERRORMODEL",     2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) putdimerrormodel,          "PUTDIMERRORMODEL",     4, 4, 0, 0},
    {(IDL_FUN_RET) getfloatdataerror,         "GETFLOATDATAERROR",    1, 1, 0, 0},
    {(IDL_FUN_RET) getfloatdimerror,          "GETFLOATDIMERROR",     2, 2, 0, 0},
    {(IDL_FUN_RET) getfloatasymmetricdataerror,   "GETFLOATASYMMETRICDATAERROR", 2, 2, 0, 0},
    {(IDL_FUN_RET) getfloatasymmetricdimerror,    "GETFLOATASYMMETRICDIMERROR",  3, 3, 0, 0},
    {(IDL_FUN_RET) getlasthandle,         "GETLASTHANDLE",    0, 0, 0, 0},
    {(IDL_FUN_RET) getdomains,            "GETDOMAINS",       2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) setidamdatatree,       "SETIDAMDATATREE",  1, 1, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) findidamtreestructurecomponent,    "FINDIDAMTREESTRUCTURECOMPONENT",   3, 3, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) findidamtreestructuredefinition,   "FINDIDAMTREESTRUCTUREDEFINITION",  3, 3, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) findidamtreestructure,         "FINDIDAMTREESTRUCTURE",        3, 3, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getidamnodeatomiccount,        "GETIDAMNODEATOMICCOUNT",       2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getidamnodeatomicrank,         "GETIDAMNODEATOMICRANK",        2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getidamnodeatomicshape,        "GETIDAMNODEATOMICSHAPE",       2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getidamnodeatomicnames,        "GETIDAMNODEATOMICNAMES",       2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getidamnodeatomictypes,        "GETIDAMNODEATOMICTYPES",       2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getidamnodeatomicpointers,     "GETIDAMNODEATOMICPOINTERS",        2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getidamnodeatomicdatacount,        "GETIDAMNODEATOMICDATACOUNT",       3, 3, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getidamnodeatomicdata,         "GETIDAMNODEATOMICDATA",        3, 3, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getidamnodestructurecount,     "GETIDAMNODESTRUCTURECOUNT",        2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getidamnodestructurerank,      "GETIDAMNODESTRUCTURERANK",     2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getidamnodestructureshape,     "GETIDAMNODESTRUCTURESHAPE",        2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getidamnodestructurenames,     "GETIDAMNODESTRUCTURENAMES",        2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getidamnodestructuretypes,     "GETIDAMNODESTRUCTURETYPES",        2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getidamnodestructurepointers,      "GETIDAMNODESTRUCTUREPOINTERS",     2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getidamnodestructuredatacount,     "GETIDAMNODESTRUCTUREDATACOUNT",    2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getidamnodestructuredatarank,      "GETIDAMNODESTRUCTUREDATARANK",     2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getidamnodestructuredatashape,     "GETIDAMNODESTRUCTUREDATASHAPE",    2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getidamnodeparent,         "GETIDAMNODEPARENT",            2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getidamnodechild,          "GETIDAMNODECHILD",         3, 3, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getidamnodechildid,            "GETIDAMNODECHILDID",           3, 3, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) getidamnodechildrencount,      "GETIDAMNODECHILDRENCOUNT",     2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) printidamtree,             "PRINTIDAMTREE",            2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) printidamtreestructurenames,           "PRINTIDAMTREESTRUCTURENAMES",      2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) printidamtreestructurecomponentnames,  "PRINTIDAMTREESTRUCTURECOMPONENTNAMES", 2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) printidamnodestructure,        "PRINTIDAMNODESTRUCTURE",       2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
    {(IDL_FUN_RET) regulariseidamvlenstructures,      "REGULARISEIDAMVLENSTRUCTURES",     2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0}
};

int getdata_startup(void)
{

    // TRUE for Functions, FALSE for Procedures

    if (!IDL_SysRtnAdd(getdata_functions, TRUE, ARRLEN(getdata_functions))) {
        return IDL_FALSE;
    }

    // Register the exit handler

    IDL_ExitRegister(getdata_exit_handler);

    return (IDL_TRUE);
}

int IDL_Load(void)
{
    if (!IDL_SysRtnAdd(getdata_functions, TRUE, ARRLEN(getdata_functions))) {
        return IDL_FALSE;
    }

    return (IDL_TRUE);
}

// Called when IDL is shutdown

void getdata_exit_handler(void)
{
    // Nothing to do!
}


// Input data Structure Passed between IDL and C
// Input Directives

typedef struct {
    IDL_LONG   exp_number;
    IDL_STRING signal;
    IDL_STRING source;
    IDL_LONG   pass;
    IDL_STRING tpass;
    IDL_STRING device;
    IDL_STRING archive;
    IDL_STRING server;
    IDL_STRING file;
    IDL_STRING format;
    IDL_STRING path;
    IDL_STRING mdstree;
    IDL_STRING mdsnode;
    IDL_LONG   mdstreenum;
    IDL_LONG   get_datadble;
    IDL_LONG   get_timedble;
    IDL_LONG   get_dimdble;
    IDL_LONG   get_scalar;
    IDL_LONG   get_bytes;
    IDL_LONG   get_bad;
    IDL_LONG   get_meta;
    IDL_LONG   get_asis;
    IDL_LONG   get_uncal;
    IDL_LONG   get_notoff;
    IDL_LONG   get_nodimdata;

    IDL_LONG   handle;
    IDL_LONG   error_code;
    IDL_STRING error_msg;
    IDL_LONG   status;
    IDL_LONG   rank;
    IDL_LONG   order;

} IDAM_SIN;

// Main Data Block Structure Passed Back to IDL

typedef struct {
    IDL_LONG   handle;
    IDL_LONG   data_type;
    IDL_LONG   data_n;
    IDL_STRING data_label;
    IDL_STRING data_units;
    IDL_STRING data_desc;
    char   *   data;
} IDAM_DOUT;


// Main Dimensional Data Block Structure Passed Back to IDL

typedef struct {
    IDL_LONG   handle;
    IDL_LONG   dim_id;
    IDL_LONG   dim_type;
    IDL_LONG   dim_n;
    IDL_STRING dim_label;
    IDL_STRING dim_units;
    char   *   dim;
} IDAM_DIMOUT;

// Error Model Structure Passed Back to IDL

typedef struct {
    IDL_LONG handle;
    IDL_LONG model;
    IDL_LONG param_n;
    float  * params;
} IDAM_MOUT;

// Dimensional Error Model Structure Passed Back to IDL

typedef struct {
    IDL_LONG handle;
    IDL_LONG dim_id;
    IDL_LONG model;
    IDL_LONG param_n;
    float  * params;
} IDAM_MDIMOUT;

//----------------------------------------------------------------------------------------------
// Meta Data Structures Passed Back to IDL

typedef struct {
    IDL_LONG    system_id;
    IDL_LONG    version;
    IDL_LONG    meta_id;
    IDL_STRING      type;
    IDL_STRING  device_name;
    IDL_STRING  system_name;
    IDL_STRING  system_desc;
    IDL_STRING  creation;
    IDL_STRING      xml;
    IDL_STRING  xml_creation;
} IDAM_DATA_SYSTEM;

typedef struct {
    IDL_LONG    config_id;
    IDL_LONG    system_id;
    IDL_LONG    meta_id;
    IDL_STRING  config_name;
    IDL_STRING  config_desc;
    IDL_STRING  creation;
    IDL_STRING      xml;
    IDL_STRING  xml_creation;
} IDAM_SYSTEM_CONFIG;

typedef struct {
    IDL_LONG    source_id;
    IDL_LONG    config_id;
    IDL_LONG    reason_id;
    IDL_LONG    run_id;
    IDL_LONG    meta_id;
    IDL_LONG    status_desc_id;
    IDL_LONG    exp_number;
    IDL_LONG    pass;
    IDL_LONG    status;
    IDL_LONG    status_reason_code;
    IDL_LONG    status_impact_code;
    IDL_STRING      access;
    IDL_STRING      reprocess;
    IDL_STRING      type;
    IDL_STRING      source_alias;
    IDL_STRING      pass_date;
    IDL_STRING      archive;
    IDL_STRING      device_name;
    IDL_STRING      format;
    IDL_STRING      path;
    IDL_STRING      filename;
    IDL_STRING      server;
    IDL_STRING      userid;
    IDL_STRING      reason_desc;
    IDL_STRING      run_desc;
    IDL_STRING      status_desc;
    IDL_STRING  creation;
    IDL_STRING  modified;
    IDL_STRING      xml;
    IDL_STRING  xml_creation;
} IDAM_DATA_SOURCE;

typedef struct {
    IDL_LONG    source_id;
    IDL_LONG    signal_desc_id;
    IDL_LONG    meta_id;
    IDL_LONG    status_desc_id;
    IDL_LONG    status;
    IDL_LONG    status_reason_code;
    IDL_LONG    status_impact_code;
    IDL_STRING      status_desc;
    IDL_STRING      access;
    IDL_STRING      reprocess;
    IDL_STRING  creation;
    IDL_STRING  modified;
    IDL_STRING      xml;
    IDL_STRING  xml_creation;
} IDAM_SIGNAL;


typedef struct {
    IDL_LONG    signal_desc_id;
    IDL_LONG    meta_id;
    IDL_LONG    rank;
    IDL_LONG    range_start;
    IDL_LONG    range_stop;
    IDL_STRING      signal_alias;
    IDL_STRING      signal_name;
    IDL_STRING      generic_name;
    IDL_STRING      description;
    IDL_STRING      signal_class;
    IDL_STRING      signal_owner;
    IDL_STRING  creation;
    IDL_STRING  modified;
    IDL_STRING      xml;
    IDL_STRING  xml_creation;
} IDAM_SIGNAL_DESC;


void freeMem(UCHAR * memPtr)
{
    //if(NDEBUG) fprintf(stdout,"freeMem: Free Address  : %x\n", (int)memPtr);
    free((void *)memPtr);
}


FILE * dbglog = NULL;       // Debug Log
FILE * errlog = NULL;       // Error Log
FILE * stdlog = NULL;       // Standard Log Output
int webout = 0;         // Enables Standard Web Reporting of Errors


//-----------------------------------------------------------------------------------------------------
//#####################################################################################################
// IDAM API

IDL_VPTR IDL_CDECL idamgetapi(int argc, IDL_VPTR argv[], char * argk)
{
    //
    // 2 Args: IDAM signal, source strings

    // calls:   int idamGetAPI(signal, source)

    int handle;
    char * signal, *source;

    typedef struct {
        IDL_KW_RESULT_FIRST_FIELD;
        IDL_LONG verbose;
        IDL_LONG debug;
        IDL_LONG help;
    } KW_RESULT;


    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = { IDL_KW_FAST_SCAN,
        {"DEBUG",     IDL_TYP_LONG,   1, IDL_KW_ZERO, 0,    IDL_KW_OFFSETOF(debug)},
        {"HELP",      IDL_TYP_LONG,   1, IDL_KW_ZERO, 0,    IDL_KW_OFFSETOF(help)},
        {"VERBOSE",   IDL_TYP_LONG,   1, IDL_KW_ZERO, 0,    IDL_KW_OFFSETOF(verbose)},
        {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug   = 0;
    kw.help    = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR *)0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\nidamGetAPI Help\n\n"
                "Returns the data handle from the API.\n\n"
                "Arguments:\n"
                "\t(string) signal \n"
                "\t(string) source \n\n");

        return (IDL_GettmpLong(-1));
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   data signal
    // Arg#2:   data source

    IDL_ENSURE_STRING(argv[0]);
    IDL_ENSURE_SCALAR(argv[0]);
    signal = IDL_STRING_STR(&(argv[0]->value.str));

    IDL_ENSURE_STRING(argv[1]);
    IDL_ENSURE_SCALAR(argv[1]);
    source = IDL_STRING_STR(&(argv[1]->value.str));

    //---------------------------------------------------------------------------------------------
    // Call API

    handle = idamGetAPI(signal, source);

    if (kw.debug) {
        fprintf(stdout, "+++ idamGetAPI +++\n");
        fprintf(stdout, "signal: %s\n", signal);
        fprintf(stdout, "Source: %s\n", source);
        fprintf(stdout, "Handle: %d\n", handle);
        fflush(NULL);
    }

    return (IDL_GettmpLong(handle));
}

//-----------------------------------------------------------------------------------------------------
// IDAM PUT API

// 2 use cases:
//  a> puting an un-named array of atomic data to a server side function
//  b> puting an unordered list of named arrays of atomic data to a server side function
// Future options: add structured types to the array class

IDL_VPTR IDL_CDECL idamputapi(int argc, IDL_VPTR argv[], char * argk)
{
    //
    // 2 Args: IDAM plugin directive (signal), data (scalar or array of structures)

    // calls:   idamPutAPI or idamPutListAPI

    typedef struct {
        IDL_KW_RESULT_FIRST_FIELD;
        IDL_LONG verbose;
        IDL_LONG debug;
        IDL_LONG help;
    } KW_RESULT;


    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = { IDL_KW_FAST_SCAN,
        {"DEBUG",     IDL_TYP_LONG,   1, IDL_KW_ZERO, 0,    IDL_KW_OFFSETOF(debug)},
        {"HELP",      IDL_TYP_LONG,   1, IDL_KW_ZERO, 0,    IDL_KW_OFFSETOF(help)},
        {"VERBOSE",   IDL_TYP_LONG,   1, IDL_KW_ZERO, 0,    IDL_KW_OFFSETOF(verbose)},
        {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug   = 0;
    kw.help    = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR *)0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\nidamPutAPI Help\n\n"
                "Returns the data handle from the API.\n\n"
                "Arguments:\n"
                "\t(string) plugin directive \n"
                "\t(string) data \n\n");

        return (IDL_GettmpLong(-1));
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   plugin directive
    // Arg#2:   data array or list (structure) of named arrays

    IDL_ENSURE_STRING(argv[0]);
    IDL_ENSURE_SCALAR(argv[0]);

    if (kw.debug) {
        fprintf(stdout, "+++ idamPutAPI +++\n");
        fprintf(stdout, "arg count: %d\n", argc);
        fprintf(stdout, "directive: %s\n", (char *)IDL_STRING_STR(&(argv[0]->value.str)));
        fprintf(stdout, "array?   : %d\n", argv[1]->flags & IDL_V_ARR);
        fprintf(stdout, "struct?  : %d\n", argv[1]->flags & IDL_V_STRUCT);
        fprintf(stdout, "struct?  : %d\n", argv[1]->type == IDL_TYP_STRUCT);

        fflush(NULL);
    }

    //-----------------------------------------------------------------------------------------
    // Test the 2nd Argument: Array only or scalar structure
    // String arrays are a pain ...

    // Array?, data[]

    PUTDATA_BLOCK putData;
    initIdamPutDataBlock(&putData);

    PUTDATA_BLOCK_LIST putDataBlockList;
    initIdamPutDataBlockList(&putDataBlockList);

    int i, rank = 0, count = 0;
    int type = IDL_TYP_UNDEF;

    if (argc >= 3 && (argv[1]->flags & IDL_V_ARR) && argv[1]->type != IDL_TYP_STRUCT) {
        IDL_ENSURE_ARRAY(argv[1]);

        if (kw.verbose) {
            fprintf(stdout, "arg #1:Array Passed\n");
        }

        putData.rank  = argv[1]->value.arr->n_dim;
        putData.count = argv[1]->value.arr->n_elts;
        type  = argv[1]->type;
        putData.data  = (char *)argv[1]->value.arr->data;

        if (putData.count == 0) {
            if (kw.verbose) {
                fprintf(stdout, "Error: Zero Count of array elements\n");
            }

            return (IDL_GettmpLong(-999));
        }

        if (putData.rank > 1) {
            putData.shape = (int *)malloc(rank * sizeof(int));

            for (i = 0; i < rank; i++) {
                putData.shape[i] = (int) argv[1]->value.arr->dim[i];
            }
        }

        if (kw.debug) {
            fprintf(stdout, "+++ idamPutAPI +++\n");
            fprintf(stdout, "rank : %d\n", putData.rank);
            fprintf(stdout, "count: %d\n", putData.count);

            if (type == IDL_TYP_FLOAT) {
                float * f = (float *)putData.data;

                for (i = 0; i < 5; i++) {
                    fprintf(stdout, "data[%d]: %f\n", i, f[i]);
                }
            }

            fflush(stdout);
        }

        switch (type) {
        case (IDL_TYP_BYTE):
            putData.data_type = TYPE_UNSIGNED_CHAR;
            break;

        case (IDL_TYP_STRING):
            putData.data_type = TYPE_STRING;
            break;

        case (IDL_TYP_UINT):
            putData.data_type = TYPE_UNSIGNED_SHORT;
            break;

        case (IDL_TYP_INT):
            putData.data_type = TYPE_SHORT;
            break;

        case (IDL_TYP_ULONG):
            putData.data_type = TYPE_UNSIGNED_INT;
            break;

        case (IDL_TYP_LONG):
            putData.data_type = TYPE_INT;
            break;

        case (IDL_TYP_ULONG64):
            putData.data_type = TYPE_UNSIGNED_LONG64;
            break;

        case (IDL_TYP_LONG64):
            putData.data_type = TYPE_LONG64;
            break;

        case (IDL_TYP_FLOAT):
            putData.data_type = TYPE_FLOAT;
            break;

        case (IDL_TYP_DOUBLE):
            putData.data_type = TYPE_DOUBLE;
            break;

        case (IDL_TYP_COMPLEX):
            putData.data_type = TYPE_COMPLEX;
            break;

        case (IDL_TYP_DCOMPLEX):
            putData.data_type = TYPE_DCOMPLEX;
            break;
        }

        if (kw.debug) {
            fprintf(stdout, "type: %d\n", putData.data_type);
            fflush(stdout);
        }

        // String arrays have to be regularised (equal length) as this method does not pass arrays of pointers

        char * new = NULL;

        if (type == IDL_TYP_STRING) {
            int maxLength = 0;
            IDL_STRING * sidl = NULL;

            for (i = 0; i < putData.count; i++) {
                sidl = (IDL_STRING *)putData.data;

                if (sidl->slen > maxLength) {
                    maxLength = sidl->slen;
                }
            }

            new = (char *)malloc(putData.count * (maxLength + 1) * sizeof(char)); // Block of memory for the strings

            for (i = 0; i < putData.count; i++) {
                sidl = (IDL_STRING *)putData.data;
                strncpy(&new[i * (maxLength + 1)], (char *)sidl->s, sidl->slen); // should be NULL terminated
            }

            putData.data = new;
        }

        // PUT the data to the server

        int h = idamPutAPI((char *)IDL_STRING_STR(&(argv[0]->value.str)), &putData);

        if (putData.rank > 1 && putData.shape != NULL) {
            free((void *)putData.shape);
        }

        if (type == IDL_TYP_STRING && new != NULL) {
            free((void *)new);
        }

        return (IDL_GettmpLong(h));

    } else

        // Passed Structure? {name, data[]}[]   Structures are always defined as an array in IDL
        // Must have two members only: a scalar string and an array
        // An array of structures with different contents must be passed by defining a structure type

        if (argc >= 3 && (argv[1]->flags & IDL_V_STRUCT) && argv[1]->type == IDL_TYP_STRUCT) {
            IDL_ENSURE_STRUCTURE(argv[1]);

            if (kw.debug) {
                fprintf(stdout, "arg #1:Structure Passed\n");
            }

            IDL_SREF s = (IDL_SREF)argv[1]->value.s;
            int memberCount = IDL_StructNumTags(s.sdef);

            if (kw.debug) {
                fprintf(stdout, "Structure member count: %d\n", memberCount);
            }

            IDL_ARRAY * arr = s.arr;
            rank = arr->n_dim;
            int memberCount2 = arr->n_elts;

            if (kw.debug) {
                fprintf(stdout, "Structure rank: %d\n", rank);
            }

            if (kw.debug) {
                fprintf(stdout, "Structure member count: %d\n", memberCount2);
            }

            count = argv[1]->value.s.arr->n_elts; // Shape is ignored for structure arrays (use case?)

            if (kw.debug) {
                fprintf(stdout, "Structure count: %d\n", count);
            }

            // Types of structure members?

            int msg_action = 0;
            IDL_VPTR tag;
            IDL_VARIABLE * var = NULL;

            for (i = 0; i < memberCount; i++) {
                int offset = IDL_StructTagInfoByIndex(s.sdef, i, msg_action, &tag);            // tag: Only information, not data
                char * name = IDL_StructTagNameByIndex(s.sdef, i, msg_action, NULL);
                var = (IDL_VARIABLE *) tag;

                if (kw.debug) {
                    fprintf(stdout, "Structure member[%d] offset : %d\n", i, offset);
                    fprintf(stdout, "Structure member[%d] name   : %s\n", i, name);
                    fprintf(stdout, "Structure member[%d] flags  : %d\n", i, var->flags);
                    fprintf(stdout, "Structure member[%d] type   : %d\n", i, var->type);
                    //fprintf(stdout,"Structure member[%d] rank : %d\n", i, var->value.arr->n_dim);
                    //fprintf(stdout,"Structure member[%d] count: %d\n", i, var->value.arr->n_elts);

                    if (var->type == IDL_TYP_STRING) {
                        IDL_STRING * sidl = (IDL_STRING *) & (argv[1]->value.arr->data[offset]);
                        fprintf(stdout, "data name : %s\n",  sidl->s);
                    }
                }
            }


            /*
                  for(i=0;i<count;i++){

                     initIdamPutData(&putData);

                 putData.data = (char *)argv[1]->value.s.arr->data[i];


                     addIdamPutDataBlockList(&putData, &putDataBlockList);
                  }

                  int h = idamPutListAPI((char *)IDL_STRING_STR(&(argv[0]->value.str)), &putDataBlockList);


                  for(i=0;i<putDataBlockList.  ;i++)
                     if(putDataBlockList.PutDataBlock[i].  == ) free((void *) ...shape);

                  freeIdamPutDataBlockList(putDataBlockList);
            */
            return (IDL_GettmpLong(-888));

        }

    return (IDL_GettmpLong(-997));

}




IDL_VPTR IDL_CDECL callidam2(int argc, IDL_VPTR argv[], char * argk)
{
    //
    //-------------------------------------------------------------------------
    // Change History:
    //
    // 04Nov2010 dgmuir Simplified version of callidam that uses standard idamGetAPI
    //-------------------------------------------------------------------------

    char * signal;
    char  source[STRING_LENGTH];
    int exp_number;

    CLIENT_BLOCK cblock = saveIdamProperties();  // preserve the current set of client properties

    int handle, status, error_code;
    char * error_msg;
    IDAM_SIN * sout ;    // Returned Structure

    IDL_VPTR ivReturn   = NULL;

    // Passed Structure

    void * psDef = NULL;
    IDL_MEMINT ilDims[IDL_MAX_ARRAY_DIM];

    // IDL tags structure (returned data structure IDAM2_SIN)

    IDL_STRUCT_TAG_DEF pTags[] = {
        {"EXP_NUMBER",  0,  (void *) IDL_TYP_LONG},
        {"SIGNAL",  0,  (void *) IDL_TYP_STRING},
        {"SOURCE",  0,  (void *) IDL_TYP_STRING},
        {"PASS",    0,  (void *) IDL_TYP_LONG},
        {"TPASS",   0,  (void *) IDL_TYP_STRING},
        {"DEVICE",  0,  (void *) IDL_TYP_STRING},
        {"ARCHIVE", 0,  (void *) IDL_TYP_STRING},
        {"SERVER",  0,  (void *) IDL_TYP_STRING},
        {"FILE",    0,  (void *) IDL_TYP_STRING},
        {"FORMAT",  0,  (void *) IDL_TYP_STRING},
        {"PATH",    0,  (void *) IDL_TYP_STRING},

        {"MDSTREE", 0,  (void *) IDL_TYP_STRING},
        {"MDSNODE", 0,  (void *) IDL_TYP_STRING},
        {"MDSTREENUM",  0,  (void *) IDL_TYP_LONG},

        {"GET_DATADBLE", 0,  (void *) IDL_TYP_LONG},
        {"GET_TIMEDBLE", 0,  (void *) IDL_TYP_LONG},
        {"GET_DIMDBLE", 0,  (void *) IDL_TYP_LONG},
        {"GET_SCALAR",  0,  (void *) IDL_TYP_LONG},
        {"GET_BYTES",   0,  (void *) IDL_TYP_LONG},

        {"GET_BAD", 0,  (void *) IDL_TYP_LONG},
        {"GET_META",    0,  (void *) IDL_TYP_LONG},
        {"GET_ASIS",    0,  (void *) IDL_TYP_LONG},
        {"GET_UNCAL",   0,  (void *) IDL_TYP_LONG},
        {"GET_NOTOFF",  0,  (void *) IDL_TYP_LONG},
        {"GET_NODIMDATA", 0, (void *) IDL_TYP_LONG},

        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"ERROR_CODE",  0,  (void *) IDL_TYP_LONG},
        {"ERROR_MSG",   0,  (void *) IDL_TYP_STRING},
        {"STATUS",  0,  (void *) IDL_TYP_LONG},
        {"RANK",    0,  (void *) IDL_TYP_LONG},
        {"ORDER",   0,  (void *) IDL_TYP_LONG},
        {0}
    };


    // Keyword Structure

    typedef struct {
        IDL_KW_RESULT_FIRST_FIELD;

        IDL_LONG verbose;
        IDL_LONG debug;
        IDL_LONG help;

        IDL_LONG get_datadble;
        IDL_LONG get_timedble;
        IDL_LONG get_dimdble;
        IDL_LONG get_scalar;
        IDL_LONG get_bytes;

        IDL_LONG get_bad;
        IDL_LONG get_meta;
        IDL_LONG get_asis;
        IDL_LONG get_uncal;
        IDL_LONG get_notoff;
        IDL_LONG get_nodimdata;

    } KW_RESULT;


    // Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {
        IDL_KW_FAST_SCAN,
        {"DEBUG",   IDL_TYP_LONG,  1, IDL_KW_ZERO, 0,                      IDL_KW_OFFSETOF(debug)},
        {"GET_ASIS",    IDL_TYP_LONG,  1, IDL_KW_ZERO, 0,                  IDL_KW_OFFSETOF(get_asis)},
        {"GET_BAD",     IDL_TYP_LONG,  1, IDL_KW_ZERO, 0,                  IDL_KW_OFFSETOF(get_bad)},
        {"GET_BYTES",   IDL_TYP_LONG,  1, IDL_KW_ZERO, 0,                  IDL_KW_OFFSETOF(get_bytes)},
        {"GET_DATADBLE", IDL_TYP_LONG,  1, IDL_KW_ZERO, 0,                  IDL_KW_OFFSETOF(get_datadble)},
        {"GET_DIMDBLE", IDL_TYP_LONG,  1, IDL_KW_ZERO, 0,                  IDL_KW_OFFSETOF(get_dimdble)},
        {"GET_META",    IDL_TYP_LONG,  1, IDL_KW_ZERO, 0,                  IDL_KW_OFFSETOF(get_meta)},
        {"GET_NODIMDATA",  IDL_TYP_LONG,  1, IDL_KW_ZERO, 0,               IDL_KW_OFFSETOF(get_nodimdata)},
        {"GET_NOTOFF",  IDL_TYP_LONG,  1, IDL_KW_ZERO, 0,                  IDL_KW_OFFSETOF(get_notoff)},
        {"GET_SCALAR",  IDL_TYP_LONG,  1, IDL_KW_ZERO, 0,                  IDL_KW_OFFSETOF(get_scalar)},
        {"GET_TIMEDBLE", IDL_TYP_LONG,  1, IDL_KW_ZERO, 0,                  IDL_KW_OFFSETOF(get_timedble)},
        {"GET_UNCAL", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0,                      IDL_KW_OFFSETOF(get_uncal)},
        {"HELP",    IDL_TYP_LONG,  1, IDL_KW_ZERO, 0,                      IDL_KW_OFFSETOF(help)},
        {"VERBOSE", IDL_TYP_LONG,  1, IDL_KW_ZERO, 0,                      IDL_KW_OFFSETOF(verbose)},
        {NULL}
    };

    KW_RESULT kw;

    kw.verbose       = 0;
    kw.debug         = 0;
    kw.help          = 0;
    kw.get_datadble  = 0;
    kw.get_dimdble   = 0;
    kw.get_timedble  = 0;
    kw.get_scalar    = 0;
    kw.get_bytes     = 0;
    kw.get_asis      = 0;
    kw.get_bad       = 0;
    kw.get_meta      = 0;
    kw.get_notoff    = 0;
    kw.get_uncal     = 0;
    kw.get_nodimdata = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR *)0, 1, &kw);

    //---------------------------------------------------------------------------
    // All Arguments & keywords
    //---------------------------------------------------------------------------
    // Identify Argument Passing Model

    if (kw.debug) {
        fprintf(stdout, "Build Date: %s\n", __DATE__);
        fprintf(stdout, "%d Arguments Passed\n", argc);
    }

    if (argc == 0) {
        if (kw.debug) {
            fprintf(stdout, "No Arguments Passed\n");
        }

        IDL_KW_FREE;
        return (IDL_GettmpLong(GDE_NO_ARGUMENTS));
    }

    // First String Argument: the Signal

    IDL_ENSURE_STRING(argv[0]);
    IDL_ENSURE_SCALAR(argv[0]);
    signal = IDL_STRING_STR(&(argv[0]->value.str));

    // Second Argument: the Source - a String or an Integer?

    IDL_ENSURE_SCALAR(argv[1]);

    if (argv[1]->type == IDL_TYP_STRING) {
        IDL_ENSURE_STRING(argv[1]);
        strcpy(source, IDL_STRING_STR(&(argv[1]->value.str)));
    } else {
        if (argv[1]->type == IDL_TYP_LONG || argv[1]->type == IDL_TYP_INT) {
            exp_number = IDL_LongScalar(argv[1]);
            sprintf(source, "%d", exp_number);
        } else {
            if (kw.debug) {
                fprintf(stdout, "The second Argument must be either a string or an integer\n");
            }

            IDL_KW_FREE;
            return (IDL_GettmpLong(GDE_NO_ARGUMENTS));
        }
    }

    if (kw.debug) {
        fprintf(stdout, "Signal : %s\n", signal);
        fprintf(stdout, "Source : %s\n", source);
    }

    //-----------------------------------------------------------------------
    // Keywords

    if (kw.debug) {
        if (kw.debug) {
            fprintf(stdout, "Debug Keyword Passed\n");
        }

        if (kw.verbose) {
            fprintf(stdout, "Verbose Keyword Passed\n");
        }

        if (kw.help) {
            fprintf(stdout, "Help Keyword Passed\n");
        }

        if (kw.get_asis) {
            fprintf(stdout, "get_asis Passed\n");
        }

        if (kw.get_bad) {
            fprintf(stdout, "get_bad Passed\n");
        }

        if (kw.get_datadble) {
            fprintf(stdout, "get_datadble Passed\n");
        }

        if (kw.get_dimdble) {
            fprintf(stdout, "get_dimdble Passed\n");
        }

        if (kw.get_timedble) {
            fprintf(stdout, "get_timedble Passed\n");
        }

        if (kw.get_scalar) {
            fprintf(stdout, "get_scalar Passed\n");
        }

        if (kw.get_bytes) {
            fprintf(stdout, "get_bytes Passed\n");
        }

        if (kw.get_meta) {
            fprintf(stdout, "get_meta Passed\n");
        }

        if (kw.get_uncal) {
            fprintf(stdout, "get_uncal Passed\n");
        }

        if (kw.get_notoff) {
            fprintf(stdout, "get_notoff Passed\n");
        }

        if (kw.get_nodimdata) {
            fprintf(stdout, "get_nodimdata Passed\n");
        }
    }

    //--------------------------------------------------------------------------
    // Call for HELP?

    if (0 && kw.help) {
        userhelp(stdout, "getidam");
        IDL_KW_FREE;
        return (IDL_GettmpLong(0));
    }

    //-----------------------------------------------------------------------
    // Process Keywords (take Priority over Passed Arguments)

    if (kw.debug) {
        setIdamProperty("debug");    // Cannot be reset (currently!)
    }

    if (kw.verbose) {
        setIdamProperty("verbose");
    }

    if (kw.get_datadble) {
        setIdamProperty("get_datadble");    // Properties passed by Keyword must be reset to the prior state
    }

    if (kw.get_dimdble) {
        setIdamProperty("get_dimdble");
    }

    if (kw.get_timedble) {
        setIdamProperty("get_timedble");
    }

    if (kw.get_scalar) {
        setIdamProperty("get_scalar");
    }

    if (kw.get_bytes) {
        setIdamProperty("get_bytes");
    }

    if (kw.get_asis) {
        setIdamProperty("get_asis");
    }

    if (kw.get_bad) {
        setIdamProperty("get_bad");
    }

    if (kw.get_meta) {
        setIdamProperty("get_meta");
    }

    if (kw.get_uncal) {
        setIdamProperty("get_uncal");
    }

    if (kw.get_notoff) {
        setIdamProperty("get_notoff");
    }

    if (kw.get_nodimdata) {
        setIdamProperty("get_nodimdata");
    }

    //--------------------------------------------------------------------------
    // Select the Appropriate API

    if (kw.debug) {
        fprintf(stdout, "idamGetAPI args: ('%s','%s')\n", signal, source);
        fflush(stdout);
    }

    handle = idamGetAPI(signal, source);

    if (kw.debug) {
        fprintf(stdout, "IDAM API Handle = %d\n", handle);
        fflush(stdout);
    }

    error_code = getIdamErrorCode(handle);
    error_msg  = (char *) getIdamErrorMsg(handle);

    if (handle < 0) {
        if (kw.verbose) {
            fprintf(stdout, "Bad Handle Returned from IDAM Call!\n");

            if (error_msg != NULL) {
                fprintf(stdout, "[%d] %s\n", error_code, error_msg);
            }
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_BAD_HANDLE));
    }

    status = getIdamDataStatus(handle);

    //--------------------------------------------------------------------------
    // Return Handle and High Level Information: Not Data - Separate Calls

    if ((sout = (IDAM_SIN *)malloc(sizeof(IDAM_SIN))) == NULL) {
        if (kw.verbose) {
            fprintf(stdout, "Heap Memory Allocation Failed!\n");
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_HEAP_ALLOC_ERROR));
    }

    sout->exp_number     = (IDL_LONG) exp_number;        // Use Default values
    sout->pass           = (IDL_LONG) - 1;
    sout->mdstreenum     = (IDL_LONG) 0;

    sout->handle     = (IDL_LONG) handle;
    sout->rank       = (IDL_LONG) getIdamRank(handle);
    sout->order      = (IDL_LONG) getIdamOrder(handle);
    sout->status     = (IDL_LONG) status;
    sout->error_code     = (IDL_LONG) error_code;

    sout->get_datadble = (IDL_LONG)getIdamProperty("get_datadble");  // (IDL_LONG) kw.get_datadble;
    sout->get_dimdble  = (IDL_LONG)getIdamProperty("get_dimdble");   // (IDL_LONG) kw.get_dimdble;
    sout->get_timedble = (IDL_LONG)getIdamProperty("get_timedble");  // (IDL_LONG) kw.get_timedble;
    sout->get_scalar   = (IDL_LONG)getIdamProperty("get_scalar");    // (IDL_LONG) kw.get_scalar;
    sout->get_bytes    = (IDL_LONG)getIdamProperty("get_bytes");     // (IDL_LONG) kw.get_bytes;
    sout->get_asis     = (IDL_LONG)getIdamProperty("get_asis");      // (IDL_LONG) kw.get_asis;
    sout->get_bad      = (IDL_LONG)getIdamProperty("get_bad");       // (IDL_LONG) kw.get_bad;
    sout->get_meta     = (IDL_LONG)getIdamProperty("get_meta");      // (IDL_LONG) kw.get_meta;
    sout->get_uncal    = (IDL_LONG)getIdamProperty("get_uncal");     // (IDL_LONG) kw.get_uncal;
    sout->get_notoff   = (IDL_LONG)getIdamProperty("get_notoff");    // (IDL_LONG) kw.get_notoff;
    sout->get_nodimdata = (IDL_LONG)getIdamProperty("get_nodimdata"); // (IDL_LONG) kw.get_nodimdata;

    IDL_StrStore(&(sout->signal),  signal);
    IDL_StrStore(&(sout->source),  source);
    IDL_StrStore(&(sout->tpass),   "");
    IDL_StrStore(&(sout->device),  "");
    IDL_StrStore(&(sout->archive), "");
    IDL_StrStore(&(sout->server),  "");
    IDL_StrStore(&(sout->file),    "");
    IDL_StrStore(&(sout->format),  "");
    IDL_StrStore(&(sout->path),    "");
    IDL_StrStore(&(sout->mdstree), "");
    IDL_StrStore(&(sout->mdsnode), "");

    IDL_StrStore(&(sout->error_msg), error_msg);

    // Create an Anonymous IDL Structure and Import into IDL

    psDef = IDL_MakeStruct(NULL, pTags);

    ilDims[0] = 1;   // import Structure as a Single element Array
    ivReturn  = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR *)sout, freeMem, psDef);

    //--------------------------------------------------------------------------
    // If Users don't check their returned error codes or pointer values then a seg fault will occur...
    // This free will force users to make the check and code quality will improve!
    //
    // The exception is bad data ... users may want to look at this data by changing the get_bad property

    if (sout->error_code != 0 && !(getIdamDataStatus(sout->handle) == MIN_STATUS && sout->error_code == DATA_STATUS_BAD)) {

        if (kw.debug) {
            fprintf(stdout, "Freeing Heap for Handle %d\n", handle);
        }

        idamFree(handle);
    }

    //--------------------------------------------------------------------------
    // Cleanup Keywords

    IDL_KW_FREE;
    restoreIdamProperties(cblock);
    return (ivReturn);
}


IDL_VPTR IDL_CDECL callidam(int argc, IDL_VPTR argv[], char * argk)
{
    //
    //-------------------------------------------------------------------------
    // Change History:
    //
    // 18Apr2007 dgmuir ilDims changed from type IDL_LONG to IDL_MEMINT
    //-------------------------------------------------------------------------

    char * signal,  *device, *archive, *server, *file, *format, *path,
         *mdstree, *mdsnode, *tpass;
    char * source = NULL;
    int  useIdamGetAPI = 0;
    static IDL_LONG exp_number, pass = -1, mdstreenum;

    CLIENT_BLOCK cblock = saveIdamProperties();  // preserve the current set of client properties

    int handle, rank, order, status, error_code;
    char * error_msg;

    char testfile[4];

    char * defArchive = "mast";  // ***** Check for Environment Variables
    char * defDevice  = "mast";

    IDAM_SIN * asin;     // Input Argument Structure
    IDAM_SIN * sout ;    // Returned Structure

    IDL_VPTR ivReturn   = NULL;

    // Passed Structure

    void * psDef = NULL;
    //IDL_LONG ilDims[IDL_MAX_ARRAY_DIM];
    IDL_MEMINT ilDims[IDL_MAX_ARRAY_DIM];

    // IDL tags structure

    IDL_STRUCT_TAG_DEF pTags[] = {
        {"EXP_NUMBER",  0,  (void *) IDL_TYP_LONG},
        {"SIGNAL",  0,  (void *) IDL_TYP_STRING},
        {"SOURCE",  0,  (void *) IDL_TYP_STRING},
        {"PASS",    0,  (void *) IDL_TYP_LONG},
        {"TPASS",   0,  (void *) IDL_TYP_STRING},
        {"DEVICE",  0,  (void *) IDL_TYP_STRING},
        {"ARCHIVE", 0,  (void *) IDL_TYP_STRING},
        {"SERVER",  0,  (void *) IDL_TYP_STRING},
        {"FILE",    0,  (void *) IDL_TYP_STRING},
        {"FORMAT",  0,  (void *) IDL_TYP_STRING},
        {"PATH",    0,  (void *) IDL_TYP_STRING},

        {"MDSTREE", 0,  (void *) IDL_TYP_STRING},
        {"MDSNODE", 0,  (void *) IDL_TYP_STRING},
        {"MDSTREENUM",  0,  (void *) IDL_TYP_LONG},

        {"GET_DATADBLE", 0,  (void *) IDL_TYP_LONG},
        {"GET_TIMEDBLE", 0,  (void *) IDL_TYP_LONG},
        {"GET_DIMDBLE", 0,  (void *) IDL_TYP_LONG},
        {"GET_SCALAR",  0,  (void *) IDL_TYP_LONG},
        {"GET_BYTES",   0,  (void *) IDL_TYP_LONG},

        {"GET_BAD", 0,  (void *) IDL_TYP_LONG},
        {"GET_META",    0,  (void *) IDL_TYP_LONG},
        {"GET_ASIS",    0,  (void *) IDL_TYP_LONG},
        {"GET_UNCAL",   0,  (void *) IDL_TYP_LONG},
        {"GET_NOTOFF",  0,  (void *) IDL_TYP_LONG},
        {"GET_NODIMDATA", 0, (void *) IDL_TYP_LONG},

        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"ERROR_CODE",  0,  (void *) IDL_TYP_LONG},
        {"ERROR_MSG",   0,  (void *) IDL_TYP_STRING},
        {"STATUS",  0,  (void *) IDL_TYP_LONG},
        {"RANK",    0,  (void *) IDL_TYP_LONG},
        {"ORDER",   0,  (void *) IDL_TYP_LONG},
        {0}
    };



    // Keyword Structure

    typedef struct {
        IDL_KW_RESULT_FIRST_FIELD;

        IDL_LONG verbose;
        IDL_LONG debug;
        IDL_LONG help;
        IDL_LONG exp_number;
        IDL_LONG pass;
        IDL_LONG mdstreenum;

        IDL_LONG get_datadble;
        IDL_LONG get_timedble;
        IDL_LONG get_dimdble;
        IDL_LONG get_scalar;
        IDL_LONG get_bytes;

        IDL_LONG get_bad;
        IDL_LONG get_meta;
        IDL_LONG get_asis;
        IDL_LONG get_uncal;
        IDL_LONG get_notoff;
        IDL_LONG get_nodimdata;

        IDL_STRING signal;
        IDL_STRING tpass;
        IDL_STRING device;
        IDL_STRING archive;
        IDL_STRING server;
        IDL_STRING file;
        IDL_STRING format;
        IDL_STRING path;
        IDL_STRING mdstree;
        IDL_STRING mdsnode;

        IDL_LONG issignal;
        IDL_LONG istpass;
        IDL_LONG isdevice;
        IDL_LONG isarchive;
        IDL_LONG isserver;
        IDL_LONG isfile;
        IDL_LONG isformat;
        IDL_LONG ispath;
        IDL_LONG ismdstree;
        IDL_LONG ismdsnode;
        IDL_LONG ismdstreenum;
        IDL_LONG isexp_number;
        IDL_LONG ispass;

        IDL_LONG istest;
        IDL_LONG testid;

    } KW_RESULT;


    // Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {
        IDL_KW_FAST_SCAN,
        {"ARCHIVE", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(isarchive),     IDL_KW_OFFSETOF(archive)},
        {"DEBUG",   IDL_TYP_LONG,  1, IDL_KW_ZERO, 0,                      IDL_KW_OFFSETOF(debug)},
        {"DEVICE",  IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(isdevice),      IDL_KW_OFFSETOF(device)},
        {"EXP_NUMBER", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(isexp_number),  IDL_KW_OFFSETOF(exp_number)},
        {"FILE",    IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(isfile),        IDL_KW_OFFSETOF(file)},
        {"FORMAT",  IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(isformat),      IDL_KW_OFFSETOF(format)},
        {"GET_ASIS",    IDL_TYP_LONG,  1, IDL_KW_ZERO, 0,                  IDL_KW_OFFSETOF(get_asis)},
        {"GET_BAD",     IDL_TYP_LONG,  1, IDL_KW_ZERO, 0,                  IDL_KW_OFFSETOF(get_bad)},
        {"GET_BYTES",   IDL_TYP_LONG,  1, IDL_KW_ZERO, 0,                  IDL_KW_OFFSETOF(get_bytes)},
        {"GET_DATADBLE", IDL_TYP_LONG,  1, IDL_KW_ZERO, 0,                  IDL_KW_OFFSETOF(get_datadble)},
        {"GET_DIMDBLE", IDL_TYP_LONG,  1, IDL_KW_ZERO, 0,                  IDL_KW_OFFSETOF(get_dimdble)},
        {"GET_META",    IDL_TYP_LONG,  1, IDL_KW_ZERO, 0,                  IDL_KW_OFFSETOF(get_meta)},
        {"GET_NODIMDATA",  IDL_TYP_LONG,  1, IDL_KW_ZERO, 0,               IDL_KW_OFFSETOF(get_nodimdata)},
        {"GET_NOTOFF",  IDL_TYP_LONG,  1, IDL_KW_ZERO, 0,                  IDL_KW_OFFSETOF(get_notoff)},
        {"GET_SCALAR",  IDL_TYP_LONG,  1, IDL_KW_ZERO, 0,                  IDL_KW_OFFSETOF(get_scalar)},
        {"GET_TIMEDBLE", IDL_TYP_LONG,  1, IDL_KW_ZERO, 0,                  IDL_KW_OFFSETOF(get_timedble)},
        {"GET_UNCAL", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0,                      IDL_KW_OFFSETOF(get_uncal)},
        {"HELP",    IDL_TYP_LONG,  1, IDL_KW_ZERO, 0,                      IDL_KW_OFFSETOF(help)},
        {"MDSNODE", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(ismdsnode),     IDL_KW_OFFSETOF(mdsnode)},
        {"MDSTREE", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(ismdstree),     IDL_KW_OFFSETOF(mdstree)},
        {"PASS",    IDL_TYP_LONG,  1, IDL_KW_ZERO, IDL_KW_OFFSETOF(ispass),        IDL_KW_OFFSETOF(pass)},
        {"PATH",    IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(ispath),        IDL_KW_OFFSETOF(path)},
        {"PULNO",   IDL_TYP_LONG,  1, IDL_KW_ZERO, IDL_KW_OFFSETOF(isexp_number),  IDL_KW_OFFSETOF(exp_number)},
        {"PULSE",   IDL_TYP_LONG,  1, IDL_KW_ZERO, IDL_KW_OFFSETOF(isexp_number),  IDL_KW_OFFSETOF(exp_number)},
        {"SERVER",  IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(isserver),      IDL_KW_OFFSETOF(server)},
        {"SHOTNO",  IDL_TYP_LONG,  1, IDL_KW_ZERO, IDL_KW_OFFSETOF(isexp_number),  IDL_KW_OFFSETOF(exp_number)},
        {"SIGNAL",  IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(issignal),      IDL_KW_OFFSETOF(signal)},
        {"SN",      IDL_TYP_LONG,  1, IDL_KW_ZERO, IDL_KW_OFFSETOF(isexp_number),  IDL_KW_OFFSETOF(exp_number)},
        {"TEST",    IDL_TYP_LONG  , 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(istest),        IDL_KW_OFFSETOF(testid)},
        {"TPASS",   IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(istpass),       IDL_KW_OFFSETOF(tpass)},
        {"TREENUM", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(ismdstreenum),    IDL_KW_OFFSETOF(mdstreenum)},
        {"VERBOSE", IDL_TYP_LONG,  1, IDL_KW_ZERO, 0,                      IDL_KW_OFFSETOF(verbose)},
        {NULL}
    };
    //{"RANGE",      IDL_TYP_LONG,   1, IDL_KW_ARRAY, &kwrange, IDL_CHARA(rangeDesc)},
    KW_RESULT kw;

    kw.issignal  = 0;
    kw.isdevice  = 0;
    kw.isarchive = 0;
    kw.isserver  = 0;
    kw.isfile    = 0;
    kw.isformat  = 0;
    kw.ispath    = 0;
    kw.ismdstree = 0;
    kw.ismdsnode = 0;
    kw.istpass   = 0;
    kw.isexp_number = 0;
    kw.ispass       = 0;
    kw.ismdstreenum = 0;
    kw.istest       = 0;
    kw.testid       = 0;

    kw.verbose       = 0;
    kw.debug         = 0;
    kw.help          = 0;
    kw.exp_number    = -1;
    kw.pass          = -1;
    kw.mdstreenum    = -1;
    kw.get_datadble  = 0;
    kw.get_dimdble   = 0;
    kw.get_timedble  = 0;
    kw.get_scalar    = 0;
    kw.get_bytes     = 0;
    kw.get_asis      = 0;
    kw.get_bad       = 0;
    kw.get_meta      = 0;
    kw.get_notoff    = 0;
    kw.get_uncal     = 0;
    kw.get_nodimdata = 0;

    exp_number = 0;
    signal     = NULL;


    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR *)0, 1, &kw);

    // Reset All Properties (the current state has been saved for later restore)

    //resetProperties();

    //---------------------------------------------------------------------------
    // All Arguments & keywords
    //---------------------------------------------------------------------------
    // Identify Argument Passing Model

    if (kw.verbose) {
        fprintf(stdout, "Build Date: %s\n", __DATE__);
    }

    if (argc == 0) {
        if (kw.verbose) {
            fprintf(stdout, "No Arguments Passed\n");
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_NO_ARGUMENTS));
    }

    if (kw.debug) {
        fprintf(stdout, "%d Arguments Passed\n", argc);
    }

    // test the First Argument: Array or Scalar? String or Structure?

    if (argv[0]->flags & IDL_V_ARR && argv[0]->type != IDL_TYP_STRUCT) {
        if (kw.verbose) {
            fprintf(stdout, "arg #1:Array Passed - Not Implemented Yet!\n");
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_NOT_IMPLEMENTED));
    }

    // Passed Structure?

    if (argc == 1 && !(argv[0]->flags & IDL_V_ARR) && argv[0]->type == IDL_TYP_STRUCT) {
        IDL_ENSURE_STRUCTURE(argv[0]);
        IDL_EXCLUDE_EXPR(argv[0]);

        if (kw.debug) {
            fprintf(stdout, "arg #1:Structure Passed\n");
        }

        asin = (IDAM_SIN *)argv[0]->value.s.arr->data;    // Copy Structure Input to Output

        exp_number = (int)asin->exp_number;
        pass       = (int)asin->pass;
        mdstreenum = (int)asin->mdstreenum;

        signal  = IDL_STRING_STR(&(asin->signal));
        source  = IDL_STRING_STR(&(asin->source));
        device  = IDL_STRING_STR(&(asin->device));
        archive = IDL_STRING_STR(&(asin->archive));
        tpass   = IDL_STRING_STR(&(asin->tpass));
        server  = IDL_STRING_STR(&(asin->server));
        file    = IDL_STRING_STR(&(asin->file));
        format  = IDL_STRING_STR(&(asin->format));
        path    = IDL_STRING_STR(&(asin->path));
        mdstree = IDL_STRING_STR(&(asin->mdstree));
        mdsnode = IDL_STRING_STR(&(asin->mdsnode));

        if ((int)asin->get_datadble) {
            setIdamProperty("get_datadble");    // Properties passed by Structure must be reset to the prior state
        }

        if ((int)asin->get_dimdble) {
            setIdamProperty("get_dimdble");
        }

        if ((int)asin->get_timedble) {
            setIdamProperty("get_timedble");
        }

        if ((int)asin->get_scalar) {
            setIdamProperty("get_scalar");
        }

        if ((int)asin->get_bytes) {
            setIdamProperty("get_bytes");
        }

        if ((int)asin->get_asis) {
            setIdamProperty("get_asis");
        }

        if ((int)asin->get_bad) {
            setIdamProperty("get_bad");
        }

        if ((int)asin->get_meta) {
            setIdamProperty("get_meta");
        }

        if ((int)asin->get_uncal) {
            setIdamProperty("get_uncal");
        }

        if ((int)asin->get_notoff) {
            setIdamProperty("get_notoff");
        }

        if ((int)asin->get_nodimdata) {
            setIdamProperty("get_nodimdata");
        }

        if (strlen(signal) > 0 && strlen(source) > 0) {
            useIdamGetAPI = 1;
        }

    } else {

        // Passed String followed by an Integer?

        if (argc >= 1 && argv[0]->type == IDL_TYP_STRING) {
            IDL_ENSURE_STRING(argv[0]);

            if (argc == 1 && kw.exp_number <= 0) {
                if (kw.verbose) {
                    fprintf(stdout, "Signal Name Passed by Argument - Exp. Number Missing?\n");
                }

                IDL_KW_FREE;
                restoreIdamProperties(cblock);
                return (IDL_GettmpLong(GDE_NO_EXP_NUMBER));
            }

            signal = IDL_STRING_STR(&(argv[0]->value.str));

            if (kw.debug) {
                fprintf(stdout, "Signal Name Passed by Argument\n");
                fprintf(stdout, "Signal : %s\n", signal);
            }

            if (argc > 1 && (argv[1]->type == IDL_TYP_LONG || argv[1]->type == IDL_TYP_INT)) {

                if (argv[1]->flags & IDL_V_ARR) {
                    if (kw.verbose) {
                        fprintf(stdout, "arg #2:Array Passed - Not Implemented Yet!\n");
                    }

                    IDL_KW_FREE;
                    restoreIdamProperties(cblock);
                    return (IDL_GettmpLong(GDE_NOT_IMPLEMENTED));
                }

                IDL_ENSURE_SCALAR(argv[1]);
                exp_number = IDL_LongScalar(argv[1]);

                if (kw.debug) {
                    fprintf(stdout, "Exp Number Passed by Argument\n" );
                    fprintf(stdout, "Shot : %ld\n", (long)exp_number);
                }
            } else {
                if (argc > 1 && argv[1]->type == IDL_TYP_STRING) {
                    IDL_ENSURE_STRING(argv[1]);
                    source = IDL_STRING_STR(&(argv[1]->value.str));
                    useIdamGetAPI = 1;

                    if (kw.debug) {
                        fprintf(stdout, "Data Source Passed by Argument\n");
                        fprintf(stdout, "Source : %s\n", source);
                    }
                } else {
                    if (!kw.isexp_number && !kw.isfile && !kw.isformat) {        // Don't need pulse no. if specific file
                        if (kw.verbose) {
                            fprintf(stdout, "No Exp. Number Passed!\n");
                        }

                        IDL_KW_FREE;
                        restoreIdamProperties(cblock);
                        return (IDL_GettmpLong(GDE_NO_EXP_NUMBER));
                    }
                }
            }

        } else {

            // Passed Integer followed by an String?

            if (argc >= 1 && (argv[0]->type == IDL_TYP_LONG || argv[0]->type == IDL_TYP_INT)) {
                IDL_ENSURE_SCALAR(argv[0]);

                if (kw.debug) {
                    fprintf(stdout, "Integer Passed\n");
                }

                if (argc == 1 && !kw.issignal) {
                    if (kw.verbose) {
                        fprintf(stdout, "Exp Number Passed by Argument - Signal Missing?\n");
                    }

                    IDL_KW_FREE;
                    restoreIdamProperties(cblock);
                    return (IDL_GettmpLong(GDE_NO_SIGNAL_ARGUMENT));
                }

                exp_number = IDL_LongScalar(argv[0]);

                if (kw.debug) {
                    fprintf(stdout, "Exp Number Passed by Argument\n");
                    fprintf(stdout, "Shot : %ld\n", (long)exp_number);
                }

                if (argc > 1 && argv[1]->type == IDL_TYP_STRING) {

                    if (argv[1]->flags & IDL_V_ARR) {
                        if (kw.verbose) {
                            fprintf(stdout, "arg #2:Array Passed - Not Implemented Yet!\n");
                        }

                        IDL_KW_FREE;
                        restoreIdamProperties(cblock);
                        return (IDL_GettmpLong(GDE_NOT_IMPLEMENTED));
                    }

                    IDL_ENSURE_STRING(argv[1]);

                    signal = IDL_STRING_STR(&(argv[1]->value.str));

                    if (kw.debug) {
                        fprintf(stdout, "String Passed by Argument\n");
                        fprintf(stdout, "Signal : %s\n", signal);
                    }

                } else {
                    if (!kw.issignal) {
                        if (kw.verbose) {
                            fprintf(stdout, "No Signal Name Passed!\n");
                        }

                        IDL_KW_FREE;
                        restoreIdamProperties(cblock);
                        return (IDL_GettmpLong(GDE_NO_SIGNAL_ARGUMENT));
                    }

                }
            }
        }

    }

    //-----------------------------------------------------------------------
    // Keywords

    if (kw.debug) {
        if (kw.issignal) {
            fprintf(stdout, "Signal Keyword Passed %s\n",  IDL_STRING_STR(&kw.signal));
        }

        if (kw.isdevice) {
            fprintf(stdout, "Device Keyword Passed %s\n",  IDL_STRING_STR(&kw.device));
        }

        if (kw.isarchive) {
            fprintf(stdout, "Archive Keyword Passed %s\n", IDL_STRING_STR(&kw.archive));
        }

        if (kw.isserver) {
            fprintf(stdout, "Server Keyword Passed %s\n",  IDL_STRING_STR(&kw.server));
        }

        if (kw.isfile) {
            fprintf(stdout, "File Keyword Passed %s\n",    IDL_STRING_STR(&kw.file));
        }

        if (kw.isformat) {
            fprintf(stdout, "Server Keyword Passed %s\n",  IDL_STRING_STR(&kw.format));
        }

        if (kw.ispath) {
            fprintf(stdout, "Path Keyword Passed %s\n",    IDL_STRING_STR(&kw.path));
        }

        if (kw.ismdstree) {
            fprintf(stdout, "Mdstree Keyword Passed %s\n", IDL_STRING_STR(&kw.mdstree));
        }

        if (kw.ismdsnode) {
            fprintf(stdout, "Mdsnode Keyword Passed %s\n", IDL_STRING_STR(&kw.mdsnode));
        }

        if (kw.istpass) {
            fprintf(stdout, "Tpass Keyword Passed %s\n",   IDL_STRING_STR(&kw.tpass));
        }

        if (kw.isexp_number) {
            fprintf(stdout, "Shot Keyword: %d\n", (int)kw.exp_number);
        }

        if (kw.ispass) {
            fprintf(stdout, "Pass Keyword: %d\n", (int)kw.pass);
        }

        if (kw.ismdstreenum) {
            fprintf(stdout, "Mdstreenum Keyword: %d\n", (int)kw.mdstreenum);
        }

        if (kw.debug) {
            fprintf(stdout, "Debug Keyword Passed\n");
        }

        if (kw.verbose) {
            fprintf(stdout, "Verbose Keyword Passed\n");
        }

        if (kw.help) {
            fprintf(stdout, "Help Keyword Passed\n");
        }

        if (kw.istest) {
            fprintf(stdout, "Test API Requested, test %ld\n", (long)kw.testid);
        }

        if (kw.get_asis) {
            fprintf(stdout, "get_asis Passed\n");
        }

        if (kw.get_bad) {
            fprintf(stdout, "get_bad Passed\n");
        }

        if (kw.get_datadble) {
            fprintf(stdout, "get_datadble Passed\n");
        }

        if (kw.get_dimdble) {
            fprintf(stdout, "get_dimdble Passed\n");
        }

        if (kw.get_timedble) {
            fprintf(stdout, "get_timedble Passed\n");
        }

        if (kw.get_scalar) {
            fprintf(stdout, "get_scalar Passed\n");
        }

        if (kw.get_bytes) {
            fprintf(stdout, "get_bytes Passed\n");
        }

        if (kw.get_meta) {
            fprintf(stdout, "get_meta Passed\n");
        }

        if (kw.get_uncal) {
            fprintf(stdout, "get_uncal Passed\n");
        }

        if (kw.get_notoff) {
            fprintf(stdout, "get_notoff Passed\n");
        }

        if (kw.get_nodimdata) {
            fprintf(stdout, "get_nodimdata Passed\n");
        }
    }

    //-----------------------------------------------------------------------
    // Process Keywords (take Priority over Passed Arguments)

    if (kw.issignal) {
        signal = IDL_STRING_STR(&kw.signal);
    }

    device  = NULL;
    archive = NULL;
    server  = NULL;
    file    = NULL;
    format  = NULL;
    path    = NULL;
    mdstree = NULL;
    mdsnode = NULL;
    tpass   = NULL;

    if (kw.isdevice) {
        device  = IDL_STRING_STR(&kw.device);
    }

    if (kw.isarchive) {
        archive = IDL_STRING_STR(&kw.archive);
    }

    if (kw.isserver) {
        server  = IDL_STRING_STR(&kw.server);
    }

    if (kw.isfile) {
        file    = IDL_STRING_STR(&kw.file);
    }

    if (kw.isformat) {
        format  = IDL_STRING_STR(&kw.format);
    }

    if (kw.ispath) {
        path    = IDL_STRING_STR(&kw.path);
    }

    if (kw.ismdstree) {
        mdstree = IDL_STRING_STR(&kw.mdstree);
    }

    if (kw.ismdsnode) {
        mdsnode = IDL_STRING_STR(&kw.mdsnode);
    }

    if (kw.istpass) {
        tpass   = IDL_STRING_STR(&kw.tpass);
    }

    if (kw.isexp_number) {
        exp_number = kw.exp_number;
    }

    if (kw.ispass) {
        pass       = kw.pass;
    }

    if (kw.ismdstreenum) {
        mdstreenum = kw.mdstreenum;
    }

    if (kw.debug) {
        setIdamProperty("debug");    // Cannot be reset (currently!)
    }

    if (kw.verbose) {
        setIdamProperty("verbose");
    }

    if (kw.get_datadble) {
        setIdamProperty("get_datadble");    // Properties passed by Keyword must be reset to the prior state
    }

    if (kw.get_dimdble) {
        setIdamProperty("get_dimdble");
    }

    if (kw.get_timedble) {
        setIdamProperty("get_timedble");
    }

    if (kw.get_scalar) {
        setIdamProperty("get_scalar");
    }

    if (kw.get_bytes) {
        setIdamProperty("get_bytes");
    }

    if (kw.get_asis) {
        setIdamProperty("get_asis");
    }

    if (kw.get_bad) {
        setIdamProperty("get_bad");
    }

    if (kw.get_meta) {
        setIdamProperty("get_meta");
    }

    if (kw.get_uncal) {
        setIdamProperty("get_uncal");
    }

    if (kw.get_notoff) {
        setIdamProperty("get_notoff");
    }

    if (kw.get_nodimdata) {
        setIdamProperty("get_nodimdata");
    }

    //--------------------------------------------------------------------------
    // Call for HELP?

    if (kw.help) {
        userhelp(stdout, "getidam");
        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(0));
    }

    //--------------------------------------------------------------------------
    // Select the Appropriate API

    if (useIdamGetAPI) {
        if (kw.debug) {
            fprintf(stdout, "idamGetAPI args: ('%s','%s')\n", signal, source);
        }

        handle = idamGetAPI(signal, source);
    } else {
        if (pass == -1 && !kw.isarchive && !kw.isdevice && !kw.isserver && !kw.ismdstree && !kw.isfile) {
            if (kw.debug) {
                fprintf(stdout, "IDAM API args: ('%s', %ld)\n", signal, (long)exp_number);
            }

            if (!kw.istest) {
                handle = idamAPI(signal, (int)exp_number) ;
            } else {
                strncpy(testfile, signal, 3);
                testfile[3] = '\0';
                handle = idamClientTestAPI(testfile, signal, -1, (int)exp_number);
            }

        } else {

            // Generic API or Targeted File or MDS+ ?

            if (!kw.isserver && !kw.ismdstree && !kw.isfile) {
                if (!kw.isarchive) {
                    archive = defArchive;
                }

                if (!kw.isdevice) {
                    device  = defDevice;
                }

                if (kw.debug) {
                    fprintf(stdout, "IDAM Gen API args: ('%s','%s','%s',%ld,%ld)\n", archive, device, signal, (long)exp_number, (long)pass);
                }

                handle = idamGenAPI(archive, device, signal, (int)exp_number, (int)pass) ;
            } else {
                if (kw.isfile && kw.isformat) {
                    if (exp_number > 0 && !strcasecmp(format, "ida") && strlen(file) == 3) {
                        if (kw.debug) {
                            fprintf(stdout, "IDAM IDA API args: ('%s','%s',%ld,%ld)\n", file, signal, (long)pass, (long)exp_number);
                        }

                        if (!kw.istest) {
                            handle = idamClientAPI(file, signal, (int)pass, (int)exp_number);
                        } else {
                            handle = idamClientTestAPI(file, signal, (int)pass, (int)exp_number);
                        }
                    } else {
                        if (kw.debug) {
                            fprintf(stdout, "IDAM File API args: ('%s','%s','%s')\n", file, signal, format);
                        }

                        handle = idamClientFileAPI(file, signal, format);
                    }
                } else {
                    if (kw.isserver && kw.ismdstree && kw.ismdsnode) {
                        if (mdstreenum == -1) {
                            mdstreenum = exp_number;
                        }

                        //if(!ismdsnode)    mdsnode    = signal;
                        if (kw.debug) fprintf(stdout, "IDAM MDS API args: "
                                                  "('%s','%s','%s',%ld)\n", server, mdstree, mdsnode, (long)mdstreenum);

                        handle = idamClientMDS(server, mdstree, mdsnode, mdstreenum);
                    } else {
                        if (kw.verbose) {
                            fprintf(stdout, "No Appropriate IDAM API Identified\n");
                            fprintf(stdout, "Check Calling Arguments and Keywords\n");
                        }

                        IDL_KW_FREE;
                        restoreIdamProperties(cblock);
                        return (IDL_GettmpLong(GDE_NO_API_IDENTIFIED));
                    }
                }
            }
        }
    }

    if (kw.debug) {
        fprintf(stdout, "IDAM API Handle = %d\n", handle);
    }

    error_code = getIdamErrorCode(handle);
    error_msg  = (char *) getIdamErrorMsg(handle);

    if (handle < 0) {
        if (kw.verbose) {
            fprintf(stdout, "Bad Handle Returned from IDAM Call!\n");

            if (error_msg != NULL) {
                fprintf(stdout, "[%d] %s\n", error_code, error_msg);
            }
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_BAD_HANDLE));
    }

    rank   = getIdamRank(handle);
    order  = getIdamOrder(handle);
    status = getIdamDataStatus(handle);

    //--------------------------------------------------------------------------
    // Return Handle and High Level Information: Not Data - Separate Calls

    if ((sout = (IDAM_SIN *)malloc(sizeof(IDAM_SIN))) == NULL) {
        if (kw.verbose) {
            fprintf(stdout, "Heap Memory Allocation Failed!\n");
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_HEAP_ALLOC_ERROR));
    }

    //initSin(sout);

    sout->exp_number     = (IDL_LONG) exp_number;        // Use Default values
    sout->pass           = (IDL_LONG) pass;
    sout->mdstreenum     = (IDL_LONG) mdstreenum;

    sout->handle     = (IDL_LONG) handle;
    sout->rank       = (IDL_LONG) rank;
    sout->order      = (IDL_LONG) order;
    sout->status     = (IDL_LONG) status;
    sout->error_code     = (IDL_LONG) error_code;

    sout->get_datadble = (IDL_LONG)getIdamProperty("get_datadble");  // (IDL_LONG) kw.get_datadble;
    sout->get_dimdble  = (IDL_LONG)getIdamProperty("get_dimdble");   // (IDL_LONG) kw.get_dimdble;
    sout->get_timedble = (IDL_LONG)getIdamProperty("get_timedble");  // (IDL_LONG) kw.get_timedble;
    sout->get_scalar   = (IDL_LONG)getIdamProperty("get_scalar");    // (IDL_LONG) kw.get_scalar;
    sout->get_bytes    = (IDL_LONG)getIdamProperty("get_bytes");     // (IDL_LONG) kw.get_bytes;
    sout->get_asis     = (IDL_LONG)getIdamProperty("get_asis");      // (IDL_LONG) kw.get_asis;
    sout->get_bad      = (IDL_LONG)getIdamProperty("get_bad");       // (IDL_LONG) kw.get_bad;
    sout->get_meta     = (IDL_LONG)getIdamProperty("get_meta");      // (IDL_LONG) kw.get_meta;
    sout->get_uncal    = (IDL_LONG)getIdamProperty("get_uncal");     // (IDL_LONG) kw.get_uncal;
    sout->get_notoff   = (IDL_LONG)getIdamProperty("get_notoff");    // (IDL_LONG) kw.get_notoff;
    sout->get_nodimdata = (IDL_LONG)getIdamProperty("get_nodimdata"); // (IDL_LONG) kw.get_nodimdata;

    IDL_StrStore(&(sout->signal),  signal);
    IDL_StrStore(&(sout->source),  source);
    IDL_StrStore(&(sout->tpass),   tpass);
    IDL_StrStore(&(sout->device),  device);
    IDL_StrStore(&(sout->archive), archive);
    IDL_StrStore(&(sout->server),  server);
    IDL_StrStore(&(sout->file),    file);
    IDL_StrStore(&(sout->format),  format);
    IDL_StrStore(&(sout->path),    path);
    IDL_StrStore(&(sout->mdstree), mdstree);
    IDL_StrStore(&(sout->mdsnode), mdsnode);

    IDL_StrStore(&(sout->error_msg), error_msg);

    // Create an Anonymous IDL Structure and Import into IDL

    psDef = IDL_MakeStruct(NULL, pTags);

    ilDims[0] = 1;   // import Structure as a Single element Array
    ivReturn  = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR *)sout, freeMem, psDef);

    //--------------------------------------------------------------------------
    // If Users don't check their returned error codes or pointer values then a seg fault will occur...
    // This free will force users to make the check and code quality will improve!
    //
    // The exception is bad data ... users may want to look at this data by changing the get_bad property

    //if(sout->error_code != 0 && !(get_bad && getIdamDataStatus(sout->handle) == MIN_STATUS && sout->error_code == DATA_STATUS_BAD)){
    if (sout->error_code != 0 && !(getIdamDataStatus(sout->handle) == MIN_STATUS && sout->error_code == DATA_STATUS_BAD)) {

        if (kw.debug) {
            fprintf(stdout, "Freeing Heap for Handle %d\n", handle);
        }

        //fprintf(stdout,"get_bad = %d\n", get_bad);
        //fprintf(stdout,"get_bad = %d\n", cblock.get_bad);
        //fprintf(stdout,"get_bad = %d\n", getIdamProperty("get_bad"));

        idamFree(handle);
    }

    //--------------------------------------------------------------------------
    // Cleanup Keywords

    IDL_KW_FREE;
    restoreIdamProperties(cblock);
    return (ivReturn);
}

//#####################################################################################################

IDL_VPTR IDL_CDECL getidamdata(int argc, IDL_VPTR argv[], char * argk)
{
    //
    // Change History:
    //
    // 13 Feb 2006  D.G.Muir    Changes the Pointer reference to the block of
    //              memory immediately following the data structure:
    //              sout->data = (void *)sout+sizeof(IDAM_DOUT);
    // 02 Aug 2006 dgm  Data Errors added to Returned Data structure. To avoid
    //          complications, data errors are cast to the same type as
    //          the data themselves. This is not the case if the errors
    //          are accessed via other DLM functions.
    // 10Aug2006    dgm Error backed out - memory overwrites etc.
    //          Simpler to access via the getError accessor!
    // 31Jan2007    dgm Changed from single scalar value to array of scalars
    // 19Feb2007    dgm Allow access to data when an error is raised and the status is bad and
    //          bad data are requested.
    // 18Apr2007    dgm ilDims changed from type IDL_LONG to IDL_MEMINT
    //          dlen[] changed from type IDL_LONG to IDL_MEMINT
    //-------------------------------------------------------------------------

    int i, data_n, rank, ndata, errtype;

    CLIENT_BLOCK cblock = saveIdamProperties();  // preserve the current set of client properties
    CLIENT_BLOCK * idamcblock = NULL;
    int data_get_bad   = 0;
    int client_get_bad = getIdamProperty("get_bad");         // Current client get_bad property

    IDAM_SIN * sin = NULL;      // Input Structure
    IDAM_DOUT * sout = NULL;    // Returned Structure

    IDL_VPTR ivReturn = NULL;

    // Passed Structure

    void * psDef = NULL;
    //IDL_LONG ilDims[IDL_MAX_ARRAY_DIM];
    IDL_MEMINT ilDims[IDL_MAX_ARRAY_DIM];

    //static IDL_LONG dlen[] = {1,1,1,1,1,1,1,1};
    static IDL_MEMINT dlen[] = {1, 1, 1, 1, 1, 1, 1, 1}; // Rank 7 data Array: Default is Rank 1, Length 1

    // IDL tags structure (type Dependent)

    IDL_STRUCT_TAG_DEF pTagsFloat[] = {              // Single Precision
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DATA_TYPE",   0,  (void *) IDL_TYP_LONG},
        {"DATA_N",  0,  (void *) IDL_TYP_LONG},
        {"DATA_LABEL",  0,  (void *) IDL_TYP_STRING},
        {"DATA_UNITS",  0,  (void *) IDL_TYP_STRING},
        {"DATA_DESC",   0,  (void *) IDL_TYP_STRING},
        {"DATA",    dlen,   (void *) IDL_TYP_FLOAT},
        {0}
    };

    IDL_STRUCT_TAG_DEF pTagsDouble[] = {             // Double Precision
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DATA_TYPE",   0,  (void *) IDL_TYP_LONG},
        {"DATA_N",  0,  (void *) IDL_TYP_LONG},
        {"DATA_LABEL",  0,  (void *) IDL_TYP_STRING},
        {"DATA_UNITS",  0,  (void *) IDL_TYP_STRING},
        {"DATA_DESC",   0,  (void *) IDL_TYP_STRING},
        {"DATA",    dlen,   (void *) IDL_TYP_DOUBLE},
        {0}
    };

    IDL_STRUCT_TAG_DEF pTagsShort[] = {              // Short Integer
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DATA_TYPE",   0,  (void *) IDL_TYP_LONG},
        {"DATA_N",  0,  (void *) IDL_TYP_LONG},
        {"DATA_LABEL",  0,  (void *) IDL_TYP_STRING},
        {"DATA_UNITS",  0,  (void *) IDL_TYP_STRING},
        {"DATA_DESC",   0,  (void *) IDL_TYP_STRING},
        {"DATA",    dlen,   (void *) IDL_TYP_INT},
        {0}
    };

    IDL_STRUCT_TAG_DEF pTagsInt[] = {                // Standard Integer
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DATA_TYPE",   0,  (void *) IDL_TYP_LONG},
        {"DATA_N",  0,  (void *) IDL_TYP_LONG},
        {"DATA_LABEL",  0,  (void *) IDL_TYP_STRING},
        {"DATA_UNITS",  0,  (void *) IDL_TYP_STRING},
        {"DATA_DESC",   0,  (void *) IDL_TYP_STRING},
        {"DATA",    dlen,   (void *) IDL_TYP_LONG},
        {0}
    };

    IDL_STRUCT_TAG_DEF pTagsLong[] = {               // Double Integer
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DATA_TYPE",   0,  (void *) IDL_TYP_LONG},
        {"DATA_N",  0,  (void *) IDL_TYP_LONG},
        {"DATA_LABEL",  0,  (void *) IDL_TYP_STRING},
        {"DATA_UNITS",  0,  (void *) IDL_TYP_STRING},
        {"DATA_DESC",   0,  (void *) IDL_TYP_STRING},
        {"DATA",    dlen,   (void *) IDL_TYP_LONG64},
        {0}
    };

    IDL_STRUCT_TAG_DEF pTagsUnsignedShort[] = {          // Unsigned Short Integer
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DATA_TYPE",   0,  (void *) IDL_TYP_LONG},
        {"DATA_N",  0,  (void *) IDL_TYP_LONG},
        {"DATA_LABEL",  0,  (void *) IDL_TYP_STRING},
        {"DATA_UNITS",  0,  (void *) IDL_TYP_STRING},
        {"DATA_DESC",   0,  (void *) IDL_TYP_STRING},
        {"DATA",    dlen,   (void *) IDL_TYP_UINT},
        {0}
    };

    IDL_STRUCT_TAG_DEF pTagsUnsignedInt[] = {            // Unsigned Standard Integer
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DATA_TYPE",   0,  (void *) IDL_TYP_LONG},
        {"DATA_N",  0,  (void *) IDL_TYP_LONG},
        {"DATA_LABEL",  0,  (void *) IDL_TYP_STRING},
        {"DATA_UNITS",  0,  (void *) IDL_TYP_STRING},
        {"DATA_DESC",   0,  (void *) IDL_TYP_STRING},
        {"DATA",    dlen,   (void *) IDL_TYP_ULONG},
        {0}
    };

    IDL_STRUCT_TAG_DEF pTagsUnsignedLong[] = {           // Unsigned Double Integer
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DATA_TYPE",   0,  (void *) IDL_TYP_LONG},
        {"DATA_N",  0,  (void *) IDL_TYP_LONG},
        {"DATA_LABEL",  0,  (void *) IDL_TYP_STRING},
        {"DATA_UNITS",  0,  (void *) IDL_TYP_STRING},
        {"DATA_DESC",   0,  (void *) IDL_TYP_STRING},
        {"DATA",    dlen,   (void *) IDL_TYP_ULONG64},
        {0}
    };

    IDL_STRUCT_TAG_DEF pTagsChar[] = {               // Character array or String array
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DATA_TYPE",   0,  (void *) IDL_TYP_LONG},
        {"DATA_N",  0,  (void *) IDL_TYP_LONG},
        {"DATA_LABEL",  0,  (void *) IDL_TYP_STRING},
        {"DATA_UNITS",  0,  (void *) IDL_TYP_STRING},
        {"DATA_DESC",   0,  (void *) IDL_TYP_STRING},
        {"DATA",    dlen,   (void *) IDL_TYP_BYTE},
        {0}
    };

    IDL_STRUCT_TAG_DEF pTagsUnsignedChar[] = {           // Unsigned Character - No Specific IDL Type available
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DATA_TYPE",   0,  (void *) IDL_TYP_LONG},
        {"DATA_N",  0,  (void *) IDL_TYP_LONG},
        {"DATA_LABEL",  0,  (void *) IDL_TYP_STRING},
        {"DATA_UNITS",  0,  (void *) IDL_TYP_STRING},
        {"DATA_DESC",   0,  (void *) IDL_TYP_STRING},
        {"DATA",    dlen,   (void *) IDL_TYP_BYTE},
        {0}
    };

    // Keyword Structure

    typedef struct {
        IDL_KW_RESULT_FIRST_FIELD;
        IDL_LONG verbose;
        IDL_LONG debug;
        IDL_LONG help;
    } KW_RESULT;


    // Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {
        IDL_KW_FAST_SCAN,
        {"DEBUG",   IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
        {"HELP",    IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
        {"VERBOSE", IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
        {NULL}
    };

    KW_RESULT kw;

    kw.verbose   = 0;
    kw.debug     = 0;
    kw.help      = 0;

    //-----------------------------------------------------------------------
    // Check a Structure was Passed

    IDL_ENSURE_STRUCTURE(argv[0]);
    IDL_EXCLUDE_EXPR(argv[0]);

    //-----------------------------------------------------------------------
    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR *)0, 1, &kw);

    if (kw.debug) {
        if (kw.debug) {
            fprintf(stdout, "Debug Keyword Passed\n");
        }

        if (kw.verbose) {
            fprintf(stdout, "Verbose Keyword Passed\n");
        }

        if (kw.help) {
            fprintf(stdout, "Help Keyword Passed\n");
        }
    }

    //--------------------------------------------------------------------------
    // Call for HELP?

    if (kw.help) {
        userhelp(stdout, "getdata");
        IDL_KW_FREE;
        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------
    // Input Structure

    sin = (IDAM_SIN *)argv[0]->value.s.arr->data;        // Input Structure

    if (sin->handle < 0) {
        if (kw.verbose) {
            fprintf(stdout, "No Valid IDAM data Handle!\n");
        }

        IDL_KW_FREE;
        return (IDL_GettmpLong(GDE_NO_VALID_HANDLE));
    }

    if ((int)sin->get_bad) {                 // Reset on exit
        setIdamProperty("get_bad");
        client_get_bad = 1;
    }


    //---------------------------------------------------------------------------
    // Data Acquisition Properties, Keyword Properties and Client Property Settings

    if ((idamcblock = getIdamDataProperties((int)sin->handle)) != NULL) {
        data_get_bad = idamcblock->get_bad;
    } else {
        data_get_bad = (int)sin->get_bad;    // Client could change this value so don't trust!
    }

    if (sin->error_code != 0 && !((client_get_bad || data_get_bad) && getIdamDataStatus(sin->handle) == MIN_STATUS && sin->error_code == DATA_STATUS_BAD)) {
        //fprintf(stdout,"error_code     = %d\n", sin->error_code);
        //fprintf(stdout,"status         = %d\n", getIdamDataStatus(sin->handle));
        //fprintf(stdout,"client_get_bad = %d\n", client_get_bad);
        //fprintf(stdout,"data_get_bad   = %d\n", data_get_bad);
        if (kw.verbose) {
            fprintf(stdout, "Data has a Raised Error Status!\n");
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_DATA_HAS_ERROR));
    }

    //---------------------------------------------------------------------------
    // Check Status and BAD Property

    //fprintf(stdout,"get_bad Structure Passed: %d\n", sin->get_bad);
    //fprintf(stdout,"data's get_bad          : %d\n", data_get_bad);
    //fprintf(stdout,"get_bad Client value    : %d\n", client_get_bad);

    if ((client_get_bad || data_get_bad) && getIdamDataStatus(sin->handle) > MIN_STATUS) {
        if (kw.verbose) {
            fprintf(stdout, "Data don't have a BAD Status but GET_BAD property set - Access to data blocked!\n");
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_NO_DATA_TO_RETURN));
    }

    //--------------------------------------------------------------------------
    // Prepare return Structure: Size is Base Structure + Data Array Length
    // Specifiy the Data Array Organisation

    rank   = getIdamRank(sin->handle);
    data_n = getIdamDataNum(sin->handle);
    ndata  = data_n;

    //fprintf(stdout,"getIdamData: Rank = %d\n", rank);
    //fprintf(stdout,"             N    = %d\n", data_n);
    //printClientBlock(stdout,Data_Block[sin->handle].client_block);
    //if(Data_Block[sin->handle].data == NULL) fprintf(stdout,"Data Block #0 is NULL!!!\n");

    if (getIdamData(sin->handle) == NULL) {
        if (kw.verbose) {
            fprintf(stdout, "Data Block Pointer is NULL!!!\n");
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_NO_DATA_TO_RETURN));
    }

    if (rank == 0 && data_n == 0) {
        if (kw.verbose) {
            fprintf(stdout, "No Data to Return!\n");
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_NO_DATA_TO_RETURN));
    }

    if (rank > 7) {
        if (kw.verbose) {
            fprintf(stdout, "Rank Too High ... 7 is Maximum!\n");
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_RANK_TOO_HIGH));
    }

    if (rank > 0) {
        dlen[0] = rank;

        for (i = 1; i <= rank; i++) {
            dlen[i] = getIdamDimNum(sin->handle, i - 1); // How the Data Array is Packed (Shape)
        }

        if (kw.debug) {
            fprintf(stdout, "Data Organisation\n");

            for (i = 0; i <= rank; i++) {
                fprintf(stdout, "[%d]  %d  \n", i, (int)dlen[i]);
            }
        }
    } else {
        dlen[0] = 1;
        dlen[1] = ndata;  // Declared as STATIC so Always re-define at run-time

        /* 31Jan2007    dgm
              dlen[1] = 1;  // Declared as STATIC so Always re-define at run-time
        */
        if (kw.debug) {
            fprintf(stdout, "Data Organisation (Rank 0 => Scalar Value)\n");
            fprintf(stdout, "[0]  %d  \n", (int)dlen[0]);
            fprintf(stdout, "[1]  %d  \n", (int)dlen[1]);
        }
    }

    switch (getIdamDataType(sin->handle)) {

    case TYPE_FLOAT:

        if (kw.debug) {
            fprintf(stdout, "Data are of Type FLOAT\n");
        }

        sout = (IDAM_DOUT *)malloc(sizeof(IDAM_DOUT) + sizeof(float) * ndata);

        if (sout == NULL) {
            break;
        }

        // This contiguous block of memory is used so that a single free releases the heap - simpler!

        memcpy((void *)&sout->data, (void *)getIdamData(sin->handle), (size_t)ndata * sizeof(float));
        psDef = IDL_MakeStruct(NULL, pTagsFloat);
        break;

    case TYPE_DOUBLE:
        if (kw.debug) {
            fprintf(stdout, "Data is of Type DOUBLE\n");
        }

        sout  = (IDAM_DOUT *)malloc(sizeof(IDAM_DOUT) + sizeof(double) * ndata);

        if (sout == NULL) {
            break;
        }

        memcpy((void *)&sout->data, (void *)getIdamData(sin->handle), (size_t)ndata * sizeof(double));
        psDef = IDL_MakeStruct(NULL, pTagsDouble);
        break;

    case TYPE_SHORT:
        sout  = (IDAM_DOUT *)malloc(sizeof(IDAM_DOUT) + sizeof(short) * ndata);

        if (sout == NULL) {
            break;
        }

        memcpy((void *)&sout->data, (void *)getIdamData(sin->handle), (size_t)ndata * sizeof(short));
        psDef = IDL_MakeStruct(NULL, pTagsShort);
        break;

    case TYPE_INT:
        sout  = (IDAM_DOUT *)malloc(sizeof(IDAM_DOUT) + sizeof(int) * ndata);

        if (sout == NULL) {
            break;
        }

        memcpy((void *)&sout->data, (void *)getIdamData(sin->handle), (size_t)ndata * sizeof(int));
        psDef = IDL_MakeStruct(NULL, pTagsInt);
        break;

    case TYPE_LONG:
        sout = (IDAM_DOUT *)malloc(sizeof(IDAM_DOUT) + sizeof(long) * ndata);

        if (sout == NULL) {
            break;
        }

        memcpy((void *)&sout->data, (void *)getIdamData(sin->handle), (size_t)ndata * sizeof(long));
        psDef = IDL_MakeStruct(NULL, pTagsLong);
        break;

    case TYPE_UNSIGNED_SHORT:
        sout  = (IDAM_DOUT *)malloc(sizeof(IDAM_DOUT) + sizeof(unsigned short) * ndata);

        if (sout == NULL) {
            break;
        }

        memcpy((void *)&sout->data, (void *)getIdamData(sin->handle), (size_t)ndata * sizeof(unsigned short));
        psDef = IDL_MakeStruct(NULL, pTagsUnsignedShort);
        break;

    case TYPE_UNSIGNED:
        sout = (IDAM_DOUT *)malloc(sizeof(IDAM_DOUT) + sizeof(unsigned int) * ndata);

        if (sout == NULL) {
            break;
        }

        memcpy((void *)&sout->data, (void *)getIdamData(sin->handle), (size_t)ndata * sizeof(unsigned int));
        psDef = IDL_MakeStruct(NULL, pTagsUnsignedInt);
        break;

    case TYPE_UNSIGNED_LONG:
        sout = (IDAM_DOUT *)malloc(sizeof(IDAM_DOUT) + sizeof(unsigned long) * ndata);

        if (sout == NULL) {
            break;
        }

        memcpy((void *)&sout->data, (void *)getIdamData(sin->handle), (size_t)ndata * sizeof(unsigned long));
        psDef = IDL_MakeStruct(NULL, pTagsUnsignedLong);
        break;

    case TYPE_CHAR:
        sout  = (IDAM_DOUT *)malloc(sizeof(IDAM_DOUT) + sizeof(char) * ndata);

        if (sout == NULL) {
            break;
        }

        memcpy((void *)&sout->data, (void *)getIdamData(sin->handle), (size_t)ndata * sizeof(char));
        psDef = IDL_MakeStruct(NULL, pTagsChar);
        break;

    case TYPE_UNSIGNED_CHAR:
        sout  = (IDAM_DOUT *)malloc(sizeof(IDAM_DOUT) + sizeof(unsigned char) * ndata);

        if (sout == NULL) {
            break;
        }

        memcpy((void *)&sout->data, (void *)getIdamData(sin->handle), (size_t)ndata * sizeof(unsigned char));
        psDef = IDL_MakeStruct(NULL, pTagsUnsignedChar);
        break;

    case TYPE_STRING:
        sout  = (IDAM_DOUT *)malloc(sizeof(IDAM_DOUT) + sizeof(char) * ndata);

        if (sout == NULL) {
            break;
        }

        memcpy((void *)&sout->data, (void *)getIdamData(sin->handle), (size_t)ndata * sizeof(char));
        psDef = IDL_MakeStruct(NULL, pTagsChar);

        //printf("ndata = %d\n", ndata);
        //printf("rank = %d\n", rank);
        //for(i=0;i<=rank;i++)printf("dlen[%d] = %d\n", i, dlen[i]);
        //fflush(NULL);

        break;


    default:
        break;

        if (kw.verbose) {
            fprintf(stdout, "The Data Type is Not Recognised [%d]\n", errtype);
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_UNKNOWN_DATA_TYPE));
    }

    if (sout == NULL) {
        if (kw.verbose) {
            fprintf(stdout, "Heap Memory Allocation Failed!\n");
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_HEAP_ALLOC_ERROR));
    }

    sout->handle    = (IDL_LONG) sin->handle;
    sout->data_type = (IDL_LONG) getIdamDataType(sin->handle);
    sout->data_n    = (IDL_LONG) data_n;

    IDL_StrStore(&(sout->data_label),  (char *) getIdamDataLabel(sin->handle));
    IDL_StrStore(&(sout->data_units),  (char *) getIdamDataUnits(sin->handle));
    IDL_StrStore(&(sout->data_desc),   (char *) getIdamDataDesc(sin->handle));

    if (kw.debug) {
        fprintf(stdout, "Labels copied to IDL Strings\n");
    }

    // Create an Anonymous IDL Structure and Import into IDL

    ilDims[0] = 1;   // import Structure as a Single element Array

    ivReturn = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR *)sout, freeMem, psDef);

    if (kw.debug) {
        fprintf(stdout, "IDL Structure Created: Ready for Return\n");
    }

    //--------------------------------------------------------------------------
    // Cleanup Keywords and IDAM Heap

    IDL_KW_FREE;
    restoreIdamProperties(cblock);
    return (ivReturn);

}

//#####################################################################################################

IDL_VPTR IDL_CDECL getdataarray(int argc, IDL_VPTR argv[], char * argk)
{
    //
    // Return the Data as an Array - Not as a Structure
    //
    // Change History:
    //
    // 19Feb2007    dgm Allow access to data when an error is raised and the status is bad and
    //          bad data are requested.
    //-------------------------------------------------------------------------

    int ndims, ndata;
    IDL_MEMINT ilDims[1];

    IDAM_SIN * sin;              // Returned Structure

    CLIENT_BLOCK cblock = saveIdamProperties();  // preserve the current set of client properties
    CLIENT_BLOCK * idamcblock = NULL;
    int data_get_bad   = 0;
    int client_get_bad = getIdamProperty("get_bad");         // Current client get_bad property

    char * dvec;
    IDL_VPTR idlArray = NULL;

    // Keyword Structure

    typedef struct {
        IDL_KW_RESULT_FIRST_FIELD;
        IDL_LONG verbose;
        IDL_LONG debug;
        IDL_LONG help;
    } KW_RESULT;

    // Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {
        IDL_KW_FAST_SCAN,
        {"DEBUG",   IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
        {"HELP",    IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
        {"VERBOSE", IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
        {NULL}
    };

    KW_RESULT kw;

    kw.verbose   = 0;
    kw.debug     = 0;
    kw.help      = 0;

    //-----------------------------------------------------------------------
    // Check a Structure was Passed

    IDL_ENSURE_STRUCTURE(argv[0]);
    IDL_EXCLUDE_EXPR(argv[0]);

    //-----------------------------------------------------------------------
    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR *)0, 1, &kw);

    //--------------------------------------------------------------------------
    // Call for HELP?

    if (kw.help) {
        userhelp(stdout, "getdataarray");
        IDL_KW_FREE;
        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------
    // Input Structure

    sin = (IDAM_SIN *)argv[0]->value.s.arr->data;        // Input Structure

    if ((int)sin->get_bad) {                 // Reset on exit
        setIdamProperty("get_bad");
        client_get_bad = 1;
    }

    if (sin->handle < 0) {
        if (kw.verbose) {
            fprintf(stdout, "No Valid IDAM data Handle!\n");
        }

        IDL_KW_FREE;
        return (IDL_GettmpLong(GDE_NO_VALID_HANDLE));
    }

    //---------------------------------------------------------------------------
    // Data Acquisition Properties, Keyword Properties and Client Property Settings

    if ((idamcblock = getIdamDataProperties((int)sin->handle)) != NULL) {
        data_get_bad = idamcblock->get_bad;
    } else {
        data_get_bad = (int)sin->get_bad;    // Client could change this value so don't trust!
    }

    if (sin->error_code != 0 && !((client_get_bad || data_get_bad) && getIdamDataStatus(sin->handle) == MIN_STATUS && sin->error_code == DATA_STATUS_BAD)) {
        if (kw.verbose) {
            fprintf(stdout, "Data has a Raised Error Status!\n");
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_DATA_HAS_ERROR));
    }

    //---------------------------------------------------------------------------
    // Check Status and BAD Property

    if ((client_get_bad || data_get_bad) && getIdamDataStatus(sin->handle) > MIN_STATUS) {
        if (kw.verbose) {
            fprintf(stdout, "Data don't have a BAD Status but GET_BAD property set - Access to data blocked!\n");
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_NO_DATA_TO_RETURN));
    }

    //--------------------------------------------------------------------------
    // Return Data

    ndims     = 1;
    ndata     = getIdamDataNum(sin->handle);
    ilDims[0] = (IDL_MEMINT) ndata;

    if (kw.debug) {
        fprintf(stdout, "No of Elements in the First Dimension = %d\n", (int)ilDims[0]);
    }

    // Allocate Heap Memory for the IDL Array and Copy the Data to it.

    switch (getIdamDataType(sin->handle)) {

    case TYPE_FLOAT:
        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_FLOAT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamData(sin->handle), (size_t)ndata * sizeof(float));
        break;

    case TYPE_DOUBLE:
        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_DOUBLE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamData(sin->handle), (size_t)ndata * sizeof(double));
        break;

    case TYPE_SHORT:
        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_INT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamData(sin->handle), (size_t)ndata * sizeof(short));
        break;

    case TYPE_INT:
        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_LONG, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamData(sin->handle), (size_t)ndata * sizeof(int));
        break;

    case TYPE_LONG:
        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_LONG64, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamData(sin->handle), (size_t)ndata * sizeof(long));
        break;

    case TYPE_UNSIGNED_SHORT:
        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_UINT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamData(sin->handle), (size_t)ndata * sizeof(unsigned short));
        break;

    case TYPE_UNSIGNED:
        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_ULONG, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamData(sin->handle), (size_t)ndata * sizeof(unsigned int));
        break;

    case TYPE_UNSIGNED_LONG:
        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_ULONG64, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamData(sin->handle), (size_t)ndata * sizeof(unsigned long));
        break;

    case TYPE_CHAR:
        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_BYTE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamData(sin->handle), (size_t)ndata * sizeof(char));
        break;

    case TYPE_UNSIGNED_CHAR:
        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_BYTE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamData(sin->handle), (size_t)ndata * sizeof(unsigned char));
        break;

    default:
        break;

        if (kw.verbose) {
            fprintf(stdout, "The Data Type is Not Recognised [%d]\n", getIdamDataType(sin->handle));
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_UNKNOWN_DATA_TYPE));

    }

    //--------------------------------------------------------------------------
    // Cleanup Keywords

    IDL_KW_FREE;
    restoreIdamProperties(cblock);
    return (idlArray);
}


//#####################################################################################################

IDL_VPTR IDL_CDECL geterrorarray(int argc, IDL_VPTR argv[], char * argk)
{
    //
    // Return the Data Errors as an Array - Not as a Structure
    //
    // Change History:
    //
    // 19Feb2007    dgm Allow access to data when an error is raised and the status is bad and
    //          bad data are requested.
    //-------------------------------------------------------------------------

    int ndims, ndata;
    IDL_MEMINT ilDims[1];
    char * ep;

    IDAM_SIN * sin;      // Returned Structure

    CLIENT_BLOCK cblock = saveIdamProperties();  //preserve the current set of client properties
    CLIENT_BLOCK * idamcblock = NULL;
    int data_get_bad   = 0;
    int client_get_bad = getIdamProperty("get_bad");         // Current client get_bad property

    char * dvec;
    IDL_VPTR idlArray = NULL;

    // Keyword Structure

    typedef struct {
        IDL_KW_RESULT_FIRST_FIELD;
        IDL_LONG verbose;
        IDL_LONG debug;
        IDL_LONG help;
    } KW_RESULT;

    // Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {
        IDL_KW_FAST_SCAN,
        {"DEBUG",   IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
        {"HELP",    IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
        {"VERBOSE", IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
        {NULL}
    };

    KW_RESULT kw;

    kw.verbose   = 0;
    kw.debug     = 0;
    kw.help      = 0;

    //-----------------------------------------------------------------------
    // Check a Structure was Passed

    IDL_ENSURE_STRUCTURE(argv[0]);
    IDL_EXCLUDE_EXPR(argv[0]);

    //-----------------------------------------------------------------------
    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR *)0, 1, &kw);

    //--------------------------------------------------------------------------
    // Call for HELP?

    if (kw.help) {
        userhelp(stdout, "geterrorarray");
        IDL_KW_FREE;
        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------
    // Input Structure

    sin = (IDAM_SIN *)argv[0]->value.s.arr->data;        // Input Structure

    if ((int)sin->get_bad) {                 // Reset on exit
        setIdamProperty("get_bad");
        client_get_bad = 1;
    }

    if (sin->handle < 0) {
        if (kw.verbose) {
            fprintf(stdout, "No Valid IDAM data Handle!\n");
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_NO_VALID_HANDLE));
    }

    //---------------------------------------------------------------------------
    // Data Acquisition Properties, Keyword Properties and Client Property Settings

    if ((idamcblock = getIdamDataProperties((int)sin->handle)) != NULL) {
        data_get_bad = idamcblock->get_bad;
    } else {
        data_get_bad = (int)sin->get_bad;    // Client could change this value so don't trust!
    }

    if (sin->error_code != 0 && !((client_get_bad || data_get_bad) && getIdamDataStatus(sin->handle) == MIN_STATUS && sin->error_code == DATA_STATUS_BAD)) {
        if (kw.verbose) {
            fprintf(stdout, "Data has a Raised Error Status!\n");
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_DATA_HAS_ERROR));
    }

    //---------------------------------------------------------------------------
    // Check Status and BAD Property

    if ((client_get_bad || data_get_bad) && getIdamDataStatus(sin->handle) > MIN_STATUS) {
        if (kw.verbose) {
            fprintf(stdout, "Data don't have a BAD Status but GET_BAD property set - Access to data blocked!\n");
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_NO_DATA_TO_RETURN));
    }

    //--------------------------------------------------------------------------
    // Return Data

    ep    = (char *) getIdamError(sin->handle);
    ndata = getIdamDataNum(sin->handle);

    ndims     = 1;
    ilDims[0] = (IDL_MEMINT) ndata;

    if (kw.debug) {
        fprintf(stdout, "No of Elements in the First Dimension = %d\n", (int)ilDims[0]);
    }

    // Allocate Heap Memory for the IDL Array and Copy the Data to it.

    switch (getIdamErrorType(sin->handle)) {

    case TYPE_FLOAT:
        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_FLOAT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(float));
        break;

    case TYPE_DOUBLE:
        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_DOUBLE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(double));
        break;

    case TYPE_SHORT:
        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_INT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(short));
        break;

    case TYPE_INT:
        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_LONG, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(int));
        break;

    case TYPE_LONG:
        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_LONG64, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(long));
        break;

    case TYPE_UNSIGNED_SHORT:
        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_UINT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(unsigned short));
        break;

    case TYPE_UNSIGNED:
        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_ULONG, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(unsigned int));
        break;

    case TYPE_UNSIGNED_LONG:
        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_ULONG64, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(unsigned long));
        break;

    case TYPE_CHAR:
        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_BYTE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(char));
        break;

    case TYPE_UNSIGNED_CHAR:
        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_BYTE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(unsigned char));
        break;

    default:
        break;

        if (kw.verbose) {
            fprintf(stdout, "The Data Error Type is Not Recognised [%d]\n", getIdamErrorType(sin->handle));
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_UNKNOWN_DATA_TYPE));

    }

    //--------------------------------------------------------------------------
    // Cleanup Keywords

    IDL_KW_FREE;
    restoreIdamProperties(cblock);
    return (idlArray);
}


//#####################################################################################################

IDL_VPTR IDL_CDECL getidamdimdata(int argc, IDL_VPTR argv[], char * argk)
{
    //
    //
    // Change History:
    //
    // 13 Feb 2006  D.G.Muir    Changes the Pointer reference to the block of
    //              memory immediately following the data structure:
    //              sout->dim = (void *)sout+sizeof(IDAM_DIMOUT);
    // 19Feb2007    dgm Allow access to data when an error is raised and the status is bad and
    //          bad data are requested.
    // 18Apr2007    dgm ilDims changed from type IDL_LONG to IDL_MEMINT
    //          dlen[] changed from type IDL_LONG to IDL_MEMINT
    //-------------------------------------------------------------------------

    int dimid, ndata;

    IDAM_SIN * sin = NULL;          // Input Structure
    IDAM_DIMOUT * sout = NULL;      // Returned Structure

    CLIENT_BLOCK cblock = saveIdamProperties();  // preserve the current set of client properties
    CLIENT_BLOCK * idamcblock = NULL;
    int data_get_bad   = 0;
    int client_get_bad = getIdamProperty("get_bad");         // Current client get_bad property

    IDL_VPTR ivReturn = NULL;

    // Passed Structure

    void * psDef = NULL;
    //IDL_LONG ilDims[IDL_MAX_ARRAY_DIM];
    IDL_MEMINT ilDims[IDL_MAX_ARRAY_DIM];

    //static IDL_LONG dlen[] = {1,1};
    static IDL_MEMINT dlen[] = {1, 1};

    // IDL tags structure (type Dependent)

    IDL_STRUCT_TAG_DEF pTagsFloat[] = {              // Single Precision
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DIM_ID",  0,  (void *) IDL_TYP_LONG},
        {"DIM_TYPE",    0,  (void *) IDL_TYP_LONG},
        {"DIM_N",   0,  (void *) IDL_TYP_LONG},
        {"DIM_LABEL",   0,  (void *) IDL_TYP_STRING},
        {"DIM_UNITS",   0,  (void *) IDL_TYP_STRING},
        {"DIM",         dlen,   (void *) IDL_TYP_FLOAT},
        //{"DIM_ERRORS",elen,   (void *) IDL_TYP_FLOAT},
        {0}
    };

    IDL_STRUCT_TAG_DEF pTagsDouble[] = {
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DIM_ID",  0,  (void *) IDL_TYP_LONG},
        {"DIM_TYPE",    0,  (void *) IDL_TYP_LONG},
        {"DIM_N",   0,  (void *) IDL_TYP_LONG},
        {"DIM_LABEL",   0,  (void *) IDL_TYP_STRING},
        {"DIM_UNITS",   0,  (void *) IDL_TYP_STRING},
        {"DIM",         dlen,   (void *) IDL_TYP_DOUBLE},
        //{"DIM_ERRORS",elen,   (void *) IDL_TYP_DOUBLE},
        {0}
    };

    IDL_STRUCT_TAG_DEF pTagsShort[] = {
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DIM_ID",  0,  (void *) IDL_TYP_LONG},
        {"DIM_TYPE",    0,  (void *) IDL_TYP_LONG},
        {"DIM_N",   0,  (void *) IDL_TYP_LONG},
        {"DIM_LABEL",   0,  (void *) IDL_TYP_STRING},
        {"DIM_UNITS",   0,  (void *) IDL_TYP_STRING},
        {"DIM",         dlen,   (void *) IDL_TYP_INT},
        //{"DIM_ERRORS",elen,   (void *) IDL_TYP_INT},
        {0}
    };

    IDL_STRUCT_TAG_DEF pTagsInt[] = {
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DIM_ID",  0,  (void *) IDL_TYP_LONG},
        {"DIM_TYPE",    0,  (void *) IDL_TYP_LONG},
        {"DIM_N",   0,  (void *) IDL_TYP_LONG},
        {"DIM_LABEL",   0,  (void *) IDL_TYP_STRING},
        {"DIM_UNITS",   0,  (void *) IDL_TYP_STRING},
        {"DIM",         dlen,   (void *) IDL_TYP_LONG},
        //{"DIM_ERRORS",elen,   (void *) IDL_TYP_LONG},
        {0}
    };

    IDL_STRUCT_TAG_DEF pTagsLong[] = {
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DIM_ID",  0,  (void *) IDL_TYP_LONG},
        {"DIM_TYPE",    0,  (void *) IDL_TYP_LONG},
        {"DIM_N",   0,  (void *) IDL_TYP_LONG},
        {"DIM_LABEL",   0,  (void *) IDL_TYP_STRING},
        {"DIM_UNITS",   0,  (void *) IDL_TYP_STRING},
        {"DIM",         dlen,   (void *) IDL_TYP_LONG64},
        //{"DIM_ERRORS",elen,   (void *) IDL_TYP_LONG64},
        {0}
    };

    IDL_STRUCT_TAG_DEF pTagsUnsignedShort[] = {
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DIM_ID",  0,  (void *) IDL_TYP_LONG},
        {"DIM_TYPE",    0,  (void *) IDL_TYP_LONG},
        {"DIM_N",   0,  (void *) IDL_TYP_LONG},
        {"DIM_LABEL",   0,  (void *) IDL_TYP_STRING},
        {"DIM_UNITS",   0,  (void *) IDL_TYP_STRING},
        {"DIM",         dlen,   (void *) IDL_TYP_UINT},
        //{"DIM_ERRORS",elen,   (void *) IDL_TYP_UINT},
        {0}
    };
    IDL_STRUCT_TAG_DEF pTagsUnsignedInt[] = {
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DIM_ID",  0,  (void *) IDL_TYP_LONG},
        {"DIM_TYPE",    0,  (void *) IDL_TYP_LONG},
        {"DIM_N",   0,  (void *) IDL_TYP_LONG},
        {"DIM_LABEL",   0,  (void *) IDL_TYP_STRING},
        {"DIM_UNITS",   0,  (void *) IDL_TYP_STRING},
        {"DIM",         dlen,   (void *) IDL_TYP_ULONG},
        //{"DIM_ERRORS",elen,   (void *) IDL_TYP_ULONG},
        {0}
    };

    IDL_STRUCT_TAG_DEF pTagsUnsignedLong[] = {
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DIM_ID",  0,  (void *) IDL_TYP_LONG},
        {"DIM_TYPE",    0,  (void *) IDL_TYP_LONG},
        {"DIM_N",   0,  (void *) IDL_TYP_LONG},
        {"DIM_LABEL",   0,  (void *) IDL_TYP_STRING},
        {"DIM_UNITS",   0,  (void *) IDL_TYP_STRING},
        {"DIM",         dlen,   (void *) IDL_TYP_ULONG64},
        //{"DIM_ERRORS",elen,   (void *) IDL_TYP_ULONG64},
        {0}
    };

    IDL_STRUCT_TAG_DEF pTagsChar[] = {
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DIM_ID",  0,  (void *) IDL_TYP_LONG},
        {"DIM_TYPE",    0,  (void *) IDL_TYP_LONG},
        {"DIM_N",   0,  (void *) IDL_TYP_LONG},
        {"DIM_LABEL",   0,  (void *) IDL_TYP_STRING},
        {"DIM_UNITS",   0,  (void *) IDL_TYP_STRING},
        {"DIM",         dlen,   (void *) IDL_TYP_BYTE},
        //{"DIM_ERRORS",elen,   (void *) IDL_TYP_BYTE},
        {0}
    };

    IDL_STRUCT_TAG_DEF pTagsUnsignedChar[] = {
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DIM_ID",  0,  (void *) IDL_TYP_LONG},
        {"DIM_TYPE",    0,  (void *) IDL_TYP_LONG},
        {"DIM_N",   0,  (void *) IDL_TYP_LONG},
        {"DIM_LABEL",   0,  (void *) IDL_TYP_STRING},
        {"DIM_UNITS",   0,  (void *) IDL_TYP_STRING},
        {"DIM",         dlen,   (void *) IDL_TYP_BYTE},
        //{"DIM_ERRORS",elen,   (void *) IDL_TYP_BYTE},
        {0}
    };

    // Keyword Structure

    typedef struct {
        IDL_KW_RESULT_FIRST_FIELD;
        IDL_LONG verbose;
        IDL_LONG debug;
        IDL_LONG help;
    } KW_RESULT;


    // Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {
        IDL_KW_FAST_SCAN,
        {"DEBUG",   IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
        {"HELP",    IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
        {"VERBOSE", IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
        {NULL}
    };

    KW_RESULT kw;

    kw.verbose   = 0;
    kw.debug     = 0;
    kw.help      = 0;

    //-----------------------------------------------------------------------
    // Check a Structure was Passed

    IDL_ENSURE_SCALAR(argv[1]);

    IDL_ENSURE_STRUCTURE(argv[0]);
    IDL_EXCLUDE_EXPR(argv[0]);

    //-----------------------------------------------------------------------
    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR *)0, 1, &kw);

    if (kw.debug) {
        if (kw.debug) {
            fprintf(stdout, "Debug Keyword Passed\n");
        }

        if (kw.verbose) {
            fprintf(stdout, "Verbose Keyword Passed\n");
        }

        if (kw.help) {
            fprintf(stdout, "Help Keyword Passed\n");
        }
    }

    //--------------------------------------------------------------------------
    // Call for HELP?

    if (kw.help) {
        userhelp(stdout, "getdimdata");
        IDL_KW_FREE;
        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------
    // Input Structure

    sin   = (IDAM_SIN *)argv[0]->value.s.arr->data;      // Input Structure
    dimid = IDL_LongScalar(argv[1]);

    if ((int)sin->get_bad) {                 // Reset on exit
        setIdamProperty("get_bad");
        client_get_bad = 1;
    }

    if (sin->handle < 0) {
        if (kw.verbose) {
            fprintf(stdout, "No Valid IDAM data Handle!\n");
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_NO_VALID_HANDLE));
    }

    //---------------------------------------------------------------------------
    // Data Acquisition Properties, Keyword Properties and Client Property Settings

    if ((idamcblock = getIdamDataProperties((int)sin->handle)) != NULL) {
        data_get_bad = idamcblock->get_bad;
    } else {
        data_get_bad = (int)sin->get_bad;    // Client could change this value so don't trust!
    }

    if (sin->error_code != 0 && !((client_get_bad || data_get_bad) && getIdamDataStatus(sin->handle) == MIN_STATUS && sin->error_code == DATA_STATUS_BAD)) {
        if (kw.verbose) {
            fprintf(stdout, "Data has a Raised Error Status!\n");
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_DATA_HAS_ERROR));
    }

    //---------------------------------------------------------------------------
    // Check Status and BAD Property

    if ((client_get_bad || data_get_bad) && getIdamDataStatus(sin->handle) > MIN_STATUS) {
        if (kw.verbose) {
            fprintf(stdout, "Data don't have a BAD Status but GET_BAD property set - Access to data blocked!\n");
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_NO_DATA_TO_RETURN));
    }

    //---------------------------------------------------------------------------
    // Check Rank

    if (dimid < 0 || dimid >= getIdamRank(sin->handle)) {
        if (kw.verbose) {
            fprintf(stdout, "Dimension is Inconsistent with the Data's Rank!\n");
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_NO_SUCH_DIMENSION));
    }

    //--------------------------------------------------------------------------
    // Prepare return Structure: Size is Base Structure + Data Array Length

    ndata   = getIdamDimNum(sin->handle, dimid);
    dlen[1] = ndata              ; // Modify the Structure Definition prior to Allocation

    switch (getIdamDimType(sin->handle, dimid)) {

    case TYPE_FLOAT:
        sout  = (IDAM_DIMOUT *)malloc(sizeof(IDAM_DIMOUT) + sizeof(float) * ndata);

        if (sout == NULL) {
            break;
        }

        memcpy((void *)&sout->dim, (void *)getIdamDimData(sin->handle, dimid), (size_t)ndata * sizeof(float));
        psDef = IDL_MakeStruct(NULL, pTagsFloat);
        break;

    case TYPE_DOUBLE:
        sout  = (IDAM_DIMOUT *)malloc(sizeof(IDAM_DIMOUT) + sizeof(double) * ndata);

        if (sout == NULL) {
            break;
        }

        memcpy((void *)&sout->dim, (void *)getIdamDimData(sin->handle, dimid), (size_t)ndata * sizeof(double));
        psDef = IDL_MakeStruct(NULL, pTagsDouble);
        break;

    case TYPE_SHORT:
        sout  = (IDAM_DIMOUT *)malloc(sizeof(IDAM_DIMOUT) + sizeof(short) * ndata);

        if (sout == NULL) {
            break;
        }

        memcpy((void *)&sout->dim, (void *)getIdamDimData(sin->handle, dimid), (size_t)ndata * sizeof(short));
        psDef = IDL_MakeStruct(NULL, pTagsShort);
        break;

    case TYPE_INT:
        sout  = (IDAM_DIMOUT *)malloc(sizeof(IDAM_DIMOUT) + sizeof(int) * ndata);

        if (sout == NULL) {
            break;
        }

        memcpy((void *)&sout->dim, (void *)getIdamDimData(sin->handle, dimid), (size_t)ndata * sizeof(int));
        psDef = IDL_MakeStruct(NULL, pTagsInt);
        break;

    case TYPE_LONG:
        sout  = (IDAM_DIMOUT *)malloc(sizeof(IDAM_DIMOUT) + sizeof(long) * ndata);

        if (sout == NULL) {
            break;
        }

        memcpy((void *)&sout->dim, (void *)getIdamDimData(sin->handle, dimid), (size_t)ndata * sizeof(long));
        psDef = IDL_MakeStruct(NULL, pTagsLong);
        break;

    case TYPE_UNSIGNED_SHORT:
        sout  = (IDAM_DIMOUT *)malloc(sizeof(IDAM_DIMOUT) + sizeof(unsigned short) * ndata);

        if (sout == NULL) {
            break;
        }

        memcpy((void *)&sout->dim, (void *)getIdamDimData(sin->handle, dimid), (size_t)ndata * sizeof(unsigned short));
        psDef = IDL_MakeStruct(NULL, pTagsUnsignedShort);
        break;

    case TYPE_UNSIGNED:
        sout  = (IDAM_DIMOUT *)malloc(sizeof(IDAM_DIMOUT) + sizeof(unsigned int) * ndata);

        if (sout == NULL) {
            break;
        }

        memcpy((void *)&sout->dim, (void *)getIdamDimData(sin->handle, dimid), (size_t)ndata * sizeof(unsigned int));
        psDef = IDL_MakeStruct(NULL, pTagsUnsignedInt);
        break;

    case TYPE_UNSIGNED_LONG:
        sout  = (IDAM_DIMOUT *)malloc(sizeof(IDAM_DIMOUT) + sizeof(unsigned long) * ndata);

        if (sout == NULL) {
            break;
        }

        memcpy((void *)&sout->dim, (void *)getIdamDimData(sin->handle, dimid), (size_t)ndata * sizeof(unsigned long));
        psDef = IDL_MakeStruct(NULL, pTagsUnsignedLong);
        break;

    case TYPE_CHAR:
        sout  = (IDAM_DIMOUT *)malloc(sizeof(IDAM_DIMOUT) + sizeof(char) * ndata);

        if (sout == NULL) {
            break;
        }

        memcpy((void *)&sout->dim, (void *)getIdamDimData(sin->handle, dimid), (size_t)ndata * sizeof(char));
        psDef = IDL_MakeStruct(NULL, pTagsChar);
        break;

    case TYPE_UNSIGNED_CHAR:
        sout  = (IDAM_DIMOUT *)malloc(sizeof(IDAM_DIMOUT) + sizeof(unsigned char) * ndata);

        if (sout == NULL) {
            break;
        }

        memcpy((void *)&sout->dim, (void *)getIdamDimData(sin->handle, dimid), (size_t)ndata * sizeof(unsigned char));
        psDef = IDL_MakeStruct(NULL, pTagsUnsignedChar);
        break;

    default:
        break;

        if (kw.verbose) {
            fprintf(stdout, "The Data Type is Not Recognised [%d]\n", getIdamDimType(sin->handle, dimid));
        }

        IDL_KW_FREE;
        return (IDL_GettmpLong(GDE_UNKNOWN_DATA_TYPE));
    }

    if (sout == NULL) {
        if (kw.verbose) {
            fprintf(stdout, "Heap Memory Allocation Failed!\n");
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_HEAP_ALLOC_ERROR));
    }

    sout->handle   = (IDL_LONG) sin->handle;
    sout->dim_id   = (IDL_LONG) dimid;
    sout->dim_type = (IDL_LONG) getIdamDimType(sin->handle, dimid);
    sout->dim_n    = (IDL_LONG) ndata;

    IDL_StrStore(&(sout->dim_units),  getIdamDimUnits(sin->handle, dimid));
    IDL_StrStore(&(sout->dim_label),  getIdamDimLabel(sin->handle, dimid));

    // Create an Anonymous IDL Structure and Import into IDL

    ilDims[0] = 1;   // import Structure as a Single element Array

    ivReturn = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR *)sout, freeMem, psDef);

    //--------------------------------------------------------------------------
    // Cleanup Keywords

    IDL_KW_FREE;
    restoreIdamProperties(cblock);
    return (ivReturn);
}

//#####################################################################################################

IDL_VPTR IDL_CDECL getdimdataarray(int argc, IDL_VPTR argv[], char * argk)
{
    //
    // Must be called before getdimdata as getdimdata frees the underlying data block.
    //
    // Change History:
    //
    // 19Feb2007    dgm Allow access to data when an error is raised and the status is bad and
    //          bad data are requested.
    //-------------------------------------------------------------------------

    int dimid, ndims, ndata;
    IDL_MEMINT ilDims[1];

    IDAM_SIN * sin;      // Returned Structure

    CLIENT_BLOCK cblock = saveIdamProperties();  // preserve the current set of client properties
    CLIENT_BLOCK * idamcblock = NULL;
    int data_get_bad   = 0;
    int client_get_bad = getIdamProperty("get_bad");         // Current client get_bad property

    char * dvec;
    IDL_VPTR idlArray = NULL;

    // Keyword Structure

    typedef struct {
        IDL_KW_RESULT_FIRST_FIELD;
        IDL_LONG verbose;
        IDL_LONG debug;
        IDL_LONG help;
    } KW_RESULT;

    // Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {
        IDL_KW_FAST_SCAN,
        {"DEBUG",   IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
        {"HELP",    IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
        {"VERBOSE", IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
        {NULL}
    };

    KW_RESULT kw;

    kw.verbose   = 0;
    kw.debug     = 0;
    kw.help      = 0;

    //-----------------------------------------------------------------------
    // Check a Structure was Passed

    IDL_ENSURE_SCALAR(argv[1]);

    IDL_ENSURE_STRUCTURE(argv[0]);
    IDL_EXCLUDE_EXPR(argv[0]);

    //-----------------------------------------------------------------------
    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR *)0, 1, &kw);

    //--------------------------------------------------------------------------
    // Call for HELP?

    if (kw.help) {
        userhelp(stdout, "getdimdata");
        IDL_KW_FREE;
        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------
    // Input Structure

    sin   = (IDAM_SIN *)argv[0]->value.s.arr->data;      // Input Structure
    dimid = IDL_LongScalar(argv[1]);

    if ((int)sin->get_bad) {                 // Reset on exit
        setIdamProperty("get_bad");
        client_get_bad = 1;
    }

    if (sin->handle < 0) {
        if (kw.verbose) {
            fprintf(stdout, "No Valid IDAM data Handle!\n");
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_NO_VALID_HANDLE));
    }

    //---------------------------------------------------------------------------
    // Data Acquisition Properties, Keyword Properties and Client Property Settings

    if ((idamcblock = getIdamDataProperties((int)sin->handle)) != NULL) {
        data_get_bad = idamcblock->get_bad;
    } else {
        data_get_bad = (int)sin->get_bad;    // Client could change this value so don't trust!
    }

    if (sin->error_code != 0 && !((client_get_bad || data_get_bad) && getIdamDataStatus(sin->handle) == MIN_STATUS && sin->error_code == DATA_STATUS_BAD)) {
        if (kw.verbose) {
            fprintf(stdout, "Data has a Raised Error Status!\n");
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_DATA_HAS_ERROR));
    }

    //---------------------------------------------------------------------------
    // Check Status and BAD Property

    if ((client_get_bad || data_get_bad) && getIdamDataStatus(sin->handle) > MIN_STATUS) {
        if (kw.verbose) {
            fprintf(stdout, "Data don't have a BAD Status but GET_BAD property set - Access to data blocked!\n");
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_NO_DATA_TO_RETURN));
    }

    //--------------------------------------------------------------------------
    // Check Rank

    if (dimid < 0 || dimid >= getIdamRank(sin->handle)) {
        if (kw.verbose) {
            fprintf(stdout, "Dimension is Inconsistent with the Data's Rank!\n");
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_NO_SUCH_DIMENSION));
    }

    //--------------------------------------------------------------------------
    // Return Data

    ndims     = 1;
    ndata     = getIdamDimNum(sin->handle, dimid);
    ilDims[0] = (IDL_MEMINT)ndata;

    if (NDEBUG) {
        fprintf(stdout, "No of Elements = %d\n", (int)ilDims[0]);
    }

    // Allocate Heap Memory for the IDL Array and Copy the Data to it.

    switch (getIdamDimType(sin->handle, dimid)) {

    case TYPE_FLOAT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_FLOAT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamDimData(sin->handle, dimid), (size_t)ndata * sizeof(float));
        break;

    case TYPE_DOUBLE:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_DOUBLE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamDimData(sin->handle, dimid), (size_t)ndata * sizeof(double));
        break;

    case TYPE_SHORT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_INT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamDimData(sin->handle, dimid), (size_t)ndata * sizeof(short));
        break;

    case TYPE_INT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_LONG, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamDimData(sin->handle, dimid),  (size_t)ndata * sizeof(int));
        break;

    case TYPE_LONG:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_LONG64, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamDimData(sin->handle, dimid), (size_t)ndata * sizeof(long));
        break;

    case TYPE_UNSIGNED_SHORT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_UINT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamDimData(sin->handle, dimid), (size_t)ndata * sizeof(unsigned short));
        break;

    case TYPE_UNSIGNED:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_ULONG, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamDimData(sin->handle, dimid), (size_t)ndata * sizeof(unsigned int));
        break;

    case TYPE_UNSIGNED_LONG:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_ULONG64, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamDimData(sin->handle, dimid), (size_t)ndata * sizeof(unsigned long));
        break;

    case TYPE_CHAR:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_BYTE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamDimData(sin->handle, dimid), (size_t)ndata * sizeof(char));
        break;

    case TYPE_UNSIGNED_CHAR:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_BYTE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamDimData(sin->handle, dimid), (size_t)ndata * sizeof(unsigned char));
        break;

    default:
        break;

        if (kw.verbose) {
            fprintf(stdout, "The Dim Data Type is Not Recognised [%d]\n", getIdamDimType(sin->handle, dimid));
        }

        IDL_KW_FREE;
        restoreIdamProperties(cblock);
        return (IDL_GettmpLong(GDE_UNKNOWN_DATA_TYPE));

    }

    //--------------------------------------------------------------------------
    // Cleanup Keywords

    IDL_KW_FREE;
    restoreIdamProperties(cblock);
    return (idlArray);
}

//#####################################################################################################
//#####################################################################################################

IDL_VPTR IDL_CDECL freeidam(int argc, IDL_VPTR argv[], char * argk)
{
    //
    // Free IDAM Heap Remaining Associated with a Particular Handle
    //-------------------------------------------------------------------------
    int handle;

    IDL_ENSURE_SCALAR(argv[0]);

    //---------------------------------------------------------------------------
    // Input Structure

    handle = IDL_LongScalar(argv[0]);

    if (NDEBUG) {
        fprintf(stdout, "Freeing IDAM data Handle %d\n", handle);
    }

    if (handle < 0) {
        if (NDEBUG) {
            fprintf(stdout, "Not a Valid IDAM data Handle!\n");
        }

        return (IDL_GettmpLong(GDE_NO_VALID_HANDLE));
    }

    //--------------------------------------------------------------------------
    // Free IDAM Heap

    idamFree(handle);

    return (IDL_GettmpLong(0));
}

//#####################################################################################################

IDL_VPTR IDL_CDECL freeidamall(int argc, IDL_VPTR argv[], char * argk)
{

    // Free All IDAM Heap + Close IDAM Server

    idamFreeAll();

    return (IDL_GettmpLong(0));
}

void userhelp(FILE * fh, char * name)
{
    int i;
    int ngetidam = 51;
    char * help_getidam[] = {"GETIDAM: Required first call for Data via IDAM. This routine",
                             "passes the request to the backend Data Server which then accesses",
                             "the data and returns it to the Client application. GETIDAM generally",
                             "returns a Structure unless a problem occured early in processing the",
                             "data request - in which case an Integer Error Code is returned.\n",
                             "Subsequent calls to IDAM functions use this returned structure.\n",
                             "The Structure returned contains two groups of information. The first",
                             "is a copy of the user's request plus the settings of Keywords. These",
                             "keywords are described below. The second group list the results of",
                             "the user's request but not the data. These allow the user to check",
                             "for error conditions and to make subsequent call for the actual data.\n",
                             "IDAM Returned Tags:-",
                             "HANDLE: Identifies the Heap Memory data block IDAM uses to store data.",
                             "ERROR_CODE: If an error was encountered, a non zero error code is set.",
                             "ERROR_MSG: If there was an error, this string will contain an explanation.",
                             "STATUS: The Status Value associated with the Signal Data.",
                             "RANK: The Rank of the Data array.",
                             "ORDER: Identifies which Dimension is the Time Dimension.\n",
                             "GETIDAM USER REQUEST and KEYWORDS.....\n",
                             "ARCHIVE - the Name of the Local or External Data Archive, e.g. PPF on JET",
                             "DEBUG - to help Debug this application.",
                             "DEVICE - the name of an external device, e.g, JET",
                             "EXP_NUMBER - the experiment pulse or model run number",
                             "FILE - the name of a local file containing the requested data",
                             "FORMAT - the format of the local file, e.g. IDA",
                             "GET_ASIS - Return Data without any Server-Side XML based Corrections applied,",
                             "GET_BAD - Return Data when it has a Bad Status Value (This option also automatically",
                             "          prevents you from accessing Good Data which is the default). ",
                             "GET_DATADBLE - Return the Data in Double Precision",
                             "GET_DIMDBLE - Return All Dimensional Data in Double Precision",
                             "GET_TIMEDBLE - Return the Time Dimensional Data in Double Precision",
                             "GET_SCALAR - Reduce a Rank 1 vector to a scalar value array if the dimensional",
                             "             data are all zero in value",
                             "GET_BYTES - Return IDA data as a block of Bytes or Integers without applying",
                             "            Calibration Factors saved with the data.",
                             "GET_META - Return all Meta Data",
                             "GET_NOTOFF - Return Data without any Server-Side XML based Timing Offset Corrections applied",
                             "GET_UNCAL - Return Data without any Server-Side XML based Calibration Corrections applied",
                             "HELP - print information about an IDL IDAM Function",
                             "MDSTREE - the name of an MDSPLUS Data Tree",
                             "MDSNODE - the Node of an MDSPLUS Data Tree where the Data is located",
                             "MDSTREENUM - the Number of an MDSPLUS Data Tree",
                             "PASS - the IDA File Re-Pass Number",
                             "PATH - no longer use!",
                             "PULNO - the experiment pulse number",
                             "PULSE - the experiment pulse number",
                             "SERVER - the name or IP address of an external data server",
                             "SHOTNO - the experiment pulse number",
                             "SIGNAL - the generic or actual name of the requested signal",
                             "SN - the experiment pulse number",
                             "TPASS - not implemented yet!",
                             "VERBOSE - print error messages when they occur",
                             "GETIDAM can be called in a variety of ways ... ",
                             "sout = getidam(shotno, signal [,keywords]) ; so = Returned structure",
                             "sout = getidam(signal, shotno [,keywords]) ",
                             "newsout = getidam(sout,[,keywords])\n",
                             "NOTE: Keywords always take priority over passed arguments\n",
                             ""
                            };

    int ngetdata = 16;
    char * help_getdata[] = {"GETDATA: Return the Data accessed by the GETIDAM call.",
                             "Data are returned in a Structure containing:-",
                             "HANDLE: Identifies the IDAM Data Block.",
                             "DATA_TYPE: Identifies the Type of Data returned, e,g, DOUBLE Precision.",
                             "DATA_N: The number of Data Items returned in the Array.",
                             "DATA_LABEL: Data Label String.",
                             "DATA_UNITS: Data Units String",
                             "DATA_DESC: Data Description String",
                             "DATA: The Data Array. Organised as ......\n",
                             "GETDATA KEYWORDS.....\n",
                             "DEBUG - to help Debug this application.",
                             "HELP - print information about an IDL IDAM Function",
                             "VERBOSE - print error messages when they occur\n",
                             "GETDATA is called using the output structure from GETIDAM",
                             "dout = getdata(so [,keywords]) ; dout = Returned Structure.\n",
                             ""
                            };


    int ngetdimdata = 18;
    char * help_getdimdata[] = {"GETDIMDATA: Return the Dimensional Data of the Data accessed by the",
                                "GETIDAM call. Dimensional Data are returned Individually as requested",
                                "in a Structure containing:-",
                                "HANDLE   : Identifies the IDAM Data Block.",
                                "DIM_TYPE : Identifies the Type of Data returned, e,g, DOUBLE Precision.",
                                "DIM_N    : The number of Data Items returned in the Array.",
                                "DIM_LABEL: Dimension Label String.",
                                "DIM_UNITS: Dimension Units String",
                                "DIM      : The Rank-1 Dimensional Data Array.\n",
                                "GETDIMDATA KEYWORDS.....\n",
                                "DEBUG - to help Debug this application.",
                                "HELP - print information about an IDL IDAM Function.",
                                "VERBOSE - print error messages when they occur.\n",
                                "GETDIMDATA is called using the output structure from GETIDAM, so, and",
                                "a dimensional identifier, dimid (<= Rank of the Data Array = so.rank).",
                                "dimout = getdimdata(so, dimid [,keywords]) ; dimout = Returned Structure.",
                                "or for the time dimension: dimout = getdimdata(so, so.order [,keywords])\n",
                                ""
                               };

    fprintf(fh, "\nIDAM: MAST Universal Data Access\n\n");

    if (!strcmp(name, "getida")) for (i = 0; i < ngetidam - 1; i++) {
            fprintf(fh, "%s\n", help_getidam[i]);
        }

    if (!strcmp(name, "getdata")) for (i = 0; i < ngetdata - 1; i++) {
            fprintf(fh, "%s\n", help_getdata[i]);
        }

    if (!strcmp(name, "getdimdata")) for (i = 0; i < ngetdimdata - 1; i++) {
            fprintf(fh, "%s\n", help_getdimdata[i]);
        }

    fflush(fh);

    return;

}

//-----------------------------------------------------------------------------
//------ Accessor Functions ---------------------------------------------------


IDL_VPTR IDL_CDECL geterrorcode(int argc, IDL_VPTR argv[], char * argk)
{
    int handle;
    IDL_ENSURE_SCALAR(argv[0]);
    handle = IDL_LongScalar(argv[0]);
    return (IDL_GettmpLong(getIdamErrorCode(handle)));
}

IDL_VPTR IDL_CDECL geterrormsg(int argc, IDL_VPTR argv[], char * argk)
{
    int handle;
    IDL_ENSURE_SCALAR(argv[0]);
    IDL_ENSURE_SCALAR(argv[1]);
    IDL_EXCLUDE_EXPR(argv[1]);
    handle = IDL_LongScalar(argv[0]);
    IDL_StoreScalarZero(argv[1], IDL_TYP_STRING);
    //char *p = getIdamErrorMsg(handle);
    //fprintf(stdout,"sizeof: %d\n", sizeof(char *));
    //fprintf(stdout,"MSG address: %p [%llu]\n", p, (unsigned long long)p);
    //fprintf(stdout,"Returning to IDL Error MSG: %s\n", p);
    //IDL_StrStore(&argv[1]->value.str, p);
    IDL_StrStore(&argv[1]->value.str, (char *)getIdamErrorMsg(handle));
    return (IDL_GettmpLong(getIdamErrorCode(handle)));
}

IDL_VPTR IDL_CDECL printerrormsgstack(int argc, IDL_VPTR argv[], char * argk)
{
    int i, stackSize;
    IDL_ENSURE_SCALAR(argv[0]);
    stackSize = getIdamServerErrorStackSize();
    fprintf(stdout, "Error Message Stack: Count %d\n", stackSize);

    for (i = 0; i < stackSize; i++) {
        fprintf(stdout, "[%d] code: %d type: %d location: %s msg: %s\n", i, getIdamServerErrorStackRecordCode(i),
                getIdamServerErrorStackRecordType(i), getIdamServerErrorStackRecordLocation(i),
                getIdamServerErrorStackRecordMsg(i));
    }

    fflush(stdout);
    return (IDL_GettmpLong(0));
}

IDL_VPTR IDL_CDECL getsourcestatus(int argc, IDL_VPTR argv[], char * argk)
{
    int handle;
    IDL_ENSURE_SCALAR(argv[0]);
    handle = IDL_LongScalar(argv[0]);
    return (IDL_GettmpLong(getIdamSourceStatus(handle)));
}

IDL_VPTR IDL_CDECL getsignalstatus(int argc, IDL_VPTR argv[], char * argk)
{
    int handle;
    IDL_ENSURE_SCALAR(argv[0]);
    handle = IDL_LongScalar(argv[0]);
    return (IDL_GettmpLong(getIdamSignalStatus(handle)));
}



IDL_VPTR IDL_CDECL getdatastatus(int argc, IDL_VPTR argv[], char * argk)
{
    int handle;
    IDL_ENSURE_SCALAR(argv[0]);
    handle = IDL_LongScalar(argv[0]);
    return (IDL_GettmpLong(getIdamDataStatus(handle)));
}


IDL_VPTR IDL_CDECL getdatanum(int argc, IDL_VPTR argv[], char * argk)
{
    int handle;
    IDL_ENSURE_SCALAR(argv[0]);
    handle = IDL_LongScalar(argv[0]);
    return (IDL_GettmpLong(getIdamDataNum(handle)));
}

IDL_VPTR IDL_CDECL getrank(int argc, IDL_VPTR argv[], char * argk)
{
    int handle;
    IDL_ENSURE_SCALAR(argv[0]);
    handle = IDL_LongScalar(argv[0]);
    return (IDL_GettmpLong(getIdamRank(handle)));
}

IDL_VPTR IDL_CDECL getorder(int argc, IDL_VPTR argv[], char * argk)
{
    int handle;
    IDL_ENSURE_SCALAR(argv[0]);
    handle = IDL_LongScalar(argv[0]);
    return (IDL_GettmpLong(getIdamOrder(handle)));
}

IDL_VPTR IDL_CDECL getdatatype(int argc, IDL_VPTR argv[], char * argk)
{
    int handle;
    IDL_ENSURE_SCALAR(argv[0]);
    handle = IDL_LongScalar(argv[0]);
    return (IDL_GettmpLong(getIdamDataType(handle)));
}

IDL_VPTR IDL_CDECL geterrortype(int argc, IDL_VPTR argv[], char * argk)
{
    int handle;
    IDL_ENSURE_SCALAR(argv[0]);
    handle = IDL_LongScalar(argv[0]);
    return (IDL_GettmpLong(getIdamErrorType(handle)));
}


IDL_VPTR IDL_CDECL getdatadata(int argc, IDL_VPTR argv[], char * argk)
{
    int handle, ndims, ndata;
    IDL_MEMINT ilDims[1];
    char * dvec;
    IDL_VPTR idlArray;

    IDL_ENSURE_SCALAR(argv[0]);
    handle = IDL_LongScalar(argv[0]);

    // Check Status and BAD Property done within Accessor

    if (getIdamData(handle) == NULL) {
        return (IDL_GettmpLong(GDE_NO_DATA_TO_RETURN));
    }

    ndims     = 1;
    ndata     = getIdamDataNum(handle);
    ilDims[0] = (IDL_MEMINT) ndata;

    // Allocate Heap Memory for the IDL Array and Copy the Data to it.

    switch (getIdamDataType(handle)) {

    case TYPE_FLOAT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_FLOAT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamData(handle), (size_t)ndata * sizeof(float));
        break;

    case TYPE_DOUBLE:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_DOUBLE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamData(handle), (size_t)ndata * sizeof(double));
        break;

    case TYPE_SHORT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_INT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamData(handle), (size_t)ndata * sizeof(short));
        break;

    case TYPE_INT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_LONG, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamData(handle), (size_t)ndata * sizeof(int));
        break;

    case TYPE_LONG:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_LONG64, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamData(handle), (size_t)ndata * sizeof(long));
        break;

    case TYPE_UNSIGNED_SHORT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_UINT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamData(handle), (size_t)ndata * sizeof(unsigned short));
        break;

    case TYPE_UNSIGNED:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_ULONG, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamData(handle), (size_t)ndata * sizeof(unsigned int));
        break;

    case TYPE_UNSIGNED_LONG:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_ULONG64, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamData(handle), (size_t)ndata * sizeof(unsigned long));
        break;

    case TYPE_CHAR:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_BYTE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamData(handle), (size_t)ndata * sizeof(char));
        break;

    case TYPE_UNSIGNED_CHAR:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_BYTE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamData(handle), (size_t)ndata * sizeof(unsigned char));
        break;

    default:
        return (IDL_GettmpLong(-1));
    }

    return (idlArray);
}


IDL_VPTR IDL_CDECL getsyntheticdata(int argc, IDL_VPTR argv[], char * argk)
{
    int handle, ndims, ndata;
    IDL_MEMINT ilDims[1];
    char * dvec;
    IDL_VPTR idlArray;

    IDL_ENSURE_SCALAR(argv[0]);
    handle = IDL_LongScalar(argv[0]);

    // Check Status and BAD Property done with Accessor function

    if (getIdamData(handle) == NULL) {
        return (IDL_GettmpLong(GDE_NO_DATA_TO_RETURN));
    }

    ndims     = 1;
    ndata     = getIdamDataNum(handle);
    ilDims[0] = (IDL_MEMINT) ndata;

    // Allocate Heap Memory for the IDL Array and Copy the Data to it.

    switch (getIdamDataType(handle)) {

    case TYPE_FLOAT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_FLOAT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamSyntheticData(handle), (size_t)ndata * sizeof(float));
        break;

    case TYPE_DOUBLE:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_DOUBLE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamSyntheticData(handle), (size_t)ndata * sizeof(double));
        break;

    case TYPE_SHORT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_INT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamSyntheticData(handle), (size_t)ndata * sizeof(short));
        break;

    case TYPE_INT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_LONG, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamSyntheticData(handle), (size_t)ndata * sizeof(int));
        break;

    case TYPE_LONG:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_LONG64, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamSyntheticData(handle), (size_t)ndata * sizeof(long));
        break;

    case TYPE_UNSIGNED_SHORT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_UINT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamSyntheticData(handle), (size_t)ndata * sizeof(unsigned short));
        break;

    case TYPE_UNSIGNED:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_ULONG, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamSyntheticData(handle), (size_t)ndata * sizeof(unsigned int));
        break;

    case TYPE_UNSIGNED_LONG:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_ULONG64, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamSyntheticData(handle), (size_t)ndata * sizeof(unsigned long));
        break;

    case TYPE_CHAR:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_BYTE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamSyntheticData(handle), (size_t)ndata * sizeof(char));
        break;


    case TYPE_UNSIGNED_CHAR:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_BYTE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamSyntheticData(handle), (size_t)ndata * sizeof(unsigned char));
        break;

    default:
        return (IDL_GettmpLong(-1));
    }

    return (idlArray);
}


IDL_VPTR IDL_CDECL getdataerror(int argc, IDL_VPTR argv[], char * argk)
{
    int handle, ndims, data_n;
    IDL_MEMINT ilDims[1];
    char * dvec, *ep = NULL;
    IDL_VPTR idlArray;

    IDL_ENSURE_SCALAR(argv[0]);
    handle = IDL_LongScalar(argv[0]);

    // Check Status and BAD Property done within Accessor function

    if (getIdamData(handle) == NULL) {
        return (IDL_GettmpLong(GDE_NO_DATA_TO_RETURN));
    }

    ndims  = 1;
    data_n = getIdamDataNum(handle);
    ep     = (char *)getIdamError(handle);

    ilDims[0] = (IDL_MEMINT)data_n;

    // Allocate Heap Memory for the IDL Array and Copy the Data to it.

    switch (getIdamErrorType(handle)) {

    case TYPE_FLOAT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_FLOAT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)data_n * sizeof(float));
        break;

    case TYPE_DOUBLE:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_DOUBLE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)data_n * sizeof(double));
        break;

    case TYPE_SHORT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_INT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)data_n * sizeof(short));
        break;

    case TYPE_INT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_LONG, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)data_n * sizeof(int));
        break;

    case TYPE_LONG:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_LONG64, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)data_n * sizeof(long));
        break;

    case TYPE_UNSIGNED_SHORT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_UINT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)data_n * sizeof(unsigned short));
        break;

    case TYPE_UNSIGNED:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_ULONG, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)data_n * sizeof(unsigned int));
        break;

    case TYPE_UNSIGNED_LONG:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_ULONG64, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)data_n * sizeof(unsigned long));
        break;

    case TYPE_CHAR:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_BYTE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)(void *)ep, (size_t)data_n * sizeof(char));
        break;

    case TYPE_UNSIGNED_CHAR:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_BYTE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)(void *)ep, (size_t)data_n * sizeof(unsigned char));
        break;

    default:
        return (IDL_GettmpLong(-1));
    }

    return (idlArray);
}


IDL_VPTR IDL_CDECL getasymmetricdataerror(int argc, IDL_VPTR argv[], char * argk)
{
    int handle, above, ndims, data_n;
    IDL_MEMINT ilDims[1];
    char * dvec, *ep = NULL;
    IDL_VPTR idlArray;

    IDL_ENSURE_SCALAR(argv[0]);
    IDL_ENSURE_SCALAR(argv[1]);
    handle = IDL_LongScalar(argv[0]);

    // Check Status and BAD Property done within Accessor function

    if (getIdamData(handle) == NULL) {
        return (IDL_GettmpLong(GDE_NO_DATA_TO_RETURN));
    }

    above  = IDL_LongScalar(argv[1]);
    ndims  = 1;
    data_n = getIdamDataNum(handle);
    ep     = (char *)getIdamAsymmetricError(handle, above);

    ilDims[0] = (IDL_MEMINT)data_n;

    // Allocate Heap Memory for the IDL Array and Copy the Data to it.

    switch (getIdamErrorType(handle)) {

    case TYPE_FLOAT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_FLOAT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)data_n * sizeof(float));
        break;

    case TYPE_DOUBLE:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_DOUBLE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)data_n * sizeof(double));
        break;

    case TYPE_SHORT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_INT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)data_n * sizeof(short));
        break;

    case TYPE_INT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_LONG, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)data_n * sizeof(int));
        break;

    case TYPE_LONG:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_LONG64, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)data_n * sizeof(long));
        break;

    case TYPE_UNSIGNED_SHORT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_UINT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)data_n * sizeof(unsigned short));
        break;

    case TYPE_UNSIGNED:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_ULONG, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)data_n * sizeof(unsigned int));
        break;

    case TYPE_UNSIGNED_LONG:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_ULONG64, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)data_n * sizeof(unsigned long));
        break;

    case TYPE_CHAR:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_BYTE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)(void *)ep, (size_t)data_n * sizeof(char));
        break;

    case TYPE_UNSIGNED_CHAR:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_BYTE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)(void *)ep, (size_t)data_n * sizeof(unsigned char));
        break;

    default:
        return (IDL_GettmpLong(-1));
    }

    return (idlArray);
}



IDL_VPTR IDL_CDECL getfloatdata(int argc, IDL_VPTR argv[], char * argk)
{
    int handle, ndims;
    IDL_MEMINT ilDims[1];
    float * dvec;
    IDL_VPTR idlArray;

    IDL_ENSURE_SCALAR(argv[0]);
    handle = IDL_LongScalar(argv[0]);

    // Check Status and BAD Property done within Accessor function

    if (getIdamData(handle) == NULL) {
        return (IDL_GettmpLong(GDE_NO_DATA_TO_RETURN));
    }

    ndims     = 1;
    ilDims[0] = (IDL_MEMINT) getIdamDataNum(handle);
    dvec = (float *)IDL_MakeTempArray((int)IDL_TYP_FLOAT, ndims, (IDL_MEMINT *)ilDims,
                                      IDL_ARR_INI_ZERO, &idlArray);
    getIdamFloatData(handle, dvec);

    return (idlArray);
}


IDL_VPTR IDL_CDECL getfloatdataerror(int argc, IDL_VPTR argv[], char * argk)
{
    int handle, ndims;
    IDL_MEMINT ilDims[1];
    float * dvec;
    IDL_VPTR idlArray;

    IDL_ENSURE_SCALAR(argv[0]);
    handle = IDL_LongScalar(argv[0]);

    // Check Status and BAD Property done within Accessor function

    if (getIdamData(handle) == NULL) {
        return (IDL_GettmpLong(GDE_NO_DATA_TO_RETURN));
    }

    ndims     = 1;
    ilDims[0] = (IDL_MEMINT)getIdamDataNum(handle);
    dvec = (float *)IDL_MakeTempArray((int)IDL_TYP_FLOAT, ndims, (IDL_MEMINT *)ilDims,
                                      IDL_ARR_INI_ZERO, &idlArray);
    getIdamFloatError(handle, dvec);

    return (idlArray);
}

IDL_VPTR IDL_CDECL getfloatasymmetricdataerror(int argc, IDL_VPTR argv[], char * argk)
{
    int handle, above, ndims;
    IDL_MEMINT ilDims[1];
    float * dvec;
    IDL_VPTR idlArray;

    IDL_ENSURE_SCALAR(argv[0]);
    IDL_ENSURE_SCALAR(argv[1]);
    handle = IDL_LongScalar(argv[0]);
    above  = IDL_LongScalar(argv[1]);

    // Check Status and BAD Property done within Accessor function

    if (getIdamData(handle) == NULL) {
        return (IDL_GettmpLong(GDE_NO_DATA_TO_RETURN));
    }

    ndims     = 1;
    ilDims[0] = (IDL_MEMINT)getIdamDataNum(handle);
    dvec = (float *)IDL_MakeTempArray((int)IDL_TYP_FLOAT, ndims, (IDL_MEMINT *)ilDims,
                                      IDL_ARR_INI_ZERO, &idlArray);
    getIdamFloatAsymmetricError(handle, above, dvec);

    return (idlArray);
}


IDL_VPTR IDL_CDECL getdatalabel(int argc, IDL_VPTR argv[], char * argk)
{
    int handle;
    IDL_ENSURE_SCALAR(argv[0]);
    IDL_ENSURE_SCALAR(argv[1]);
    IDL_EXCLUDE_EXPR(argv[1]);
    handle = IDL_LongScalar(argv[0]);
    IDL_StoreScalarZero(argv[1], IDL_TYP_STRING);
    IDL_StrStore(&argv[1]->value.str, (char *)getIdamDataLabel(handle));
    return (IDL_GettmpLong(0));
}

IDL_VPTR IDL_CDECL getdataunits(int argc, IDL_VPTR argv[], char * argk)
{
    int handle;
    IDL_ENSURE_SCALAR(argv[0]);
    IDL_ENSURE_SCALAR(argv[1]);
    IDL_EXCLUDE_EXPR(argv[1]);
    handle = IDL_LongScalar(argv[0]);
    IDL_StoreScalarZero(argv[1], IDL_TYP_STRING);
    IDL_StrStore(&argv[1]->value.str, (char *)getIdamDataUnits(handle));
    return (IDL_GettmpLong(0));
}

IDL_VPTR IDL_CDECL getdatadesc(int argc, IDL_VPTR argv[], char * argk)
{
    int handle;
    IDL_ENSURE_SCALAR(argv[0]);
    IDL_ENSURE_SCALAR(argv[1]);
    IDL_EXCLUDE_EXPR(argv[1]);
    handle = IDL_LongScalar(argv[0]);
    IDL_StoreScalarZero(argv[1], IDL_TYP_STRING);
    IDL_StrStore(&argv[1]->value.str, (char *)getIdamDataDesc(handle));
    return (IDL_GettmpLong(0));
}

//##############################################################################################

IDL_VPTR IDL_CDECL getdimnum(int argc, IDL_VPTR argv[], char * argk)
{
    int handle, dimid;
    IDL_ENSURE_SCALAR(argv[0]);
    IDL_ENSURE_SCALAR(argv[1]);
    handle = IDL_LongScalar(argv[0]);
    dimid  = IDL_LongScalar(argv[1]);
    return (IDL_GettmpLong(getIdamDimNum(handle, dimid)));
}

IDL_VPTR IDL_CDECL getdimtype(int argc, IDL_VPTR argv[], char * argk)
{
    int handle, dimid;
    IDL_ENSURE_SCALAR(argv[0]);
    IDL_ENSURE_SCALAR(argv[1]);
    handle = IDL_LongScalar(argv[0]);
    dimid  = IDL_LongScalar(argv[1]);
    return (IDL_GettmpLong(getIdamDimType(handle, dimid)));
}

IDL_VPTR IDL_CDECL getdimerrortype(int argc, IDL_VPTR argv[], char * argk)
{
    int handle, dimid;
    IDL_ENSURE_SCALAR(argv[0]);
    IDL_ENSURE_SCALAR(argv[1]);
    handle = IDL_LongScalar(argv[0]);
    dimid  = IDL_LongScalar(argv[1]);
    return (IDL_GettmpLong(getIdamDimErrorType(handle, dimid)));
}

IDL_VPTR IDL_CDECL getdimdata(int argc, IDL_VPTR argv[], char * argk)
{
    int handle, dimid, ndims, ndata;
    IDL_MEMINT ilDims[1];
    char * dvec;
    IDL_VPTR idlArray;

    IDL_ENSURE_SCALAR(argv[0]);
    IDL_ENSURE_SCALAR(argv[1]);
    handle = IDL_LongScalar(argv[0]);
    dimid  = IDL_LongScalar(argv[1]);

    ndims     = 1;
    ndata     = getIdamDimNum(handle, dimid);
    ilDims[0] = (IDL_MEMINT)ndata;

    // Allocate Heap Memory for the IDL Array and Copy the Data to it.

    switch (getIdamDimType(handle, dimid)) {

    case TYPE_FLOAT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_FLOAT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamDimData(handle, dimid), (size_t)ndata * sizeof(float));
        break;

    case TYPE_DOUBLE:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_DOUBLE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamDimData(handle, dimid), (size_t)ndata * sizeof(double));
        break;

    case TYPE_SHORT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_INT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamDimData(handle, dimid), (size_t)ndata * sizeof(short));
        break;

    case TYPE_INT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_LONG, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamDimData(handle, dimid), (size_t)ndata * sizeof(int));
        break;

    case TYPE_LONG:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_LONG64, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamDimData(handle, dimid), (size_t)ndata * sizeof(long));
        break;

    case TYPE_UNSIGNED_SHORT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_UINT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamDimData(handle, dimid), (size_t)ndata * sizeof(unsigned short));
        break;

    case TYPE_UNSIGNED:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_ULONG, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamDimData(handle, dimid), (size_t)ndata * sizeof(unsigned int));
        break;

    case TYPE_UNSIGNED_LONG:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_ULONG64, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamDimData(handle, dimid), (size_t)ndata * sizeof(unsigned long));
        break;

    case TYPE_CHAR:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_BYTE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamDimData(handle, dimid), (size_t)ndata * sizeof(char));
        break;

    case TYPE_UNSIGNED_CHAR:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_BYTE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamDimData(handle, dimid), (size_t)ndata * sizeof(unsigned char));
        break;

    default:
        return (IDL_GettmpLong(-1));

    }

    return (idlArray);
}

IDL_VPTR IDL_CDECL getsyntheticdimdata(int argc, IDL_VPTR argv[], char * argk)
{
    int handle, dimid, ndims, ndata;
    IDL_MEMINT ilDims[1];
    char * dvec;
    IDL_VPTR idlArray;

    IDL_ENSURE_SCALAR(argv[0]);
    IDL_ENSURE_SCALAR(argv[1]);
    handle = IDL_LongScalar(argv[0]);
    dimid  = IDL_LongScalar(argv[1]);

    ndims     = 1;
    ndata     = getIdamDimNum(handle, dimid);
    ilDims[0] = (IDL_MEMINT)ndata;

    // Allocate Heap Memory for the IDL Array and Copy the Data to it.

    switch (getIdamDimType(handle, dimid)) {

    case TYPE_FLOAT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_FLOAT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamSyntheticDimData(handle, dimid), (size_t)ndata * sizeof(float));
        break;

    case TYPE_DOUBLE:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_DOUBLE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamSyntheticDimData(handle, dimid), (size_t)ndata * sizeof(double));
        break;

    case TYPE_SHORT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_INT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamSyntheticDimData(handle, dimid), (size_t)ndata * sizeof(short));
        break;

    case TYPE_INT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_LONG, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamSyntheticDimData(handle, dimid), (size_t)ndata * sizeof(int));
        break;

    case TYPE_LONG:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_LONG64, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamSyntheticDimData(handle, dimid), (size_t)ndata * sizeof(long));
        break;

    case TYPE_UNSIGNED_SHORT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_UINT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamSyntheticDimData(handle, dimid), (size_t)ndata * sizeof(unsigned short));
        break;

    case TYPE_UNSIGNED:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_ULONG, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamSyntheticDimData(handle, dimid), (size_t)ndata * sizeof(unsigned int));
        break;

    case TYPE_UNSIGNED_LONG:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_ULONG64, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamSyntheticDimData(handle, dimid), (size_t)ndata * sizeof(unsigned long));
        break;

    case TYPE_CHAR:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_BYTE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamSyntheticDimData(handle, dimid), (size_t)ndata * sizeof(char));
        break;

    case TYPE_UNSIGNED_CHAR:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_BYTE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)getIdamSyntheticDimData(handle, dimid), (size_t)ndata * sizeof(unsigned char));
        break;

    default:
        return (IDL_GettmpLong(-1));

    }

    return (idlArray);
}


IDL_VPTR IDL_CDECL getdimerror(int argc, IDL_VPTR argv[], char * argk)
{
    int handle, dimid, ndims, ndata;
    IDL_MEMINT ilDims[1];
    char * dvec;
    char * ep;
    IDL_VPTR idlArray;

    IDL_ENSURE_SCALAR(argv[0]);
    IDL_ENSURE_SCALAR(argv[1]);
    handle = IDL_LongScalar(argv[0]);
    dimid  = IDL_LongScalar(argv[1]);

    ndims     = 1;
    ndata     = getIdamDimNum(handle, dimid);
    ep        = (char *) getIdamDimError(handle, dimid);
    ilDims[0] = (IDL_MEMINT)ndata;

    // Allocate Heap Memory for the IDL Array and Copy the Data to it.

    switch (getIdamDimErrorType(handle, dimid)) {

    case TYPE_FLOAT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_FLOAT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(float));
        break;

    case TYPE_DOUBLE:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_DOUBLE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(double));
        break;

    case TYPE_SHORT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_INT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(short));
        break;

    case TYPE_INT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_LONG, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(int));
        break;

    case TYPE_LONG:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_LONG64, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(long));
        break;

    case TYPE_UNSIGNED_SHORT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_UINT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(unsigned short));
        break;

    case TYPE_UNSIGNED:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_ULONG, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(unsigned int));
        break;

    case TYPE_UNSIGNED_LONG:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_ULONG64, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(unsigned long));
        break;

    case TYPE_CHAR:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_BYTE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(char));
        break;

    case TYPE_UNSIGNED_CHAR:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_BYTE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(unsigned char));
        break;

    default:
        return (IDL_GettmpLong(-1));

    }

    return (idlArray);
}

IDL_VPTR IDL_CDECL getasymmetricdimerror(int argc, IDL_VPTR argv[], char * argk)
{
    int handle, dimid, above, ndims, ndata;
    IDL_MEMINT ilDims[1];
    char * dvec;
    char * ep;
    IDL_VPTR idlArray;

    IDL_ENSURE_SCALAR(argv[0]);
    IDL_ENSURE_SCALAR(argv[1]);
    IDL_ENSURE_SCALAR(argv[2]);
    handle = IDL_LongScalar(argv[0]);
    dimid  = IDL_LongScalar(argv[1]);
    above  = IDL_LongScalar(argv[2]);

    ndims     = 1;
    ndata     = getIdamDimNum(handle, dimid);
    ep        = (char *) getIdamDimAsymmetricError(handle, dimid, above);
    ilDims[0] = (IDL_MEMINT)ndata;

    // Allocate Heap Memory for the IDL Array and Copy the Data to it.

    switch (getIdamDimErrorType(handle, dimid)) {

    case TYPE_FLOAT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_FLOAT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(float));
        break;

    case TYPE_DOUBLE:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_DOUBLE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(double));
        break;

    case TYPE_SHORT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_INT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(short));
        break;

    case TYPE_INT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_LONG, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(int));
        break;

    case TYPE_LONG:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_LONG64, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(long));
        break;

    case TYPE_UNSIGNED_SHORT:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_UINT, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(unsigned short));
        break;

    case TYPE_UNSIGNED:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_ULONG, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(unsigned int));
        break;

    case TYPE_UNSIGNED_LONG:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_ULONG64, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(unsigned long));
        break;

    case TYPE_CHAR:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_BYTE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(char));
        break;

    case TYPE_UNSIGNED_CHAR:

        dvec = (char *)IDL_MakeTempArray((int)IDL_TYP_BYTE, ndims, (IDL_MEMINT *)ilDims,
                                         IDL_ARR_INI_ZERO, &idlArray);
        memcpy((void *)dvec, (void *)ep, (size_t)ndata * sizeof(unsigned char));
        break;

    default:
        return (IDL_GettmpLong(-1));

    }

    return (idlArray);
}


IDL_VPTR IDL_CDECL getfloatdimdata(int argc, IDL_VPTR argv[], char * argk)
{
    int handle, dimid, ndims;
    IDL_MEMINT ilDims[1];
    float * dvec;
    IDL_VPTR idlArray;

    IDL_ENSURE_SCALAR(argv[0]);
    IDL_ENSURE_SCALAR(argv[1]);
    handle = IDL_LongScalar(argv[0]);
    dimid  = IDL_LongScalar(argv[1]);

    ndims     = 1;
    ilDims[0] = (IDL_MEMINT)getIdamDimNum(handle, dimid);
    dvec = (float *)IDL_MakeTempArray((int)IDL_TYP_FLOAT, ndims, (IDL_MEMINT *)ilDims,
                                      IDL_ARR_INI_ZERO, &idlArray);

    getIdamFloatDimData(handle, dimid, dvec);

    return (idlArray);
}

IDL_VPTR IDL_CDECL getfloatdimerror(int argc, IDL_VPTR argv[], char * argk)
{
    int handle, dimid, ndims;
    IDL_MEMINT ilDims[1];
    float * dvec;
    IDL_VPTR idlArray;

    IDL_ENSURE_SCALAR(argv[0]);
    IDL_ENSURE_SCALAR(argv[1]);
    handle = IDL_LongScalar(argv[0]);
    dimid  = IDL_LongScalar(argv[1]);

    ndims     = 1;
    ilDims[0] = (IDL_MEMINT)getIdamDimNum(handle, dimid);
    dvec = (float *)IDL_MakeTempArray((int)IDL_TYP_FLOAT, ndims, (IDL_MEMINT *)ilDims,
                                      IDL_ARR_INI_ZERO, &idlArray);

    getIdamFloatDimError(handle, dimid, dvec);

    return (idlArray);
}

IDL_VPTR IDL_CDECL getfloatasymmetricdimerror(int argc, IDL_VPTR argv[], char * argk)
{
    int handle, dimid, above, ndims;
    IDL_MEMINT ilDims[1];
    float * dvec;
    IDL_VPTR idlArray;

    IDL_ENSURE_SCALAR(argv[0]);
    IDL_ENSURE_SCALAR(argv[1]);
    IDL_ENSURE_SCALAR(argv[2]);
    handle = IDL_LongScalar(argv[0]);
    dimid  = IDL_LongScalar(argv[1]);
    above  = IDL_LongScalar(argv[2]);

    ndims     = 1;
    ilDims[0] = (IDL_MEMINT)getIdamDimNum(handle, dimid);
    dvec = (float *)IDL_MakeTempArray((int)IDL_TYP_FLOAT, ndims, (IDL_MEMINT *)ilDims,
                                      IDL_ARR_INI_ZERO, &idlArray);

    getIdamFloatDimAsymmetricError(handle, dimid, above, dvec);

    return (idlArray);
}


IDL_VPTR IDL_CDECL getdimlabel(int argc, IDL_VPTR argv[], char * argk)
{
    int handle, dimid;
    IDL_ENSURE_SCALAR(argv[0]);
    IDL_ENSURE_SCALAR(argv[1]);
    IDL_ENSURE_SCALAR(argv[2]);
    IDL_EXCLUDE_EXPR(argv[2]);
    handle = IDL_LongScalar(argv[0]);
    dimid  = IDL_LongScalar(argv[1]);
    //fprintf(stdout,"getdimlabel: Handle = %d\n", handle);
    //fprintf(stdout,"getdimlabel: Dim id = %d\n", dimid);
    //fprintf(stdout,"getdimlabel: Label  = %s\n", (char *)getIdamDimLabel(handle, dimid));
    IDL_StoreScalarZero(argv[2], IDL_TYP_STRING);
    IDL_StrStore(&argv[2]->value.str, (char *)getIdamDimLabel(handle, dimid));
    return (IDL_GettmpLong(0));
}

IDL_VPTR IDL_CDECL getdimunits(int argc, IDL_VPTR argv[], char * argk)
{
    int handle, dimid;
    IDL_ENSURE_SCALAR(argv[0]);
    IDL_ENSURE_SCALAR(argv[1]);
    IDL_ENSURE_SCALAR(argv[2]);
    IDL_EXCLUDE_EXPR(argv[2]);
    handle = IDL_LongScalar(argv[0]);
    dimid  = IDL_LongScalar(argv[1]);
    //fprintf(stdout,"getdimunits: Handle = %d\n", handle);
    //fprintf(stdout,"getdimunits: Dim id = %d\n", dimid);
    //fprintf(stdout,"getdimunits: Units  = %s\n", (char *)getIdamDimUnits(handle, dimid));
    IDL_StoreScalarZero(argv[2], IDL_TYP_STRING);
    IDL_StrStore(&argv[2]->value.str, (char *)getIdamDimUnits(handle, dimid));
    return (IDL_GettmpLong(0));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

IDL_VPTR IDL_CDECL getdatasystemmeta(int argc, IDL_VPTR argv[], char * argk)
{
    //
    // Returns a Structure containing the IDAM Database Data_System table Record
    //-------------------------------------------------------------------------
    // Change History:
    //
    // 18Apr2007    dgm ilDims changed from type IDL_LONG to IDL_MEMINT
    //-------------------------------------------------------------------------

    int handle;
    DATA_SYSTEM * data_system;
    char typestr[2] = " ";

    IDAM_DATA_SYSTEM * sout;     // Returned Structure
    IDL_VPTR          ivReturn = NULL;

    // Passed Structure

    void * psDef = NULL;
    //IDL_LONG ilDims[IDL_MAX_ARRAY_DIM];
    IDL_MEMINT ilDims[IDL_MAX_ARRAY_DIM];

    // IDL tags structure

    IDL_STRUCT_TAG_DEF pTags[] = {
        {"SYSTEM_ID",   0,  (void *) IDL_TYP_LONG},
        {"VERSON",  0,  (void *) IDL_TYP_LONG},
        {"META_ID", 0,  (void *) IDL_TYP_LONG},
        {"TYPE",    0,  (void *) IDL_TYP_STRING},
        {"DEVICE_NAME", 0,  (void *) IDL_TYP_STRING},
        {"SYSTEM_NAME", 0,  (void *) IDL_TYP_STRING},
        {"SYSTEM_DESC", 0,  (void *) IDL_TYP_STRING},
        {"CREATION",    0,  (void *) IDL_TYP_STRING},
        {"XML",     0,  (void *) IDL_TYP_STRING},
        {"XML_CREATION", 0,  (void *) IDL_TYP_STRING},
        {0}
    };

    // Keyword Structure

    typedef struct {
        IDL_KW_RESULT_FIRST_FIELD;
        IDL_LONG verbose;
        IDL_LONG debug;
        IDL_LONG help;
    } KW_RESULT;


    // Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {
        IDL_KW_FAST_SCAN,
        {"DEBUG",   IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
        {"HELP",    IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
        {"VERBOSE", IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
        {NULL}
    };

    KW_RESULT kw;

    kw.verbose   = 0;
    kw.debug     = 0;
    kw.help      = 0;

    //-----------------------------------------------------------------------
    // Check an Integer was Passed

    IDL_ENSURE_SCALAR(argv[0]);
    handle = IDL_LongScalar(argv[0]);

    //-----------------------------------------------------------------------
    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR *)0, 1, &kw);

    if (kw.debug) {
        if (kw.debug) {
            fprintf(stdout, "Debug Keyword Passed\n");
        }

        if (kw.verbose) {
            fprintf(stdout, "Verbose Keyword Passed\n");
        }

        if (kw.help) {
            fprintf(stdout, "Help Keyword Passed\n");
        }
    }

    //--------------------------------------------------------------------------
    // Call for HELP?

    if (kw.help) {
        userhelp(stdout, "getdatasystemmeta");
        IDL_KW_FREE;
        return (IDL_GettmpLong(0));
    }

    //--------------------------------------------------------------------------
    // Access Structure to be Returned

    data_system = getIdamDataSystem(handle);

    if (data_system == NULL) {   // Nothing to Return
        IDL_KW_FREE;
        return (IDL_GettmpLong(GDE_NO_DATA_STRUCTURE_TO_RETURN));
    }

    //--------------------------------------------------------------------------
    // Prepare return Structure

    if ((sout = (IDAM_DATA_SYSTEM *)malloc(sizeof(IDAM_DATA_SYSTEM)) ) == NULL) {
        if (kw.verbose) {
            fprintf(stdout, "Heap Memory Allocation Failed!\n");
        }

        IDL_KW_FREE;
        return (IDL_GettmpLong(GDE_HEAP_ALLOC_ERROR));
    }

    psDef = IDL_MakeStruct(NULL, pTags);

    sout->system_id = (IDL_LONG) data_system->system_id;
    sout->version   = (IDL_LONG) data_system->version;
    sout->meta_id   = (IDL_LONG) data_system->meta_id;

    typestr[0] = data_system->type;

    IDL_StrStore(&(sout->type),        typestr);
    IDL_StrStore(&(sout->device_name), data_system->device_name);
    IDL_StrStore(&(sout->system_name), data_system->system_name);
    IDL_StrStore(&(sout->system_desc), data_system->system_desc);
    IDL_StrStore(&(sout->creation),    data_system->creation);
    IDL_StrStore(&(sout->xml),         data_system->xml);
    IDL_StrStore(&(sout->xml_creation), data_system->xml_creation);

    if (kw.debug) {
        fprintf(stdout, "Meta Data copied to IDL Structure\n");
    }

    // Create an Anonymous IDL Structure and Import into IDL

    ilDims[0] = 1;   // import Structure as a Single element Array

    ivReturn = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR *)sout, freeMem, psDef);

    if (kw.debug) {
        fprintf(stdout, "IDL Structure Created: Ready for Return\n");
    }

    //--------------------------------------------------------------------------
    // Cleanup Keywords and IDAM Heap

    IDL_KW_FREE;
    return (ivReturn);
}

IDL_VPTR IDL_CDECL getsystemconfigmeta(int argc, IDL_VPTR argv[], char * argk)
{
    //
    // Returns a Structure containing the IDAM Database System_Config table Record
    //-------------------------------------------------------------------------
    // Change History:
    //
    // 18Apr2007    dgm ilDims changed from type IDL_LONG to IDL_MEMINT
    //-------------------------------------------------------------------------

    int handle;
    SYSTEM_CONFIG * system_config;

    IDAM_SYSTEM_CONFIG * sout;       // Returned Structure
    IDL_VPTR          ivReturn = NULL;

    // Passed Structure

    void * psDef = NULL;
    //IDL_LONG ilDims[IDL_MAX_ARRAY_DIM];
    IDL_MEMINT ilDims[IDL_MAX_ARRAY_DIM];

    // IDL tags structure

    IDL_STRUCT_TAG_DEF pTags[] = {
        {"CONFIG_ID",   0,  (void *) IDL_TYP_LONG},
        {"SYSTEM_ID",   0,  (void *) IDL_TYP_LONG},
        {"META_ID", 0,  (void *) IDL_TYP_LONG},
        {"CONFIG_NAME", 0,  (void *) IDL_TYP_STRING},
        {"CONFIG_DESC", 0,  (void *) IDL_TYP_STRING},
        {"CREATION",    0,  (void *) IDL_TYP_STRING},
        {"XML",     0,  (void *) IDL_TYP_STRING},
        {"XML_CREATION", 0,  (void *) IDL_TYP_STRING},
        {0}
    };

    // Keyword Structure

    typedef struct {
        IDL_KW_RESULT_FIRST_FIELD;
        IDL_LONG verbose;
        IDL_LONG debug;
        IDL_LONG help;
    } KW_RESULT;


    // Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {
        IDL_KW_FAST_SCAN,
        {"DEBUG",   IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
        {"HELP",    IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
        {"VERBOSE", IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
        {NULL}
    };

    KW_RESULT kw;

    kw.verbose   = 0;
    kw.debug     = 0;
    kw.help      = 0;

    //-----------------------------------------------------------------------
    // Check an Integer was Passed

    IDL_ENSURE_SCALAR(argv[0]);
    handle = IDL_LongScalar(argv[0]);

    //-----------------------------------------------------------------------
    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR *)0, 1, &kw);

    if (kw.debug) {
        if (kw.debug) {
            fprintf(stdout, "Debug Keyword Passed\n");
        }

        if (kw.verbose) {
            fprintf(stdout, "Verbose Keyword Passed\n");
        }

        if (kw.help) {
            fprintf(stdout, "Help Keyword Passed\n");
        }
    }

    //--------------------------------------------------------------------------
    // Call for HELP?

    if (kw.help) {
        userhelp(stdout, "getsystemconfigmeta");
        IDL_KW_FREE;
        return (IDL_GettmpLong(0));
    }

    //--------------------------------------------------------------------------
    // Access Structure to be Returned

    system_config = getIdamSystemConfig(handle);

    if (system_config == NULL) {     // Nothing to Return
        IDL_KW_FREE;
        return (IDL_GettmpLong(GDE_NO_DATA_STRUCTURE_TO_RETURN));
    }

    //--------------------------------------------------------------------------
    // Prepare return Structure

    if ((sout = (IDAM_SYSTEM_CONFIG *)malloc(sizeof(IDAM_SYSTEM_CONFIG)) ) == NULL) {
        if (kw.verbose) {
            fprintf(stdout, "Heap Memory Allocation Failed!\n");
        }

        IDL_KW_FREE;
        return (IDL_GettmpLong(GDE_HEAP_ALLOC_ERROR));
    }

    psDef = IDL_MakeStruct(NULL, pTags);

    sout->config_id = (IDL_LONG) system_config->config_id;
    sout->system_id = (IDL_LONG) system_config->system_id;
    sout->meta_id   = (IDL_LONG) system_config->meta_id;

    IDL_StrStore(&(sout->config_name), system_config->config_name);
    IDL_StrStore(&(sout->config_desc), system_config->config_desc);
    IDL_StrStore(&(sout->creation),    system_config->creation);
    IDL_StrStore(&(sout->xml),         system_config->xml);
    IDL_StrStore(&(sout->xml_creation), system_config->xml_creation);

    if (kw.debug) {
        fprintf(stdout, "Meta Data copied to IDL Structure\n");
    }

    // Create an Anonymous IDL Structure and Import into IDL

    ilDims[0] = 1;   // import Structure as a Single element Array

    ivReturn = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR *)sout, freeMem, psDef);

    if (kw.debug) {
        fprintf(stdout, "IDL Structure Created: Ready for Return\n");
    }

    //--------------------------------------------------------------------------
    // Cleanup Keywords and IDAM Heap

    IDL_KW_FREE;
    return (ivReturn);
}

IDL_VPTR IDL_CDECL getdatasourcemeta(int argc, IDL_VPTR argv[], char * argk)
{
    //
    // Returns a Structure containing the IDAM Database Data_System table Record
    //-------------------------------------------------------------------------
    // Change History:
    //
    // 18Apr2007    dgm ilDims changed from type IDL_LONG to IDL_MEMINT
    //-------------------------------------------------------------------------
    int handle;
    DATA_SOURCE * data_source;
    char typestr[2] = " ";

    IDAM_DATA_SOURCE * sout;     // Returned Structure
    IDL_VPTR          ivReturn = NULL;

    // Passed Structure

    void * psDef = NULL;
    //IDL_LONG ilDims[IDL_MAX_ARRAY_DIM];
    IDL_MEMINT ilDims[IDL_MAX_ARRAY_DIM];

    // IDL tags structure

    IDL_STRUCT_TAG_DEF pTags[] = {
        {"SOURCE_ID",   0,  (void *) IDL_TYP_LONG},
        {"CONFIG_ID",   0,  (void *) IDL_TYP_LONG},
        {"REASON_ID",   0,  (void *) IDL_TYP_LONG},
        {"RUN_ID",  0,  (void *) IDL_TYP_LONG},
        {"META_ID", 0,  (void *) IDL_TYP_LONG},
        {"STATUS_DESC_ID",  0,  (void *) IDL_TYP_LONG},
        {"EXP_NUMBER",  0,  (void *) IDL_TYP_LONG},
        {"PASS",    0,  (void *) IDL_TYP_LONG},
        {"STATUS",  0,  (void *) IDL_TYP_LONG},
        {"STATUS_REASON_CODE",    0,    (void *) IDL_TYP_LONG},
        {"STATUS_IMPACT_CODE",    0,    (void *) IDL_TYP_LONG},
        {"ACCESS",  0,  (void *) IDL_TYP_STRING},
        {"REPROCESS",   0,  (void *) IDL_TYP_STRING},
        {"TYPE",    0,  (void *) IDL_TYP_STRING},
        {"SOURCE_ALIAS", 0,  (void *) IDL_TYP_STRING},
        {"PASS_DATE",   0,  (void *) IDL_TYP_STRING},
        {"ARCHIVE", 0,  (void *) IDL_TYP_STRING},
        {"DEVICE_NAME", 0,  (void *) IDL_TYP_STRING},
        {"FORMAT",  0,  (void *) IDL_TYP_STRING},
        {"PATH",    0,  (void *) IDL_TYP_STRING},
        {"FILENAME",    0,  (void *) IDL_TYP_STRING},
        {"SERVER",  0,  (void *) IDL_TYP_STRING},
        {"USERID",  0,  (void *) IDL_TYP_STRING},
        {"REASON_DESC", 0,  (void *) IDL_TYP_STRING},
        {"RUN_DESC",    0,  (void *) IDL_TYP_STRING},
        {"STATUS_DESC", 0,  (void *) IDL_TYP_STRING},
        {"CREATION",    0,  (void *) IDL_TYP_STRING},
        {"MODIFIED",    0,  (void *) IDL_TYP_STRING},
        {"XML",     0,  (void *) IDL_TYP_STRING},
        {"XML_CREATION", 0,  (void *) IDL_TYP_STRING},
        {0}
    };

    // Keyword Structure

    typedef struct {
        IDL_KW_RESULT_FIRST_FIELD;
        IDL_LONG verbose;
        IDL_LONG debug;
        IDL_LONG help;
    } KW_RESULT;


    // Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {
        IDL_KW_FAST_SCAN,
        {"DEBUG",   IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
        {"HELP",    IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
        {"VERBOSE", IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
        {NULL}
    };

    KW_RESULT kw;

    kw.verbose   = 0;
    kw.debug     = 0;
    kw.help      = 0;

    //-----------------------------------------------------------------------
    // Check an Integer was Passed

    IDL_ENSURE_SCALAR(argv[0]);
    handle = IDL_LongScalar(argv[0]);

    //-----------------------------------------------------------------------
    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR *)0, 1, &kw);

    if (kw.debug) {
        if (kw.debug) {
            fprintf(stdout, "Debug Keyword Passed\n");
        }

        if (kw.verbose) {
            fprintf(stdout, "Verbose Keyword Passed\n");
        }

        if (kw.help) {
            fprintf(stdout, "Help Keyword Passed\n");
        }
    }

    //--------------------------------------------------------------------------
    // Call for HELP?

    if (kw.help) {
        userhelp(stdout, "getdatasourcemeta");
        IDL_KW_FREE;
        return (IDL_GettmpLong(0));
    }

    //--------------------------------------------------------------------------
    // Access Structure to be Returned

    data_source = getIdamDataSource(handle);

    if (data_source == NULL) {   // Nothing to Return
        IDL_KW_FREE;
        return (IDL_GettmpLong(GDE_NO_DATA_STRUCTURE_TO_RETURN));
    }

    //--------------------------------------------------------------------------
    // Prepare return Structure

    if ((sout = (IDAM_DATA_SOURCE *)malloc(sizeof(IDAM_DATA_SOURCE)) ) == NULL) {
        if (kw.verbose) {
            fprintf(stdout, "Heap Memory Allocation Failed!\n");
        }

        IDL_KW_FREE;
        return (IDL_GettmpLong(GDE_HEAP_ALLOC_ERROR));
    }

    psDef = IDL_MakeStruct(NULL, pTags);

    sout->source_id          = (IDL_LONG) data_source->source_id;
    sout->config_id          = (IDL_LONG) data_source->config_id;
    sout->reason_id          = (IDL_LONG) data_source->reason_id;
    sout->run_id             = (IDL_LONG) data_source->run_id;
    sout->meta_id            = (IDL_LONG) data_source->meta_id;
    sout->status_desc_id     = (IDL_LONG) data_source->status_desc_id;
    sout->exp_number         = (IDL_LONG) data_source->exp_number;
    sout->pass               = (IDL_LONG) data_source->pass;
    sout->status             = (IDL_LONG) data_source->status;
    sout->status_reason_code = (IDL_LONG) data_source->status_reason_code;
    sout->status_impact_code = (IDL_LONG) data_source->status_impact_code;

    //typestr[0] = data_source->status;
    //IDL_StrStore(&(sout->status), typestr);
    typestr[0] = data_source->access;
    IDL_StrStore(&(sout->access), typestr);
    typestr[0] = data_source->reprocess;
    IDL_StrStore(&(sout->reprocess), typestr);
    typestr[0] = data_source->type;
    IDL_StrStore(&(sout->type), typestr);

    IDL_StrStore(&(sout->source_alias), data_source->source_alias);
    IDL_StrStore(&(sout->pass_date),    data_source->pass_date);
    IDL_StrStore(&(sout->archive),      data_source->archive);
    IDL_StrStore(&(sout->device_name),  data_source->device_name);
    IDL_StrStore(&(sout->format),       data_source->format);
    IDL_StrStore(&(sout->path),         data_source->path);
    IDL_StrStore(&(sout->filename),     data_source->filename);
    IDL_StrStore(&(sout->server),       data_source->server);
    IDL_StrStore(&(sout->userid),       data_source->userid);
    IDL_StrStore(&(sout->reason_desc),  data_source->reason_desc);
    IDL_StrStore(&(sout->run_desc),     data_source->run_desc);
    IDL_StrStore(&(sout->status_desc),  data_source->status_desc);
    IDL_StrStore(&(sout->creation),     data_source->creation);
    IDL_StrStore(&(sout->modified),     data_source->modified);
    IDL_StrStore(&(sout->xml),          data_source->xml);
    IDL_StrStore(&(sout->xml_creation), data_source->xml_creation);

    if (kw.debug) {
        fprintf(stdout, "Meta Data copied to IDL Structure\n");
    }

    // Create an Anonymous IDL Structure and Import into IDL

    ilDims[0] = 1;   // import Structure as a Single element Array

    ivReturn = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR *)sout, freeMem, psDef);

    if (kw.debug) {
        fprintf(stdout, "IDL Structure Created: Ready for Return\n");
    }

    //--------------------------------------------------------------------------
    // Cleanup Keywords and IDAM Heap

    IDL_KW_FREE;
    return (ivReturn);
}

IDL_VPTR IDL_CDECL getsignalmeta(int argc, IDL_VPTR argv[], char * argk)
{
    //
    // Returns a Structure containing the IDAM Database Data_System table Record
    //-------------------------------------------------------------------------
    // Change History:
    //
    // 18Apr2007    dgm ilDims changed from type IDL_LONG to IDL_MEMINT
    //-------------------------------------------------------------------------
    int handle;
    SIGNAL * signal_rec;
    char typestr[2] = " ";

    IDAM_SIGNAL * sout;      // Returned Structure
    IDL_VPTR          ivReturn = NULL;

    // Passed Structure

    void * psDef = NULL;
    //IDL_LONG ilDims[IDL_MAX_ARRAY_DIM];
    IDL_MEMINT ilDims[IDL_MAX_ARRAY_DIM];

    // IDL tags structure

    IDL_STRUCT_TAG_DEF pTags[] = {
        {"SOURCE_ID",     0,    (void *) IDL_TYP_LONG},
        {"SIGNAL_DESC_ID", 0,    (void *) IDL_TYP_LONG},
        {"META_ID",   0,    (void *) IDL_TYP_LONG},
        {"STATUS_DESC_ID", 0,    (void *) IDL_TYP_LONG},
        {"STATUS",    0,    (void *) IDL_TYP_LONG},
        {"STATUS_REASON_CODE",    0,    (void *) IDL_TYP_LONG},
        {"STATUS_IMPACT_CODE",    0,    (void *) IDL_TYP_LONG},
        //{"STATUS",      0,    (void *) IDL_TYP_STRING},
        {"STATUS_DESC", 0,  (void *) IDL_TYP_STRING},
        {"ACCESS",    0,    (void *) IDL_TYP_STRING},
        {"REPROCESS",     0,    (void *) IDL_TYP_STRING},
        {"CREATION",      0,    (void *) IDL_TYP_STRING},
        {"MODIFIED",      0,    (void *) IDL_TYP_STRING},
        {"XML",       0,    (void *) IDL_TYP_STRING},
        {"XML_CREATION",  0,    (void *) IDL_TYP_STRING},
        {0}
    };

    // Keyword Structure

    typedef struct {
        IDL_KW_RESULT_FIRST_FIELD;
        IDL_LONG verbose;
        IDL_LONG debug;
        IDL_LONG help;
    } KW_RESULT;


    // Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {
        IDL_KW_FAST_SCAN,
        {"DEBUG",   IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
        {"HELP",    IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
        {"VERBOSE", IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
        {NULL}
    };

    KW_RESULT kw;

    kw.verbose   = 0;
    kw.debug     = 0;
    kw.help      = 0;

    //-----------------------------------------------------------------------
    // Check an Integer was Passed

    IDL_ENSURE_SCALAR(argv[0]);
    handle = IDL_LongScalar(argv[0]);

    //-----------------------------------------------------------------------
    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR *)0, 1, &kw);

    if (kw.debug) {
        if (kw.debug) {
            fprintf(stdout, "Debug Keyword Passed\n");
        }

        if (kw.verbose) {
            fprintf(stdout, "Verbose Keyword Passed\n");
        }

        if (kw.help) {
            fprintf(stdout, "Help Keyword Passed\n");
        }
    }

    //--------------------------------------------------------------------------
    // Call for HELP?

    if (kw.help) {
        userhelp(stdout, "getsignalmeta");
        IDL_KW_FREE;
        return (IDL_GettmpLong(0));
    }

    //--------------------------------------------------------------------------
    // Access Structure to be Returned

    signal_rec = getIdamSignal(handle);

    if (signal_rec == NULL) {    // Nothing to Return
        IDL_KW_FREE;
        return (IDL_GettmpLong(GDE_NO_DATA_STRUCTURE_TO_RETURN));
    }

    //--------------------------------------------------------------------------
    // Prepare return Structure

    if ((sout = (IDAM_SIGNAL *)malloc(sizeof(IDAM_SIGNAL)) ) == NULL) {
        if (kw.verbose) {
            fprintf(stdout, "Heap Memory Allocation Failed!\n");
        }

        IDL_KW_FREE;
        return (IDL_GettmpLong(GDE_HEAP_ALLOC_ERROR));
    }

    psDef = IDL_MakeStruct(NULL, pTags);

    sout->source_id          = (IDL_LONG) signal_rec->source_id;
    sout->signal_desc_id     = (IDL_LONG) signal_rec->signal_desc_id;
    sout->meta_id            = (IDL_LONG) signal_rec->meta_id;
    sout->status_desc_id     = (IDL_LONG) signal_rec->status_desc_id;
    sout->status             = (IDL_LONG) signal_rec->status;
    sout->status_reason_code = (IDL_LONG) signal_rec->status_reason_code;
    sout->status_impact_code = (IDL_LONG) signal_rec->status_impact_code;

    //typestr[0] = signal_rec->status;
    //IDL_StrStore(&(sout->status), typestr);
    typestr[0] = signal_rec->access;
    IDL_StrStore(&(sout->access), typestr);
    typestr[0] = signal_rec->reprocess;
    IDL_StrStore(&(sout->reprocess), typestr);

    IDL_StrStore(&(sout->status_desc), signal_rec->status_desc);
    IDL_StrStore(&(sout->creation),    signal_rec->creation);
    IDL_StrStore(&(sout->modified),    signal_rec->modified);
    IDL_StrStore(&(sout->xml),         signal_rec->xml);
    IDL_StrStore(&(sout->xml_creation), signal_rec->xml_creation);

    if (kw.debug) {
        fprintf(stdout, "Meta Data copied to IDL Structure\n");
    }

    // Create an Anonymous IDL Structure and Import into IDL

    ilDims[0] = 1;   // import Structure as a Single element Array

    ivReturn = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR *)sout, freeMem, psDef);

    if (kw.debug) {
        fprintf(stdout, "IDL Structure Created: Ready for Return\n");
    }

    //--------------------------------------------------------------------------
    // Cleanup Keywords and IDAM Heap

    IDL_KW_FREE;
    return (ivReturn);
}

IDL_VPTR IDL_CDECL getsignaldescmeta(int argc, IDL_VPTR argv[], char * argk)
{
    //
    // Returns a Structure containing the IDAM Database Data_System table Record
    //-------------------------------------------------------------------------
    // Change History:
    //
    // 18Apr2007    dgm ilDims changed from type IDL_LONG to IDL_MEMINT
    //-------------------------------------------------------------------------
    int handle;
    SIGNAL_DESC * signal_desc;

    IDAM_SIGNAL_DESC * sout;     // Returned Structure
    IDL_VPTR          ivReturn = NULL;

    // Passed Structure

    void * psDef = NULL;
    //IDL_LONG ilDims[IDL_MAX_ARRAY_DIM];
    IDL_MEMINT ilDims[IDL_MAX_ARRAY_DIM];

    // IDL tags structure

    IDL_STRUCT_TAG_DEF pTags[] = {
        {"SIGNAL_DESC_ID", 0,   (void *) IDL_TYP_LONG},
        {"META_ID",    0,   (void *) IDL_TYP_LONG},
        {"RANK",       0,   (void *) IDL_TYP_LONG},
        {"RANGE_START",    0,   (void *) IDL_TYP_LONG},
        {"RANGE_STOP",     0,   (void *) IDL_TYP_LONG},
        {"SIGNAL_ALIAS",   0,   (void *) IDL_TYP_STRING},
        {"SIGNAL_NAME",    0,   (void *) IDL_TYP_STRING},
        {"GENERIC_NAME",   0,   (void *) IDL_TYP_STRING},
        {"DESCRIPTION",    0,   (void *) IDL_TYP_STRING},
        {"SIGNAL_CLASS",   0,   (void *) IDL_TYP_STRING},
        {"SIGNAL_OWNER",   0,   (void *) IDL_TYP_STRING},
        {"CREATION",       0,   (void *) IDL_TYP_STRING},
        {"MODIFIED",       0,   (void *) IDL_TYP_STRING},
        {"XML",        0,   (void *) IDL_TYP_STRING},
        {"XML_CREATION",   0,   (void *) IDL_TYP_STRING},
        {0}
    };

    // Keyword Structure

    typedef struct {
        IDL_KW_RESULT_FIRST_FIELD;
        IDL_LONG verbose;
        IDL_LONG debug;
        IDL_LONG help;
    } KW_RESULT;


    // Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {
        IDL_KW_FAST_SCAN,
        {"DEBUG",   IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
        {"HELP",    IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
        {"VERBOSE", IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
        {NULL}
    };

    KW_RESULT kw;

    kw.verbose   = 0;
    kw.debug     = 0;
    kw.help      = 0;

    //-----------------------------------------------------------------------
    // Check an Integer was Passed

    IDL_ENSURE_SCALAR(argv[0]);
    handle = IDL_LongScalar(argv[0]);

    //-----------------------------------------------------------------------
    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR *)0, 1, &kw);

    if (kw.debug) {
        if (kw.debug) {
            fprintf(stdout, "Debug Keyword Passed\n");
        }

        if (kw.verbose) {
            fprintf(stdout, "Verbose Keyword Passed\n");
        }

        if (kw.help) {
            fprintf(stdout, "Help Keyword Passed\n");
        }
    }

    //--------------------------------------------------------------------------
    // Call for HELP?

    if (kw.help) {
        userhelp(stdout, "getsignaldescmeta");
        IDL_KW_FREE;
        return (IDL_GettmpLong(0));
    }

    //--------------------------------------------------------------------------
    // Access Structure to be Returned

    signal_desc = getIdamSignalDesc(handle);

    if (signal_desc == NULL) {   // Nothing to Return
        IDL_KW_FREE;
        return (IDL_GettmpLong(GDE_NO_DATA_STRUCTURE_TO_RETURN));
    }

    //--------------------------------------------------------------------------
    // Prepare return Structure

    if ((sout = (IDAM_SIGNAL_DESC *)malloc(sizeof(IDAM_SIGNAL_DESC)) ) == NULL) {
        if (kw.verbose) {
            fprintf(stdout, "Heap Memory Allocation Failed!\n");
        }

        IDL_KW_FREE;
        return (IDL_GettmpLong(GDE_HEAP_ALLOC_ERROR));
    }

    psDef = IDL_MakeStruct(NULL, pTags);

    sout->signal_desc_id = (IDL_LONG) signal_desc->signal_desc_id;
    sout->meta_id        = (IDL_LONG) signal_desc->meta_id;
    sout->rank           = (IDL_LONG) signal_desc->rank;
    sout->range_start    = (IDL_LONG) signal_desc->range_start;
    sout->range_stop     = (IDL_LONG) signal_desc->range_stop;

    IDL_StrStore(&(sout->signal_alias), signal_desc->signal_alias);
    IDL_StrStore(&(sout->signal_name),  signal_desc->signal_name);
    IDL_StrStore(&(sout->generic_name), signal_desc->generic_name);
    IDL_StrStore(&(sout->description),  signal_desc->description);
    IDL_StrStore(&(sout->signal_class), signal_desc->signal_class);
    IDL_StrStore(&(sout->signal_owner), signal_desc->signal_owner);
    IDL_StrStore(&(sout->creation),     signal_desc->creation);
    IDL_StrStore(&(sout->modified),     signal_desc->modified);
    IDL_StrStore(&(sout->xml),          signal_desc->xml);
    IDL_StrStore(&(sout->xml_creation), signal_desc->xml_creation);

    if (kw.debug) {
        fprintf(stdout, "Meta Data copied to IDL Structure\n");
    }

    // Create an Anonymous IDL Structure and Import into IDL

    ilDims[0] = 1;   // import Structure as a Single element Array

    ivReturn = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR *)sout, freeMem, psDef);

    if (kw.debug) {
        fprintf(stdout, "IDL Structure Created: Ready for Return\n");
    }

    //--------------------------------------------------------------------------
    // Cleanup Keywords and IDAM Heap

    IDL_KW_FREE;
    return (ivReturn);
}

IDL_VPTR IDL_CDECL getxmldoc(int argc, IDL_VPTR argv[], char * argk)
{
    //
    // Returns a Structure containing the IDAM Opaque XML Data Record
    //-------------------------------------------------------------------------
    // Change History:
    //
    // 25Jun2012    dgm Original version
    //-------------------------------------------------------------------------
    int handle;
    IDL_ENSURE_SCALAR(argv[0]);
    IDL_ENSURE_SCALAR(argv[1]);
    IDL_EXCLUDE_EXPR(argv[1]);

    handle = IDL_LongScalar(argv[0]);

    IDL_StoreScalarZero(argv[1], IDL_TYP_STRING);

    DATA_BLOCK * data_block = getIdamDataBlock(handle);

    /*
       printIdamDataBlock(stdout, *data_block);
       fprintf(stdout,"Handle      : %d\n",handle);
       fprintf(stdout,"Opaque type : %d\n",data_block->opaque_type);
       fprintf(stdout,"Opaque count: %d\n",data_block->opaque_count);
       if(data_block->opaque_block != NULL) fprintf(stdout,"Opaque block: %s\n",data_block->opaque_block);
       fflush(stdout);
    */
    if (data_block->opaque_type == OPAQUE_TYPE_XML_DOCUMENT && data_block->opaque_count > 0 && data_block->opaque_block != NULL) {
        IDL_StrStore(&argv[1]->value.str, (char *)data_block->opaque_block);
    } else {
        return (IDL_GettmpLong(1));
    }

    return (IDL_GettmpLong(0));
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

IDL_VPTR IDL_CDECL setproperty(int argc, IDL_VPTR argv[], char * argk)
{
    IDL_ENSURE_STRING(argv[0]);  // Single String
    IDL_ENSURE_SCALAR(argv[0]);
    setIdamProperty((char *) IDL_STRING_STR(&argv[0]->value.str));
    return (IDL_GettmpLong(0));
}

IDL_VPTR IDL_CDECL resetproperty(int argc, IDL_VPTR argv[], char * argk)
{
    IDL_ENSURE_STRING(argv[0]);  // Single String
    IDL_ENSURE_SCALAR(argv[0]);
    resetIdamProperty((char *) IDL_STRING_STR(&argv[0]->value.str));
    return (IDL_GettmpLong(0));
}

IDL_VPTR IDL_CDECL resetproperties(int argc, IDL_VPTR argv[], char * argk)
{
    resetIdamProperties();
    return (IDL_GettmpLong(0));
}

IDL_VPTR IDL_CDECL setidamclientflag(int argc, IDL_VPTR argv[], char * argk)
{
    IDL_ENSURE_SCALAR(argv[0]);
    setIdamClientFlag((unsigned int)IDL_ULongScalar(argv[0]));
    return (IDL_GettmpLong(0));
}

IDL_VPTR IDL_CDECL resetidamclientflag(int argc, IDL_VPTR argv[], char * argk)
{
    IDL_ENSURE_SCALAR(argv[0]);
    resetIdamClientFlag((unsigned int)IDL_ULongScalar(argv[0]));
    return (IDL_GettmpLong(0));
}

IDL_VPTR IDL_CDECL getdatatypeid(int argc, IDL_VPTR argv[], char * argk)
{
    IDL_ENSURE_STRING(argv[0]);  // Single String
    IDL_ENSURE_SCALAR(argv[0]);
    return (IDL_GettmpLong(getIdamDataTypeId((char *) IDL_STRING_STR(&argv[0]->value.str))));
}

IDL_VPTR IDL_CDECL geterrormodelid(int argc, IDL_VPTR argv[], char * argk)
{
    IDL_ENSURE_STRING(argv[0]);  // Single String
    IDL_ENSURE_SCALAR(argv[0]);
    return (IDL_GettmpLong(getIdamErrorModelId((char *) IDL_STRING_STR(&argv[0]->value.str))));
}

IDL_VPTR IDL_CDECL putidamserverhost(int argc, IDL_VPTR argv[], char * argk)
{
    IDL_ENSURE_STRING(argv[0]);  // Single String
    IDL_ENSURE_SCALAR(argv[0]);
    putIdamServerHost((char *) IDL_STRING_STR(&argv[0]->value.str));
    return (IDL_GettmpLong(0));
}

IDL_VPTR IDL_CDECL putidamserverport(int argc, IDL_VPTR argv[], char * argk)
{
    IDL_ENSURE_SCALAR(argv[0]);
    putIdamServerPort((int) IDL_LongScalar(argv[0]));
    return (IDL_GettmpLong(0));
}

IDL_VPTR IDL_CDECL getidamserverhost(int argc, IDL_VPTR argv[], char * argk)
{
    IDL_ENSURE_SCALAR(argv[0]);
    IDL_StoreScalarZero(argv[0], IDL_TYP_STRING);
    IDL_StrStore(&argv[0]->value.str, (char *)getIdamServerHost());
    return (IDL_GettmpLong(0));
}

IDL_VPTR IDL_CDECL getidamserverport(int argc, IDL_VPTR argv[], char * argk)
{
    return (IDL_GettmpLong(getIdamServerPort()));
}

IDL_VPTR IDL_CDECL getidamclientversion(int argc, IDL_VPTR argv[], char * argk)
{
    return (IDL_GettmpLong(getIdamClientVersion()));
}

IDL_VPTR IDL_CDECL getidamserverversion(int argc, IDL_VPTR argv[], char * argk)
{
    return (IDL_GettmpLong(getIdamServerVersion()));
}

IDL_VPTR IDL_CDECL getidamserversocket(int argc, IDL_VPTR argv[], char * argk)
{
    return (IDL_GettmpLong(getIdamServerSocket()));
}
IDL_VPTR IDL_CDECL putidamserversocket(int argc, IDL_VPTR argv[], char * argk)
{
    IDL_ENSURE_SCALAR(argv[0]);
    putIdamServerSocket((int) IDL_LongScalar(argv[0]));
    return (IDL_GettmpLong(0));
}

IDL_VPTR IDL_CDECL geterrorasymmetry(int argc, IDL_VPTR argv[], char * argk)
{
    int handle;
    IDL_ENSURE_SCALAR(argv[0]);
    handle  = (int)IDL_LongScalar(argv[0]);
    return (IDL_GettmpLong(getIdamErrorAsymmetry(handle)));
}

IDL_VPTR IDL_CDECL puterrormodel(int argc, IDL_VPTR argv[], char * argk)
{
    int handle, model, param_n;
    float * params;
    IDL_ENSURE_SCALAR(argv[0]);
    IDL_ENSURE_SCALAR(argv[1]);
    IDL_ENSURE_ARRAY(argv[2]);
    handle  = (int)IDL_LongScalar(argv[0]);
    model   = (int)IDL_LongScalar(argv[1]);
    param_n = (int)argv[2]->value.arr->n_elts;
    params  = (float *)argv[2]->value.arr->data;     // Need to apply a type test
    //fprintf(stdout,"Param[0] = %f\n", params[0]);
    //fprintf(stdout,"Param[1] = %f\n", params[1]);
    putIdamErrorModel(handle, model, param_n, params);
    return (IDL_GettmpLong(0));
}

IDL_VPTR IDL_CDECL geterrormodel(int argc, IDL_VPTR argv[], char * argk)
{
    //
    // Change History:
    //
    // 18Apr2007    dgm ilDims changed from type IDL_LONG to IDL_MEMINT
    //          dlen[] changed from type IDL_LONG to IDL_MEMINT
    //-------------------------------------------------------------------------

    int handle;

    DATA_BLOCK * data_block = NULL;

    IDAM_MOUT * sout;    // Returned Model Structure

    IDL_VPTR ivReturn = NULL;

    // Passed Structure

    void * psDef = NULL;
    //IDL_LONG ilDims[IDL_MAX_ARRAY_DIM];
    IDL_MEMINT ilDims[IDL_MAX_ARRAY_DIM];

    static IDL_MEMINT dlen[] = {1, 1};   // Rank 7 data Array: Default is Rank 1, Length 1
    //static IDL_LONG dlen[] = {1,1};

    // IDL tags structure (type Dependent)

    IDL_STRUCT_TAG_DEF pTagsModel[] = {              // Single Precision
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"MODEL",   0,  (void *) IDL_TYP_LONG},
        {"PARAM_N", 0,  (void *) IDL_TYP_LONG},
        {"PARAMS",  dlen,   (void *) IDL_TYP_FLOAT},
        {0}
    };

    // Keyword Structure

    typedef struct {
        IDL_KW_RESULT_FIRST_FIELD;
        IDL_LONG verbose;
        IDL_LONG debug;
        IDL_LONG help;
    } KW_RESULT;


    // Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {
        IDL_KW_FAST_SCAN,
        {"DEBUG",   IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
        {"HELP",    IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
        {"VERBOSE", IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
        {NULL}
    };

    KW_RESULT kw;

    kw.verbose   = 0;
    kw.debug     = 0;
    kw.help      = 0;

    //-----------------------------------------------------------------------
    // Check a Scalar was Passed

    IDL_ENSURE_SCALAR(argv[0]);
    handle = IDL_LongScalar(argv[0]);

    //-----------------------------------------------------------------------
    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR *)0, 1, &kw);

    if (kw.debug) {
        if (kw.debug) {
            fprintf(stdout, "Debug Keyword Passed\n");
        }

        if (kw.verbose) {
            fprintf(stdout, "Verbose Keyword Passed\n");
        }

        if (kw.help) {
            fprintf(stdout, "Help Keyword Passed\n");
        }
    }

    //--------------------------------------------------------------------------
    // Call for HELP?

    if (kw.help) {
        userhelp(stdout, "geterrormodel");
        IDL_KW_FREE;
        return (IDL_GettmpLong(0));
    }

    //--------------------------------------------------------------------------
    // Extract the appropriate Data Block structure

    data_block = getIdamDataBlock(handle);

    if (data_block == NULL) {
        if (kw.verbose) {
            fprintf(stdout, "No Data associated with this handle!\n");
        }

        IDL_KW_FREE;
        return (IDL_GettmpLong(GDE_HEAP_ALLOC_ERROR));
    }

    //--------------------------------------------------------------------------
    // Prepare return Structure: Size is Base Structure + Params Array Length

    sout = (IDAM_MOUT *)malloc(sizeof(IDAM_MOUT) + sizeof(float) * data_block->error_param_n);

    if (sout == NULL) {
        if (kw.verbose) {
            fprintf(stdout, "Heap Memory Allocation Failed!\n");
        }

        IDL_KW_FREE;
        return (IDL_GettmpLong(GDE_HEAP_ALLOC_ERROR));
    }

    // This contiguous block of memory is used so that a single free releases the heap - simpler!

    memcpy((void *)&sout->params, (void *)data_block->errparams,
           (size_t)data_block->error_param_n * sizeof(float));

    dlen[0] = 1;
    dlen[1] = (IDL_LONG) data_block->error_param_n;

    psDef = IDL_MakeStruct(NULL, pTagsModel);

    sout->handle  = (IDL_LONG) handle;
    sout->model   = (IDL_LONG) data_block->error_model;
    sout->param_n = (IDL_LONG) data_block->error_param_n;

    if (kw.debug) {
        fprintf(stdout, "Handle     : %ld\n", (long)sout->handle);
        fprintf(stdout, "Error Model: %ld\n", (long)sout->model);
        fprintf(stdout, "No. Params : %ld\n", (long)sout->param_n);
    }

    // Create an Anonymous IDL Structure and Import into IDL

    ilDims[0] = 1;   // import Structure as a Single element Array

    ivReturn = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR *)sout, freeMem, psDef);

    if (kw.debug) {
        fprintf(stdout, "IDL Structure Created: Ready for Return\n");
    }

    //--------------------------------------------------------------------------
    // Cleanup Keywords and IDAM Heap

    IDL_KW_FREE;
    return (ivReturn);
}

//================================================================================================
//================================================================================================

IDL_VPTR IDL_CDECL getdimerrorasymmetry(int argc, IDL_VPTR argv[], char * argk)
{
    int handle, dimid;
    IDL_ENSURE_SCALAR(argv[0]);
    IDL_ENSURE_SCALAR(argv[1]);
    handle = (int)IDL_LongScalar(argv[0]);
    dimid  = (int)IDL_LongScalar(argv[1]);
    return (IDL_GettmpLong(getIdamDimErrorAsymmetry(handle, dimid)));
}

IDL_VPTR IDL_CDECL putdimerrormodel(int argc, IDL_VPTR argv[], char * argk)
{
    int handle, dimid, model, param_n;
    float * params;
    IDL_ENSURE_SCALAR(argv[0]);
    IDL_ENSURE_SCALAR(argv[1]);
    IDL_ENSURE_SCALAR(argv[2]);
    IDL_ENSURE_ARRAY(argv[3]);
    handle  = (int)IDL_LongScalar(argv[0]);
    dimid   = (int)IDL_LongScalar(argv[1]);
    model   = (int)IDL_LongScalar(argv[2]);
    param_n = (int)argv[3]->value.arr->n_elts;
    params  = (float *)argv[3]->value.arr->data;         // Need a type check!
    putIdamDimErrorModel(handle, dimid, model, param_n, params);
    return (IDL_GettmpLong(0));
}

IDL_VPTR IDL_CDECL getdimerrormodel(int argc, IDL_VPTR argv[], char * argk)
{
    //
    // Change History:
    //
    // 16Nov2006  D.G.Muir  Original Version
    // 18Apr2007    dgm ilDims changed from type IDL_LONG to IDL_MEMINT
    //          dlen[] changed from type IDL_LONG to IDL_MEMINT
    //-------------------------------------------------------------------------
    int handle, dimid;

    DIMS * dims = NULL;

    IDAM_MDIMOUT * sout;     // Returned Model Structure

    IDL_VPTR ivReturn = NULL;

    // Passed Structure

    void * psDef = NULL;
    //IDL_LONG ilDims[IDL_MAX_ARRAY_DIM];
    IDL_MEMINT ilDims[IDL_MAX_ARRAY_DIM];

    static IDL_MEMINT dlen[] = {1, 1};   // Rank 7 data Array: Default is Rank 1, Length 1
    //static IDL_LONG dlen[] = {1,1};

    // IDL tags structure (type Dependent)

    IDL_STRUCT_TAG_DEF pTagsModel[] = {              // Single Precision
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DIM_ID",  0,  (void *) IDL_TYP_LONG},
        {"MODEL",   0,  (void *) IDL_TYP_LONG},
        {"PARAM_N", 0,  (void *) IDL_TYP_LONG},
        {"PARAMS",  dlen,   (void *) IDL_TYP_FLOAT},
        {0}
    };

    // Keyword Structure

    typedef struct {
        IDL_KW_RESULT_FIRST_FIELD;
        IDL_LONG verbose;
        IDL_LONG debug;
        IDL_LONG help;
    } KW_RESULT;


    // Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {
        IDL_KW_FAST_SCAN,
        {"DEBUG",   IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
        {"HELP",    IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
        {"VERBOSE", IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
        {NULL}
    };

    KW_RESULT kw;

    kw.verbose   = 0;
    kw.debug     = 0;
    kw.help      = 0;

    //-----------------------------------------------------------------------
    // Check a Scalar was Passed

    IDL_ENSURE_SCALAR(argv[0]);
    IDL_ENSURE_SCALAR(argv[1]);

    handle = IDL_LongScalar(argv[0]);
    dimid  = IDL_LongScalar(argv[1]);

    //-----------------------------------------------------------------------
    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR *)0, 1, &kw);

    if (kw.debug) {
        if (kw.debug) {
            fprintf(stdout, "Debug Keyword Passed\n");
        }

        if (kw.verbose) {
            fprintf(stdout, "Verbose Keyword Passed\n");
        }

        if (kw.help) {
            fprintf(stdout, "Help Keyword Passed\n");
        }
    }

    //--------------------------------------------------------------------------
    // Call for HELP?

    if (kw.help) {
        userhelp(stdout, "getdimerrormodel");
        IDL_KW_FREE;
        return (IDL_GettmpLong(0));
    }

    //--------------------------------------------------------------------------
    // Extract the appropriate Data Block structure

    dims = getIdamDimBlock(handle, dimid);

    if (dims == NULL) {
        if (kw.verbose) {
            fprintf(stdout, "No Dimensional Data associated with this handle and dimension id!\n");
        }

        IDL_KW_FREE;
        return (IDL_GettmpLong(GDE_HEAP_ALLOC_ERROR));
    }

    //--------------------------------------------------------------------------
    // Prepare return Structure: Size is Base Structure + Params Array Length

    sout = (IDAM_MDIMOUT *)malloc(sizeof(IDAM_MDIMOUT) + sizeof(float) * dims->error_param_n);

    if (sout == NULL) {
        if (kw.verbose) {
            fprintf(stdout, "Heap Memory Allocation Failed!\n");
        }

        IDL_KW_FREE;
        return (IDL_GettmpLong(GDE_HEAP_ALLOC_ERROR));
    }

    // This contiguous block of memory is used so that a single free releases the heap - simpler!

    memcpy((void *)&sout->params, (void *)dims->errparams, (size_t)dims->error_param_n * sizeof(float));

    dlen[0] = 1;
    dlen[1] = (IDL_LONG) dims->error_param_n;

    psDef = IDL_MakeStruct(NULL, pTagsModel);

    sout->handle  = (IDL_LONG) handle;
    sout->dim_id  = (IDL_LONG) dimid;
    sout->model   = (IDL_LONG) dims->error_model;
    sout->param_n = (IDL_LONG) dims->error_param_n;

    if (kw.debug) {
        fprintf(stdout, "Handle      : %ld\n", (long)sout->handle);
        fprintf(stdout, "Dimension Id: %ld\n", (long)sout->dim_id);
        fprintf(stdout, "Error Model : %ld\n", (long)sout->model);
        fprintf(stdout, "No. Params  : %ld\n", (long)sout->param_n);
    }

    // Create an Anonymous IDL Structure and Import into IDL

    ilDims[0] = 1;   // import Structure as a Single element Array

    ivReturn = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR *)sout, freeMem, psDef);

    if (kw.debug) {
        fprintf(stdout, "IDL Structure Created: Ready for Return\n");
    }

    //--------------------------------------------------------------------------
    // Cleanup Keywords and IDAM Heap

    IDL_KW_FREE;
    return (ivReturn);
}

IDL_VPTR IDL_CDECL getlasthandle(int argc, IDL_VPTR argv[], char * argk)
{
    return (IDL_GettmpLong(getIdamLastHandle()));
}

//====================================================================================


//#####################################################################################################

IDL_VPTR IDL_CDECL getdomains(int argc, IDL_VPTR argv[], char * argk)
{
    //
    // Returns IDA File Domain details
    //
    // Change History:
    //
    // 22Jun2007    dgm Original Version
    //-------------------------------------------------------------------------

    struct DOMAINS {
        int  handle;     // Data Block Handle
        int  dimid;      // Dimension id
        int  method;     // Compression Method
        int      udoms;      // Number of Domains
        long   *  sams;      // Array of Domain Lengths         == sams[udoms]
        char   *  offs;      // Array of Domain Starting Values == offs[udoms]
        char   *  ints;      // Array of Domain Interval Values == ints[udoms]
    } ;
    typedef struct DOMAINS DOMAINS ;

    int i, handle, dimid, dsize;
    DIMS  *  dim = NULL;
    DOMAINS * sout = NULL;

    float * fp1, *fp2;
    double * dp1, *dp2;
    short * sp1, *sp2;
    int  *  ip1, *ip2;
    long  * lp1, *lp2;
    unsigned int * up1, *up2;

    IDL_VPTR ivReturn = NULL;

    // Passed Structure

    void * psDef = NULL;
    IDL_MEMINT ilDims[IDL_MAX_ARRAY_DIM];

    static IDL_MEMINT dlen[] = {1, 1};   // Rank 1 domain data arrays

    // IDL tags structure (type Dependent)

    IDL_STRUCT_TAG_DEF pTagsFloat[] = {              // Single Precision
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DIMID",   0,  (void *) IDL_TYP_LONG},
        {"METHOD",  0,  (void *) IDL_TYP_LONG},
        {"UDOMS",   0,  (void *) IDL_TYP_LONG},
        {"SAMS",    dlen,   (void *) IDL_TYP_LONG},
        {"OFFS",    dlen,   (void *) IDL_TYP_FLOAT},
        {"INTS",    dlen,   (void *) IDL_TYP_FLOAT},
        {0}
    };

    IDL_STRUCT_TAG_DEF pTagsDouble[] = {             // Double Precision
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DIMID",   0,  (void *) IDL_TYP_LONG},
        {"METHOD",  0,  (void *) IDL_TYP_LONG},
        {"UDOMS",   0,  (void *) IDL_TYP_LONG},
        {"SAMS",    dlen,   (void *) IDL_TYP_LONG},
        {"OFFS",    dlen,   (void *) IDL_TYP_DOUBLE},
        {"INTS",    dlen,   (void *) IDL_TYP_DOUBLE},
        {0}
    };

    IDL_STRUCT_TAG_DEF pTagsShort[] = {              // Short Integer
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DIMID",   0,  (void *) IDL_TYP_LONG},
        {"METHOD",  0,  (void *) IDL_TYP_LONG},
        {"UDOMS",   0,  (void *) IDL_TYP_LONG},
        {"SAMS",    dlen,   (void *) IDL_TYP_LONG},
        {"OFFS",    dlen,   (void *) IDL_TYP_INT},
        {"INTS",    dlen,   (void *) IDL_TYP_INT},
        {0}
    };

    IDL_STRUCT_TAG_DEF pTagsInt[] = {                // Standard Integer
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DIMID",   0,  (void *) IDL_TYP_LONG},
        {"METHOD",  0,  (void *) IDL_TYP_LONG},
        {"UDOMS",   0,  (void *) IDL_TYP_LONG},
        {"SAMS",    dlen,   (void *) IDL_TYP_LONG},
        {"OFFS",    dlen,   (void *) IDL_TYP_LONG},
        {"INTS",    dlen,   (void *) IDL_TYP_LONG},
        {0}
    };

    IDL_STRUCT_TAG_DEF pTagsLong[] = {               // Double Integer
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DIMID",   0,  (void *) IDL_TYP_LONG},
        {"METHOD",  0,  (void *) IDL_TYP_LONG},
        {"UDOMS",   0,  (void *) IDL_TYP_LONG},
        {"SAMS",    dlen,   (void *) IDL_TYP_LONG},
        {"OFFS",    dlen,   (void *) IDL_TYP_LONG64},
        {"INTS",    dlen,   (void *) IDL_TYP_LONG64},
        {0}
    };

    IDL_STRUCT_TAG_DEF pTagsUnsignedShort[] = {          // Unsigned Short Integer
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DIMID",   0,  (void *) IDL_TYP_LONG},
        {"METHOD",  0,  (void *) IDL_TYP_LONG},
        {"UDOMS",   0,  (void *) IDL_TYP_LONG},
        {"SAMS",    dlen,   (void *) IDL_TYP_LONG},
        {"OFFS",    dlen,   (void *) IDL_TYP_UINT},
        {"INTS",    dlen,   (void *) IDL_TYP_UINT},
        {0}
    };
    IDL_STRUCT_TAG_DEF pTagsUnsignedInt[] = {
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DIMID",   0,  (void *) IDL_TYP_LONG},
        {"METHOD",  0,  (void *) IDL_TYP_LONG},
        {"UDOMS",   0,  (void *) IDL_TYP_LONG},
        {"SAMS",    dlen,   (void *) IDL_TYP_LONG},
        {"OFFS",    dlen,   (void *) IDL_TYP_ULONG},
        {"INTS",    dlen,   (void *) IDL_TYP_ULONG},
        {0}
    };

    IDL_STRUCT_TAG_DEF pTagsUnsignedLong[] = {           // Unsigned Double Integer
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DIMID",   0,  (void *) IDL_TYP_LONG},
        {"METHOD",  0,  (void *) IDL_TYP_LONG},
        {"UDOMS",   0,  (void *) IDL_TYP_LONG},
        {"SAMS",    dlen,   (void *) IDL_TYP_LONG},
        {"OFFS",    dlen,   (void *) IDL_TYP_ULONG64},
        {"INTS",    dlen,   (void *) IDL_TYP_ULONG64},
        {0}
    };

    IDL_STRUCT_TAG_DEF pTagsChar[] = {               // Char
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DIMID",   0,  (void *) IDL_TYP_LONG},
        {"METHOD",  0,  (void *) IDL_TYP_LONG},
        {"UDOMS",   0,  (void *) IDL_TYP_LONG},
        {"SAMS",    dlen,   (void *) IDL_TYP_LONG},
        {"OFFS",    dlen,   (void *) IDL_TYP_BYTE},
        {"INTS",    dlen,   (void *) IDL_TYP_BYTE},
        {0}
    };

    IDL_STRUCT_TAG_DEF pTagsUnsignedChar[] = {           // Unsigned Char
        {"HANDLE",  0,  (void *) IDL_TYP_LONG},
        {"DIMID",   0,  (void *) IDL_TYP_LONG},
        {"METHOD",  0,  (void *) IDL_TYP_LONG},
        {"UDOMS",   0,  (void *) IDL_TYP_LONG},
        {"SAMS",    dlen,   (void *) IDL_TYP_LONG},
        {"OFFS",    dlen,   (void *) IDL_TYP_BYTE},
        {"INTS",    dlen,   (void *) IDL_TYP_BYTE},
        {0}
    };

    // Keyword Structure

    typedef struct {
        IDL_KW_RESULT_FIRST_FIELD;
        IDL_LONG verbose;
        IDL_LONG debug;
        IDL_LONG help;
    } KW_RESULT;


    // Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {
        IDL_KW_FAST_SCAN,
        {"DEBUG",   IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
        {"HELP",    IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
        {"VERBOSE", IDL_TYP_LONG,  1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
        {NULL}
    };

    KW_RESULT kw;

    kw.verbose   = 0;
    kw.debug     = 0;
    kw.help      = 0;

    //-----------------------------------------------------------------------
    // Check two Scalars were Passed

    IDL_ENSURE_SCALAR(argv[0]);
    IDL_ENSURE_SCALAR(argv[1]);

    handle = IDL_LongScalar(argv[0]);
    dimid  = IDL_LongScalar(argv[1]);

    //-----------------------------------------------------------------------
    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR *)0, 1, &kw);

    if (kw.debug) {
        if (kw.debug) {
            fprintf(stdout, "Debug Keyword Passed\n");
        }

        if (kw.verbose) {
            fprintf(stdout, "Verbose Keyword Passed\n");
        }

        if (kw.help) {
            fprintf(stdout, "Help Keyword Passed\n");
        }
    }

    //--------------------------------------------------------------------------
    // Call for HELP?

    if (kw.help) {
        userhelp(stdout, "getdomains");
        IDL_KW_FREE;
        return (IDL_GettmpLong(0));
    }

    //--------------------------------------------------------------------------
    // Pointer to the Data Block containing Domain

    if ((dim = getIdamDimBlock(handle, dimid)) == NULL) {
        if (kw.verbose) {
            fprintf(stdout, "getDomains: Domain Data pointer is NULL!!!\n");
        }

        if (kw.debug) {
            fprintf(stdout, "Handle: %d\n", handle);
            fprintf(stdout, "Dim id: %d\n", dimid);
            printIdamDataBlock(stdout, *getIdamDataBlock(handle));
        }

        IDL_KW_FREE;
        return (IDL_GettmpLong(GDE_NO_DATA_TO_RETURN));
    }

    if (kw.debug) {
        printIdamDataBlock(stdout, *getIdamDataBlock(handle));
    }

    //--------------------------------------------------------------------------
    // Prepare Return Structure

    if ((dsize = dim->udoms) == 0) {
        dsize = 1;
    }

    dlen[0] = 1;
    dlen[1] = dsize;

    if (kw.debug) {
        fprintf(stdout, "Data Organisation (Rank 1)\n");
        fprintf(stdout, "[0]  %d  \n", (int)dlen[0]);
        fprintf(stdout, "[1]  %d  \n", (int)dlen[1]);
    }

    switch (dim->data_type) {

    case TYPE_FLOAT:

        if (kw.debug) {
            fprintf(stdout, "Domains are of Type FLOAT\n");
        }

        sout = (DOMAINS *)malloc(sizeof(DOMAINS) + sizeof(long) * dsize + 2 * sizeof(float) * dsize);

        if (sout == NULL) {
            break;
        }

        // This contiguous block of memory is used so that a single free releases the heap - simpler!

        fp1 = (float *)(&sout->sams) + dsize * sizeof(long);
        fp2 = (float *)(&sout->sams) + dsize * (sizeof(long) + sizeof(float));

        if (dim->udoms == 0 || dim->method < 1 || dim->method > 3) {
            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                fp1[i] = (float)0.0 ;
                fp2[i] = (float)0.0 ;
            }
        }

        if (dim->method == 1) {
            memcpy((void *)&sout->sams,                    (void *)dim->sams, (size_t)dsize * sizeof(long));
            memcpy((void *)&sout->sams + dsize * sizeof(long),                (void *)dim->offs, (size_t)dsize * sizeof(float));
            memcpy((void *)&sout->sams + dsize * (sizeof(long) + sizeof(float)), (void *)dim->ints, (size_t)dsize * sizeof(float));
        }

        if (dim->method == 2) {
            memcpy((void *)&sout->sams + dsize * sizeof(long), (void *)dim->offs, (size_t)dsize * sizeof(float));

            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                fp2[i] = (float)0.0 ;
            }
        }

        if (dim->method == 3) {
            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                fp1[i] = (float)0.0 ;
                fp2[i] = (float)0.0 ;
            }

            fp1[0] = (float) * ((float *)dim->offs);
            fp2[0] = (float) * ((float *)dim->ints);
        }

        psDef = IDL_MakeStruct(NULL, pTagsFloat);
        break;

    case TYPE_DOUBLE:

        if (kw.debug) {
            fprintf(stdout, "Domains are of Type DOUBLE\n");
        }

        sout = (DOMAINS *)malloc(sizeof(DOMAINS) + sizeof(long) * dsize + 2 * sizeof(double) * dsize);

        if (sout == NULL) {
            break;
        }

        dp1 = (double *)(&sout->sams) + dsize * sizeof(long);
        dp2 = (double *)(&sout->sams) + dsize * (sizeof(long) + sizeof(double));

        if (dim->udoms == 0 || dim->method < 1 || dim->method > 3) {
            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                dp1[i] = (double)0.0E0 ;
                dp2[i] = (double)0.0E0 ;
            }
        }

        if (dim->method == 1) {
            memcpy((void *)&sout->sams,                     (void *)dim->sams, (size_t)dsize * sizeof(long));
            memcpy((void *)&sout->sams + dsize * sizeof(long),                 (void *)dim->offs, (size_t)dsize * sizeof(double));
            memcpy((void *)&sout->sams + dsize * (sizeof(long) + sizeof(double)), (void *)dim->ints, (size_t)dsize * sizeof(double));
        }

        if (dim->method == 2) {
            memcpy((void *)&sout->sams + dsize * sizeof(long), (void *)dim->offs, (size_t)dsize * sizeof(double));

            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                dp2[i] = (double)0.0E0 ;
            }
        }

        if (dim->method == 3) {
            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                dp1[i] = (double)0.0E0 ;
                dp2[i] = (double)0.0E0 ;
            }

            dp1[0] = (double) * ((double *)dim->offs);
            dp2[0] = (double) * ((double *)dim->ints);
        }

        psDef = IDL_MakeStruct(NULL, pTagsDouble);
        break;

    case TYPE_SHORT:

        if (kw.debug) {
            fprintf(stdout, "Domains are of Type SHORT\n");
        }

        sout = (DOMAINS *)malloc(sizeof(DOMAINS) + sizeof(long) * dsize + 2 * sizeof(short) * dsize);

        if (sout == NULL) {
            break;
        }

        sp1 = (short *)(&sout->sams) + dsize * sizeof(long);
        sp2 = (short *)(&sout->sams) + dsize * (sizeof(long) + sizeof(short));


        if (dim->udoms == 0 || dim->method < 1 || dim->method > 3) {
            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                sp1[i] = (short)0;
                sp2[i] = (short)0;
            }
        }

        if (dim->method == 1) {
            memcpy((void *)&sout->sams,                    (void *)dim->sams, (size_t)dsize * sizeof(long));
            memcpy((void *)&sout->sams + dsize * sizeof(long),                (void *)dim->offs, (size_t)dsize * sizeof(short));
            memcpy((void *)&sout->sams + dsize * (sizeof(long) + sizeof(short)), (void *)dim->ints, (size_t)dsize * sizeof(short));
        }

        if (dim->method == 2) {
            memcpy((void *)&sout->sams + dsize * sizeof(long), (void *)dim->offs, (size_t)dsize * sizeof(short));

            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                sp2[i] = (short)0;
            }
        }

        if (dim->method == 3) {
            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                sp1[i] = (short)0;
                sp2[i] = (short)0;
            }

            sp1[0] = (short) * ((short *)dim->offs);
            sp2[0] = (short) * ((short *)dim->ints);
        }

        psDef = IDL_MakeStruct(NULL, pTagsShort);
        break;

    case TYPE_INT:

        if (kw.debug) {
            fprintf(stdout, "Domains are of Type INT\n");
        }

        sout = (DOMAINS *)malloc(sizeof(DOMAINS) + sizeof(long) * dsize + 2 * sizeof(int) * dsize);

        if (sout == NULL) {
            break;
        }

        ip1 = (int *)(&sout->sams) + dsize * sizeof(long);
        ip2 = (int *)(&sout->sams) + dsize * (sizeof(long) + sizeof(int));

        if (dim->udoms == 0 || dim->method < 1 || dim->method > 3) {
            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                ip1[i] = (int)0;
                ip2[i] = (int)0;
            }
        }

        if (dim->method == 1) {
            memcpy((void *)&sout->sams,                  (void *)dim->sams, (size_t)dsize * sizeof(long));
            memcpy((void *)&sout->sams + dsize * sizeof(long),              (void *)dim->offs, (size_t)dsize * sizeof(int));
            memcpy((void *)&sout->sams + dsize * (sizeof(long) + sizeof(int)), (void *)dim->ints, (size_t)dsize * sizeof(int));
        }

        if (dim->method == 2) {
            memcpy((void *)&sout->sams + dsize * sizeof(long), (void *)dim->offs, (size_t)dsize * sizeof(int));

            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                ip2[i] = (int)0;
            }
        }

        if (dim->method == 3) {
            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                ip1[i] = (int)0;
                ip2[i] = (int)0;
            }

            ip1[0] = (int) * ((int *)dim->offs);
            ip2[0] = (int) * ((int *)dim->ints);
        }

        psDef = IDL_MakeStruct(NULL, pTagsInt);
        break;

    case TYPE_LONG:

        if (kw.debug) {
            fprintf(stdout, "Domains are of Type LONG\n");
        }

        sout = (DOMAINS *)malloc(sizeof(DOMAINS) + sizeof(long) * dsize + 2 * sizeof(long) * dsize);

        if (sout == NULL) {
            break;
        }

        lp1 = (long *)(&sout->sams) + dsize * sizeof(long);
        lp2 = (long *)(&sout->sams) + dsize * (sizeof(long) + sizeof(long));

        if (dim->udoms == 0 || dim->method < 1 || dim->method > 3) {
            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                lp1[i] = (long)0;
                lp2[i] = (long)0;
            }
        }

        if (dim->method == 1) {
            memcpy((void *)&sout->sams,                   (void *)dim->sams, (size_t)dsize * sizeof(long));
            memcpy((void *)&sout->sams + dsize * sizeof(long),               (void *)dim->offs, (size_t)dsize * sizeof(long));
            memcpy((void *)&sout->sams + dsize * (sizeof(long) + sizeof(long)), (void *)dim->ints, (size_t)dsize * sizeof(long));
        }

        if (dim->method == 2) {
            memcpy((void *)&sout->sams + dsize * sizeof(long), (void *)dim->offs, (size_t)dsize * sizeof(long));

            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                lp2[i] = (long)0;
            }
        }

        if (dim->method == 3) {
            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                lp1[i] = (long)0;
                lp2[i] = (long)0;
            }

            lp1[0] = (long) * ((long *)dim->offs);
            lp2[0] = (long) * ((long *)dim->ints);
        }

        psDef = IDL_MakeStruct(NULL, pTagsLong);
        break;

    case TYPE_UNSIGNED_SHORT: {
        unsigned short * p1, *p2;

        if (kw.debug) {
            fprintf(stdout, "Domains are of Type UNSIGNED SHORT\n");
        }

        sout = (DOMAINS *)malloc(sizeof(DOMAINS) + sizeof(long) * dsize + 2 * sizeof(unsigned short) * dsize);

        if (sout == NULL) {
            break;
        }

        p1 = (unsigned short *)(&sout->sams) + dsize * sizeof(long);
        p2 = (unsigned short *)(&sout->sams) + dsize * (sizeof(long) + sizeof(unsigned short));


        if (dim->udoms == 0 || dim->method < 1 || dim->method > 3) {
            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                p1[i] = (unsigned short)0;
                p2[i] = (unsigned short)0;
            }
        }

        if (dim->method == 1) {
            memcpy((void *)&sout->sams,                             (void *)dim->sams, (size_t)dsize * sizeof(long));
            memcpy((void *)&sout->sams + dsize * sizeof(long),                         (void *)dim->offs, (size_t)dsize * sizeof(unsigned short));
            memcpy((void *)&sout->sams + dsize * (sizeof(long) + sizeof(unsigned short)), (void *)dim->ints, (size_t)dsize * sizeof(unsigned short));
        }

        if (dim->method == 2) {
            memcpy((void *)&sout->sams + dsize * sizeof(long), (void *)dim->offs, (size_t)dsize * sizeof(unsigned short));

            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                p2[i] = (unsigned short)0;
            }
        }

        if (dim->method == 3) {
            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                p1[i] = (unsigned short)0;
                p2[i] = (unsigned short)0;
            }

            p1[0] = (unsigned short) * ((unsigned short *)dim->offs);
            p2[0] = (unsigned short) * ((unsigned short *)dim->ints);
        }

        psDef = IDL_MakeStruct(NULL, pTagsUnsignedShort);
        break;
    }

    case TYPE_UNSIGNED:

        if (kw.debug) {
            fprintf(stdout, "Domains are of Type UNSIGNED\n");
        }

        sout = (DOMAINS *)malloc(sizeof(DOMAINS) + sizeof(long) * dsize + 2 * sizeof(unsigned int) * dsize);

        if (sout == NULL) {
            break;
        }

        up1 = (unsigned int *)(&sout->sams) + dsize * sizeof(long);
        up2 = (unsigned int *)(&sout->sams) + dsize * (sizeof(long) + sizeof(unsigned int));

        if (dim->udoms == 0 || dim->method < 1 || dim->method > 3) {
            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                up1[i] = (unsigned int)0;
                up2[i] = (unsigned int)0;
            }
        }

        if (dim->method == 1) {
            memcpy((void *)&sout->sams,                           (void *)dim->sams, (size_t)dsize * sizeof(long));
            memcpy((void *)&sout->sams + dsize * sizeof(long),                       (void *)dim->offs, (size_t)dsize * sizeof(unsigned int));
            memcpy((void *)&sout->sams + dsize * (sizeof(long) + sizeof(unsigned int)), (void *)dim->ints, (size_t)dsize * sizeof(unsigned int));
        }

        if (dim->method == 2) {
            memcpy((void *)&sout->sams + dsize * sizeof(long), (void *)dim->offs, (size_t)dsize * sizeof(unsigned int));

            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                up2[i] = (unsigned int)0;
            }
        }

        if (dim->method == 3) {
            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                up1[i] = (unsigned int)0;
                up2[i] = (unsigned int)0;
            }

            up1[0] = (unsigned int) * ((unsigned int *)dim->offs);
            up2[0] = (unsigned int) * ((unsigned int *)dim->ints);
        }

        psDef = IDL_MakeStruct(NULL, pTagsUnsignedInt);
        break;

    case TYPE_UNSIGNED_LONG: {
        unsigned long * p1, *p2;

        if (kw.debug) {
            fprintf(stdout, "Domains are of Type UNSIGNED LONG\n");
        }

        sout = (DOMAINS *)malloc(sizeof(DOMAINS) + sizeof(long) * dsize + 2 * sizeof(unsigned long) * dsize);

        if (sout == NULL) {
            break;
        }

        p1 = (unsigned long *)(&sout->sams) + dsize * sizeof(long);
        p2 = (unsigned long *)(&sout->sams) + dsize * (sizeof(long) + sizeof(unsigned long));

        if (dim->udoms == 0 || dim->method < 1 || dim->method > 3) {
            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                p1[i] = (unsigned long)0;
                p2[i] = (unsigned long)0;
            }
        }

        if (dim->method == 1) {
            memcpy((void *)&sout->sams,                            (void *)dim->sams, (size_t)dsize * sizeof(long));
            memcpy((void *)&sout->sams + dsize * sizeof(long),                        (void *)dim->offs, (size_t)dsize * sizeof(unsigned long));
            memcpy((void *)&sout->sams + dsize * (sizeof(long) + sizeof(unsigned long)), (void *)dim->ints, (size_t)dsize * sizeof(unsigned long));
        }

        if (dim->method == 2) {
            memcpy((void *)&sout->sams + dsize * sizeof(long), (void *)dim->offs, (size_t)dsize * sizeof(unsigned long));

            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                p2[i] = (unsigned long)0;
            }
        }

        if (dim->method == 3) {
            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                p1[i] = (unsigned long)0;
                p2[i] = (unsigned long)0;
            }

            p1[0] = (unsigned long) * ((unsigned long *)dim->offs);
            p2[0] = (unsigned long) * ((unsigned long *)dim->ints);
        }

        psDef = IDL_MakeStruct(NULL, pTagsUnsignedLong);
        break;
    }

    case TYPE_CHAR: {
        char * p1, *p2;

        if (kw.debug) {
            fprintf(stdout, "Domains are of Type CHAR\n");
        }

        sout = (DOMAINS *)malloc(sizeof(DOMAINS) + sizeof(long) * dsize + 2 * sizeof(char) * dsize);

        if (sout == NULL) {
            break;
        }

        p1 = (char *)(&sout->sams) + dsize * sizeof(long);
        p2 = (char *)(&sout->sams) + dsize * (sizeof(long) + sizeof(char));

        if (dim->udoms == 0 || dim->method < 1 || dim->method > 3) {
            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                p1[i] = (char)0;
                p2[i] = (char)0;
            }
        }

        if (dim->method == 1) {
            memcpy((void *)&sout->sams,                   (void *)dim->sams, (size_t)dsize * sizeof(long));
            memcpy((void *)&sout->sams + dsize * sizeof(long),               (void *)dim->offs, (size_t)dsize * sizeof(char));
            memcpy((void *)&sout->sams + dsize * (sizeof(long) + sizeof(char)), (void *)dim->ints, (size_t)dsize * sizeof(char));
        }

        if (dim->method == 2) {
            memcpy((void *)&sout->sams + dsize * sizeof(long), (void *)dim->offs, (size_t)dsize * sizeof(char));

            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                p2[i] = (char)0;
            }
        }

        if (dim->method == 3) {
            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                p1[i] = (char)0;
                p2[i] = (char)0;
            }

            p1[0] = (char) * ((char *)dim->offs);
            p2[0] = (char) * ((char *)dim->ints);
        }

        psDef = IDL_MakeStruct(NULL, pTagsChar);
        break;
    }

    case TYPE_UNSIGNED_CHAR: {
        unsigned char * p1, *p2;

        if (kw.debug) {
            fprintf(stdout, "Domains are of Type UNSIGNED CHAR\n");
        }

        sout = (DOMAINS *)malloc(sizeof(DOMAINS) + sizeof(long) * dsize + 2 * sizeof(unsigned char) * dsize);

        if (sout == NULL) {
            break;
        }

        p1 = (unsigned char *)(&sout->sams) + dsize * sizeof(long);
        p2 = (unsigned char *)(&sout->sams) + dsize * (sizeof(long) + sizeof(unsigned char));

        if (dim->udoms == 0 || dim->method < 1 || dim->method > 3) {
            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                p1[i] = (unsigned char)0;
                p2[i] = (unsigned char)0;
            }
        }

        if (dim->method == 1) {
            memcpy((void *)&sout->sams,                            (void *)dim->sams, (size_t)dsize * sizeof(long));
            memcpy((void *)&sout->sams + dsize * sizeof(long),                        (void *)dim->offs, (size_t)dsize * sizeof(unsigned char));
            memcpy((void *)&sout->sams + dsize * (sizeof(long) + sizeof(unsigned char)), (void *)dim->ints, (size_t)dsize * sizeof(unsigned char));
        }

        if (dim->method == 2) {
            memcpy((void *)&sout->sams + dsize * sizeof(long), (void *)dim->offs, (size_t)dsize * sizeof(unsigned char));

            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                p2[i] = (unsigned char)0;
            }
        }

        if (dim->method == 3) {
            for (i = 0; i < dsize; i++) {
                *(sout->sams + i) = (long)0 ;
                p1[i] = (unsigned char)0;
                p2[i] = (unsigned char)0;
            }

            p1[0] = (unsigned char) * ((unsigned char *)dim->offs);
            p2[0] = (unsigned char) * ((unsigned char *)dim->ints);
        }

        psDef = IDL_MakeStruct(NULL, pTagsUnsignedChar);
        break;
    }

    default:
        break;

        if (kw.verbose) {
            fprintf(stdout, "The Domain Data Type is Not Recognised [%d]\n", dim->data_type);
        }

        IDL_KW_FREE;
        return (IDL_GettmpLong(GDE_UNKNOWN_DATA_TYPE));
    }

    if (sout == NULL) {
        if (kw.verbose) {
            fprintf(stdout, "Heap Memory Allocation for Domain Structure Failed!\n");
        }

        IDL_KW_FREE;
        return (IDL_GettmpLong(GDE_HEAP_ALLOC_ERROR));
    }

    sout->handle = (IDL_LONG) handle;
    sout->dimid  = (IDL_LONG) dimid;
    sout->method = (IDL_LONG) dim->method;
    sout->udoms  = (IDL_LONG) dim->udoms;

    // Create an Anonymous IDL Structure and Import into IDL

    ilDims[0] = 1;   // import Structure as a Single element Array

    ivReturn = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR *)sout, freeMem, psDef);

    if (kw.debug) {
        fprintf(stdout, "IDL Structure Created: Ready for Return\n");
    }

    //--------------------------------------------------------------------------
    // Cleanup Keywords and IDAM Heap

    IDL_KW_FREE;
    return (ivReturn);
}