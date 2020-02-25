#ifndef UDA_SERVER_SQLLIB_H
#define UDA_SERVER_SQLLIB_H

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

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

LIBRARY_API PGconn *startSQL(const ENVIRONMENT* environment);
LIBRARY_API PGconn *openDatabase(const char* host, int port, const char* dbname, const char* user);
LIBRARY_API PGconn *startSQL_CPF(const ENVIRONMENT* environment);

LIBRARY_API void sqlReason(PGconn *DBConnect, char *reason_id, char *reason);
LIBRARY_API void sqlMeta(PGconn *DBConnect, const char *table, char *meta_id, char *xml, char *creation);
LIBRARY_API int sqlGeneric(PGconn *DBConnect, char *signal, int exp_number, int pass, char *tpass,
               SIGNAL *signal_str,  SIGNAL_DESC *signal_desc_str,
               DATA_SOURCE *data_source_str);
LIBRARY_API int sqlDocument(PGconn *DBConnect, char *signal, int exp_number, int pass,
                SIGNAL_DESC *signal_desc_str, DATA_SOURCE *data_source_str);
LIBRARY_API int sqlComposite(PGconn *DBConnect, char *signal, int exp_number, SIGNAL_DESC *signal_desc_str);
LIBRARY_API int sqlDerived(PGconn *DBConnect, char *signal, int exp_number, SIGNAL_DESC *signal_desc_str);
LIBRARY_API int sqlExternalGeneric(PGconn *DBConnect, char *archive, char *device, char *signal,
                       int exp_number, int pass,
                       SIGNAL *signal_str,  SIGNAL_DESC *signal_desc_str,
                       DATA_SOURCE *data_source_str);
LIBRARY_API int sqlNoSignal(PGconn *DBConnect, char *archive, char *device, char *signal,
                int exp_number, int pass,
                SIGNAL *signal_str,  SIGNAL_DESC *signal_desc_str,
                DATA_SOURCE *data_source_str);
LIBRARY_API int sqlDataSystem(PGconn *DBConnect, int pkey, DATA_SYSTEM *str);
LIBRARY_API int sqlSystemConfig(PGconn *DBConnect, int pkey, SYSTEM_CONFIG *str);
LIBRARY_API int sqlArchive(PGconn *DBConnect, char *archive, DATA_SOURCE *data_source_str);
LIBRARY_API int sqlLatestPass(PGconn *DBConnect, char *source_alias, char type, int exp_number, char *maxpass);
LIBRARY_API int sqlAltData(PGconn *DBConnect, REQUEST_BLOCK request_block, int rank, SIGNAL_DESC *signal_desc, char *mapping);
LIBRARY_API int sqlMapPrivateData(PGconn *DBConnect, REQUEST_BLOCK request_block, SIGNAL_DESC *signal_desc);

LIBRARY_API int sqlMatch(PGconn *DBConnect, int signal_desc_id, char *originalSourceAlias, char *type,
             int exp_number, int pass, char *originalTPass, int *source_id);
LIBRARY_API int sqlMapData(PGconn *DBConnect, int signal_desc_id, int exp_number, SIGNAL_DESC *signal_desc);
LIBRARY_API int sqlNoIdamSignal(PGconn *DBConnect, char *originalSignal, int exp_number, int pass, char *originalTPass,
                    SIGNAL *signal_str,  SIGNAL_DESC *signal_desc, DATA_SOURCE *data_source);

#ifndef NOTGENERICENABLED
#  define PREFIXCASE      "3"
#  define PREFIXNOCASE    "1"
#  define NOPREFIXCASE    "2"
#  define NOPREFIXNOCASE  "0"
#endif

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_SQLLIB_H

