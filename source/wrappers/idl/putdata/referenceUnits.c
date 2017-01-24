//! $LastChangedRevision: 12 $
//! $LastChangedDate: 2007-12-14 14:52:30 +0000 (Fri, 14 Dec 2007) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/plugins/bytes/readBytesNonOptimally.c $

/*---------------------------------------------------------------
* Open the Units Reference File for Standard Physics and Engineering units.
*
* Physics quantities have a Scale Factor and a Dimensional Unit
* e.g. Frequency is measured in GHz - G is the Scale and Hz is the dimension.
* Only a single Scale is required.
* Dimensions are made up of basic SI units.
*
* To enable compliance testing of the data units, scale and dimension must be specified separately. 
* This avoids problems when scale and dimension have the same symbol, e.g., T (Tera) and T (Tesla)
*
* Scales can be specified as either a symbol or as a named scale, e.g., G or Giga
* By default, the scale is unity.
*
* Change History
*
* 1.0	25Feb2009	D.G.Muir	Original Version
*-----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define MAXRECORDLENGTH	512
#define MAXRECORDCOUNT	100
#define IDAM_UNITS	"/funsrv1/home/dgm/IDAM/source/idl/netcdf4/referenceUnits"

static char quantity[MAXRECORDCOUNT][MAXRECORDLENGTH]; 
static char unit[MAXRECORDCOUNT][MAXRECORDLENGTH]; 
static char symbol[MAXRECORDCOUNT][MAXRECORDLENGTH];
static int nunits=0;

static char scalevalue[MAXRECORDCOUNT][MAXRECORDLENGTH]; 
static char scaleunit[MAXRECORDCOUNT][MAXRECORDLENGTH]; 
static char scalesymbol[MAXRECORDCOUNT][MAXRECORDLENGTH];
static int nscales=0;

static void swapUnitSymbol(char *token);
static void swapScaleSymbol(char *token);
static int unitPowersTest(char *token, char *work);
static int unitSymbolsTest(char *token, int nunits, char symbol[MAXRECORDCOUNT][MAXRECORDLENGTH], int debug);
static int referenceUnitsTest(char *units, int isScale, int verbose, int debug);
static int referenceUnitsRead(int verbose, int debug);
static void listReferenceSymbols();

int main(){

   int i, match, isScale=0;
   int verbose=0, debug=0;
   int err = referenceUnitsRead(verbose, debug);    
    
   char *list[] =  {"rad",    "m/m",    "steradian", "sr",      "Celsius", 
                    "K",      "farad",  "F",         "C/V",     "coulomb", 
		    "C",      "A s",    "siemens",   "S",       "A/V", 
		    "henry",  "H",      "Wb/A",      "volt",    "V", 
		    "W/A",    "ohm",    "O",         "V/A",     "joule", 
		    "J",      "Nm",     "newton",    "N",       "kg m/s^2",
		    "hertz",  "Hz",     "1/s",       "s^-1",    "lux", 
		    "lx",     "lm/m^2", "lumen",     "lm",      "cd sr", 
                    "weber",  "Wb",     "V s",       "tesla",   "T",
		    "Wb/m^2", "watt",   "W",         "J/s",     "pascal",
		    "Pa",     "N/m2",   "becquerel", "Bq",      "1/s", 
		    "gray",   "Gy",     "J/kg",      "sievert", "Sv", 
		    "J/kg",   "katal",  "kat",       "mol/s"    "m",
		    "m/s"     
		   };
   int nlist = 65;		    
/*		   
   char *list[] =  {"m/s",    "m3",	"m-3",       "m+3",     "/m4",
		    "/m-4",
		    "m/s^1",    "m^3",	"m^-3",       "m^+3",     "/m^4",
		    "/m^-4",
		   };
   int nlist = 12;
*/

   char *scalelist[] = {"Giga", "giga", "G", "g", "1.0E6", "10G"};
   int nscalelist = 6;		    		   
nlist = 0;

     
   isScale = 0;
   for(i=0;i<nlist;i++){
      match = referenceUnitsTest(list[i], isScale, verbose, debug);
      fprintf(stdout, "%d [%s]\n", match, list[i]);
      
      verbose = 0;
      if(!match && verbose) swapUnitSymbol(list[i]);
   }
 
   isScale = 1;
   for(i=0;i<nscalelist;i++){
      match = referenceUnitsTest(scalelist[i], isScale, verbose, debug);
      fprintf(stdout, "%d [%s]\n", match, scalelist[i]);
      
      verbose = 0;
      if(!match && 1) swapScaleSymbol(scalelist[i]);
   }
     
   //listReferenceSymbols();   
  
return 0;
}

int unitPowersTest(char *token, char *work){
   int i, offset, ltoken, lwork, ndigits;
   char *p = strstr(token,"^");			// Identify the power symbol
   strcpy(work, token);
   
   if(p == NULL){				// maybe a digit (use as first character is an error) is used instead?
      ltoken = strlen(token);
      if(ltoken == 1) return 0;
        
      for(i=1; i<ltoken; i++){
         if(token[i] >= '0' && token[i] <= '9'){
	    p = &token[i-1];			// start at the previous character
	    break;
         }
      }
       
      if(p == NULL) return 0;            
      if(p == token && (work[0] == '+' || work[0] == '-') ) return 0;		// Will force an error
      
      offset = p-token;
      if(work[offset] == '+' || work[offset] == '-') work[offset] = ' ';	// Ignore signs        
      offset++ ;								// Drop previous character
   } else {
      offset = p-token;
      work[offset++] = ' ';  							// replace ^ with a space
      if(work[offset] == '+' || work[offset] == '-') work[offset++] = ' ';	// Ignore signs        
   }
            
   lwork = strlen(&work[offset]);
   ndigits = 0;
   for(i=0; i<lwork; i++){
      if(isdigit(work[offset+i])){
         work[offset+i] = ' '; 
	 ndigits++;				// Count the digits and replace with spaces
      } else break;
   }
   TrimString(work);
   LeftTrimString(work);			// Compact
   MidTrimString(work);
   
   return(ndigits);
} 

void swapUnitSymbol(char *token){

   int i;
   int ltoken, lunit;
   
   if((ltoken = strlen(token)) == 0) return;
   
// Search for possible unit descriptions 

   for(i=0; i<nunits; i++){						// Scan Units for a Full Match
      lunit = strlen(unit[i]);
      if(lunit != ltoken) continue;
      if(!strcmp(token, unit[i])){ 		 
         fprintf(stderr,"%s contains the standard unit: %s. Consider changing this to %s\n", token, unit[i], symbol[i]);
         return;
      } 
      if(!strcasecmp(token, unit[i])){ 		 
         fprintf(stderr,"%s has the wrong case. Please change to the standard unit %s.\n", token, symbol[i]);
         return;
      } 	 
   } 
   
   for(i=0; i<nunits; i++){						// Scan Units for Partial Matches
      lunit = strlen(unit[i]);
      if(lunit <= ltoken && strstr(token, unit[i]) != NULL){ 		 
         fprintf(stderr,"%s contains the standard unit: %s. Consider changing this to %s\n", token, unit[i], symbol[i]);
      }  	 
   }   
   return; 
}

void swapScaleSymbol(char *token){

   int i, offset;
   int ltoken, lunit;
   char work[MAXRECORDLENGTH];
   
   if((ltoken = strlen(token)) == 0) return;
   
   strcpy(work, token);
   
// Is it an Integer Numerical Scale? Search for possible matches

   if(work[0] == '+' || work[0] == '-' || isdigit(work[0])){
      for(i=0; i<nscales; i++){						// Scan Scales for a Full Match
         lunit = strlen(scalevalue[i]);
         if(lunit != ltoken) continue;
         if(!strcasecmp(work, scalevalue[i])){ 		 
            fprintf(stderr,"%s has the standard scale Value : %s. Consider changing this to %s or %s\n", token, scalevalue[i], 
	                   scaleunit[i], scalesymbol[i]);
            return;
         }      	 
      }                      
   }   
   
   
// Search for possible Scale Units 

   for(i=0; i<nscales; i++){						// Scan Scales for a Full Match
      lunit = strlen(scaleunit[i]);
      if(lunit != ltoken) continue;
      if(!strcmp(work, scaleunit[i])){ 		 
         fprintf(stderr,"%s contains the standard scale label: %s. Consider changing this to %s or %s\n", token, scaleunit[i],
	                 scaleunit[i], scalesymbol[i]);
         return;
      }
      if(!strcasecmp(work, scaleunit[i])){ 		 
         fprintf(stderr,"%s has the wrong case. Please change to the standard scale %s or %s\n", token, scaleunit[i], scalesymbol[i]);
         return;
      }       	 
   } 
   
   for(i=0; i<nscales; i++){						// Scan Scales for Partial Matches
      lunit = strlen(scaleunit[i]);
      if(lunit <= ltoken && strstr(work, scaleunit[i]) != NULL){ 		 
         fprintf(stderr,"%s contains the standard scale: %s. Consider changing this to %s or %s\n", token, scaleunit[i], scaleunit[i], scalesymbol[i]);
      }  	 
   }

// Search for possible Scale Symbols   

   for(i=0; i<nscales; i++){						// Scan Scales for a Full Match
      lunit = strlen(scalesymbol[i]);
      if(lunit != ltoken) continue;
      if(!strcmp(work, scalesymbol[i])){ 		 
         fprintf(stderr,"%s contains the standard scale: %s. Consider changing this to %s or %s\n", token, scaleunit[i], scaleunit[i], scalesymbol[i]);
         return;
      }
      if(!strcasecmp(work, scalesymbol[i])){ 		 
         fprintf(stderr,"%s has the wrong case. Please change to the standard scale %s or %s\n", token, scaleunit[i], scalesymbol[i]);
         return;
      }       	 
   } 
   
   for(i=0; i<nscales; i++){						// Scan Scales for Partial Matches
      lunit = strlen(scalesymbol[i]);
      if(lunit <= ltoken && strstr(work, scalesymbol[i]) != NULL){ 		 
         fprintf(stderr,"%s contains the standard scale: %s. Consider changing this to %s or %s\n", token, scaleunit[i], scaleunit[i], scalesymbol[i]);
      }  	 
   }   
         
   return; 
}    


int unitSymbolsTest(char *token, int nunits, char symbol[MAXRECORDCOUNT][MAXRECORDLENGTH], int debug){

   int i, matchcount = 0, matchcount2 = 0, match[MAXRECORDCOUNT];
   int ltoken, lsymbol;
   char *p;
   char work[MAXRECORDLENGTH];
   
   if((ltoken = strlen(token)) == 0) return 0;
   
// Start with first character: decide on probable symbols 

   for(i=0; i<nunits; i++){						// Scan Symbols
      lsymbol = strlen(symbol[i]);
      if(!strncmp(token, symbol[i], lsymbol) && lsymbol <= ltoken){ 		// Full or Partial Match Found
         match[i] = 1;			// Mark
         matchcount++;			// Count
      } else
         match[i] = 0; 	 
   }
   
   if(matchcount == 0){		// No match found: failed Test
      if(debug) fprintf(stdout, "The units token [%s] cannot be matched to the Reference set.\n", token); 
      return 0;					
   }   
   
// For all first round matches, subset the token and drill down recursively    
   
   for(i=0; i<nunits; i++){
      if(match[i]){
	 lsymbol = strlen(symbol[i]);
	 if(ltoken == lsymbol && matchcount >= 1) return 1;			// Full Match Found so Exit
         strcpy(work, &token[lsymbol]);						// Test the next part of the token
         LeftTrimString(work);							// Remove leading spaces
	 match[i] = unitSymbolsTest(work, nunits, symbol, debug) > 0;		// Count Matches.
         matchcount2 = matchcount2 + match[i];
      }
   }
   
   if(matchcount2 == 0 && debug)fprintf(stdout, "The units token [%s] cannot be matched to the Reference set.\n", token); 
   
   return matchcount2; 
}  

int referenceUnitsTest(char *units, int isScale, int verbose, int debug){

// Assume the units string is modelled on a/b/c/d etc. or abc or ab/c or a^n
// if isScale == 1 then the units string is the units scale string

   int err=0, nmatches=0, npower;
   char *p, *token;
   char work[MAXRECORDLENGTH];
   char work2[MAXRECORDLENGTH];

// Tokenise the Units String

   strcpy(work, units);
   TrimString(work);
   LeftTrimString(work);
         
   if((token = strtok(work, "/")) != NULL){

      if(debug){
         fprintf(stdout,"Testing Units/Scale : %s\n", units);
	 fprintf(stdout,"Testing Token       : %s\n", token);
      }

      if(!isScale){
         npower = unitPowersTest(token, work2);		// Look for power terms ^n, n = integer, Remove and Compact
         if(debug){
            fprintf(stdout,"Power         : %d\n", npower);
	    fprintf(stdout,"Testing Token : %s\n", work2);
         }
      } else {
         strcpy(work2, token);
      }	 
      
      if(!isScale){
         nmatches = unitSymbolsTest(work2, nunits, symbol, debug);
      } else {
         if(work2[0] == '+' || work2[0] == '-' || isdigit(work2[0])){		// Integer Prefix?      
            int i, offset = 0;
	    int lwork2 = strlen(work2);
            if(work2[0] == '+' || work2[0] == '-') offset = 1;               
	    for(i=offset; i<lwork2; i++){
               if(isdigit(work2[i]))
	          work2[i] = ' ';						// Remove number prefixes to prefixes
	       else 
	          break;   
            }
            LeftTrimString(work2);
	 }
	 nmatches = unitSymbolsTest(work2, nscales, scalesymbol, debug);			// Either a symbol or a word
	 if(nmatches == 0) nmatches = unitSymbolsTest(work2, nscales, scaleunit, debug);     
      }	 
      
      if(nmatches == 0){
         if(verbose){
	    if(!isScale){
	       fprintf(stderr,"The units symbol [%s] cannot be matched to the Reference set.\n", token);
	       fprintf(stderr,"Please refer to the MAST Wiki for the set of acceptable units.\n" 
	                      "If your data cannot be represented by this standard set, discuss the problem\n"
			      "with the IDAM RO or the MDMC chairperson - new validated entries to the standard\n"
			      "set can be made very quickly.\n");
            } else {
	       fprintf(stderr,"The units scale [%s] cannot be matched to the Reference set.\n", token);
	       fprintf(stderr,"Please refer to the MAST Wiki for the set of acceptable scales.\n" 
	                      "If your data cannot be represented by this standard set, discuss the problem\n"
			      "with the IDAM RO or the MDMC chairperson - new validated entries to the standard\n"
			      "set can be made very quickly.\n");
	    }
	 }
	 return 0;			   	    
      }
      
      if(debug) fprintf(stdout,"Match Count #1: %d\n", nmatches);       
  
      while((token = strtok(NULL, "/")) != NULL){		 
         
	 if(debug)fprintf(stdout,"Testing Token : %s\n", token);
	
	 if(!isScale){
	    npower = unitPowersTest(token, work2);		// Look for power terms ^n, n = integer, Remove and Compact         
	    if(debug){
               fprintf(stdout,"Power         : %d\n", npower);
	       fprintf(stdout,"Testing Token : %s\n", work2);
            }
	 } else {
	    strcpy(work2, token);
	 }  
	 
	 if(!isScale){
            nmatches = unitSymbolsTest(work2, nunits, symbol, debug);
         } else {
            if(work2[0] == '+' || work2[0] == '-' || isdigit(work2[0])){		// Integer Prefix?      
               int i, offset = 0;
	       int lwork2 = strlen(work2);
               if(work2[0] == '+' || work2[0] == '-') offset = 1;               
	       for(i=offset; i<lwork2; i++){
                  if(isdigit(work2[i]))
	             work2[i] = ' ';						// Remove number prefixes to prefixes
	          else 
	             break;   
               }
               LeftTrimString(work2);
	    }
	    nmatches = unitSymbolsTest(work2, nscales, scalesymbol, debug);
	    if(nmatches == 0) nmatches = unitSymbolsTest(work2, nscales, scaleunit, debug);
         }	 

         if(nmatches == 0){
            if(verbose){
	       if(!isScale){
	          fprintf(stderr,"The units symbol [%s] cannot be matched to the Reference set.\n", token);
	          fprintf(stderr,"Please refer to the MAST Wiki for the set of acceptable units.\n" 
	                         "If your data cannot be represented by this standard set, discuss the problem\n"
			         "with the IDAM RO or the MDMC chairperson - new validated entries to the standard\n"
			         "set can be made very quickly.\n");
               } else {
	          fprintf(stderr,"The units scale [%s] cannot be matched to the Reference set.\n", token);
	          fprintf(stderr,"Please refer to the MAST Wiki for the set of acceptable scales.\n" 
	                         "If your data cannot be represented by this standard set, discuss the problem\n"
			         "with the IDAM RO or the MDMC chairperson - new validated entries to the standard\n"
			         "set can be made very quickly.\n");
	       }
	    }     
	    return 0;			   	    	 
	 }
	 
         if(debug)fprintf(stdout,"Match Count #2: %d\n", nmatches);
      } 
   }

   return 1;

}



int referenceUnitsRead(int verbose, int debug){
	      
   int i, err, rc, isScales=0, nchar;
   int offset, bufsize, nread;	      

   int size = 0;    
   FILE *fh=NULL, *ph=NULL;   
   
   char rec[MAXRECORDLENGTH], file[MAXRECORDLENGTH];
   char *env, *q, *p, *s, *u;
   
//----------------------------------------------------------------------
// Location and Name of Reference Units File if changed by Environment Variable

   if((env = getenv("IDAM_UNITS")) != NULL){
      strcpy(file, env);
   } else {
      strcpy(file, IDAM_UNITS);
   }   
   
//----------------------------------------------------------------------
// Open the File 
      
   err     = 0;
   errno   = 0;
   nunits  = 0;
   
   fh = fopen(file, "r");
   
   if(fh == NULL || ferror(fh) || errno != 0) {
      if(fh != NULL) fclose(fh);
      if(verbose)fprintf(stderr,"Error: Unable to Open the Reference Units File\n");
      return 1;
   }	
      
//----------------------------------------------------------------------                  
// Read File 

   while(fgets(rec, MAXRECORDLENGTH, fh) != NULL && nunits < MAXRECORDCOUNT){
      if(rec[0] == '#' || rec[0] == ' ') continue;	// Ignore lines beginning with # or space
      
      if(!strncmp(rec,"-----", 5)){
          isScales = 1;
          continue;					// Identifies the Scales group 
      }
      
      if((q = strstr(rec, ",")) == NULL) continue;	// Ignore line if no delimiter  
      q[0] = '\0';					// Tokenise 	


      if(!isScales){            
         strcpy(quantity[nunits], rec);
         convertNonPrintable(quantity[nunits]);			 
         TrimString(quantity[nunits]);
         LeftTrimString(quantity[nunits]);

         if((u = strstr(&q[1], ",")) == NULL) continue;	// Ignore line if no delimiter  
         u[0] = '\0';					// Tokenise 	
         strcpy(unit[nunits], &q[1]);
         convertNonPrintable(unit[nunits]);
         TrimString(unit[nunits]);
         LeftTrimString(unit[nunits]);
      
         strcpy(symbol[nunits], &u[1]);
         convertNonPrintable(symbol[nunits]);
         TrimString(symbol[nunits]);
         LeftTrimString(symbol[nunits]);

         if(debug) fprintf(stdout, "[%d] [%s] [%s] [%s]\n", nunits, quantity[nunits], unit[nunits], symbol[nunits]);      
      
         nunits++;            
      } else {
         strcpy(scalevalue[nscales], rec);
         convertNonPrintable(scalevalue[nscales]);			 
         TrimString(scalevalue[nscales]);
         LeftTrimString(scalevalue[nscales]);

         if((u = strstr(&q[1], ",")) == NULL) continue;	// Ignore line if no delimiter  
         u[0] = '\0';					// Tokenise 	
         strcpy(scaleunit[nscales], &q[1]);
         convertNonPrintable(scaleunit[nscales]);
         TrimString(scaleunit[nscales]);
         LeftTrimString(scaleunit[nscales]);
      
         strcpy(scalesymbol[nscales], &u[1]);
         convertNonPrintable(scalesymbol[nscales]);
         TrimString(scalesymbol[nscales]);
         LeftTrimString(scalesymbol[nscales]);

         if(debug) fprintf(stdout, "[%d] [%s] [%s] [%s]\n", nscales, scalevalue[nscales], scaleunit[nscales], scalesymbol[nscales]);      
      
         nscales++;
      }
   }
         
//---------------------------------------------------------------------- 
// Housekeeping

   fclose(fh);		// Close the File
      
   return 0;
}

void listReferenceSymbols(){
   int i;
   fprintf(stdout,"\n\n#    %-32s%-20s%-10s\n\n","Quantity", "Unit", "Symbol");
   for(i=0;i<nunits;i++){
      fprintf(stdout,"[%2d] %-32s%-20s%-10s\n", i, quantity[i], unit[i], symbol[i]);
   }
   
   fprintf(stdout,"\n\n#    %-32s%-20s%-10s\n\n","Quantity", "Scale", "Symbol");
   for(i=0;i<nscales;i++){
      fprintf(stdout,"[%2d] %-32s%-20s%-10s\n", i, scalevalue[i], scaleunit[i], scalesymbol[i]);
   }
}
 
