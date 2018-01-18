MODULE getdata
DESCRIPTION Universal Data Access (IDAM) toolset  
VERSION 4.1
SOURCE David Muir (MAST)
BUILD_DATE 19 Nov 2013
FUNCTION  idamgetapi                                    2 2 KEYWORDS
FUNCTION  idamputapi                                    2 2 KEYWORDS

FUNCTION  callidam                                      1 2 KEYWORDS
FUNCTION  callidam2                                     2 2 KEYWORDS
FUNCTION  idamgetdata                                   1 1 KEYWORDS
FUNCTION  idamgetdimdata                                2 2 KEYWORDS
FUNCTION  idamgetdataarray                              1 1 KEYWORDS
FUNCTION  idamgeterrorarray                             1 1 KEYWORDS
FUNCTION  idamgetdimdataarray                           2 2 KEYWORDS
FUNCTION  idamfree                                      1 1
FUNCTION  idamfreeall                                   0 0
FUNCTION  idamgeterrorcode                              1 1
FUNCTION  idamgeterrormsg                               2 2
FUNCTION  idamprinterrormsgstack                            1 1
FUNCTION  idamgetsourcestatus                           1 1
FUNCTION  idamgetsignalstatus                           1 1
FUNCTION  idamgetdatastatus                             1 1
FUNCTION  idamgetdatanum                                1 1
FUNCTION  idamgetrank                                   1 1
FUNCTION  idamgetorder                                  1 1
FUNCTION  idamgetdatatype                               1 1
FUNCTION  idamgeterrortype                              1 1
FUNCTION  idamgetdatadata                               1 1
FUNCTION  idamgetdataerror                              1 1
FUNCTION  idamgetasymmetricdataerror                    2 2
FUNCTION  idamgetfloatdata                              1 1
FUNCTION  idamgetfloatdataerror                         1 1
FUNCTION  idamgetfloatasymmetricdataerror               2 2
FUNCTION  idamgetsyntheticdata                          1 1
FUNCTION  idamgetdatalabel                              2 2
FUNCTION  idamgetdataunits                              2 2
FUNCTION  idamgetdatadesc                               2 2
FUNCTION  idamgetdimnum                                 2 2
FUNCTION  idamgetdimtype                                2 2
FUNCTION  idamgetdimerrortype                           2 2
FUNCTION  idamgetdimdata                                2 2
FUNCTION  idamgetdimerror                               2 2
FUNCTION  idamgetasymmetricdimerror                     3 3
FUNCTION  idamgetfloatdimdata                           2 2
FUNCTION  idamgetfloatdimerror                          2 2
FUNCTION  idamgetfloatasymmetricdimerror                3 3
FUNCTION  idamgetsyntheticdimdata                       2 2
FUNCTION  idamgetdimlabel                               3 3
FUNCTION  idamgetdimunits                               3 3
FUNCTION  idamgetdatasystemmeta                         1 1 KEYWORDS
FUNCTION  idamgetsystemconfigmeta                       1 1 KEYWORDS
FUNCTION  idamgetdatasourcemeta                         1 1 KEYWORDS
FUNCTION  idamgetsignalmeta                             1 1 KEYWORDS
FUNCTION  idamgetsignaldescmeta                         1 1 KEYWORDS
FUNCTION  idamgetxmldoc                                 2 2 KEYWORDS
FUNCTION  idamsetproperty                                   1 1
FUNCTION  idamresetproperty                                 1 1
FUNCTION  idamresetproperties                               0 0
FUNCTION  idamsetclientflag                             1 1
FUNCTION  idamresetclientflag                           1 1
FUNCTION  idamputserverhost                             1 1
FUNCTION  idamputserverport                             1 1
FUNCTION  idamgetserverhost                             1 1
FUNCTION  idamgetserverport                             0 0
FUNCTION  idamgetclientversion                          0 0
FUNCTION  idamgetserverversion                          0 0
FUNCTION  idamputserversocket                           1 1
FUNCTION  idamgetserversocket                           0 0
FUNCTION  idamputerrormodel                                 3 3
FUNCTION  idamgeterrormodel                             1 1 KEYWORDS
FUNCTION  idamputdimerrormodel                              4 4
FUNCTION  idamgetdimerrormodel                          2 2 KEYWORDS
FUNCTION  idamgetdatatypeid                             1 1
FUNCTION  idamgeterrormodelid                           1 1
FUNCTION  idamgeterrorasymmetry                         1 1
FUNCTION  idamgetdimerrorasymmetry                      2 2
FUNCTION  idamgetlasthandle                             0 0
FUNCTION  idamgetdomains                                2 2 KEYWORDS
FUNCTION  idamsetdatatree                               1 1 KEYWORDS
FUNCTION  idamfindtreestructurecomponent                3 3 KEYWORDS
FUNCTION  idamfindtreestructuredefinition               3 3 KEYWORDS
FUNCTION  idamfindtreestructure                         3 3 KEYWORDS
FUNCTION  idamgetnodeatomiccount                        2 2 KEYWORDS
FUNCTION  idamgetnodeatomicrank                         2 2 KEYWORDS
FUNCTION  idamgetnodeatomicshape                        2 2 KEYWORDS
FUNCTION  idamgetnodeatomicnames                        2 2 KEYWORDS
FUNCTION  idamgetnodeatomictypes                        2 2 KEYWORDS
FUNCTION  idamgetnodeatomicpointers                     2 2 KEYWORDS
FUNCTION  idamgetnodeatomicdatacount                    3 3 KEYWORDS
FUNCTION  idamgetnodeatomicdata                         3 3 KEYWORDS

FUNCTION  idamgetnodestructurecount                     2 2 KEYWORDS
FUNCTION  idamgetnodestructurenames                     2 2 KEYWORDS
FUNCTION  idamgetnodestructurerank                      2 2 KEYWORDS
FUNCTION  idamgetnodestructureshape                     2 2 KEYWORDS
FUNCTION  idamgetnodestructuretypes                     2 2 KEYWORDS
FUNCTION  idamgetnodestructurepointers                  2 2 KEYWORDS

FUNCTION  idamgetnodestructuredatacount                 2 2 KEYWORDS
FUNCTION  idamgetnodestructuredatarank                  2 2 KEYWORDS
FUNCTION  idamgetnodestructuredatashape                 2 2 KEYWORDS

FUNCTION  idamgetnodeparent                             2 2 KEYWORDS
FUNCTION  idamgetnodechild                              3 3 KEYWORDS
FUNCTION  idamgetnodechildrencount                      2 2 KEYWORDS
FUNCTION  idamgetnodechildid                            3 3 KEYWORDS
FUNCTION  idamprinttree                                 2 2 KEYWORDS
FUNCTION  idamprinttreestructurenames                   2 2 KEYWORDS
FUNCTION  idamprinttreestructurecomponentnames          2 2 KEYWORDS
FUNCTION  idamprintnodestructure                        2 2 KEYWORDS
FUNCTION  idamregularisevlenstructures                  2 2 KEYWORDS

