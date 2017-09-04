#ifndef IDAM_WRAPPERS_IDL_MASTCPF_H
#define IDAM_WRAPPERS_IDL_MASTCPF_H

#include <stdio.h>
#include "idl_export.h"     // IDL API Header

#include <libpq-fe.h>

// Useful macro

#define ARRLEN(arr) (sizeof(arr)/sizeof(arr[0]))

// Error management

extern IDL_MSG_BLOCK msg_block;

#define HOST        "fuslwn"
#define PORT        "56566"
#define USER        "mast_db"
#define DATABASE    "cpf"

#define MAXSQL          1024*1024
#define MAXSTR          56
#define MAXARRAYNR      256
#define MAXARRAYNS      512

#define CPF_SERVER_CONNECT_ERROR        1
#define CPF_QUERY_FAILED                2
#define CPF_QUERY_INCORRECT             3
#define CPF_NO_ROWS                     4
#define CPF_NO_COLUMNS                  5
#define CPF_HEAP_ALLOC_ERROR            6
#define CPF_BUFFER_TOO_SMALL            7
#define CPF_SECURITY_NOT_AUTHORISED     8
#define CPF_QUERY_RECORD_NOT_NEW        9
#define CPF_INSERT_FAILED               10
#define CPF_UPDATE_FAILED               11
#define CPF_FIRST_COLUMN_NOT_ARRAY_TYPE 12
#define CPF_FIRST_COLUMN_IS_ARRAY_TYPE  13
#define CPF_1D_ARRAY_TOO_LARGE          14
#define CPF_2D_ARRAY_TOO_LARGE          15
#define CPF_EXCESSIVE_COLUMNS           16

// Globals

//extern PGconn * DBConnect;
extern int     nGroupRows;

// Prototypes

extern void mastCPF_exit_handler(void);
extern int  mastCPF_Startup(void);

PGconn * StartDbSQL(char * dbName, int verbose);
char * TrimString( char * szSource );
int checkSecurity(PGconn * DBConnect, char * table, char permission, int verbose);
void userid(char * uid);

#endif // IDAM_WRAPPERS_IDL_MASTCPF_H