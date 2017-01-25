// Routines to return Ordered Group Counts from the MAST CPF Database
//
// Usage:
//
//	result = countCPF(names, table, where_clause [,/verbose][,/help][,/debug])
//
// where the keywords VERBOSE means print any error messages.
//
// The returned Array of Structures contains resultset column name-value pairs for 
// each database table record (rows) that satisfies the query. 
//
//	result.sql    		= Query against the CPF Database
//	result.rows		= Number of Table Rows
//	result.cols		= Number of Table Columns
//	result.names[cols]  	= names of each resultset column
//	result.values[rows,cols]= Group Values for each resultset Row
//	result.counts[rows]     = Group Count Integer Values for each resultset Row
//		 
//
// Note: The PostgreSQL Library libpg.so must be available to IDL. A soft 
// link to the standard library is sufficient.
//------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <string.h>

#include "export.h"		// IDL API Header
#include "mastCPF.h"   

// Function Prototypes

extern void countCPF_exit_handler(void);
extern int  countCPF_Startup(void);

extern IDL_VPTR IDL_CDECL countCPF(int argc, IDL_VPTR argv[], char *argk);
 
static IDL_SYSFUN_DEF2 countCPF_functions[] = {
   {(IDL_FUN_RET) countCPF,      "COUNTCPF", 	        3, 3, IDL_SYSFUN_DEF_F_KEYWORDS, 0}
};

int countCPF_startup(void){ 
 
    if (!IDL_SysRtnAdd(countCPF_functions, TRUE,
        ARRLEN(countCPF_functions))) {return IDL_FALSE;}

// Register the exit handler 

    IDL_ExitRegister(countCPF_exit_handler);
    
    return(IDL_TRUE);
}

int IDL_Load(void)
{  
    if (!IDL_SysRtnAdd(countCPF_functions, TRUE,
        ARRLEN(countCPF_functions)))  {return IDL_FALSE;}
    return(IDL_TRUE);
}

// Called when IDL is shutdown
 
void countCPF_exit_handler(void){
   
// Close the CPF SQL Connection 

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

   PGresult *DBQuery = NULL;
   
   ExecStatusType DBQueryStatus; 

//-------------------------------------------------------------
// Test if Socket already Open

   if(DBConnect != NULL) return(DBConnect);	 

//-------------------------------------------------------------
// Identify the SQL Host and Database
   
// CPF SQL Server Host Name

   if((env = getenv("IDAM_SQLHOST")) !=NULL) strcpy(pghost, env);
   if((env = getenv("CPF_SQLHOST")) !=NULL) strcpy(pghost, env);     

// CPF SQL Server Port name 

   if((env = getenv("IDAM_SQLPORT")) !=NULL) strcpy(pgport, env); 
   if((env = getenv("CPF_SQLPORT")) !=NULL) strcpy(pgport, env); 
      
// IDAM SQL Database name 

   if((env = getenv("IDAM_SQLDBNAME")) !=NULL) strcpy(dbname, env); 
   if((env = getenv("CPF_SQLDBNAME")) !=NULL) strcpy(dbname, env); 
   if(dbName != NULL) strcpy(dbname, dbName);        

// IDAM SQL Access username 

   if((env = getenv("IDAM_SQLUSER")) !=NULL) strcpy(user, env);
   if((env = getenv("CPF_SQLUSER")) !=NULL) strcpy(user, env); 
   
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
 

// Main Dimensional Data Block Structure Passed Back to IDL
/*
typedef struct {
  int        count; 
  IDL_STRING group1;	// Comma delimited Group Values!
  IDL_STRING group2;
  IDL_STRING group3;
} CSOUT;
*/

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

void freeMem(UCHAR *memPtr){
   free((void *)memPtr);
} 

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

int nGroupRows = 0; 

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



/*
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

   int i, j, rc, nrows, ncols, exp_number;
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
   	    
   CSOUT *sout;			// Pointer to the Returned IDL/C Structure Array;
   CSOUT *s;
      
   IDL_VPTR ivReturn = IDL_GettmpLong(0);
   
   void *psDef       = NULL;
   static IDL_LONG ilDims[IDL_MAX_ARRAY_DIM];
     
   IDL_STRUCT_TAG_DEF pTags[] = {
        {"COUNT",	0,	(void *) IDL_TYP_LONG},
	{"GROUP1",	0,	(void *) IDL_TYP_STRING},
	{"GROUP2",	0,	(void *) IDL_TYP_STRING},
	{"GROUP3",	0,	(void *) IDL_TYP_STRING},					
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
   
   ncols = argv[0]->value.arr->n_elts;				// Number of Required Column Names
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

   if((sout = (CSOUT *)malloc(nrows*sizeof(CSOUT))) == NULL){
      if(kw.verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - Unable to Allocate Heap!");
      IDL_KW_FREE;
      PQclear(DBQuery);
      return(IDL_GettmpLong(CPF_HEAP_ALLOC_ERROR));
   }  
   
   for(i=0;i<nrows;i++){
      
      sout[i].count = (int)atoi(PQgetvalue(DBQuery,i,ncols-1));
      
      s = sout+i; 
        
      //IDL_StrEnsureLength(&(s->group1),0);		// Causes a Segmentation Error 
           
      IDL_StrStore(&(s->group1),"");
      IDL_StrStore(&(s->group2),"");
      IDL_StrStore(&(s->group3),"");
         
      for(j=0;j<ncols;j++) {

         value = PQgetvalue(DBQuery,i,j);
	 
	 if(kw.debug){
	    sprintf(msg,"[%d, %d]  [%s]\n", i, j, value);
            IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, msg);  
	 }   
   	 
	 if(j < 3 && j != (ncols-1)){
	    if(j == 0){
               if((iStringLength = strlen(value)) > 0 ){				
                  //IDL_StrEnsureLength(&(s->group1),iStringLength);
	          IDL_StrStore(&(s->group1),value);
               } 	  
            } else {
	       if(j == 1){
                  if((iStringLength = strlen(value)) > 0 ){				
	             IDL_StrStore(&(s->group2),value);
                  }   
               } else {
	          if(j == 2){
                     if((iStringLength = strlen(value)) > 0 ){				
	                IDL_StrStore(&(s->group3),value);
                     }  
                  }
	       }
	    }    	 
	 }
 	  	 	 	 	     		 
      }                  	 
   } 
   	            
//--------------------------------------------------------------------------      
// Create the IDL Structure 
   
   ilDims[0] = nrows;	// Number of Structure Array Elements
   
   psDef     = IDL_MakeStruct(NULL, pTags);	 
   ivReturn  = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR *)sout, freeMem, psDef);  
              	     
//--------------------------------------------------------------------------      
// Cleanup Keywords & PG Heap 
 
   IDL_KW_FREE;
   PQclear(DBQuery);    
      
   return(ivReturn);           
}
*/

