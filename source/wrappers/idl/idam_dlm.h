#ifndef UDA_WRAPPERS_IDL_IDAM_H
#define UDA_WRAPPERS_IDL_IDAM_H

#define     GDE_NO_ARGUMENTS                1
#define     GDE_NO_EXP_NUMBER               2
#define     GDE_NO_SIGNAL_ARGUMENT          3
#define     GDE_BAD_HANDLE                  4
#define     GDE_NO_VALID_HANDLE             5
#define     GDE_DATA_HAS_ERROR              6
#define     GDE_UNKNOWN_DATA_TYPE           7
#define     GDE_NO_SUCH_DIMENSION           8
#define     GDE_NO_DATA_TO_RETURN           9
#define     GDE_RANK_TOO_HIGH               10
#define     GDE_HEAP_ALLOC_ERROR            11
#define     GDE_NO_API_IDENTIFIED           12
#define     GDE_NO_DATA_STRUCTURE_TO_RETURN 13
#define     GDE_NOT_IMPLEMENTED             20

#define ARRLEN(arr) (sizeof(arr)/sizeof(arr[0]))

#ifdef FATCLIENT
#  define idamgetapi                            fatidamgetapi
#  define idamputapi                            fatidamputapi
#  define callidam                              callfatidam
#  define callidam2                             callfatidam2
#  define idamgetdata                           fatidamgetdata
#  define idamgetdimdata                        fatidamgetdimdata
#  define idamgetdataarray                      fatidamgetdataarray
#  define idamgeterrorarray                     fatidamgeterrorarray
#  define idamgetdimdataarray                   fatidamgetdimdataarray
#  define idamfree                              fatidamfree
#  define idamfreeall                           fatidamfreeall
#  define idamgeterrorcode                      fatidamgeterrorcode
#  define idamgeterrormsg                       fatidamgeterrormsg
#  define idamprinterrormsgstack                fatidamprinterrormsgstack
#  define idamgetsourcestatus                   fatidamgetsourcestatus
#  define idamgetsignalstatus                   fatidamgetsignalstatus
#  define idamgetdatastatus                     fatidamgetdatastatus
#  define idamgetdatanum                        fatidamgetdatanum
#  define idamgetrank                           fatidamgetrank
#  define idamgetorder                          fatidamgetorder
#  define idamgetdatatype                       fatidamgetdatatype
#  define idamgeterrortype                      fatidamgeterrortype
#  define idamgeterrorasymmetry                 fatidamgeterrorasymmetry
#  define idamgetdatadata                       fatidamgetdatadata
#  define idamgetsyntheticdata                  fatidamgetsyntheticdata
#  define idamgetdataerror                      fatidamgetdataerror
#  define idamgetfloatdataerror                 fatidamgetfloatdataerror
#  define idamgetfloatasymmetricdataerror       fatidamgetfloatasymmetricdataerror
#  define idamgetasymmetricdataerror            fatidamgetasymmetricdataerror
#  define idamgetfloatdata                      fatidamgetfloatdata
#  define idamgetdatalabel                      fatidamgetdatalabel
#  define idamgetdataunits                      fatidamgetdataunits
#  define idamgetdatadesc                       fatidamgetdatadesc
#  define idamgetdimnum                         fatidamgetdimnum
#  define idamgetdimtype                        fatidamgetdimtype
#  define idamgetdimerrortype                   fatidamgetdimerrortype
#  define idamgetdimerrorasymmetry              fatidamgetdimerrorasymmetry
#  define idamgetdimdata                        fatidamgetdimdata
#  define idamgetsyntheticdimdata               fatidamgetsyntheticdimdata
#  define idamgetdimerror                       fatidamgetdimerror
#  define idamgetasymmetricdimerror             fatidamgetasymmetricdimerror
#  define idamgetfloatdimdata                   fatidamgetfloatdimdata
#  define idamgetfloatdimerror                  fatidamgetfloatdimerror
#  define idamgetfloatasymmetricdimerror        fatidamgetfloatasymmetricdimerror
#  define idamgetdimlabel                       fatidamgetdimlabel
#  define idamgetdimunits                       fatidamgetdimunits
#  define idamgetdatasystemmeta                 fatidamgetdatasystemmeta
#  define idamgetsystemconfigmeta               fatidamgetsystemconfigmeta
#  define idamgetdatasourcemeta                 fatidamgetdatasourcemeta
#  define idamgetsignalmeta                     fatidamgetsignalmeta
#  define idamgetsignaldescmeta                 fatidamgetsignaldescmeta
#  define idamgetxmldoc                         fatidamgetxmldoc
#  define idamsetproperty                       fatidamsetproperty
#  define idamresetproperty                     fatidamresetproperty
#  define idamresetproperties                   fatidamresetproperties
#  define idamgetdatatypeid                     fatidamgetdatatypeid
#  define idamgeterrormodelid                   fatidamgeterrormodelid
#  define idamsetclientflag                     fatidamsetclientflag
#  define idamresetclientflag                   fatidamresetclientflag
#  define idamputserverhost                     fatidamputserverhost
#  define idamputserverport                     fatidamputserverport
#  define idamgetserverhost                     fatidamgetserverhost
#  define idamgetserverport                     fatidamgetserverport
#  define idamgetclientversion                  fatidamgetclientversion
#  define idamgetserverversion                  fatidamgetserverversion
#  define idamgetserversocket                   fatidamgetserversocket
#  define idamputserversocket                   fatidamputserversocket
#  define idamputerrormodel                     fatidamputerrormodel
#  define idamgeterrormodel                     fatidamgeterrormodel
#  define idamputdimerrormodel                  fatidamputdimerrormodel
#  define idamgetdimerrormodel                  fatidamgetdimerrormodel
#  define idamgetlasthandle                     fatidamgetlasthandle
#  define idamgetdomains                        fatidamgetdomains
#  define idamsetdatatree                       fatidamsetdatatree
#  define idamfindtreestructurecomponent        fatidamfindtreestructurecomponent
#  define idamfindtreestructuredefinition       fatidamfindtreestructuredefinition
#  define idamfindtreestructure                 fatidamfindtreestructure
#  define idamgetnodeatomiccount                fatidamgetnodeatomiccount
#  define idamgetnodeatomicrank                 fatidamgetnodeatomicrank
#  define idamgetnodeatomicshape                fatidamgetnodeatomicshape
#  define idamgetnodeatomicnames                fatidamgetnodeatomicnames
#  define idamgetnodeatomictypes                fatidamgetnodeatomictypes
#  define idamgetnodeatomicpointers             fatidamgetnodeatomicpointers
#  define idamgetnodeatomicdatacount            fatidamgetnodeatomicdatacount
#  define idamgetnodeatomicdata                 fatidamgetnodeatomicdata
#  define idamgetnodestructurecount             fatidamgetnodestructurecount
#  define idamgetnodestructurerank              fatidamgetnodestructurerank
#  define idamgetnodestructureshape             fatidamgetnodestructureshape
#  define idamgetnodestructurenames             fatidamgetnodestructurenames
#  define idamgetnodestructuretypes             fatidamgetnodestructuretypes
#  define idamgetnodestructurepointers          fatidamgetnodestructurepointers
#  define idamgetnodestructuredatacount         fatidamgetnodestructuredatacount
#  define idamgetnodestructuredatarank          fatidamgetnodestructuredatarank
#  define idamgetnodestructuredatashape         fatidamgetnodestructuredatashape
#  define idamgetnodeparent                     fatidamgetnodeparent
#  define idamgetnodechild                      fatidamgetnodechild
#  define idamgetnodechildrencount              fatidamgetnodechildrencount
#  define idamgetnodechildid                    fatidamgetnodechildid
#  define idamprinttree                         fatidamprinttree
#  define idamprinttreestructurenames           fatidamprinttreestructurenames
#  define idamprinttreestructurecomponentnames  fatidamprinttreestructurecomponentnames
#  define idamprintnodestructure                fatidamprintnodestructure
#  define idamregularisevlenstructures          fatidamregularisevlenstructures

#  define LIB_NAME "FATIDAM"
#else
#  define LIB_NAME "IDAM"
#endif

#endif // UDA_WRAPPERS_IDL_IDAM_H
