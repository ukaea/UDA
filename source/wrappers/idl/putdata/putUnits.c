
#include "putdata.h"		// IDL DLM API Header
   
int testUnitsCompliance(char * units){

// Return TRUE (1) or FALSE (0)
    
   wchar_t wc;            
   ut_unit *encoded  = NULL;
   unsigned encoding = UT_ASCII; 	// UT_UTF8
       
//--------------------------------------------------------------------------      	 
// Check Units Compliance
   
   if(unitSystem == NULL){
      unitSystem = ut_read_xml(NULL);		// Read and Parse Standard Units Definition XML Documents       
      if (!setlocale(LC_CTYPE, "")) {
         if(kw->verbose) fprintf(stderr, "Can't set the specified locale! Check LANG, LC_CTYPE, LC_ALL.\n");
         return 0;
      }
   }
       
   if((encoded = ut_parse(unitSystem, units, encoding)) == NULL){
      if(kw->verbose){
         int code = (int)ut_get_status();
         fprintf(stderr, "Units [%s] are Not SI Compliant. Please correct\n");
	 if(code == UT_SYNTAX){
	    fprintf(stderr, "Units contain a Syntax Error\n");
	 } else {
	    if(code == UT_UNKNOWN){
	       fprintf(stderr, "Units contain an Unknown Identifier\n");
	    } else fprintf(stderr, "Error Code: %d\n", (int)code); 
	 }   
      }	 
      return 0;
   } 
   
   ut_free(encoded);		// Free Resources     
      
   return(1); 
} 
