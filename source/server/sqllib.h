#ifndef IDAM_IDAMFILESQLLIB_H
#define IDAM_IDAMFILESQLLIB_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NOTGENERICENABLED
#  include <libpq-fe.h>	// SQL Library Header
#endif

#ifdef NOTGENERICENABLED
typedef int PGconn;
#else
#include <libpq-fe.h>
#endif

#include <clientserver/udaStructs.h>

#define NOPREFIXNOCASE  "0"
#define PREFIXNOCASE    "1"
#define NOPREFIXCASE    "2"
#define PREFIXCASE      "3"

extern PGconn *gDBConnect;

PGconn *startSQL(const ENVIRONMENT* environment);
PGconn *openDatabase(const char* host, int port, const char* dbname, const char* user);
PGconn *startSQL_CPF(const ENVIRONMENT* environment);

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

#endif // IDAM_IDAMFILESQLLIB_H

