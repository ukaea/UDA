MODULE getdata
DESCRIPTION Universal Data Access (IDAM) toolset  
VERSION 4.1
SOURCE David Muir (MAST)
BUILD_DATE 19 Nov 2013
FUNCTION  idamgetapi       2 2 KEYWORDS
FUNCTION  idamputapi       2 2 KEYWORDS

FUNCTION  callidam         1 2 KEYWORDS
FUNCTION  callidam2        2 2 KEYWORDS
FUNCTION  getidamdata      1 1 KEYWORDS
FUNCTION  getidamdimdata   2 2 KEYWORDS
FUNCTION  getdataarray     1 1 KEYWORDS
FUNCTION  geterrorarray    1 1 KEYWORDS
FUNCTION  getdimdataarray  2 2 KEYWORDS
FUNCTION  freeidam      1 1
FUNCTION  freeidamall   0 0
FUNCTION  geterrorcode  1 1
FUNCTION  geterrormsg   2 2
FUNCTION  printerrormsgstack   1 1
FUNCTION  getsourcestatus 1 1
FUNCTION  getsignalstatus 1 1
FUNCTION  getdatastatus 1 1
FUNCTION  getdatanum    1 1
FUNCTION  getrank       1 1
FUNCTION  getorder      1 1
FUNCTION  getdatatype   1 1
FUNCTION  geterrortype  1 1
FUNCTION  getdatadata   1 1
FUNCTION  getdataerror  1 1
FUNCTION  getasymmetricdataerror  2 2
FUNCTION  getfloatdata  1 1
FUNCTION  getfloatdataerror  1 1
FUNCTION  getfloatasymmetricdataerror  2 2
FUNCTION  getsyntheticdata  1 1
FUNCTION  getdatalabel  2 2
FUNCTION  getdataunits  2 2
FUNCTION  getdatadesc   2 2
FUNCTION  getdimnum     2 2
FUNCTION  getdimtype    2 2
FUNCTION  getdimerrortype 2 2
FUNCTION  getdimdata    2 2
FUNCTION  getdimerror   2 2
FUNCTION  getasymmetricdimerror   3 3
FUNCTION  getfloatdimdata 2 2
FUNCTION  getfloatdimerror 2 2
FUNCTION  getfloatasymmetricdimerror 3 3
FUNCTION  getsyntheticdimdata 2 2
FUNCTION  getdimlabel   3 3
FUNCTION  getdimunits   3 3
FUNCTION  getdatasystemmeta   1 1 KEYWORDS
FUNCTION  getsystemconfigmeta 1 1 KEYWORDS
FUNCTION  getdatasourcemeta   1 1 KEYWORDS
FUNCTION  getsignalmeta       1 1 KEYWORDS
FUNCTION  getsignaldescmeta   1 1 KEYWORDS
FUNCTION  getxmldoc           2 2 KEYWORDS
FUNCTION  setproperty         1 1
FUNCTION  resetproperty       1 1
FUNCTION  resetproperties     0 0
FUNCTION  setidamclientflag   1 1
FUNCTION  resetidamclientflag 1 1
FUNCTION  putidamserverhost   1 1
FUNCTION  putidamserverport   1 1
FUNCTION  getidamserverhost   1 1
FUNCTION  getidamserverport   0 0
FUNCTION  getidamclientversion    0 0
FUNCTION  getidamserverversion    0 0
FUNCTION  putidamserversocket    1 1
FUNCTION  getidamserversocket    0 0
FUNCTION  puterrormodel        3 3
FUNCTION  geterrormodel           1 1 KEYWORDS
FUNCTION  putdimerrormodel        4 4
FUNCTION  getdimerrormodel        2 2 KEYWORDS
FUNCTION  getdatatypeid        1 1
FUNCTION  geterrormodelid        1 1
FUNCTION  geterrorasymmetry    1 1
FUNCTION  getdimerrorasymmetry    2 2
FUNCTION  getlasthandle        0 0
FUNCTION  getdomains        2 2 KEYWORDS
FUNCTION  setidamdatatree            1 1 KEYWORDS
FUNCTION  findidamtreestructurecomponent    3 3 KEYWORDS
FUNCTION  findidamtreestructuredefinition    3 3 KEYWORDS
FUNCTION  findidamtreestructure            3 3 KEYWORDS
FUNCTION  getidamnodeatomiccount        2 2 KEYWORDS
FUNCTION  getidamnodeatomicrank            2 2 KEYWORDS
FUNCTION  getidamnodeatomicshape        2 2 KEYWORDS
FUNCTION  getidamnodeatomicnames        2 2 KEYWORDS
FUNCTION  getidamnodeatomictypes        2 2 KEYWORDS
FUNCTION  getidamnodeatomicpointers        2 2 KEYWORDS
FUNCTION  getidamnodeatomicdatacount        3 3 KEYWORDS
FUNCTION  getidamnodeatomicdata            3 3 KEYWORDS

FUNCTION  getidamnodestructurecount        2 2 KEYWORDS
FUNCTION  getidamnodestructurenames        2 2 KEYWORDS
FUNCTION  getidamnodestructurerank        2 2 KEYWORDS
FUNCTION  getidamnodestructureshape        2 2 KEYWORDS
FUNCTION  getidamnodestructuretypes        2 2 KEYWORDS
FUNCTION  getidamnodestructurepointers        2 2 KEYWORDS

FUNCTION  getidamnodestructuredatacount        2 2 KEYWORDS
FUNCTION  getidamnodestructuredatarank        2 2 KEYWORDS
FUNCTION  getidamnodestructuredatashape        2 2 KEYWORDS

FUNCTION  getidamnodeparent            2 2 KEYWORDS
FUNCTION  getidamnodechild            3 3 KEYWORDS
FUNCTION  getidamnodechildrencount        2 2 KEYWORDS
FUNCTION  getidamnodechildid            3 3 KEYWORDS
FUNCTION  printidamtree                2 2 KEYWORDS
FUNCTION  printidamtreestructurenames        2 2 KEYWORDS
FUNCTION  printidamtreestructurecomponentnames    2 2 KEYWORDS
FUNCTION  printidamnodestructure        2 2 KEYWORDS
FUNCTION  regulariseidamvlenstructures        2 2 KEYWORDS

