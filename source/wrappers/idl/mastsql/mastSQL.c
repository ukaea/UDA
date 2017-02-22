// Routines to Query a Postgres Database
//
// Usage:
//
//	result = querySQL(names, table, where_clause [,/verbose][,/help][,/debug])
//
// where the keywords VERBOSE means print any error messages.
//
// The returned Array of Structures contains resultset column name-value pairs for 
// each database table record (rows) that satisfies the query. 
//
//	result.sql    		= Query against the Database
//	result.rows		= Number of Table Rows
//	result.cols		= Number of Table Columns
//	result.names[cols]  	= names of each resultset column
//	result.values[rows,cols]= Column Values (as Doubles) for each resultset Row
//		 
//
// Note: The PostgreSQL Library libpg.so must be available to IDL. A soft 
// link to the standard library is sufficient.
//------------------------------------------------------------------
#include "mastSQL.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <string.h>

#include "export.h"		// IDL API Header

// Function Prototypes

extern IDL_VPTR IDL_CDECL querySQL(int argc, IDL_VPTR argv[], char *argk);
extern IDL_VPTR IDL_CDECL queryStrSQL(int argc, IDL_VPTR argv[], char *argk);
extern IDL_VPTR IDL_CDECL queryArraySQL(int argc, IDL_VPTR argv[], char *argk);
extern IDL_VPTR IDL_CDECL querySingleArraySQL(int argc, IDL_VPTR argv[], char *argk);
extern IDL_VPTR IDL_CDECL groupCountSQL(int argc, IDL_VPTR argv[], char *argk);
extern IDL_VPTR IDL_CDECL putSQL(int argc, IDL_VPTR argv[], char *argk);
extern IDL_VPTR IDL_CDECL killSQL(int argc, IDL_VPTR argv[], char *argk);
extern IDL_VPTR IDL_CDECL closeSQL(int argc, IDL_VPTR argv[], char *argk);
extern IDL_VPTR IDL_CDECL countSQL(int argc, IDL_VPTR argv[], char *argk);
 
static IDL_SYSFUN_DEF2 mastSQL_functions[] = {
   {(IDL_FUN_RET) querySQL,	 "QUERYSQL", 		3, 3, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
   {(IDL_FUN_RET) queryStrSQL,	 "QUERYSTRSQL", 	3, 3, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
   {(IDL_FUN_RET) queryArraySQL, "QUERYARRAYSQL", 	3, 3, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
   {(IDL_FUN_RET) querySingleArraySQL, "QUERYSINGLEARRAYSQL", 	3, 3, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
   {(IDL_FUN_RET) groupCountSQL, "GROUPCOUNTSQL",       3, 3, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
   {(IDL_FUN_RET) putSQL,   	 "PUTSQL",   		4, 4, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
   {(IDL_FUN_RET) killSQL,  	 "KILLSQL",  		2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
   {(IDL_FUN_RET) countSQL,      "COUNTSQL", 	        3, 3, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
   {(IDL_FUN_RET) closeSQL,  	 "CLOSESQL",  		0, 0, 0, 0} 
};

int mastSQL_startup(void){ 
 
    if (!IDL_SysRtnAdd(mastSQL_functions, TRUE,
        ARRLEN(mastSQL_functions))) {return IDL_FALSE;}

// Register the exit handler 

    IDL_ExitRegister(mastSQL_exit_handler);
    
    return(IDL_TRUE);
}

int IDL_Load(void)
{  
    if (!IDL_SysRtnAdd(mastSQL_functions, TRUE,
        ARRLEN(mastSQL_functions)))  {return IDL_FALSE;}
    return(IDL_TRUE);
}

// Called when IDL is shutdown
 
void mastSQL_exit_handler(void){
   
// Close the SQL Connection 

   if(DBConnect != NULL) PQfinish(DBConnect);
   DBConnect = NULL;
     
   //fprintf(stdout,"Closing the SQL database Connection\n");
}

// Open the Connection Once with the PostgreSQL Database

PGconn * DBConnect = NULL;	// Keep Socket Open until IDL Closes down

PGconn *StartDbSQL(char *dbName, int verbose){
   
   char pghost[MAXSTR] = HOST;			
   char pgport[MAXSTR] = PORT;
   char dbname[MAXSTR] = DATABASE;
   char user[MAXSTR]   = USER;

   char *pgoptions = NULL;  
   char *pgtty     = NULL;   
   char *pswrd     = NULL;
   char *env       = NULL;

   PGresult *DBQuery = NULL;
   
   ExecStatusType DBQueryStatus; 

//-------------------------------------------------------------
// Test if Socket already Open

   if(DBConnect != NULL) return(DBConnect);	 

//-------------------------------------------------------------
// Identify the SQL Host and Database
   
// SQL Server Host Name

   if((env = getenv("DB_SQLHOST")) !=NULL) strcpy(pghost, env);     

// SQL Server Port name 

   if((env = getenv("DB_SQLPORT")) !=NULL) strcpy(pgport, env); 
      
// SQL Database name 

   if((env = getenv("DB_SQLDBNAME")) !=NULL) strcpy(dbname, env); 
   if(dbName != NULL) strcpy(dbname, dbName);        

// ISQL Access username 

   if((env = getenv("DB_SQLUSER")) !=NULL) strcpy(user, env); 
   
/*
   if(DEBUG){
      fprintf(dbgout,"Host:   %s \n", pghost);
      fprintf(dbgout,"Port:   %s \n", pgport);
      fprintf(dbgout,"DB  :   %s \n", dbName);
      fprintf(dbgout,"User:   %s \n", user);
   }
*/           
   
//-------------------------------------------------------------
// Connect to the Database Server
    
   if ((DBConnect = PQsetdbLogin(pghost, pgport, pgoptions, pgtty, dbname, user, pswrd)) == NULL){
      PQfinish(DBConnect);  
      return NULL;
   }
      
   if (PQstatus(DBConnect) == CONNECTION_BAD){
      if(verbose) fprintf(stdout,"Bad SQL Server Connect Status \n");
      PQfinish(DBConnect);
      return NULL;
   }

   return(DBConnect);
}


char *TrimString( char *szSource )
{
    char *pszEOS;

// Set pointer to end of string to point to the character just
// before the 0 at the end of the string.

    pszEOS = szSource + strlen( szSource ) - 1;

    while( pszEOS >= szSource && *pszEOS == ' ' )
        *pszEOS-- = '\0';

    return szSource;
}
    
void freeMem(UCHAR *memPtr){
   //fprintf(stdout,"freeMem: Free Address  : %x\n", (int)memPtr);
   free((void *)memPtr);
} 

// Main Dimensional Data Block Structure Passed Back to IDL

typedef struct {
  IDL_STRING sql;
  IDL_LONG   rows;
  IDL_LONG   cols;
  double    *values;
} SOUT;

typedef struct {
  IDL_STRING sql;
  IDL_LONG   rows;
  IDL_LONG   cols;
  IDL_LONG   n_elements;
  double    *values;
} SOUTARR;

typedef struct {
  IDL_STRING sql;
  IDL_LONG   rows;
  IDL_LONG   cols;
  IDL_LONG   nr_elements;
  IDL_LONG   ns_elements;
  double    *values;
} SOUTARR2; 

typedef struct {
  IDL_STRING sql;
  IDL_LONG   rows;
  IDL_LONG   n_elements;
  IDL_LONG  *key;
  double    *values;
} SOUTSARR;

typedef struct {
  IDL_STRING sql;
  IDL_LONG   rows;
  IDL_LONG   nr_elements;
  IDL_LONG   ns_elements;
  IDL_LONG  *key;
  double    *values;
} SOUTSARR2; 

typedef struct {
  int        count; 
  IDL_STRING group;	// Comma delimited Group Values!
} CSOUT1;

typedef struct {
  int        count; 
  IDL_STRING group[2];	 
} CSOUT2;

typedef struct {
  int        count; 
  IDL_STRING group[3];	 
} CSOUT3;
   


IDL_VPTR IDL_CDECL closeSQL(int argc, IDL_VPTR argv[], char *argk){
//
// IDL DLM Function to Close the Socket Connection to the SQL Database Server
//
// No Arguments:
//
// If the IDL Exit Handler Routine was called as expected (Not the case!), this
// function would Not be Needed.  
//
// v0.01	31Jan2006	D.G.Muir	Original Release
//-------------------------------------------------------------------------	      
   if(DBConnect != NULL) PQfinish(DBConnect);
   DBConnect = NULL;   
   return(IDL_GettmpLong(0));
}   
   
   

IDL_VPTR IDL_CDECL queryCPF(int argc, IDL_VPTR argv[], char *argk){
//
// IDL DLM Function to Query the CPF Database for Scalar Type values
//
// 3 Arguments:
//
//	argv[0] - Array of Column Names for the SQL Select
//	argv[1] - the CPF Table Name
//	argv[2] - the where clause criteria 
//
// 1 Keywords:
//
//	debug   - Print Debug Messages
//	help    - Print a useful Help Message
//	verbose - Print Error Messages
//
// v0.01	November 2005	D.G.Muir	Original Release
//-------------------------------------------------------------------------	      
   int i, j, rc, nrows, ncols, exp_number;
   
   char     *DBName    = DATABASE;
   PGresult *DBQuery   = NULL;  
   ExecStatusType DBQueryStatus; 
      
   IDL_STRING *pIDLstrDesc;
   char *table, *where, *value; 
   
   char sql[MAXSQL] = "select ";
   char slimit[MAXSTR];
   	    
   SOUT     *sout;			// Pointer to the Scalar Oriented Returned IDL/C Structure;
      
   double *valArray;
   IDL_VPTR ivReturn = NULL;
   
   void *psDef       = NULL;
   static IDL_LONG ilDims[IDL_MAX_ARRAY_DIM];
   
   static IDL_LONG vdlen[]    = {2,1,1};		// Scalar Value Array
   
   IDL_STRUCT_TAG_DEF pTags[] = {
   	{"SQL",		0,	(void *) IDL_TYP_STRING},
	{"ROWS",	0,	(void *) IDL_TYP_LONG},
	{"COLS",	0,	(void *) IDL_TYP_LONG},			
	{"VALUES",	vdlen,	(void *) IDL_TYP_DOUBLE},	
	{0}
   };
   
// Keyword Structure

   typedef struct{
      	IDL_KW_RESULT_FIRST_FIELD;	
	IDL_LONG limit;
	IDL_LONG islimit;
	IDL_LONG verbose;
	IDL_LONG debug;
	IDL_LONG help;
   } KW_RESULT;
   
   
// Maintain Alphabetical Order of Keywords
 
   static IDL_KW_PAR kw_pars[] = 
   { IDL_KW_FAST_SCAN,
     {"DEBUG",   IDL_TYP_LONG,  1,IDL_KW_ZERO,0, 			IDL_KW_OFFSETOF(debug)},    
     {"HELP",    IDL_TYP_LONG,  1,IDL_KW_ZERO,0, 			IDL_KW_OFFSETOF(help)},
     {"LIMIT",   IDL_TYP_LONG,  1,IDL_KW_ZERO,IDL_KW_OFFSETOF(islimit), IDL_KW_OFFSETOF(limit)},
     {"VERBOSE", IDL_TYP_LONG,  1,IDL_KW_ZERO,0, 			IDL_KW_OFFSETOF(verbose)},    			
   {NULL}};    
 
   KW_RESULT kw;
   
   kw.limit     = 0;
   kw.islimit   = 0;
   kw.verbose   = 0;
   kw.debug     = 0;
   kw.help      = 0; 
   
//-----------------------------------------------------------------------
// Get Passed Keywords and Parameters

   IDL_KWProcessByOffset(argc,argv,argk,kw_pars,(IDL_VPTR *)0,1,&kw);   
    
   if(kw.debug){    
      if(kw.debug)  fprintf(stdout,"Debug Keyword Passed\n");
      if(kw.verbose)fprintf(stdout,"Verbose Keyword Passed\n");   
      if(kw.help)   fprintf(stdout,"Help Keyword Passed\n");
      if(kw.islimit)fprintf(stdout,"Limit Keyword Passed: limit = %d\n", kw.limit);
   }   
   
//--------------------------------------------------------------------------      
// Call for HELP?

   if(kw.help){
      fprintf(stdout,"queryCPF: Help Not Implemented ... yet!\n");
      IDL_KW_FREE;
      return(IDL_GettmpLong(0));
   }    
   
//-----------------------------------------------------------------------
// Check a String was Passed
   
   IDL_ENSURE_ARRAY(argv[0]);
   IDL_ENSURE_STRING(argv[0]);
   
   IDL_ENSURE_STRING(argv[1]);
   //IDL_ENSURE_SCALAR(argv[1]);
   
   IDL_ENSURE_STRING(argv[2]);
   //IDL_ENSURE_SCALAR(argv[2]);
      
//-----------------------------------------------------------------------
// Create the SQL Query
   
   ncols = argv[0]->value.arr->n_elts;				// Number of Required Column Names
   pIDLstrDesc = (IDL_STRING *) argv[0]->value.arr->data; 
   
   for(i=0;i<ncols;i++) {
      strcat(sql, (char *)(*pIDLstrDesc++).s) ;			// Names
      if(i < (ncols-1))strcat(sql, ",");
   }
   strcat(sql, " from ");
   
   table = (char *)IDL_STRING_STR(&(argv[1]->value.str));	// Table Name
   strcat(sql, table);	 
   
   where = (char *)IDL_STRING_STR(&(argv[2]->value.str));	// Where Clause
   if(strlen(where) > 0){
      strcat(sql, " where ");     
      strcat(sql, where);
   }   
   
   if(kw.islimit){
      sprintf(slimit," limit %d;", kw.limit);
      strcat(sql, slimit);
   }
   
   if(kw.debug){
      fprintf(stdout,"Number of Column Names: %d\n", ncols);
      pIDLstrDesc = (IDL_STRING *) argv[0]->value.arr->data;
      for(i=0;i<ncols;i++) fprintf(stdout,"Column Names: %d  %s\n", i,(char *)(*pIDLstrDesc++).s);
      fprintf(stdout,"SQL: %s\n", sql);
   }   
            
//--------------------------------------------------------------------------      
// Connect to the Database Server

   if (!(DBConnect = StartDbSQL(DBName, kw.verbose))){
      if(kw.verbose) fprintf(stdout,"queryCPF: SQL Server Connect Error \n");
      IDL_KW_FREE;
      return(IDL_GettmpLong(CPF_SERVER_CONNECT_ERROR)); 
   }         

//-------------------------------------------------------------  
// Execute the SQL

   if ((DBQuery = PQexec(DBConnect, sql)) == NULL){
      if(kw.verbose){
         fprintf(stdout,"querySQL: ERROR - Failure to Execute the Query: %s\n", sql);
         fprintf(stdout,"querySQL: ERROR - %s\n", PQresultErrorMessage(DBQuery)); 
      }
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_QUERY_FAILED));     
   } 
   
   DBQueryStatus = PQresultStatus(DBQuery);        
    
   if (DBQueryStatus != PGRES_TUPLES_OK){
      if(kw.verbose){  
         fprintf(stdout,"querySQL: ERROR - Query Incorrectly Processed %s\n", sql);
         fprintf(stdout,"querySQL: ERROR - %s\n", PQresultErrorMessage(DBQuery)); 
      }
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_QUERY_INCORRECT));
   }
   
//-------------------------------------------------------------  
// Extract the Resultset 
    
   nrows = PQntuples(DBQuery); 		// Number of Rows
   ncols = PQnfields(DBQuery);		// Number of Columns
	 
   if(nrows == 0){
      if(kw.verbose)fprintf(stdout,"querySQL: ERROR - No Record Satisfy the Query %s\n", sql);
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_NO_ROWS));
   }  
    	    
   if(ncols == 0){
      if(kw.verbose)fprintf(stdout,"querySQL: ERROR - No Columns Created by the Query %s\n",sql);  
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_NO_COLUMNS));
   }  

// Returned Values (Scalar Only)

   valArray = (double *) malloc(nrows*ncols*sizeof(double));

   for(i=0;i<nrows;i++){
      for(j=0;j<ncols;j++) {
      
// Is the Column of Array type?

         value = PQgetvalue(DBQuery,i,j);
         if(value[0] == '{' ){				// Array Type Found
	    if(kw.verbose)fprintf(stdout,"querySQL: ERROR - The First Data Column Must be an Array Type\n");  
            IDL_KW_FREE;
            PQclear(DBQuery);
	    if(valArray != NULL) free((void *)valArray);
            return(IDL_GettmpLong(CPF_FIRST_COLUMN_IS_ARRAY_TYPE));
	 }
	    
         valArray[i*ncols+j] = (double)atof(PQgetvalue(DBQuery,i,j));	 		 
      }	 	 
   } 
   
// Allocate the Appropriate Return Structure

   if((sout = (SOUT *)malloc(sizeof(SOUT)+nrows*ncols*sizeof(double))) == NULL){
      if(kw.verbose)fprintf(stdout,"querySQL: ERROR - Unable to Allocate Heap!\n");
      IDL_KW_FREE;
      PQclear(DBQuery);
      if(valArray != NULL) free((void *)valArray);
      return(IDL_GettmpLong(CPF_HEAP_ALLOC_ERROR));
   }  
   	 
   IDL_StrStore(&(sout->sql), sql); 
   
   sout->rows = (IDL_LONG) nrows;
   sout->cols = (IDL_LONG) ncols;
   
   sout->values = (double *)(sout+sizeof(SOUT));
   memcpy((void *)&sout->values, (void *)valArray,(size_t)nrows*ncols*sizeof(double));
           
// Free Local Heap Buffers
   
   if(valArray != NULL) free((void *)valArray);

//--------------------------------------------------------------------------      
// Create the IDL Structure 
   
   ilDims[0] = 1;	// Number of Structure Array Elements
   
   vdlen[2]  = nrows;	// Number of Records
   vdlen[1]  = ncols;	// Number of Values
   
   psDef     = IDL_MakeStruct(NULL, pTags);	 
   ivReturn  = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR *)sout, freeMem, psDef);  
              	     

//--------------------------------------------------------------------------      
// Cleanup Keywords & PG Heap 
 
   IDL_KW_FREE;
   PQclear(DBQuery);    
       
   return(ivReturn);           
}


IDL_VPTR IDL_CDECL queryStrCPF(int argc, IDL_VPTR argv[], char *argk){
//
// IDL DLM Function to Query the CPF Database for String Type values
//
// 3 Arguments:
//
//	argv[0] - Array of Column Names for the SQL Select
//	argv[1] - the CPF Table Name
//	argv[2] - the where clause criteria 
//
// 1 Keywords:
//
//	debug   - Print Debug Messages
//	help    - Print a useful Help Message
//	verbose - Print Error Messages
//
// v0.01	07June 2006	D.G.Muir	Original Release
//-------------------------------------------------------------------------	      
   int i, rc, nrows, ncols;
   
   char     *DBName    = DATABASE;
   PGresult *DBQuery   = NULL;  
   ExecStatusType DBQueryStatus; 
      
   //IDL_STRING *pIDLstrDesc;
   char *colname, *table, *where, *value; 
   
   char sql[MAXSQL] = "select ";
   char slimit[MAXSTR];
   
   IDL_STRING *pisArray;
   IDL_LONG lDimensions[IDL_MAX_ARRAY_DIM];
   IDL_VPTR ivReturn;
   int iStringLength;
   
// Keyword Structure

   typedef struct{
      	IDL_KW_RESULT_FIRST_FIELD;	
	IDL_LONG limit;
	IDL_LONG islimit;
	IDL_LONG verbose;
	IDL_LONG debug;
	IDL_LONG help;
   } KW_RESULT;
   
   
// Maintain Alphabetical Order of Keywords
 
   static IDL_KW_PAR kw_pars[] = 
   { IDL_KW_FAST_SCAN,
     {"DEBUG",   IDL_TYP_LONG,  1,IDL_KW_ZERO,0, 			IDL_KW_OFFSETOF(debug)},    
     {"HELP",    IDL_TYP_LONG,  1,IDL_KW_ZERO,0, 			IDL_KW_OFFSETOF(help)},
     {"LIMIT",   IDL_TYP_LONG,  1,IDL_KW_ZERO,IDL_KW_OFFSETOF(islimit), IDL_KW_OFFSETOF(limit)},
     {"VERBOSE", IDL_TYP_LONG,  1,IDL_KW_ZERO,0, 			IDL_KW_OFFSETOF(verbose)},    			
   {NULL}};    
 
   KW_RESULT kw;
   
   kw.limit     = 0;
   kw.islimit   = 0;
   kw.verbose   = 0;
   kw.debug     = 0;
   kw.help      = 0; 
   
//-----------------------------------------------------------------------
// Get Passed Keywords and Parameters

   IDL_KWProcessByOffset(argc,argv,argk,kw_pars,(IDL_VPTR *)0,1,&kw);   
    
   if(kw.debug){    
      if(kw.debug)  fprintf(stdout,"Debug Keyword Passed\n");
      if(kw.verbose)fprintf(stdout,"Verbose Keyword Passed\n");   
      if(kw.help)   fprintf(stdout,"Help Keyword Passed\n");
      if(kw.islimit)fprintf(stdout,"Limit Keyword Passed: limit = %d\n", kw.limit);
   }   
   
//--------------------------------------------------------------------------      
// Call for HELP?

   if(kw.help){
      fprintf(stdout,"queryStrCPF: Help Not Implemented ... yet!\n");
      IDL_KW_FREE;
      return(IDL_GettmpLong(0));
   }    
   
//-----------------------------------------------------------------------
// Check Single Strings were Passed - 
   
   IDL_ENSURE_STRING(argv[0]);
   IDL_ENSURE_SCALAR(argv[0]);
   
   IDL_ENSURE_STRING(argv[1]);
   IDL_ENSURE_SCALAR(argv[1]);
   
   IDL_ENSURE_STRING(argv[2]);
   IDL_ENSURE_SCALAR(argv[2]);
      
//-----------------------------------------------------------------------
// Create the SQL Query
   
   colname = (char *)IDL_STRING_STR(&argv[0]->value.str);	// Table Column Name
   strcat(sql, colname ) ;		
   
   strcat(sql, " from ");
   
   table = (char *)IDL_STRING_STR(&(argv[1]->value.str));	// Table Name
   strcat(sql, table);	 
   
   where = (char *)IDL_STRING_STR(&(argv[2]->value.str));	// Where Clause
   
   if(strlen(where) > 0){
      strcat(sql, " where ");     
      strcat(sql, where);
   }   
   
   if(kw.islimit){
      sprintf(slimit," limit %d;", kw.limit);
      strcat(sql, slimit);
   }
   
   if(kw.debug)fprintf(stdout,"SQL: %s\n", sql);
            
//--------------------------------------------------------------------------      
// Connect to the Database Server

   if (!(DBConnect = StartDbSQL(DBName, kw.verbose))){
      if(kw.verbose) fprintf(stdout,"queryCPF: SQL Server Connect Error \n");
      IDL_KW_FREE;
      return(IDL_GettmpLong(CPF_SERVER_CONNECT_ERROR)); 
   }         

//-------------------------------------------------------------  
// Execute the SQL

   if ((DBQuery = PQexec(DBConnect, sql)) == NULL){
      if(kw.verbose){
         fprintf(stdout,"querySQL: ERROR - Failure to Execute the Query: %s\n", sql);
         fprintf(stdout,"querySQL: ERROR - %s\n", PQresultErrorMessage(DBQuery)); 
      }
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_QUERY_FAILED));     
   } 
   
   DBQueryStatus = PQresultStatus(DBQuery);        
    
   if (DBQueryStatus != PGRES_TUPLES_OK){
      if(kw.verbose){  
         fprintf(stdout,"querySQL: ERROR - Query Incorrectly Processed %s\n", sql);
         fprintf(stdout,"querySQL: ERROR - %s\n", PQresultErrorMessage(DBQuery)); 
      }
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_QUERY_INCORRECT));
   }
   
//-------------------------------------------------------------  
// Extract the Resultset 
    
   nrows = PQntuples(DBQuery); 		// Number of Rows
   ncols = PQnfields(DBQuery);		// Number of Columns
	 
   if(nrows == 0){
      if(kw.verbose)fprintf(stdout,"querySQL: ERROR - No Record Satisfy the Query %s\n", sql);
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_NO_ROWS));
   }  
    	    
   if(ncols == 0){
      if(kw.verbose)fprintf(stdout,"querySQL: ERROR - No Columns Created by the Query %s\n",sql);  
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_NO_COLUMNS));
   }
   
   if(ncols > 1){
      if(kw.verbose)fprintf(stdout,"querySQL: ERROR - Too Many Columns Created by the Query %s\n",sql);  
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_EXCESSIVE_COLUMNS));
   }  

//-------------------------------------------------------------------------------------------------
// Return String Array to IDL 

   lDimensions[0] = nrows;
   pisArray = (IDL_STRING *)IDL_MakeTempArray((int)IDL_TYP_STRING,1,lDimensions,IDL_ARR_INI_NOP,&ivReturn);

   for(i=0;i<nrows;i++){
      value = PQgetvalue(DBQuery,i,0);
      iStringLength = strlen(value);
      if(iStringLength > 0 ){				
         IDL_StrEnsureLength(&(pisArray[i]),iStringLength);
	 IDL_StrStore(&(pisArray[i]),value);
	 pisArray[i].slen=iStringLength; 	 		 
      } else {
         IDL_StrEnsureLength(&(pisArray[i]),0);
	 IDL_StrStore(&(pisArray[i]),"");
	 pisArray[i].slen=0;
      }	 	 
   } 
             	     
//--------------------------------------------------------------------------      
// Cleanup Keywords & PG Heap 
 
   IDL_KW_FREE;
   PQclear(DBQuery);    
       
   return(ivReturn);           
}

int nGroupRows = 0; 

IDL_VPTR IDL_CDECL groupCountCPF(int argc, IDL_VPTR argv[], char *argk){
//
// IDL DLM Function to Group Count CPF Database entries
//
// 3 Arguments:
//
//	argv[0] - Array of Column Names for the Group (Max is 3)
//	argv[1] - the CPF Table Name
//	argv[2] - the where clause criteria 
//
// 1 Keywords:
//
//	debug   - Print Debug Messages
//	help    - Print a useful Help Message
//	verbose - Print Error Messages
//
// v0.01	November 2005	D.G.Muir	Original Release
//-------------------------------------------------------------------------	      

   int i, j, rc, err, nrows, ncols, ngrps, exp_number;
   int iStringLength;
   
   char     *DBName    = DATABASE;
   PGresult *DBQuery   = NULL;  
   ExecStatusType DBQueryStatus; 
      
   IDL_STRING *pIDLstrDesc;
   IDL_STRING *pisArray;
   char *colname, *table, *where, *value; 
   
   char sql[MAXSQL] = "select ";
   char slimit[MAXSTR];
   char msg[MAXSTR];
   	    
   CSOUT1 *sout1;			// Pointer to the Returned IDL/C Structure Array;
   CSOUT1 *s1;
   CSOUT2 *sout2;			// Pointer to the Returned IDL/C Structure Array;
   CSOUT2 *s2;
   CSOUT3 *sout3;			// Pointer to the Returned IDL/C Structure Array;
   CSOUT3 *s3;
      
   IDL_VPTR ivReturn = NULL; 
   
   void *psDef       = NULL;
   static IDL_LONG ilDims[IDL_MAX_ARRAY_DIM];
      
   static IDL_LONG vdlen[]    = {1,1};		// Scalar Value Array
     
   IDL_STRUCT_TAG_DEF pTags[] = {
        {"COUNT",	0,	(void *) IDL_TYP_LONG},
	{"GROUP",	vdlen,	(void *) IDL_TYP_STRING},				
	{0}
   };
   
// Keyword Structure

   typedef struct{
      	IDL_KW_RESULT_FIRST_FIELD;	
	IDL_LONG   limit;
	IDL_LONG   islimit;
	IDL_LONG   verbose;
	IDL_LONG   debug;
	IDL_LONG   help;
	IDL_LONG   isorderby;
	IDL_STRING orderby;
   } KW_RESULT;
   
   
// Maintain Alphabetical Order of Keywords
 
   static IDL_KW_PAR kw_pars[] = 
   { IDL_KW_FAST_SCAN,
     {"DEBUG",   IDL_TYP_LONG,  1,IDL_KW_ZERO,0, 			  IDL_KW_OFFSETOF(debug)},    
     {"HELP",    IDL_TYP_LONG,  1,IDL_KW_ZERO,0, 			  IDL_KW_OFFSETOF(help)},
     {"LIMIT",   IDL_TYP_LONG,  1,IDL_KW_ZERO,IDL_KW_OFFSETOF(islimit),   IDL_KW_OFFSETOF(limit)},
     {"ORDERBY", IDL_TYP_STRING,1,IDL_KW_ZERO,IDL_KW_OFFSETOF(isorderby), IDL_KW_OFFSETOF(orderby)},     			
     {"VERBOSE", IDL_TYP_LONG,  1,IDL_KW_ZERO,0, 			  IDL_KW_OFFSETOF(verbose)},
   {NULL}};    
 
   KW_RESULT kw;
   
   kw.limit     = 0;
   kw.islimit   = 0;
   kw.verbose   = 0;
   kw.debug     = 0;
   kw.help      = 0;
   kw.isorderby = 0; 
   
//-----------------------------------------------------------------------
// Get Passed Keywords and Parameters

   IDL_KWProcessByOffset(argc,argv,argk,kw_pars,(IDL_VPTR *)0,1,&kw);   
       
//--------------------------------------------------------------------------      
// Call for HELP?

   if(kw.help){
      IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO,"Help Not Implemented ... yet!");
      IDL_KW_FREE;
      return(IDL_GettmpLong(0));
   }    
   
//-----------------------------------------------------------------------
// Check a String was Passed
   
   IDL_ENSURE_ARRAY(argv[0]);
   IDL_ENSURE_STRING(argv[0]);
   
   IDL_ENSURE_STRING(argv[1]);
   
   IDL_ENSURE_STRING(argv[2]);
   
//-----------------------------------------------------------------------
// Create the SQL Query
   
   ngrps = ncols = argv[0]->value.arr->n_elts;			// Number of Required Column (group) Names
   
   if(ncols > 3){
      IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO,"WARNING - Only First 3 Group Values will be Returned");  
      ngrps = 3;
   }   
   
   pIDLstrDesc = (IDL_STRING *) argv[0]->value.arr->data; 
   
   for(i=0;i<ncols;i++) {
      strcat(sql, (char *)(*pIDLstrDesc++).s) ;			// Names
      strcat(sql, ",");
   }
   strcat(sql, " count(*) as count from ");
   
   table = (char *)IDL_STRING_STR(&(argv[1]->value.str));	// Table Name
   strcat(sql, table);	 
   
   where = (char *)IDL_STRING_STR(&(argv[2]->value.str));	// Where Clause
   if(strlen(where) > 0){
      strcat(sql, " where ");     
      strcat(sql, where);
   }   
   
   if(kw.islimit){
      sprintf(slimit," limit %d;", kw.limit);
      strcat(sql, slimit);
   }
   
   pIDLstrDesc = (IDL_STRING *) argv[0]->value.arr->data;
   strcat(sql, " group by ");
   for(i=0;i<ncols;i++) {
      strcat(sql, (char *)(*pIDLstrDesc++).s) ;			// Return Ordering
      if(i < (ncols-1))strcat(sql, ",");
   }
   
   if(kw.isorderby){
      strcat(sql, " order by ") ;	 
      strcat(sql, IDL_STRING_STR(&kw.orderby) ); 
   }
      
   if(kw.debug){
      sprintf(msg,"Number of Column Names: %d", ncols);
      IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, msg);
      
      pIDLstrDesc = (IDL_STRING *) argv[0]->value.arr->data;
      for(i=0;i<ncols;i++){
         sprintf(msg,"Column Names: %d  %s", i,(char *)(*pIDLstrDesc++).s);
         IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, msg);	 
      }
      IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO,sql);
   } 
   
//--------------------------------------------------------------------------      
// Connect to the Database Server

   if (!(DBConnect = StartDbSQL(DBName, kw.verbose))){
      if(kw.verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "SQL Server Connect Error");
      IDL_KW_FREE;
      return(IDL_GettmpLong(CPF_SERVER_CONNECT_ERROR)); 
   }         

//-------------------------------------------------------------  
// Execute the SQL

   if ((DBQuery = PQexec(DBConnect, sql)) == NULL){
      if(kw.verbose){
         sprintf(msg,"ERROR - Failure to Execute the Query: %s", PQresultErrorMessage(DBQuery));
         IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, msg);
      }
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_QUERY_FAILED));     
   } 
   
   DBQueryStatus = PQresultStatus(DBQuery);        
    
   if (DBQueryStatus != PGRES_TUPLES_OK){
      if(kw.verbose){ 
         sprintf(msg,"ERROR - Query Incorrectly Processed: %s", PQresultErrorMessage(DBQuery));
         IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, msg); 
      }
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_QUERY_INCORRECT));
   }
   
//-------------------------------------------------------------  
// Extract the Resultset 
    
   nrows = PQntuples(DBQuery); 		// Number of Rows
   ncols = PQnfields(DBQuery);		// Number of Columns
	 
   if(nrows == 0){
      if(kw.verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - No Record Satisfies the Query");
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_NO_ROWS));
   }  
    	    
   if(ncols == 0){
      if(kw.verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - No Columns Created by the Query");  
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_NO_COLUMNS));
   }  

// Allocate the Appropriate Return Structure Array
 
   err = 0;

   switch(ngrps){
      
      case 1: sout1 = (CSOUT1 *) malloc(nrows*sizeof(CSOUT1));   
              if(sout1 == NULL) err = 1;              
              break;
      
      case 2: sout2 = (CSOUT2 *) malloc(nrows*sizeof(CSOUT2));   
              if(sout2 == NULL) err = 1;              
              break;
      
      case 3: sout3 = (CSOUT3 *) malloc(nrows*sizeof(CSOUT3));   
              if(sout3 == NULL) err = 1;              
              break;
   }

   if(err){
      if(kw.verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - Unable to Allocate Heap!");
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_HEAP_ALLOC_ERROR));
   }  
    
   for(i=0;i<nrows;i++){
      
      switch(ngrps){
         case 1: sout1[i].count = (int)atoi(PQgetvalue(DBQuery,i,ncols-1));
	         s1 = sout1+i;
		 IDL_StrStore(&(s1->group),"");           
                 break;
		 
	 case 2: sout2[i].count = (int)atoi(PQgetvalue(DBQuery,i,ncols-1));
	         s2 = sout2+i;
		 IDL_StrStore(&(s2->group[0]),"");
                 IDL_StrStore(&(s2->group[1]),"");           
                 break;	
		 	 
	 case 3: sout3[i].count = (int)atoi(PQgetvalue(DBQuery,i,ncols-1));
	         s3 = sout3+i;
		 IDL_StrStore(&(s3->group[0]),"");
                 IDL_StrStore(&(s3->group[1]),"");
                 IDL_StrStore(&(s3->group[2]),"");           
                 break;		 
      }
      	              
      //IDL_StrEnsureLength(&(s->group1),0);		// Causes a Segmentation Error 
          
         
      for(j=0;j<ncols;j++) {

         value = PQgetvalue(DBQuery,i,j);
	 
	 if(kw.debug){
	    sprintf(msg,"[%d, %d]  [%s]\n", i, j, value);
            IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, msg);  
	 }   
   	 
	 if(j < 3 && j != (ncols-1)){
	    if(j == 0){
               if((iStringLength = strlen(value)) > 0 ){				
                  switch(ngrps){
		     case 1: IDL_StrStore(&(s1->group),value);          
                             break;
		     case 2: IDL_StrStore(&(s2->group[0]),value);          
                             break;
		     case 3: IDL_StrStore(&(s3->group[0]),value);          
                             break;	 
                  }
               } 	  
            } else {
	       if(j == 1){
                  if((iStringLength = strlen(value)) > 0 ){				
	             switch(ngrps){
		        case 2: IDL_StrStore(&(s2->group[1]),value);          
                                break;
		        case 3: IDL_StrStore(&(s3->group[1]),value);          
                                break;	 
                     }
                  }   
               } else {
	          if(j == 2){
                     if((iStringLength = strlen(value)) > 0 ){				
	                IDL_StrStore(&(s3->group[2]),value);
                     }  
                  }
	       }
	    }    	 
	 }
 	  	 	 	 	     		 
      }                  	 
   } 
   	            
//--------------------------------------------------------------------------      
// Create the IDL Structure 
   
   nGroupRows = nrows;		// retain for Heap Memory Management
   ilDims[0]  = nrows;		// Number of Structure Array Elements
   vdlen[1]   = ngrps;		// Number of Groups
   
   psDef = IDL_MakeStruct(NULL, pTags); 
    
   switch(ngrps){
      case 1: ivReturn = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR *)sout1, freeMem, psDef);          
              break;
      case 2: ivReturn = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR *)sout2, freeMem, psDef);          
              break;
      case 3: ivReturn = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR *)sout3, freeMem, psDef);          
              break;      	            
   }       
               	     
//--------------------------------------------------------------------------      
// Cleanup Keywords & PG Heap 

   IDL_KW_FREE;
   PQclear(DBQuery);          
   return(ivReturn);
}


IDL_VPTR IDL_CDECL queryArrayCPF(int argc, IDL_VPTR argv[], char *argk){
//
// IDL DLM Function to Query the CPF Database for Array Value Types
//
// 3 Arguments:
//
//	argv[0] - Array of Column Names for the SQL Select
//	argv[1] - the CPF Table Name
//	argv[2] - the where clause criteria 
//
// 1 Keywords:
//
//	debug   - Print Debug Messages
//	help    - Print a useful Help Message
//	verbose - Print Error Messages
//
// v0.01	31Jan2006	D.G.Muir	Original Release
//-------------------------------------------------------------------------	      
   int i, j, k, kk, m, maxcount, maxnr, maxns, rc, nrows, ncols, exp_number, is2d=0;
   int larray, test;
   
   char     *DBName    = DATABASE;
   PGresult *DBQuery   = NULL;  
   ExecStatusType DBQueryStatus; 
      
   IDL_STRING *pIDLstrDesc;
   char *table, *where, *value, *token, *token2; 
   char tokvalue[13];
   
   char sql[MAXSQL] = "select ";
   char slimit[MAXSTR];
   	    
   SOUTARR  *sout;			// Pointer to the Array Oriented Returned IDL/C Structure;
   SOUTARR2 *sout2;
   
   double *valArray;
   
   IDL_VPTR ivReturn = NULL;   
   void *psDef       = NULL;
   void *psDef2      = NULL;
   
   static IDL_LONG ilDims[IDL_MAX_ARRAY_DIM];
   
   static IDL_LONG vdlen[]  = {3,1,1,1};	// 1-D Array Value Array
   static IDL_LONG vdlen2[] = {4,1,1,1,1};	// 2-D Array Value Array
      
   IDL_STRUCT_TAG_DEF pTags[] = {
   	{"SQL",		0,		(void *) IDL_TYP_STRING},
	{"ROWS",	0,		(void *) IDL_TYP_LONG},
	{"COLS",	0,		(void *) IDL_TYP_LONG},
	{"N_ELEMENTS",	0,		(void *) IDL_TYP_LONG},
	{"VALUES",	vdlen,		(void *) IDL_TYP_DOUBLE},		
	{0}
   };	
   
      IDL_STRUCT_TAG_DEF pTags2[] = {
   	{"SQL",		0,		(void *) IDL_TYP_STRING},
	{"ROWS",	0,		(void *) IDL_TYP_LONG},
	{"COLS",	0,		(void *) IDL_TYP_LONG},
	{"NR_ELEMENTS",	0,		(void *) IDL_TYP_LONG},
	{"NS_ELEMENTS",	0,		(void *) IDL_TYP_LONG},
	{"VALUES",	vdlen2,		(void *) IDL_TYP_DOUBLE},		
	{0}
   };	
   
// Keyword Structure

   typedef struct{
      	IDL_KW_RESULT_FIRST_FIELD;	
	IDL_LONG limit;
	IDL_LONG islimit;
	IDL_LONG verbose;
	IDL_LONG debug;
	IDL_LONG help;
   } KW_RESULT;
   
   
// Maintain Alphabetical Order of Keywords
 
   static IDL_KW_PAR kw_pars[] = 
   { IDL_KW_FAST_SCAN,
     {"DEBUG",   IDL_TYP_LONG,  1,IDL_KW_ZERO,0, 			IDL_KW_OFFSETOF(debug)},    
     {"HELP",    IDL_TYP_LONG,  1,IDL_KW_ZERO,0, 			IDL_KW_OFFSETOF(help)},
     {"LIMIT",   IDL_TYP_LONG,  1,IDL_KW_ZERO,IDL_KW_OFFSETOF(islimit), IDL_KW_OFFSETOF(limit)},
     {"VERBOSE", IDL_TYP_LONG,  1,IDL_KW_ZERO,0, 			IDL_KW_OFFSETOF(verbose)},    			
   {NULL}};    
 
   KW_RESULT kw;
   
   kw.limit     = 0;
   kw.islimit   = 0;
   kw.verbose   = 0;
   kw.debug     = 0;
   kw.help      = 0; 
   
//-----------------------------------------------------------------------
// Get Passed Keywords and Parameters

   IDL_KWProcessByOffset(argc,argv,argk,kw_pars,(IDL_VPTR *)0,1,&kw);   
    
   if(kw.debug){    
      if(kw.debug)  fprintf(stdout,"Debug Keyword Passed\n");
      if(kw.verbose)fprintf(stdout,"Verbose Keyword Passed\n");   
      if(kw.help)   fprintf(stdout,"Help Keyword Passed\n");
      if(kw.islimit)fprintf(stdout,"Limit Keyword Passed: limit = %d\n", kw.limit);
   }   
   
//--------------------------------------------------------------------------      
// Call for HELP?

   if(kw.help){
      fprintf(stdout,"queryCPF: Help Not Implemented ... yet!\n");
      IDL_KW_FREE;
      return(IDL_GettmpLong(0));
   }    
   
//-----------------------------------------------------------------------
// Check a String was Passed
   
   IDL_ENSURE_ARRAY(argv[0]);
   IDL_ENSURE_STRING(argv[0]);
   
   IDL_ENSURE_STRING(argv[1]);
   //IDL_ENSURE_SCALAR(argv[1]);
   
   IDL_ENSURE_STRING(argv[2]);
   //IDL_ENSURE_SCALAR(argv[2]);
      
//-----------------------------------------------------------------------
// Create the SQL Query
   
   ncols = argv[0]->value.arr->n_elts;		// Number of Required Column Names
   pIDLstrDesc = (IDL_STRING *) argv[0]->value.arr->data; 
   
   for(i=0;i<ncols;i++) {
      strcat(sql, (char *)(*pIDLstrDesc++).s) ;	// Names
      if(i < (ncols-1))strcat(sql, ",");
   }
   strcat(sql, " from ");
   
   table = (char *)IDL_STRING_STR(&(argv[1]->value.str));	// Table Name
   strcat(sql, table);	 
   
   where = (char *)IDL_STRING_STR(&(argv[2]->value.str));	// Where Clause
   if(strlen(where) > 0){
      strcat(sql, " where ");     
      strcat(sql, where);
   }   
   
   if(kw.islimit){
      sprintf(slimit," limit %d;", kw.limit);
      strcat(sql, slimit);
   }
            
//--------------------------------------------------------------------------      
// Connect to the Database Server

   if (!(DBConnect = StartDbSQL(DBName, kw.verbose))){
      if(kw.verbose) fprintf(stdout,"queryCPF: SQL Server Connect Error \n");
      IDL_KW_FREE;
      return(IDL_GettmpLong(CPF_SERVER_CONNECT_ERROR)); 
   }         

//-------------------------------------------------------------  
// Execute the SQL

   if ((DBQuery = PQexec(DBConnect, sql)) == NULL){
      if(kw.verbose){
         fprintf(stdout,"querySQL: ERROR - Failure to Execute the Query: %s\n", sql);
         fprintf(stdout,"querySQL: ERROR - %s\n", PQresultErrorMessage(DBQuery)); 
      }
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_QUERY_FAILED));     
   } 
   
   DBQueryStatus = PQresultStatus(DBQuery);        
    
   if (DBQueryStatus != PGRES_TUPLES_OK){
      if(kw.verbose){  
         fprintf(stdout,"querySQL: ERROR - Query Incorrectly Processed %s\n", sql);
         fprintf(stdout,"querySQL: ERROR - %s\n", PQresultErrorMessage(DBQuery)); 
      }
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_QUERY_INCORRECT));
   }
   
//-------------------------------------------------------------  
// Extract the Resultset 
    
   nrows = PQntuples(DBQuery); 		// Number of Rows
   ncols = PQnfields(DBQuery);		// Number of Columns
	 
   if(nrows == 0){
      if(kw.verbose)fprintf(stdout,"querySQL: ERROR - No Record Satisfy the Query %s\n", sql);
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_NO_ROWS));
   }  
    	    
   if(ncols == 0){
      if(kw.verbose)fprintf(stdout,"querySQL: ERROR - No Columns Created by the Query %s\n",sql);  
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_NO_COLUMNS));
   }  

// Returned Values (Scalar and Array)

   maxcount = MAXARRAYNR*MAXARRAYNS; 
   maxnr    = 0;			// Set First time through the 2D Array Section
   maxns    = 0;
   kk       = 0;			// Offset Counter for 2D Arrays 
    
   if((valArray = (double *) malloc(nrows*ncols*MAXARRAYNR*MAXARRAYNS*sizeof(double))) == NULL){   
      if(kw.verbose){
         fprintf(stdout,"queryArraySQL: ERROR - Unable to Allocate Heap!\n");
	 fprintf(stdout,"nrows: %d\nncols: %d\n",nrows, ncols);
	 unsigned long long q = nrows*ncols*MAXARRAYNR*MAXARRAYNS;
	 fprintf(stdout,"n: %ull\n",q);
	 fflush(NULL);
      }
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_HEAP_ALLOC_ERROR));
   }  	
    
   for(i=0;i<nrows;i++){
      for(j=0;j<ncols;j++) {
      
// Check the First Column is of Array type?

         value = PQgetvalue(DBQuery,i,j);
         if(i ==0 && j ==0 && value[0] != '{'){		 
	    if(kw.verbose)fprintf(stdout,"querySQL: ERROR - The First Data Column Must be an Array Type\n");  
	    IDL_KW_FREE;
	    PQclear(DBQuery);
	    free((void *) valArray);
	    return(IDL_GettmpLong(CPF_FIRST_COLUMN_NOT_ARRAY_TYPE));
	 } 
	 
	 if(kw.debug){
	    fprintf(stdout,"Array of Values Returned\n");
	    fprintf(stdout,"%s\n", PQgetvalue(DBQuery,i,j));
	 }   
	  
	 if(value[0] == '{'){	// This Column Type is Array so Parse for Values
	 
	    if(value[1] == '{'){	// This Column Type is 2-D Array
//----------	    
	       is2d = 1;	// Flag Array type
	    
	       if(kw.debug)fprintf(stdout,"2-D Array of Values Returned\n");
	       	       
	       token  = value+2;		// Starting point
	       larray = strlen(token);		// Length of String to Scan
	       test   = 0;			// Counter
	       
	       //fprintf(stdout,"%s\n",token);
	       do{
	          for(k=0;k<12;k++){		// 12 digit number is maximum
		     //if((larray-test) < 110){
		     //   fprintf(stdout,"%d %c\n",k,*(token+k));
		     //}
	             if(*(token+k) == ',' || *(token+k) == '}'){		// Delimiter Found
		        strncpy(tokvalue,token,k);
		        tokvalue[k] = '\0';
			
		        valArray[kk++] = (double)atof(tokvalue);
			
		        if(*(token+k) == ','){ 
		           if(i == 0  && j == 0 && maxnr == 0) maxns++;
			   //fprintf(stdout,"maxns = %d\n",maxns);
		        } else 
			   if(*(token+k) == '}'){
			      if(i == 0  && j == 0 && maxnr == 0) maxns++; 
		              if(i == 0  && j == 0) maxnr++;			      
			      //fprintf(stdout,"maxnr = %d\n",maxnr);
			   }  
			   
			//fprintf(stdout,"[%d][%d] = %f\n",maxnr,maxns,valArray[(i*ncols+j)*maxcount+kk]);
			   
			if(kk >= nrows*ncols*MAXARRAYNR*MAXARRAYNS){
                           if(kw.verbose)fprintf(stdout,"queryArraySQL: ERROR - 2-D Array Too Large!!\n");
                           IDL_KW_FREE;
                           PQclear(DBQuery);
                           if(valArray != NULL) free((void *)valArray);
                           return(IDL_GettmpLong(CPF_2D_ARRAY_TOO_LARGE));
                        }  	   
			    
			break;   
		     }		   
	          }
		  token = token+k+1;
		  test=test+k;
		  
		  if(*token == ',' && *(token+1) == '{'){
		     token = token+2;
		     test  = test +2;
		  }
		  if(*token == '}'){
		     test = larray;		// Last Delimiter Found
		  }   
		  
		  //if((larray-test) > 110)
		  //   fprintf(stdout,"%d %d %d %s\n",k, test, larray, tokvalue);
		  //else 
		  //   fprintf(stdout,"%d %d %d %s %s\n",k, test, larray, tokvalue, token);  
	       } while(test < larray);	
	       
	       if(i == 0 && j == 0) maxcount = kk;	  	       
		            
	    } else {  
	 	       	        
	       token = NULL;	 
	       for(k=0;k<maxcount;k++){
                  if(k==0)
                     token = strtok(value+1, ",}");	// parse String for Array Element Values
                  else
                     token = strtok(NULL, ",}");	 
                  if(token == NULL) break;	       
	          valArray[(i*ncols+j)*maxcount+k] = (double)atof(token);
	          if(kw.debug) fprintf(stdout,"[%d]   %s\n", k, token);  
		  
		  if((i*ncols+j)*maxcount+k == nrows*ncols*MAXARRAYNR*MAXARRAYNS){
                     if(kw.verbose)fprintf(stdout,"queryArraySQL: ERROR - 2-D Array Too Large!!\n");
                     IDL_KW_FREE;
                     PQclear(DBQuery);
                     if(valArray != NULL) free((void *)valArray);
                     return(IDL_GettmpLong(CPF_1D_ARRAY_TOO_LARGE));
                  }  
		    
               }
	    
	       if(i == 0 && j == 0) maxcount = k;		// All Arrays have this First Array Length

	       if(kw.debug && i == 0) fprintf(stdout,"Max Array Length = %d\n", maxcount); 
	    
	       if(k < maxcount)for(kk=k;kk<maxcount;kk++)valArray[(i*ncols+j)*maxcount+kk] = 0.0;
	    }      
	 } else {	    
	    valArray[(i*ncols+j)*maxcount] = (double)atof(PQgetvalue(DBQuery,i,j));
	    for(k=1;k<maxcount;k++)valArray[(i*ncols+j)*maxcount+k] = 0.0;
	 }    	 		 
      }	 	 
   } 
   
// Allocate the Appropriate Return Structure

   if(is2d){
   
      fprintf(stdout,"maxnr = %d\n", maxnr);
      fprintf(stdout,"maxns = %d\n", maxns); 
        
      if((sout2 = (SOUTARR2 *)malloc(sizeof(SOUTARR2)+nrows*ncols*maxnr*maxns*sizeof(double))) == NULL){
         if(kw.verbose)fprintf(stdout,"queryArraySQL: ERROR - Unable to Allocate Heap!\n");
         IDL_KW_FREE;
         PQclear(DBQuery);
         if(valArray != NULL) free((void *)valArray);
         return(IDL_GettmpLong(CPF_HEAP_ALLOC_ERROR));
      }  	
    
      IDL_StrStore(&(sout2->sql), sql); 
   
      sout2->rows        = (IDL_LONG) nrows;
      sout2->cols        = (IDL_LONG) ncols;
      sout2->nr_elements = (IDL_LONG) maxnr;
      sout2->ns_elements = (IDL_LONG) maxns;
      
      sout2->values = (double *)(sout+sizeof(SOUTARR2));
      memcpy((void *)&sout2->values, (void *)valArray, (size_t)maxnr*maxns*nrows*ncols*sizeof(double));
                    
// Free Local Heap Buffers
   
      if(valArray != NULL) free((void *)valArray);
      
// Create the IDL Structure 
   
      ilDims[0] = 1;			// Number of Structure Array Elements
    
      vdlen2[4]  = nrows;		// Number of Records
      vdlen2[3]  = ncols;		// Number of Column Values (Scalar and Array Types)
      vdlen2[2]  = maxnr;	        // Array Type Length [maxns][maxnr]
      vdlen2[1]  = maxns;	        // Array Type Length	 
   
      psDef     = IDL_MakeStruct(NULL, pTags2);	 
      ivReturn  = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR *)sout2, freeMem, psDef); 

   } else {

      if((sout = (SOUTARR *)malloc(sizeof(SOUTARR)+nrows*ncols*maxcount*sizeof(double))) == NULL){
         if(kw.verbose)fprintf(stdout,"querySQL: ERROR - Unable to Allocate Heap!\n");
         IDL_KW_FREE;
         PQclear(DBQuery);
         if(valArray != NULL) free((void *)valArray);
         return(IDL_GettmpLong(CPF_HEAP_ALLOC_ERROR));
      }  	
    
      IDL_StrStore(&(sout->sql), sql); 
   
      sout->rows       = (IDL_LONG) nrows;
      sout->cols       = (IDL_LONG) ncols;
      sout->n_elements = (IDL_LONG) maxcount;
      
      sout->values = (double *)(sout+sizeof(SOUTARR));
      memcpy((void *)&sout->values, (void *)valArray, (size_t)maxcount*nrows*ncols*sizeof(double));
                    
// Free Local Heap Buffers
   
      if(valArray != NULL) free((void *)valArray);
      
// Create the IDL Structure 
   
      ilDims[0] = 1;			// Number of Structure Array Elements
    
      vdlen[3]  = nrows;		// Number of Records
      vdlen[2]  = ncols;		// Number of Column Values (Scalar and Array Types)
      vdlen[1]  = maxcount;	        // Array Type Length	 
   
      psDef     = IDL_MakeStruct(NULL, pTags);	 
      ivReturn  = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR *)sout, freeMem, psDef); 
 
   }
      
//--------------------------------------------------------------------------      
// Cleanup Keywords & PG Heap 
 
   IDL_KW_FREE;
   PQclear(DBQuery);    
       
   return(ivReturn);           
}




IDL_VPTR IDL_CDECL querySingleArrayCPF(int argc, IDL_VPTR argv[], char *argk){
//
// IDL DLM Function to Query the CPF Database for Array Value Types
//
// 3 Arguments:
//
//	argv[0] - Name of Column for the SQL Select
//	argv[1] - the CPF Table Name
//	argv[2] - the where clause criteria 
//
// 1 Keywords:
//
//	debug   - Print Debug Messages
//	help    - Print a useful Help Message
//	verbose - Print Error Messages
//
// v0.01	23Nov2010	D.G.Muir	Original Release
//-------------------------------------------------------------------------	      
   int i, irow, k, kk, m, maxcount, maxnr, maxns, rc, nrows, exp_number, is2d=0;
   int larray, test, lstr;
   int *exp_numbers;
   
   char     *DBName    = DATABASE;
   PGresult *DBQuery   = NULL;  
   ExecStatusType DBQueryStatus; 
      
   IDL_STRING *pIDLstrDesc;
   char *table, *where, *value, *token, *token2; 
   char tokvalue[13];
   
   char sql[MAXSQL] = "select exp_number, ";
   char slimit[MAXSTR];
   	    
   SOUTSARR  *sout;			// Pointer to the Array Oriented Returned IDL/C Structure;
   SOUTSARR2 *sout2;
   
   double *valArray;
   
   IDL_VPTR ivReturn = NULL;   
   void *psDef       = NULL;
   void *psDef2      = NULL;
   
   static IDL_LONG ilDims[IDL_MAX_ARRAY_DIM];
   
   static IDL_LONG vdlen[]  = {2,1,1};		// 1-D Array Value Array + 1
   static IDL_LONG vdlen2[] = {3,1,1,1};	// 2-D Array Value Array + 1
   
   static IDL_LONG vdlen3[] = {1,1};		// 1-D Array Value Array
      
   IDL_STRUCT_TAG_DEF pTags[] = {
   	{"SQL",		0,		(void *) IDL_TYP_STRING},
	{"ROWS",	0,		(void *) IDL_TYP_LONG},
	{"N_ELEMENTS",	0,		(void *) IDL_TYP_LONG},
	{"EXP_NUMBERS",	vdlen3,		(void *) IDL_TYP_LONG},
	{"VALUES",	vdlen,		(void *) IDL_TYP_DOUBLE},		
	{0}
   };	
   
      IDL_STRUCT_TAG_DEF pTags2[] = {
   	{"SQL",		0,		(void *) IDL_TYP_STRING},
	{"ROWS",	0,		(void *) IDL_TYP_LONG},
	{"NR_ELEMENTS",	0,		(void *) IDL_TYP_LONG},
	{"NS_ELEMENTS",	0,		(void *) IDL_TYP_LONG},
	{"EXP_NUMBERS",	vdlen3,		(void *) IDL_TYP_LONG},
	{"VALUES",	vdlen2,		(void *) IDL_TYP_DOUBLE},		
	{0}
   };	
   
// Keyword Structure

   typedef struct{
      	IDL_KW_RESULT_FIRST_FIELD;	
	IDL_LONG limit;
	IDL_LONG islimit;
	IDL_LONG verbose;
	IDL_LONG debug;
	IDL_LONG help;
   } KW_RESULT;
   
   
// Maintain Alphabetical Order of Keywords
 
   static IDL_KW_PAR kw_pars[] = 
   { IDL_KW_FAST_SCAN,
     {"DEBUG",   IDL_TYP_LONG,  1,IDL_KW_ZERO,0, 			IDL_KW_OFFSETOF(debug)},    
     {"HELP",    IDL_TYP_LONG,  1,IDL_KW_ZERO,0, 			IDL_KW_OFFSETOF(help)},
     {"LIMIT",   IDL_TYP_LONG,  1,IDL_KW_ZERO,IDL_KW_OFFSETOF(islimit), IDL_KW_OFFSETOF(limit)},
     {"VERBOSE", IDL_TYP_LONG,  1,IDL_KW_ZERO,0, 			IDL_KW_OFFSETOF(verbose)},    			
   {NULL}};    
 
   KW_RESULT kw;
   
   kw.limit     = 0;
   kw.islimit   = 0;
   kw.verbose   = 0;
   kw.debug     = 0;
   kw.help      = 0; 
   
//-----------------------------------------------------------------------
// Get Passed Keywords and Parameters

   IDL_KWProcessByOffset(argc,argv,argk,kw_pars,(IDL_VPTR *)0,1,&kw);   
    
   if(kw.debug){    
      if(kw.debug)  fprintf(stdout,"Debug Keyword Passed\n");
      if(kw.verbose)fprintf(stdout,"Verbose Keyword Passed\n");   
      if(kw.help)   fprintf(stdout,"Help Keyword Passed\n");
      if(kw.islimit)fprintf(stdout,"Limit Keyword Passed: limit = %d\n", kw.limit);
   }   
   
//--------------------------------------------------------------------------      
// Call for HELP?

   if(kw.help){
      fprintf(stdout,"querySingleArrayCPF: Help Not Implemented ... yet!\n");
      IDL_KW_FREE;
      return(IDL_GettmpLong(0));
   }    
   
//-----------------------------------------------------------------------
// Check a String was Passed
   
   IDL_ENSURE_STRING(argv[0]); 		// Array Variable Column Name  
   IDL_ENSURE_STRING(argv[1]);		// Database Table   
   IDL_ENSURE_STRING(argv[2]);		// WHERE Clause
      
//-----------------------------------------------------------------------
// Create the SQL Query
   
   strcat(sql, (char *)IDL_STRING_STR(&(argv[0]->value.str)));	// Column Name
   strcat(sql, " from ");   
   table = (char *)IDL_STRING_STR(&(argv[1]->value.str));	// Table Name
   strcat(sql, table);	    

   where = (char *)IDL_STRING_STR(&(argv[2]->value.str));	// Where Clause
   if(strlen(where) > 0){
      strcat(sql, " where ");     
      strcat(sql, where);
   }   
   
   if(kw.islimit){
      sprintf(slimit," limit %d;", kw.limit);
      strcat(sql, slimit);
   }
            
//--------------------------------------------------------------------------      
// Connect to the Database Server

   if (!(DBConnect = StartDbSQL(DBName, kw.verbose))){
      if(kw.verbose) fprintf(stdout,"querySingleArrayCPF: SQL Server Connect Error \n");
      IDL_KW_FREE;
      return(IDL_GettmpLong(CPF_SERVER_CONNECT_ERROR)); 
   }         

//-------------------------------------------------------------  
// Execute the SQL

   if ((DBQuery = PQexec(DBConnect, sql)) == NULL){
      if(kw.verbose){
         fprintf(stdout,"querySingleArrayCPF: ERROR - Failure to Execute the Query: %s\n", sql);
         fprintf(stdout,"querySingleArrayCPF: ERROR - %s\n", PQresultErrorMessage(DBQuery)); 
      }
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_QUERY_FAILED));     
   } 
   
   DBQueryStatus = PQresultStatus(DBQuery);        
    
   if (DBQueryStatus != PGRES_TUPLES_OK){
      if(kw.verbose){  
         fprintf(stdout,"querySingleArrayCPF: ERROR - Query Incorrectly Processed %s\n", sql);
         fprintf(stdout,"querySingleArrayCPF: ERROR - %s\n", PQresultErrorMessage(DBQuery)); 
      }
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_QUERY_INCORRECT));
   }
   
//-------------------------------------------------------------  
// Extract the Resultset 
    
   nrows = PQntuples(DBQuery); 		// Number of Rows
	 
   if(nrows == 0){
      if(kw.verbose)fprintf(stdout,"querySingleArrayCPF: ERROR - No Record Satisfy the Query %s\n", sql);
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_NO_ROWS));
   }  
    	    
// Returned Array Values 

   maxcount = MAXARRAYNR*MAXARRAYNS; 
   maxnr    = 0;			// Set First time through the 2D Array Section
   maxns    = 0;
   kk       = 0;			// Offset Counter for 2D Arrays 
    
   if((valArray = (double *) malloc(MAXARRAYNR*MAXARRAYNS*sizeof(double))) == NULL){   
      if(kw.verbose) fprintf(stdout,"querySingleArrayCPF: ERROR - Unable to Allocate Heap!\n");
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_HEAP_ALLOC_ERROR));
   }
       
   if((exp_numbers = (int *) malloc(nrows*sizeof(int))) == NULL){   
      if(kw.verbose) fprintf(stdout,"querySingleArrayCPF: ERROR - Unable to Allocate Heap!\n");
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_HEAP_ALLOC_ERROR));
   }      	
    
   irow  = 0;
   value = NULL;
   
   for(i=0;i<nrows;i++){
      
// Check the Column is of Array type?

      if((lstr = strlen(PQgetvalue(DBQuery,i,1))) == 0) continue;
      
      value = (char *)malloc((lstr+1)*sizeof(char));
      strcpy(value, PQgetvalue(DBQuery,i,1));
      TrimString(value);
      
      if(kw.debug){
         fprintf(stdout,"Array of Values Returned\n");
         fprintf(stdout,"%s\n", value);
      } 
      
      if(value[0] == '\0') continue;  
           
      if(irow == 0 && value[0] != '{'){		 
	 if(kw.verbose)fprintf(stdout,"querySingleArrayCPF: ERROR - The Requested Data Must be an Array Type\n");  
	 IDL_KW_FREE;
	 PQclear(DBQuery);
	 free((void *) value);
	 free((void *) valArray);
	 return(IDL_GettmpLong(CPF_FIRST_COLUMN_NOT_ARRAY_TYPE));
      } 
	 
	  
      if(value[1] == '{'){	// This Column Type is 2-D Array
	 is2d = 1;		// Flag Array type
	    
	 if(kw.debug)fprintf(stdout,"2-D Array of Values Returned\n");
	       	       
         token  = value+2;		// Starting point
	 larray = strlen(token);	// Length of String to Scan
	 test   = 0;			// Counter
	       
	 do{
	    for(k=0;k<12;k++){		// 12 digit number is maximum
               if(*(token+k) == ',' || *(token+k) == '}'){		// Delimiter Found
                  strncpy(tokvalue,token,k);
                  tokvalue[k] = '\0';
			
                  valArray[kk++] = (double)atof(tokvalue);
			
                  if(*(token+k) == ','){ 
                     if(irow == 0  && maxnr == 0) maxns++;			// Count 
                  } else { 
                     if(*(token+k) == '}'){
                        if(irow == 0 && maxnr == 0) maxns++; 
                        if(irow == 0) maxnr++;			      
                     } 
		  }    
			   			   
		  if(kk >= MAXARRAYNR*MAXARRAYNS){
                     if(kw.verbose)fprintf(stdout,"querySingleArraySQL: ERROR - 2-D Array is Too Large!!\n");
                     IDL_KW_FREE;
                     PQclear(DBQuery);
                     free((void *) value);
		     if(valArray != NULL) free((void *)valArray);
                     return(IDL_GettmpLong(CPF_2D_ARRAY_TOO_LARGE));
                  }  	   
			    
		  break;   
	       }		   
	    }
	       
	    token = token+k+1;
	    test  = test+k;
		  
	    if(*token == ',' && *(token+1) == '{'){
	       token = token+2;
	       test  = test +2;
	    }
	    if(*token == '}'){
	       test = larray;		// Last Delimiter Found

	       if((valArray = (double *)realloc((void *)valArray, nrows*kk*sizeof(double))) == NULL){   
	          if(kw.verbose)fprintf(stdout,"queryArraySQL: ERROR - Unable to Allocate Heap!\n");
	          IDL_KW_FREE;
	          PQclear(DBQuery);
	          free((void *) value);
		  return(IDL_GettmpLong(CPF_HEAP_ALLOC_ERROR));
	       }
	    }   
		  
	 } while(test < larray);
	 	 		       
	 if(irow == 0 ) maxcount = kk;		   	       
	
      } else {				// 1-D Array
      
         if(kw.debug)fprintf(stdout,"1-D Array of Values Returned\n"); 
      
         kk = 0;
	 token = NULL;
	 	 
         for(k=0;k<maxcount;k++){
            if(k==0)
               token = strtok(value+1, ",}");	// parse String for Array Element Values
            else
               token = strtok(NULL, ",}");	             
	    
	    if(token == NULL){			// End of the Array; Allocate correct Size	       	       
	       if(irow == 0){
	          maxcount = kk;		// All Arrays have this First Array Length
		  
		  if(kw.debug) fprintf(stdout,"Max Array Length = %d\n", maxcount);
		  
		  if((valArray = (double *)realloc((void *)valArray, nrows*maxcount*sizeof(double))) == NULL){   
                     if(kw.verbose)fprintf(stdout,"queryArraySQL: ERROR - Unable to Allocate Heap!\n");
                     IDL_KW_FREE;
                     PQclear(DBQuery);
                     free((void *) value);
		     return(IDL_GettmpLong(CPF_HEAP_ALLOC_ERROR));
                  }
               }	       
	       if(kw.debug) fprintf(stdout,"End of Array, maxcount[%d] = %d\n", irow, maxcount);	    
	       break;
	    }   
	    	       
            if(irow == 0){
	       valArray[kk++] = (double)atof(token);
	    } else {
	       valArray[irow*maxcount+k] = (double)atof(token);
	    }	
            
	    if(kw.debug) fprintf(stdout,"[%d]   %s\n", k, token);  
		   		    
         }	    
      }
      
      exp_numbers[irow++] = atof(PQgetvalue(DBQuery,i,0)); 
      free((void *) value);	 
   } 
   
   if(irow == 0){   
      if(kw.verbose)fprintf(stdout,"querySingleArraySQL: ERROR - No Valid Array Data Found!\n");
      IDL_KW_FREE;
      PQclear(DBQuery);
      if(value != NULL)free((void *) value);
      return(IDL_GettmpLong(999));
   }
   
   
   
// Allocate the Appropriate Return Structure

   if(is2d){
   
      fprintf(stdout,"maxnr = %d\n", maxnr);
      fprintf(stdout,"maxns = %d\n", maxns); 
        
      if((sout2 = (SOUTSARR2 *)malloc(sizeof(SOUTSARR2)+irow*maxnr*maxns*sizeof(double)+irow*sizeof(int))) == NULL){
         if(kw.verbose)fprintf(stdout,"querySingleArraySQL: ERROR - Unable to Allocate Heap!\n");
         IDL_KW_FREE;
         PQclear(DBQuery);
         if(valArray != NULL) free((void *)valArray);
         return(IDL_GettmpLong(CPF_HEAP_ALLOC_ERROR));
      }  	
    
      IDL_StrStore(&(sout2->sql), sql); 
   
      sout2->rows        = (IDL_LONG) irow;
      sout2->nr_elements = (IDL_LONG) maxnr;
      sout2->ns_elements = (IDL_LONG) maxns;
      
      sout2->key = (IDL_LONG *)(sout2+sizeof(SOUTSARR2));
      memcpy((void *)&sout2->key, (void *)exp_numbers, (size_t)irow*sizeof(int));
      
      sout2->values = (double *)(sout2+sizeof(SOUTSARR2)+irow*sizeof(int));
      memcpy((void *)&sout2->values, (void *)valArray, (size_t)maxnr*maxns*irow*sizeof(double));
                    
// Free Local Heap Buffers
   
      if(exp_numbers != NULL) free((void *)exp_numbers);
      if(valArray != NULL) free((void *)valArray);
      
// Create the IDL Structure 
   
      ilDims[0] = 1;			// Number of Structure Array Elements
    
      vdlen2[3] = irow;			// Number of Records
      vdlen2[2] = maxnr;	        // Array Type Length [maxns][maxnr]
      vdlen2[1] = maxns;	        // Array Type Length	 
      vdlen3[1] = irow;			// Number of Records
   
      psDef     = IDL_MakeStruct(NULL, pTags2);	 
      ivReturn  = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR *)sout2, freeMem, psDef); 

   } else {

      if((sout = (SOUTSARR *)malloc(sizeof(SOUTSARR)+irow*maxcount*sizeof(double)+irow*sizeof(int))) == NULL){
         if(kw.verbose)fprintf(stdout,"querySingleArraySQL: ERROR - Unable to Allocate Heap!\n");
         IDL_KW_FREE;
         PQclear(DBQuery);
         if(valArray != NULL) free((void *)valArray);
         return(IDL_GettmpLong(CPF_HEAP_ALLOC_ERROR));
      }  	
    
      IDL_StrStore(&(sout->sql), sql); 
fprintf(stdout,"nrows    = %d\n", irow);
fprintf(stdout,"maxcount = %d\n", maxcount);   
      sout->rows       = (IDL_LONG) irow;
      sout->n_elements = (IDL_LONG) maxcount;
      
      //sout->key    = (IDL_LONG *)exp_numbers;
      //sout->values = (double *)valArray;
      
      //sout->key = (IDL_LONG *)(sout+sizeof(SOUTSARR));    
      memcpy((void *)&sout->key, (void *)exp_numbers, (size_t)irow*sizeof(int));

      //sout->values = (double *)(sout+sizeof(SOUTSARR)+irow*sizeof(int));
      memcpy((void *)&sout->key+irow*sizeof(int), (void *)valArray, (size_t)maxcount*irow*sizeof(double));
                          
// Create the IDL Structure 
   
      ilDims[0] = 1;			// Number of Structure Array Elements
    
      vdlen[2]  = irow;			// Number of Records
      vdlen[1]  = maxcount;	        // Array Type Length	 
      vdlen3[1] = irow;			// Number of Records
   
      psDef     = IDL_MakeStruct(NULL, pTags);	 
      ivReturn  = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR *)sout, freeMem, psDef); 

// Free Local Heap Buffers
   
      if(exp_numbers != NULL) free((void *)exp_numbers);
      if(valArray != NULL) free((void *)valArray);
 
   }
      
//--------------------------------------------------------------------------      
// Cleanup Keywords & PG Heap 
 
   IDL_KW_FREE;
   PQclear(DBQuery);    
       
   return(ivReturn);           
}







IDL_VPTR IDL_CDECL putCPF(int argc, IDL_VPTR argv[], char *argk){
//
// IDL DLM Wrapper to Function to Query the CPF Database
//
// 3 Arguments:
//
//	argv[0] - Pulse Number
//	argv[1] - the CPF Table Name
//	argv[2] - Field/Column Name
//	argv[3] - Value as a String
//
// Returns: Status Value, 0 => OK
//
// 1 Keywords:
//
//	new     - Create a New Record
//	debug   - Print Debug Messages
//	help    - Print a useful Help Message
//	verbose - Print Error Messages
//
// v0.01	31Jan2006	D.G.Muir	Original Release
//-------------------------------------------------------------------------	      
   int i, j, rc, nrows, ncols, exp_number;
   
   char     *DBName    = DATABASE;
   PGresult *DBQuery   = NULL;  
   ExecStatusType DBQueryStatus; 
   
   char sql[MAXSQL];
   char s_exp_number[MAXSTR];
   
   char *table, *field, *value ; 
   
// Keyword Structure

   typedef struct{
      	IDL_KW_RESULT_FIRST_FIELD;	
	IDL_LONG new;
	IDL_LONG verbose;
	IDL_LONG debug;
	IDL_LONG help;
	IDL_STRING where;
	IDL_INT  iswhere;
   } KW_RESULT;
   
   
// Maintain Alphabetical Order of Keywords
 
   static IDL_KW_PAR kw_pars[] = 
   { IDL_KW_FAST_SCAN,
     {"DEBUG",   IDL_TYP_LONG,    1,IDL_KW_ZERO,0, 			 IDL_KW_OFFSETOF(debug)},    
     {"HELP",    IDL_TYP_LONG,    1,IDL_KW_ZERO,0, 			 IDL_KW_OFFSETOF(help)},
     {"NEW",     IDL_TYP_LONG,    1,IDL_KW_ZERO,0, 			 IDL_KW_OFFSETOF(new)},
     {"VERBOSE", IDL_TYP_LONG,    1,IDL_KW_ZERO,0, 			 IDL_KW_OFFSETOF(verbose)},
     {"WHERE",   IDL_TYP_STRING,  1,IDL_KW_ZERO,IDL_KW_OFFSETOF(iswhere),IDL_KW_OFFSETOF(where)},     			
   {NULL}};    
 
   KW_RESULT kw;
   
   kw.new       = 0;
   kw.verbose   = 0;
   kw.debug     = 0;
   kw.help      = 0; 
   kw.iswhere   = 0;
   
//-----------------------------------------------------------------------
// Get Passed Keywords and Parameters

   IDL_KWProcessByOffset(argc,argv,argk,kw_pars,(IDL_VPTR *)0,1,&kw);   
    
   if(kw.debug){    
      if(kw.debug)  fprintf(stdout,"Debug Keyword Passed\n");
      if(kw.verbose)fprintf(stdout,"Verbose Keyword Passed\n");   
      if(kw.help)   fprintf(stdout,"Help Keyword Passed\n");
      if(kw.iswhere)fprintf(stdout,"Where Clause Passed: Where = %s\n", kw.where);
   }   
   
//--------------------------------------------------------------------------      
// Call for HELP?

   if(kw.help){
      fprintf(stdout,"putCPF: Help Not Implemented ... yet!\n");
      IDL_KW_FREE;
      return(IDL_GettmpLong(0));
   }    
   
//-----------------------------------------------------------------------
// Check a String was Passed
   
   IDL_ENSURE_SCALAR(argv[0]);		// Pulse Number
   exp_number = IDL_LongScalar(argv[0]);
   sprintf(s_exp_number,"%d",exp_number);
   
   IDL_ENSURE_STRING(argv[1]);		 
   //IDL_ENSURE_SCALAR(argv[1]);
   table = (char *)IDL_STRING_STR(&(argv[1]->value.str));       // Table Name
        
   IDL_ENSURE_STRING(argv[2]);		// Field Name
   //IDL_ENSURE_SCALAR(argv[2]);
   field = (char *)IDL_STRING_STR(&(argv[2]->value.str));	// Field Name
   
   IDL_ENSURE_STRING(argv[3]);		// Value as a String
   //IDL_ENSURE_SCALAR(argv[3]);
   value = (char *)IDL_STRING_STR(&(argv[3]->value.str));	// Value     

//--------------------------------------------------------------------------      
// Connect to the Database Server  

   if (!(DBConnect = StartDbSQL(DBName, kw.verbose))){
      if(kw.verbose) fprintf(stdout,"putCPF: SQL Server Connect Error \n");
      IDL_KW_FREE;
      return(IDL_GettmpLong(CPF_SERVER_CONNECT_ERROR)); 
   }   
   
//-----------------------------------------------------------------------
// Create the SQL Query 

   if(kw.new){
    
      if(kw.debug){
         fprintf(stdout,"NEW Keyword Passed\n");
         fprintf(stdout,"Pulse      :%d\n", exp_number);
         fprintf(stdout,"Table      :%s\n", table);
	 fprintf(stdout,"Field      :%s\n", field);
	 fprintf(stdout,"Value      :%s\n", value);
         if(kw.iswhere) fprintf(stdout,"Where        :%s\n", kw.where); 
      }

//-------------------------------------------------------------  
// Security Check

      if((rc = checkSecurity(DBConnect, table, 'N', kw.verbose)) != 1){
         if(kw.verbose){
            fprintf(stdout,"putCPF: You are Not Permitted to Create New CPF Database Records.\n");
            fprintf(stdout,"putCPF: For permission please contact the CPF Database Administrator.\n");
         }	 
	 IDL_KW_FREE;
         return(IDL_GettmpLong(CPF_SECURITY_NOT_AUTHORISED));
      }	 
   
//-------------------------------------------------------------  
// Check a Record Does Not Already Exist 

      strcpy(sql, "SELECT exp_number from ");
      strcat(sql, table);
      strcat(sql, " where exp_number = ");       
      strcat(sql, s_exp_number);

      if ((DBQuery = PQexec(DBConnect, sql)) == NULL){
         if(kw.verbose){ 
            fprintf(stdout,"putCPF: ERROR - Failure to Check for an Existing Record: %s\n", sql);
            fprintf(stdout,"putCPF: ERROR - %s\n", PQresultErrorMessage(DBQuery));
         }	  
         IDL_KW_FREE;
         PQclear(DBQuery);
         return(IDL_GettmpLong(CPF_QUERY_FAILED));
      } else {
         DBQueryStatus = PQresultStatus(DBQuery);       
         if (DBQueryStatus != PGRES_TUPLES_OK){
            if(kw.verbose){  
               fprintf(stdout,"putCPF: ERROR - Problem Testing for Existing Record: %s\n", sql);
               fprintf(stdout,"putCPF: ERROR - %s\n", PQresultErrorMessage(DBQuery)); 
            }
            IDL_KW_FREE;
            PQclear(DBQuery);
            return(IDL_GettmpLong(CPF_QUERY_INCORRECT));   
         } else {  	   
            nrows = PQntuples(DBQuery);
         }
      }

      if (nrows >= 1){
         if(kw.verbose) fprintf(stdout,"putCPF: Existing CPF.%s record for pulse %s Exists!\n", table,s_exp_number);
         IDL_KW_FREE;
         PQclear(DBQuery);
         return(IDL_GettmpLong(CPF_QUERY_RECORD_NOT_NEW));	 
      } 
   
// INSERT a New Record
       
      strcpy(sql, "INSERT INTO ");
      strcat(sql, table);
      strcat(sql, " (exp_number,");
      strcat(sql, field);
      strcat(sql, ") VALUES (");
      strcat(sql, s_exp_number);
      strcat(sql, ",");
      strcat(sql, value);
      strcat(sql, ");"); 
      
      if(kw.debug) fprintf(stdout, "SQL: %s\n", sql);	  

      if ((DBQuery = PQexec(DBConnect, sql)) == NULL){
	 if(kw.verbose){ 
	    fprintf(stdout,"putCPF: ERROR - Failure to Execute SQL\n");
	    fprintf(stdout,"putCPF: ERROR - %s\n", PQresultErrorMessage(DBQuery)); 
	 }
         IDL_KW_FREE;
         PQclear(DBQuery);
         return(IDL_GettmpLong(CPF_QUERY_FAILED));
      } else {	 
	 DBQueryStatus = PQresultStatus(DBQuery); 
    
	 if (DBQueryStatus != PGRES_COMMAND_OK){
	    if(kw.verbose){          
	       fprintf(stdout,"putCPF: ERROR - Record Incorrecly INSERTEd\n");
	       fprintf(stdout,"putCPF: ERROR - %s\n", PQresultErrorMessage(DBQuery));
	    }
            IDL_KW_FREE;
            PQclear(DBQuery);
            return(IDL_GettmpLong(CPF_INSERT_FAILED));
         }
      }	   
      PQclear(DBQuery);
      
   } else {
   
//-------------------------------------------------------------   
// Update an Existing Record
       
      if(kw.debug){
         fprintf(stdout,"Update Record\n");
         fprintf(stdout,"Pulse      :%d\n", exp_number);
         fprintf(stdout,"Table      :%s\n", table);
	 fprintf(stdout,"Field      :%s\n", field);
	 fprintf(stdout,"Value      :%s\n", value);
         if(kw.iswhere) fprintf(stdout,"Where        :%s\n", kw.where); 
      }

//-------------------------------------------------------------  
// Security Check

      if((rc = checkSecurity(DBConnect, table, 'U', kw.verbose)) != 1){
         if(kw.verbose){
            fprintf(stdout,"putCPF: You are Not Permitted to Update CPF Database Records.\n");
            fprintf(stdout,"putCPF: For permission please contact the CPF Database Administrator.\n");
         }	 
	 IDL_KW_FREE;
         return(IDL_GettmpLong(CPF_SECURITY_NOT_AUTHORISED));
      }	 
   
//-------------------------------------------------------------  
// Create and Execute Update SQL 

      strcpy(sql, "UPDATE ");
      strcat(sql, table);  
      strcat(sql, " SET ");      
      strcat(sql, field);
      strcat(sql, " = ");
      
      //if (is_string){
      //   strcat(sql,"'");
       //  strcat(sql, value); 
       //  strcat(sql,"' WHERE exp_number = ");     
      //} else {
         strcat(sql, value);    
         strcat(sql, " WHERE exp_number = ");
      //}   
      strcat(sql, s_exp_number);
   
      if(kw.debug) fprintf(stdout,"SQL: %s\n", sql);
   
      if ((DBQuery = PQexec(DBConnect, sql)) == NULL){
         if(kw.verbose){ 
            fprintf(stdout,"putCPF: ERROR - Failure to Update CPF Table: %s\n", sql);
            fprintf(stdout,"putCPF: ERROR - %s\n", PQresultErrorMessage(DBQuery)); 
         }
         IDL_KW_FREE;
         PQclear(DBQuery);
         return(IDL_GettmpLong(CPF_QUERY_FAILED));
      }  
   
      DBQueryStatus = PQresultStatus(DBQuery); 
    
      if (DBQueryStatus != PGRES_COMMAND_OK){ 
         if(kw.verbose){ 
            fprintf(stdout,"putCPF: ERROR - Table Update Incorrectly Processed %s\n", sql);
            fprintf(stdout,"putCPF: ERROR - %s\n", PQresultErrorMessage(DBQuery)); 
         }
         IDL_KW_FREE;
         PQclear(DBQuery);
         return(IDL_GettmpLong(CPF_UPDATE_FAILED));
      }
      	      
      PQclear(DBQuery);
   }     
      
        
   return(IDL_GettmpLong(0));
}

   	 
//------------------------------------------------------------------------------------------------------------- 
IDL_VPTR IDL_CDECL killCPF(int argc, IDL_VPTR argv[], char *argk){
//
// IDL DLM Function to Delete a set of CPF Records
//
// 3 Arguments:
//
//	argv[0] - the CPF Table Name
//	argv[1] - the where clause criteria 
//
// Returns: Status Code - 
//
// 1 Keywords:
//
//	debug   - Print Debug Messages
//	help    - Print a useful Help Message
//	verbose - Print Error Messages 
//
// v0.01	31Jan2006	D.G.Muir	Original Release
//-------------------------------------------------------------------------	      
   int i, j, rc, nrows, ncols;
   
   char     *DBName    = DATABASE;   
   PGresult *DBQuery   = NULL;  
   ExecStatusType DBQueryStatus; 
   
   char *table, *where ; 
   
   char sql[MAXSQL] = "DELETE from ";
      
// Keyword Structure

   typedef struct{
      	IDL_KW_RESULT_FIRST_FIELD;	
	IDL_LONG verbose;
	IDL_LONG debug;
	IDL_LONG help;
   } KW_RESULT;
   
   
// Maintain Alphabetical Order of Keywords
 
   static IDL_KW_PAR kw_pars[] = 
   { IDL_KW_FAST_SCAN,
     {"DEBUG",   IDL_TYP_LONG,  1,IDL_KW_ZERO,0, 			IDL_KW_OFFSETOF(debug)},    
     {"HELP",    IDL_TYP_LONG,  1,IDL_KW_ZERO,0, 			IDL_KW_OFFSETOF(help)},
     {"VERBOSE", IDL_TYP_LONG,  1,IDL_KW_ZERO,0, 			IDL_KW_OFFSETOF(verbose)},    			
   {NULL}};    
 
   KW_RESULT kw;
   
   kw.verbose   = 0;
   kw.debug     = 0;
   kw.help      = 0; 
   
//-----------------------------------------------------------------------
// Get Passed Keywords and Parameters

   IDL_KWProcessByOffset(argc,argv,argk,kw_pars,(IDL_VPTR *)0,1,&kw);   
    
   if(kw.debug){    
      if(kw.debug)  fprintf(stdout,"Debug Keyword Passed\n");
      if(kw.verbose)fprintf(stdout,"Verbose Keyword Passed\n");   
      if(kw.help)   fprintf(stdout,"Help Keyword Passed\n");
   }   
   
//--------------------------------------------------------------------------      
// Call for HELP?

   if(kw.help){
      fprintf(stdout,"killCPF: Help Not Implemented ... yet!\n");
      IDL_KW_FREE;
      return(IDL_GettmpLong(0));
   }    
   
//-----------------------------------------------------------------------
// Check Strings was Passed
   
   IDL_ENSURE_STRING(argv[0]);
   //IDL_ENSURE_SCALAR(argv[0]);
   
   IDL_ENSURE_STRING(argv[1]);
   //IDL_ENSURE_SCALAR(argv[1]);
   
   table = (char *)IDL_STRING_STR(&(argv[0]->value.str));	// Table Name
   where = (char *)IDL_STRING_STR(&(argv[1]->value.str));	// Where Clause

//--------------------------------------------------------------------------      
// Connect to the Database Server

   if (!(DBConnect = StartDbSQL(DBName, kw.verbose))){
      if(kw.verbose) fprintf(stdout,"killCPF: SQL Server Connect Error \n");
      IDL_KW_FREE;
      return(IDL_GettmpLong(CPF_SERVER_CONNECT_ERROR)); 
   }    
   
//-------------------------------------------------------------  
// Security Check

   if((rc = checkSecurity(DBConnect, table, 'U', kw.verbose)) != 1){
      if(kw.verbose){
         fprintf(stdout,"killCPF: You are Not Permitted to Delete CPF Database Records.\n");
         fprintf(stdout,"killCPF: For permission please contact the Database Administrator.\n");
      }	 
      IDL_KW_FREE;
      return(IDL_GettmpLong(CPF_SECURITY_NOT_AUTHORISED));
   }	    
      
//-----------------------------------------------------------------------
// Create the SQL Query
   
   strcat(sql, table);
   
   if(strlen(where) > 0){
      strcat(sql, " where ");     
      strcat(sql, where);
   }  
  
   if(kw.debug) fprintf(stdout,"SQL: %s\n", sql);
 
   if ((DBQuery = PQexec(DBConnect, sql)) == NULL){
      if(kw.verbose){
         fprintf(stdout,"killCPF: ERROR - Failure to Execute SQL: %s\n", sql);
         fprintf(stdout,"killCPF: ERROR - %s\n", PQresultErrorMessage(DBQuery));
      }
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_QUERY_FAILED));  
   } else {	  
      DBQueryStatus = PQresultStatus(DBQuery); 
      
      if (DBQueryStatus != PGRES_COMMAND_OK){
         if(kw.verbose){      
            fprintf(stdout,"killCPF: ERROR - Problem Deleting Specified Records [%s, %s]\n", table, where);
            fprintf(stdout,"killCPF: ERROR - %s\n", PQresultErrorMessage(DBQuery));
	 }
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_QUERY_INCORRECT));
      }    
   }   
   
//--------------------------------------------------------------------------      
// Cleanup Keywords & PG Heap 
 
   IDL_KW_FREE;
   PQclear(DBQuery);    
       
   return(IDL_GettmpLong(0));           
}


int checkSecurity(PGconn *DBConnect, char *table, char permission, int verbose){
   int rc = 0, nrows, ncols;
   char sql[MAXSQL];
   static char uid[MAXSTR] = "xxx";
   char dbperm[2]   = "r";
   
   static int start_uid = 0;
   
   PGresult * DBQuery = NULL;  
   ExecStatusType DBQueryStatus;

   if(permission != 'N' && permission != 'U' && 
      permission != 'n' && permission != 'u') return 0;	// Read Only
 
//-------------------------------------------------------------  
// Get User's ID (Done Once per IDL Session!)

   if (start_uid == 0) {
      userid(uid);
      start_uid = 1;
   }    

//-------------------------------------------------------------  
// Build SQL

   strcpy(sql, "SELECT permission FROM security WHERE tablename ='");
   strcat(sql, table);
   strcat(sql, "' AND username = '");      
   strcat(sql, uid);
   strcat(sql, "';");
 
   //fprintf(stdout,"SQL: %s\n", sql);
   
   if ((DBQuery = PQexec(DBConnect, sql)) == NULL){
      if(verbose){ 
         fprintf(stdout,"checkSecurity: ERROR - Failure to Check Security Permissions: %s\n", sql);
         fprintf(stdout,"checkSecurity: ERROR - %s\n", PQresultErrorMessage(DBQuery));
	 fprintf(stdout,"checkSecurity: Granting Read Access Only\n"); 
      }
      rc = 0;
   } else {
   
      DBQueryStatus = PQresultStatus(DBQuery); 
  //fprintf(stdout,"Status: %d\n", (int) DBQueryStatus);    
      if (DBQueryStatus != PGRES_TUPLES_OK){
         if(verbose){ 
            fprintf(stdout,"checkSecurity: ERROR - Failure Executing Security Check %s\n", sql);
            fprintf(stdout,"checkSecurity: ERROR - %s\n", PQresultErrorMessage(DBQuery));
	    fprintf(stdout,"checkSecurity: Granting Read Access Only\n"); 
         }
	 rc = 0;
      } else {      	
         if((nrows = PQntuples(DBQuery)) != 1){ 		// Number of Rows
	    if(nrows == 0){
	       if(verbose) {
	          fprintf(stdout,"checkSecurity: ERROR - No Record Found for user %s\n", uid);
		  fprintf(stdout,"checkSecurity: Granting Read Access Only\n");
	       }	  
	       rc = 0;
	    } else { 
	       if(verbose) {
	          fprintf(stdout,"checkSecurity: ERROR - Too Many Record Found for user %s\n", uid);
		  fprintf(stdout,"checkSecurity: Granting Read Access Only\n"); 
	       }	  
	       rc = 0;
	    }       
	 } else {
            ncols = PQnfields(DBQuery);		// Number of Columns	    
	    if(ncols != 1){
	       if(verbose) {
	          fprintf(stdout,"checkSecurity: ERROR - Too Few Record Column Found (%d) "
	                         "for user %s\n", ncols, uid);
	          fprintf(stdout,"checkSecurity: Granting Read Access Only\n");
	       } 	  			 
	       rc = 0;		 
	    } else {
	    
// Check Permissions against requested action:
// U ==> Update including New Records
// N ==> New Records Only: No Update Allowed	    
// Default is Read Only

	       rc = 0;
	       strcpy(dbperm, PQgetvalue(DBQuery,0,0));
	       dbperm[0] = tolower(dbperm[0]);
	       
	       //if(DEBUG) fprintf(stdout,"checkSecurity: Query Result = [%s]\n", dbperm);
	       
	       if (STR_EQUALS(dbperm, "u"))	// Can Do Anything!
	          rc = 1;
	       else 				// Create New Records Only
	          if (STR_EQUALS(dbperm, "n") && (permission=='n' || permission== 'N')) rc = 1;
	
		  
	       //if(DEBUG)fprintf(stdout,"Allowed:[%s] Requested:[%c] Granted:[%d]\n",dbperm,permission,rc) ;
	    }                   
         }     	 
      }	
   }     
   
   PQclear(DBQuery); 
	
   return rc;
}


void userid(char *uid){
   int i,l;
   FILE *ph = NULL;
   uid[0] = '\0';

//------------------------------------------------------------------------------------
// Execute the Command and Open a Pipe to the Output for Reading 

   if((ph = popen("whoami", "r")) == NULL) {
      fprintf(stdout,"userid: Problem Opening Command Pipe\n");
      return;
   } else 
      if(!feof(ph)) fgets(uid,MAXSTR-1,ph);     	 	 	 	 

   fclose(ph);
   
   l = strlen(uid);
   for (i=0; i<l; i++){
      if(!isprint(uid[i])) uid[i] = ' ';
   }      
   TrimString(uid);
   
   //fprintf(stdout,"userid: [%s]\n", uid);

   return;     
}


//=========================================================================================

void freeGrpMem1(UCHAR *memPtr){
   CSOUT1 *s = (CSOUT1 *) memPtr;
   int i;
   IDL_MEMINT ndesc = 1; 
   for(i=0;i<nGroupRows;i++) IDL_StrDelete((IDL_STRING *) &(s[i].group), ndesc); 
   free((void *)memPtr);
} 
void freeGrpMem2(UCHAR *memPtr){
   CSOUT2 *s = (CSOUT2 *) memPtr;
   int i;
   IDL_MEMINT ndesc = 1; 
   for(i=0;i<nGroupRows;i++){
      IDL_StrDelete((IDL_STRING *) &(s[i].group[0]), ndesc); 
      IDL_StrDelete((IDL_STRING *) &(s[i].group[1]), ndesc);
   }   
   free((void *)memPtr);
} 
void freeGrpMem3(UCHAR *memPtr){
   CSOUT3 *s = (CSOUT3 *) memPtr;
   int i;
   IDL_MEMINT ndesc = 1; 
   for(i=0;i<nGroupRows;i++){
      IDL_StrDelete((IDL_STRING *) &(s[i].group[0]), ndesc); 
      IDL_StrDelete((IDL_STRING *) &(s[i].group[1]), ndesc);
      IDL_StrDelete((IDL_STRING *) &(s[i].group[3]), ndesc);
   }   
   free((void *)memPtr);
} 

IDL_VPTR IDL_CDECL countCPF(int argc, IDL_VPTR argv[], char *argk){
//
// IDL DLM Function to Group Count CPF Database entries
//
// 3 Arguments:
//
//	argv[0] - Array of Column Names for the Group (Max is 3)
//	argv[1] - the CPF Table Name
//	argv[2] - the where clause criteria 
//
// 1 Keywords:
//
//	debug   - Print Debug Messages
//	help    - Print a useful Help Message
//	verbose - Print Error Messages
//
// v0.01	November 2005	D.G.Muir	Original Release
//-------------------------------------------------------------------------	      

   int i, j, rc, err, nrows, ncols, ngrps, exp_number;
   int iStringLength;
   
   char     *DBName    = DATABASE;
   PGresult *DBQuery   = NULL;  
   ExecStatusType DBQueryStatus; 
      
   IDL_STRING *pIDLstrDesc;
   IDL_STRING *pisArray;
   char *colname, *table, *where, *value; 
   
   char sql[MAXSQL] = "select ";
   char slimit[MAXSTR];
   char msg[MAXSTR];
   	    
   CSOUT1 *sout1;			// Pointer to the Returned IDL/C Structure Array;
   CSOUT1 *s1;
   CSOUT2 *sout2;			// Pointer to the Returned IDL/C Structure Array;
   CSOUT2 *s2;
   CSOUT3 *sout3;			// Pointer to the Returned IDL/C Structure Array;
   CSOUT3 *s3;
      
   IDL_VPTR ivReturn = NULL; IDL_GettmpLong(0);
   IDL_VPTR p = NULL;
   
   void *psDef       = NULL;
   static IDL_LONG ilDims[IDL_MAX_ARRAY_DIM];
      
   static IDL_LONG vdlen[]    = {1,1};		// Scalar Value Array
     
   IDL_STRUCT_TAG_DEF pTags[] = {
        {"COUNT",	0,	(void *) IDL_TYP_LONG},
	{"GROUP",	vdlen,	(void *) IDL_TYP_STRING},				
	{0}
   };
   
// Keyword Structure

   typedef struct{
      	IDL_KW_RESULT_FIRST_FIELD;	
	IDL_LONG limit;
	IDL_LONG islimit;
	IDL_LONG verbose;
	IDL_LONG debug;
	IDL_LONG help;
   } KW_RESULT;
   
   
// Maintain Alphabetical Order of Keywords
 
   static IDL_KW_PAR kw_pars[] = 
   { IDL_KW_FAST_SCAN,
     {"DEBUG",   IDL_TYP_LONG,  1,IDL_KW_ZERO,0, 			IDL_KW_OFFSETOF(debug)},    
     {"HELP",    IDL_TYP_LONG,  1,IDL_KW_ZERO,0, 			IDL_KW_OFFSETOF(help)},
     {"LIMIT",   IDL_TYP_LONG,  1,IDL_KW_ZERO,IDL_KW_OFFSETOF(islimit), IDL_KW_OFFSETOF(limit)},
     {"VERBOSE", IDL_TYP_LONG,  1,IDL_KW_ZERO,0, 			IDL_KW_OFFSETOF(verbose)},    			
   {NULL}};    
 
   KW_RESULT kw;
   
   kw.limit     = 0;
   kw.islimit   = 0;
   kw.verbose   = 0;
   kw.debug     = 0;
   kw.help      = 0; 
   
//-----------------------------------------------------------------------
// Get Passed Keywords and Parameters

   IDL_KWProcessByOffset(argc,argv,argk,kw_pars,(IDL_VPTR *)0,1,&kw);   
       
//--------------------------------------------------------------------------      
// Call for HELP?

   if(kw.help){
      IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO,"Help Not Implemented ... yet!");
      IDL_KW_FREE;
      return(IDL_GettmpLong(0));
   }    
   
//-----------------------------------------------------------------------
// Check a String was Passed
   
   IDL_ENSURE_ARRAY(argv[0]);
   IDL_ENSURE_STRING(argv[0]);
   
   IDL_ENSURE_STRING(argv[1]);
   
   IDL_ENSURE_STRING(argv[2]);
   
//-----------------------------------------------------------------------
// Create the SQL Query
   
   ngrps = ncols = argv[0]->value.arr->n_elts;			// Number of Required Column (group) Names
   
   if(ncols > 3){
      IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO,"WARNING - Only First 3 Group Values will be Returned");  
      ngrps = 3;
   }   
   
   pIDLstrDesc = (IDL_STRING *) argv[0]->value.arr->data; 
   
   for(i=0;i<ncols;i++) {
      strcat(sql, (char *)(*pIDLstrDesc++).s) ;			// Names
      strcat(sql, ",");
   }
   strcat(sql, " count(*) as count from ");
   
   table = (char *)IDL_STRING_STR(&(argv[1]->value.str));	// Table Name
   strcat(sql, table);	 
   
   where = (char *)IDL_STRING_STR(&(argv[2]->value.str));	// Where Clause
   if(strlen(where) > 0){
      strcat(sql, " where ");     
      strcat(sql, where);
   }   
   
   if(kw.islimit){
      sprintf(slimit," limit %d;", kw.limit);
      strcat(sql, slimit);
   }
   
   pIDLstrDesc = (IDL_STRING *) argv[0]->value.arr->data;
   strcat(sql, " group by ");
   for(i=0;i<ncols;i++) {
      strcat(sql, (char *)(*pIDLstrDesc++).s) ;			// Names
      if(i < (ncols-1))strcat(sql, ",");
   }
   
   if(kw.debug){
      sprintf(msg,"Number of Column Names: %d", ncols);
      IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, msg);
      
      pIDLstrDesc = (IDL_STRING *) argv[0]->value.arr->data;
      for(i=0;i<ncols;i++){
         sprintf(msg,"Column Names: %d  %s", i,(char *)(*pIDLstrDesc++).s);
         IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, msg);	 
      }
      IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO,sql);
   } 
   
//--------------------------------------------------------------------------      
// Connect to the Database Server

   if (!(DBConnect = StartDbSQL(DBName, kw.verbose))){
      if(kw.verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "SQL Server Connect Error");
      IDL_KW_FREE;
      return(IDL_GettmpLong(CPF_SERVER_CONNECT_ERROR)); 
   }         

//-------------------------------------------------------------  
// Execute the SQL

   if ((DBQuery = PQexec(DBConnect, sql)) == NULL){
      if(kw.verbose){
         sprintf(msg,"ERROR - Failure to Execute the Query: %s", PQresultErrorMessage(DBQuery));
         IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, msg);
      }
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_QUERY_FAILED));     
   } 
   
   DBQueryStatus = PQresultStatus(DBQuery);        
    
   if (DBQueryStatus != PGRES_TUPLES_OK){
      if(kw.verbose){ 
         sprintf(msg,"ERROR - Query Incorrectly Processed: %s", PQresultErrorMessage(DBQuery));
         IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, msg); 
      }
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_QUERY_INCORRECT));
   }
   
//-------------------------------------------------------------  
// Extract the Resultset 
    
   nrows = PQntuples(DBQuery); 		// Number of Rows
   ncols = PQnfields(DBQuery);		// Number of Columns
	 
   if(nrows == 0){
      if(kw.verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - No Record Satisfies the Query");
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_NO_ROWS));
   }  
    	    
   if(ncols == 0){
      if(kw.verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - No Columns Created by the Query");  
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_NO_COLUMNS));
   }  

// Allocate the Appropriate Return Structure Array
 
   err = 0;

   switch(ngrps){
      
      case 1: sout1 = (CSOUT1 *) malloc(nrows*sizeof(CSOUT1));  //IDL_GetScratch(&p,(IDL_MEMINT) nrows,(IDL_MEMINT)sizeof(CSOUT1)); 
              if(sout1 == NULL) err = 1;              
              break;
      
      case 2: sout2 = (CSOUT2 *) malloc(nrows*sizeof(CSOUT2));  //IDL_GetScratch(&p,(IDL_MEMINT) nrows,(IDL_MEMINT)sizeof(CSOUT2)); 
              if(sout2 == NULL) err = 1;              
              break;
      
      case 3: sout3 = (CSOUT3 *) malloc(nrows*sizeof(CSOUT3));  //IDL_GetScratch(&p,(IDL_MEMINT) nrows,(IDL_MEMINT)sizeof(CSOUT3));
              if(sout3 == NULL) err = 1;              
              break;
   }

   if(err){
      if(kw.verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - Unable to Allocate Heap!");
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_HEAP_ALLOC_ERROR));
   }  
    
   for(i=0;i<nrows;i++){
      
      switch(ngrps){
         case 1: sout1[i].count = (int)atoi(PQgetvalue(DBQuery,i,ncols-1));
	         s1 = sout1+i;
		 IDL_StrStore(&(s1->group),"");           
                 break;
		 
	 case 2: sout2[i].count = (int)atoi(PQgetvalue(DBQuery,i,ncols-1));
	         s2 = sout2+i;
		 IDL_StrStore(&(s2->group[0]),"");
                 IDL_StrStore(&(s2->group[1]),"");           
                 break;	
		 	 
	 case 3: sout3[i].count = (int)atoi(PQgetvalue(DBQuery,i,ncols-1));
	         s3 = sout3+i;
		 IDL_StrStore(&(s3->group[0]),"");
                 IDL_StrStore(&(s3->group[1]),"");
                 IDL_StrStore(&(s3->group[2]),"");           
                 break;		 
      }
      	              
      //IDL_StrEnsureLength(&(s->group1),0);		// Causes a Segmentation Error 
          
         
      for(j=0;j<ncols;j++) {

         value = PQgetvalue(DBQuery,i,j);
	 
	 if(kw.debug){
	    sprintf(msg,"[%d, %d]  [%s]\n", i, j, value);
            IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, msg);  
	 }   
   	 
	 if(j < 3 && j != (ncols-1)){
	    if(j == 0){
               if((iStringLength = strlen(value)) > 0 ){				
                  switch(ngrps){
		     case 1: IDL_StrStore(&(s1->group),value);          
                             break;
		     case 2: IDL_StrStore(&(s2->group[0]),value);          
                             break;
		     case 3: IDL_StrStore(&(s3->group[0]),value);          
                             break;	 
                  }
               } 	  
            } else {
	       if(j == 1){
                  if((iStringLength = strlen(value)) > 0 ){				
	             switch(ngrps){
		        case 2: IDL_StrStore(&(s2->group[1]),value);          
                                break;
		        case 3: IDL_StrStore(&(s3->group[1]),value);          
                                break;	 
                     }
                  }   
               } else {
	          if(j == 2){
                     if((iStringLength = strlen(value)) > 0 ){				
	                IDL_StrStore(&(s3->group[2]),value);
                     }  
                  }
	       }
	    }    	 
	 }
 	  	 	 	 	     		 
      }                  	 
   } 
   	            
//--------------------------------------------------------------------------      
// Create the IDL Structure 
   
   nGroupRows = nrows;		// retain for Heap Memory Management
   ilDims[0]  = nrows;		// Number of Structure Array Elements
   vdlen[1]   = ngrps;		// Number of Groups
   
   psDef = IDL_MakeStruct(NULL, pTags); 
    
   switch(ngrps){
      case 1: ivReturn = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR *)sout1, freeMem, psDef);          
              break;
      case 2: ivReturn = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR *)sout2, freeMem, psDef);          
              break;
      case 3: ivReturn = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR *)sout3, freeMem, psDef);          
              break;      	            
   }       
               	     
//--------------------------------------------------------------------------      
// Cleanup Keywords & PG Heap 

   IDL_KW_FREE;
   PQclear(DBQuery);          
   return(ivReturn);
}




