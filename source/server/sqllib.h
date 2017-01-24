//! $LastChangedRevision: 283 $
//! $LastChangedDate: 2011-12-01 11:56:55 +0000 (Thu, 01 Dec 2011) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/include/sqllib.h $

//
// Change History
//
// 15Feb2008 dgm	added tpass to sqlGeneric prototype
//			added sqlLatestPass prototype
// 02Apr2008 dgm	C++ test added for inclusion of extern "C"
// 08Jul2009 dgm	All prototypes changed to static class
//--------------------------------------------------------------------------------------------------------------------

#ifndef IDAM_SQLLIB_H
#define IDAM_SQLLIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "idamserverconfig.h"

#if defined GENERIC_ENABLE && !defined NOTGENERICENABLED
#  include <libpq-fe.h>	// SQL Library Header
#else
//typedef int PGconn;
#endif

//#ifndef OTHERSERVER
#include <libpq-fe.h> // SQL Library Header
//#else
//typedef int PGconn;
//#endif

#include "idamclientserver.h"
#include "idamserver.h"

#define NOPREFIXNOCASE  "0"
#define PREFIXNOCASE    "1"
#define NOPREFIXCASE    "2"
#define PREFIXCASE      "3"

extern PGconn *gDBConnect;

PGconn *startSQL();
PGconn *startSQL_CPF();
void sqlReason(PGconn *DBConnect, char *reason_id, char *reason);
void sqlMeta(PGconn *DBConnect, char *table, char *meta_id, char *xml, char *creation);
int sqlGeneric(PGconn *DBConnect, char *signal, int exp_number, int pass, char *tpass,
               SIGNAL *signal_str,  SIGNAL_DESC *signal_desc_str,
               DATA_SOURCE *data_source_str);
int sqlDocument(PGconn *DBConnect, char *signal, int exp_number, int pass,
                SIGNAL_DESC *signal_desc_str, DATA_SOURCE *data_source_str);
int sqlComposite(PGconn *DBConnect, char *signal, int exp_number, SIGNAL_DESC *signal_desc_str);
int sqlDerived(PGconn *DBConnect, char *signal, int exp_number, SIGNAL_DESC *signal_desc_str);
int sqlExternalGeneric(PGconn *DBConnect, char *archive, char *device, char *signal,
                       int exp_number, int pass,
                       SIGNAL *signal_str,  SIGNAL_DESC *signal_desc_str,
                       DATA_SOURCE *data_source_str);
int sqlNoSignal(PGconn *DBConnect, char *archive, char *device, char *signal,
                int exp_number, int pass,
                SIGNAL *signal_str,  SIGNAL_DESC *signal_desc_str,
                DATA_SOURCE *data_source_str);
int sqlDataSystem(PGconn *DBConnect, int pkey, DATA_SYSTEM *str);
int sqlSystemConfig(PGconn *DBConnect, int pkey, SYSTEM_CONFIG *str);
int sqlArchive(PGconn *DBConnect, char *archive, DATA_SOURCE *data_source_str);
int sqlLatestPass(PGconn *DBConnect, char *source_alias, char type, int exp_number, char *maxpass);
int sqlAltData(PGconn *DBConnect, REQUEST_BLOCK request_block, int rank, SIGNAL_DESC *signal_desc, char *mapping);
int sqlMapPrivateData(PGconn *DBConnect, REQUEST_BLOCK request_block, SIGNAL_DESC *signal_desc);

int sqlMatch(PGconn *DBConnect, int signal_desc_id, char *originalSourceAlias, char *type,
             int exp_number, int pass, char *originalTPass, int *source_id);
int sqlMapData(PGconn *DBConnect, int signal_desc_id, int exp_number, SIGNAL_DESC *signal_desc);
int sqlNoIdamSignal(PGconn *DBConnect, char *originalSignal, int exp_number, int pass, char *originalTPass,
                    SIGNAL *signal_str,  SIGNAL_DESC *signal_desc, DATA_SOURCE *data_source);
#ifndef NOTGENERICENABLED

#define PREFIXCASE      "3"
#define PREFIXNOCASE    "1"
#define NOPREFIXCASE    "2"
#define NOPREFIXNOCASE  "0"

int sqlMatch(PGconn *DBConnect, int signal_desc_id, char *originalSourceAlias, char *type,
             int exp_number, int pass, char *originalTPass, int *source_id);
int sqlMapData(PGconn *DBConnect, int signal_desc_id, int exp_number, SIGNAL_DESC *signal_desc);
int sqlNoIdamSignal(PGconn *DBConnect, char *originalSignal, int exp_number, int pass, char *originalTPass,
                    SIGNAL *signal_str,  SIGNAL_DESC *signal_desc, DATA_SOURCE *data_source);
#endif

#ifdef __cplusplus
}
#endif

#endif // IDAM_SQLLIB_H

