
/*  
* IDAM XML Parser for EFIT Hierarchical Data Structures 
* 
* Input Arguments:	char *xml 
* 
* Returns:		0 if parse was successful 
* 
*			IDAM_EFIT - Data Structure	                        
* 
* Change Control: 
* 
* 1.0  12Jul2007 D.G.Muir	Consolidation of Previous Versions 
* 23Oct2007	dgm	ERRORSTACK Components added 
*-------------------------------------------------------------------------*/ 
#include "efitmagxml.h"

#include <idampluginfiles.h>
#include <idamserver.h>
#include <idamErrorLog.h>
#include <managePluginFiles.h>
#include <initStructs.h>
#include <makeServerRequestBlock.h>
#include <client/accAPI_C.h>
#include <client/IdamAPI.h>
#include <freeDataBlock.h>
#include <structures/struct.h>
#include <structures/accessors.h>
#include <clientserver/TrimString.h>
#include <clientserver/idamErrors.h> 
 
// Simple Tags with Delimited List of Floating Point Values  
// Assume No Attributes  

int debugon = 0;
FILE *dbgout = NULL;
 
float *parseFloatArray(xmlDocPtr doc, xmlNodePtr cur, char *target, int *n){ 
   xmlChar *key; 
   float *value = NULL; 
   *n = 0; 
   char *delim = " "; 
   char *item; 
   int nco = 0; 
    
   cur = cur->xmlChildrenNode; 
   while(cur != NULL){    
      if((!xmlStrcmp(cur->name, (const xmlChar *)target))){ 
         key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1); 
	 convertNonPrintable(key); 
	 if(strlen(key) > 0){ 
	    if(debugon)fprintf(dbgout,"parseFloatArray: %s %s \n", target, key);	     
	    item = strtok((char *)key, delim); 
	    if (item != NULL) { 
	       nco++; 
	       if(debugon)fprintf(dbgout,"parseFloatArray: [%d] %s \n", nco, item);	        
	       value = (float *)realloc((void *)value, nco * sizeof(float)); 
	       value[nco-1] = atof(item);	        
	       if(debugon)fprintf(dbgout,"parseFloatArray: [%d] %s %f\n", nco, item, value[nco-1]); 
	       while((item = strtok(NULL, delim)) != NULL && nco <= XMLMAXLOOP){ 
	          nco++; 
	          value = (float *)realloc((void *)value, nco * sizeof(float)); 
	          value[nco-1] = atof(item); 
		  if(debugon)fprintf(dbgout,"parseFloatArray: [%d] %s %f\n", nco, item, value[nco-1]); 
	       } 
	    }    
	 }  
	 *n = nco; 
	 xmlFree(key); 
	 break; 
      } 
      cur = cur->next; 
   } 
   return value; 
} 
 
// Simple Tags with Floating Point Values 
// Assume No Attributes 
 
void parseFloat(xmlDocPtr doc, xmlNodePtr cur, char *target, float *value){ 
   xmlChar *key; 
   *value = 0.0; 
    
   cur = cur->xmlChildrenNode; 
   while(cur != NULL){    
      if((!xmlStrcmp(cur->name, (const xmlChar *)target))){ 
         key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1); 
	 if(strlen((char *)key) > 0) *value = atof((char *)key); 
	 if(debugon)fprintf(dbgout,"parseFloat: %s  %s\n", target, (char *)key); 
	 xmlFree(key); 
	 break; 
      } 
      cur = cur->next; 
   } 
   return; 
} 
 
// Simple Tags with Integer Values 
// Assume No Attributes 
 
void parseInt(xmlDocPtr doc, xmlNodePtr cur, char *target, int *value){ 
   xmlChar *key; 
   *value = 0; 
    
   cur = cur->xmlChildrenNode; 
   while(cur != NULL){    
      if((!xmlStrcmp(cur->name, (const xmlChar *)target))){ 
         key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1); 
	 if(strlen((char *)key) > 0) *value = atoi((char *)key); 
	 if(debugon)fprintf(dbgout,"parseInt: %s  %s\n", target, (char *)key); 
	 xmlFree(key); 
	 break; 
      } 
      cur = cur->next; 
   } 
   return; 
} 
 
int *parseIntArray(xmlDocPtr doc, xmlNodePtr cur, char *target, int *n){ 
   xmlChar *key; 
   int *value = NULL; 
   *n = 0; 
   char *delim = " "; 
   char *item; 
   int nco = 0; 
    
   cur = cur->xmlChildrenNode; 
   while(cur != NULL){    
      if((!xmlStrcmp(cur->name, (const xmlChar *)target))){ 
         key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1); 
	 convertNonPrintable((char *)key); 
	 if(strlen((char *)key) > 0){ 
	    if(debugon)fprintf(dbgout,"parseIntArray: %s %s \n", target, (char *)key);	     
	    item = strtok((char *)(char *)key, delim); 
	    if (item != NULL) { 
	       nco++; 
	       if(debugon)fprintf(dbgout,"parseIntArray: [%d] %s \n", nco, item);	        
	       value = (int *)realloc((void *)value, nco * sizeof(int)); 
	       value[nco-1] = atoi(item);	        
	       if(debugon)fprintf(dbgout,"parseIntArray: [%d] %s %d\n", nco, item, value[nco-1]); 
	       while((item = strtok(NULL, delim)) != NULL && nco <= XMLMAXLOOP){ 
	          nco++; 
	          value = (int *)realloc((void *)value, nco * sizeof(int)); 
	          value[nco-1] = atoi(item); 
		  if(debugon)fprintf(dbgout,"parseIntArray: [%d] %s %d\n", nco, item, value[nco-1]); 
	       } 
	    }    
	 }  
	 *n = nco; 
	 xmlFree(key); 
	 break; 
      } 
      cur = cur->next; 
   } 
   return value; 
} 
 
 
// Simple Tags with Delimited List of Floating Point Values  
// Assume No Attributes 
 
float *parseFloatAngleArray(xmlDocPtr doc, xmlNodePtr cur, char *target, int *n){ 
   xmlChar *key, *att; 
   float *value = NULL; 
   *n = 0; 
   char *delim = " "; 
   char *item; 
   int i, nco = 0; 
   float factor=1.0; 
    
   cur = cur->xmlChildrenNode; 
   while(cur != NULL){    
      if((!xmlStrcmp(cur->name, (const xmlChar *)target))){ 
         key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1); 
	 convertNonPrintable((char *)key); 
	 if(strlen((char *)key) > 0){ 
	    if(debugon)fprintf(dbgout,"parseFloatAngleArray: %s %s \n", target, (char *)key); 
	    item = strtok((char *)key, delim); 
	    if (item != NULL) { 
	       nco++; 
	       if(debugon)fprintf(dbgout,"parseFloatAngleArray: [%d] %s \n", nco, item); 
	       value = (float *)realloc((void *)value, nco * sizeof(float)); 
	       value[nco-1] = atof(item); 
	       if(debugon)fprintf(dbgout,"parseFloatAngleArray: [%d] %s %f\n", nco, item, value[nco-1]); 
	       while((item = strtok(NULL, delim)) != NULL && nco <= XMLMAXLOOP){ 
	          nco++; 
	          value = (float *)realloc((void *)value, nco * sizeof(float)); 
	          value[nco-1] = atof(item); 
		  if(debugon)fprintf(dbgout,"parseFloatAngleArray: [%d] %s %f\n", nco, item, value[nco-1]); 
	       } 
	    }    
	 }  
	 *n = nco; 
	 xmlFree(key); 
	  
	 factor = 3.1415927 / 180.0 ;    // Default is Degrees 
	  
	 if((att = xmlGetProp(cur, (xmlChar *)"units")) != NULL){ 
	    if(strlen((char *)att) > 0) { 
	       if(!strcmp((char *)att,"pi"))      factor = 3.1415927 ; 
	       if(!strcmp((char *)att,"radians")) factor = 1.0 ;	       	  
	       xmlFree(att);	       	        
	    }    
	 }   
	  
	 for(i=0;i<nco;i++) value[i] = value[i]*factor;  
	  
	 break; 
      } 
      cur = cur->next; 
   } 
   return value; 
} 
 
// Simple Tags with Floating Point Values 
// Assume No Attributes 
 
void parseFloatAngle(xmlDocPtr doc, xmlNodePtr cur, char *target, float *value){ 
   xmlChar *key, *att; 
   float factor = 1.0; 
   *value = 0.0; 
    
   cur = cur->xmlChildrenNode; 
   while(cur != NULL){    
      if((!xmlStrcmp(cur->name, (const xmlChar *)target))){ 
         key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1); 
	 if(strlen((char *)key) > 0) *value = atof((char *)key); 
	 if(debugon)fprintf(dbgout,"parseFloatAngle: %s  %s\n", target, (char *)key); 
	 xmlFree(key); 
 
	 if((att = xmlGetProp(cur, (xmlChar *)"units")) != NULL){ 
	    if(strlen((char *)att) > 0) { 
	       if(!strcmp((char *)att,"pi")) *value = *value * 3.1415927 ; 
	       if(!strcmp((char *)att,"degrees")) *value = *value * 3.1415927 / 180.0 ;	  
	       xmlFree(att); 
	    }    
	 }  
	  
	 factor = 3.1415927 / 180.0 ;    // Default is Degrees 
	  
	 if((att = xmlGetProp(cur, (xmlChar *)"units")) != NULL){ 
	    if(strlen((char *)att) > 0) { 
	       if(!strcmp((char *)att,"pi"))      factor = 3.1415927 ; 
	       if(!strcmp((char *)att,"radians")) factor = 1.0 ;	       	  
	       xmlFree(att);	       	        
	    }    
	 }   
	  
	 *value = *value * factor;    
  
	 break; 
      } 
      cur = cur->next; 
   } 
   return; 
} 

//========================================================================================================================================== 
// Instance Attributes (Signal Identification) 
 
void parseInstance(xmlNodePtr cur, INSTANCE *str){ 
   xmlChar *att;	  
 	 	  
   if((att = xmlGetProp(cur, "archive")) != NULL){ 
      if(strlen(att) > 0) strcpy(str->archive, att); 
	if(debugon)fprintf(dbgout,"Archive: %s\n", str->archive);	  
	xmlFree(att); 
   }  
   if((att = xmlGetProp(cur, "file")) != NULL){ 
      if(strlen(att) > 0) strcpy(str->file, att); 
	if(debugon)fprintf(dbgout,"File: %s\n", str->file); 
	xmlFree(att); 
   }    
   if((att = xmlGetProp(cur, "signal")) != NULL){ 
      if(strlen(att) > 0) strcpy(str->signal, att); 
	if(debugon)fprintf(dbgout,"Signal: %s\n", str->signal); 
	xmlFree(att); 
   }      
    if((att = xmlGetProp(cur, "owner")) != NULL){ 
      if(strlen(att) > 0) strcpy(str->owner, att); 
	if(debugon)fprintf(dbgout,"Owner: %s\n", str->owner); 
	xmlFree(att); 
   }     	  
   if((att = xmlGetProp(cur, "format")) != NULL){ 
	if(strlen(att) > 0) strcpy(str->format, att); 
	if(debugon)fprintf(dbgout,"Format: %s\n", str->format); 
	xmlFree(att); 
   }    
	  
   if((att = xmlGetProp(cur, "seq")) != NULL){ 
	if(strlen(att) > 0) str->seq = atoi(att); 
	if(debugon)fprintf(dbgout,"Seq: %d\n", str->seq); 
	xmlFree(att); 
   }   
   if((att = xmlGetProp(cur, "status")) != NULL){ 
	if(strlen(att) > 0) str->status = atoi(att); 
	if(debugon)fprintf(dbgout,"Status: %d\n", str->status); 
	xmlFree(att); 
   }   
   if((att = xmlGetProp(cur, "factor")) != NULL){ 
	if(strlen(att) > 0) str->factor = atof(att); 
	if(debugon)fprintf(dbgout,"Factor: %f\n", str->factor); 
	xmlFree(att); 
   }    
  
   return; 
} 
 
 
// Magnetic Probe Data 
// Assume multiple tags per document 
 
MAGPROBE *parseMagProbe(xmlDocPtr doc, xmlNodePtr cur, MAGPROBE *str, int *np){ 
 
   int n = 0; 
   xmlChar *att;	// General Input of tag attribute values 
    
   *np = 0;  
   cur = cur->xmlChildrenNode; 
   while(cur != NULL){ 
      if(debugon)fprintf(dbgout,"parseMagProbe: %s\n", (char *)cur->name);     
      if((!xmlStrcmp(cur->name, (const xmlChar *)"instance"))){ 
         n++; 
         str = (MAGPROBE *)realloc((void *)str, n*sizeof(MAGPROBE)); 
	  
	 if(debugon)fprintf(dbgout,"parseMagProbe#%d: %p\n", n, str); 
	 initMagProbe(&str[n-1]); 
	  
// Instance Attributes 
 
         if((att = xmlGetProp(cur, "id")) != NULL){ 
	      if(strlen(att) > 0) strcpy(str[n-1].id, att); 
	      if(debugon)fprintf(dbgout,"Mag Probe ID: %s\n", str[n-1].id); 
	      xmlFree(att); 
	   }    
	 	  
         parseInstance(cur, &str[n-1].instance); 
 
// Child Tags	  
	    
	   parseFloat(doc, cur, "r",          &str[n-1].r); 
	   parseFloat(doc, cur, "z",          &str[n-1].z); 
	  
	   parseFloatAngle(doc, cur, "angle", &str[n-1].angle); 
	  
	   parseFloat(doc, cur, "abs_error",  &str[n-1].aerr); 
	   parseFloat(doc, cur, "rel_error",  &str[n-1].rerr);	  
	  
         if(debugon) printMagProbe(dbgout, str[n-1]); 
      } 
      cur = cur->next; 
   } 
   *np = n;	// Number of Tags Found 
   return str; 
}		       
	 
	 
// Flux Loop Data 
// Assume multiple tags per document 
 
FLUXLOOP *parseFluxLoop(xmlDocPtr doc, xmlNodePtr cur, FLUXLOOP *str, int *np){ 
 
   int n = 0; 
   int nco; 
   xmlChar *att;	// General Input of tag attribute values 
    
   *np = 0;  
   cur = cur->xmlChildrenNode; 
   while(cur != NULL){ 
      if(debugon)fprintf(dbgout,"parseFluxLoop: %s\n", (char *)cur->name);     
      if((!xmlStrcmp(cur->name, (const xmlChar *)"instance"))){ 
         n++; 
         str = (FLUXLOOP *)realloc((void *)str, n*sizeof(FLUXLOOP)); 
	  
	   if(debugon)fprintf(dbgout,"parseFluxLoop#%d: %p\n", n, str); 
	   initFluxLoop(&str[n-1]); 
	  
// Attributes 
 
         if((att = xmlGetProp(cur, "id")) != NULL){ 
	      if(strlen(att) > 0) strcpy(str[n-1].id, att); 
	      if(debugon)fprintf(dbgout,"Flux Loop ID: %s\n", str[n-1].id); 
	      xmlFree(att); 
	   }    
	 	  
         parseInstance(cur, &str[n-1].instance); 
 
// Child Tags	  
	   str[n-1].r    = (float *)parseFloatArray(     doc, cur, "r",    &str[n-1].nco); 
	   str[n-1].z    = (float *)parseFloatArray(     doc, cur, "z",    &nco); 
	   str[n-1].dphi = (float *)parseFloatAngleArray(doc, cur, "dphi", &nco); 
	  
           parseFloat(doc, cur, "abs_error",  &str[n-1].aerr); 
	   parseFloat(doc, cur, "rel_error",  &str[n-1].rerr);	  
 
         if(debugon) printFluxLoop(dbgout, str[n-1]); 
      } 
      cur = cur->next; 
   } 
   *np = n;	// Number of Tags Found 
   return str; 
}	 
 
	 
// PF Coil Data 
// Assume multiple tags per document 
 
PFCOILS *parsePfCoils(xmlDocPtr doc, xmlNodePtr cur, PFCOILS *str, int *np){ 
 
   int i, n = 0; 
   int nco; 
   int *nrnz; 
   xmlChar *att;	// General Input of tag attribute values 
    
   *np = 0;  
   cur = cur->xmlChildrenNode; 
   while(cur != NULL){ 
      if(debugon)fprintf(dbgout,"parsePfCoils: %s\n", (char *)cur->name);     
      if((!xmlStrcmp(cur->name, (const xmlChar *)"instance"))){ 
         n++; 
         str = (PFCOILS *)realloc((void *)str, n*sizeof(PFCOILS)); 
	  
	   if(debugon)fprintf(dbgout,"parsePfCoils#%d: %p\n", n, str); 
	   initPfCoils(&str[n-1]); 
	  
// Attributes 
 
         if((att = xmlGetProp(cur, "id")) != NULL){ 
	      if(strlen(att) > 0) strcpy(str[n-1].id, att); 
	      if(debugon)fprintf(dbgout,"PF Coil ID: %s\n", str[n-1].id); 
	      xmlFree(att); 
	   }    
	 	  
         parseInstance(cur, &str[n-1].instance); 
 
// Child Tags	  
	    
	   str[n-1].r  = (float *)parseFloatArray(doc, cur, "r",  &str[n-1].nco); 
	   str[n-1].z  = (float *)parseFloatArray(doc, cur, "z",  &nco); 
	   str[n-1].dr = (float *)parseFloatArray(doc, cur, "dr", &nco); 
	   str[n-1].dz = (float *)parseFloatArray(doc, cur, "dz", &nco); 
	  
	   parseInt  (doc, cur, "turnsperelement",    &str[n-1].turns); 
	   parseFloat(doc, cur, "turnsperelement",    &str[n-1].fturns); 
	   parseFloat(doc, cur, "abs_error",          &str[n-1].aerr); 
	   parseFloat(doc, cur, "rel_error",          &str[n-1].rerr); 
	  
	   nrnz = parseIntArray(doc, cur, "modelnrnz", &nco); 
	   if(nco > 0 && nco <=2 && nrnz != NULL){ 
	      for(i=0;i<nco;i++) str[n-1].modelnrnz[i]=nrnz[i]; 
	      free(nrnz); 
	   }	   
	  
         if(debugon) printPfCoils(dbgout, str[n-1]); 
      } 
      cur = cur->next; 
   } 
   *np = n;	// Number of Tags Found 
   return str; 
}	 
 
	 
// PF Passive Circuit Elements 
// Assume multiple tags per document 
 
PFPASSIVE *parsePfPassive(xmlDocPtr doc, xmlNodePtr cur, PFPASSIVE *str, int *np){ 
 
   int i, n = 0; 
   int nco; 
   int *nrnz; 
   xmlChar *att;	// General Input of tag attribute values 
    
   *np = 0;  
   cur = cur->xmlChildrenNode; 
   while(cur != NULL){ 
      if(debugon)fprintf(dbgout,"parsePfPassive: %s\n", (char *)cur->name);     
      if((!xmlStrcmp(cur->name, (const xmlChar *)"instance"))){ 
         n++; 
         str = (PFPASSIVE *)realloc((void *)str, n*sizeof(PFPASSIVE)); 
	  
	   if(debugon)fprintf(dbgout,"parsePfPassive#%d: %p\n", n, str); 
	   initPfPassive(&str[n-1]); 
	  
// Attributes 
 
         if((att = xmlGetProp(cur, "id")) != NULL){ 
	      if(strlen(att) > 0) strcpy(str[n-1].id, att); 
	      if(debugon)fprintf(dbgout,"Pf Passive ID: %s\n", str[n-1].id); 
	      xmlFree(att); 
	   }    
	 	  
         parseInstance(cur, &str[n-1].instance); 
 
// Child Tags	  
	    
	   str[n-1].r    = (float *)parseFloatArray(     doc, cur, "r",    &str[n-1].nco); 
	   str[n-1].z    = (float *)parseFloatArray(     doc, cur, "z",    &nco); 
	   str[n-1].dr   = (float *)parseFloatArray(     doc, cur, "dr",   &nco); 
	   str[n-1].dz   = (float *)parseFloatArray(     doc, cur, "dz",   &nco); 
	   str[n-1].ang1 = (float *)parseFloatAngleArray(doc, cur, "ang1", &nco); 
	   str[n-1].ang2 = (float *)parseFloatAngleArray(doc, cur, "ang2", &nco); 
	   str[n-1].res  = (float *)parseFloatArray(doc, cur, "resistance", &nco); 
	    
//str[n-1].res = str[n-1].r    ;  
// Also fix FREE HEAP as commented out for res 
	   parseFloat(doc, cur, "abs_error", &str[n-1].aerr); 
	   parseFloat(doc, cur, "rel_error", &str[n-1].rerr); 
	  
	   nrnz = parseIntArray(doc, cur, "modelnrnz", &nco); 
	   if(nco >= 0 && nco <=2 && nrnz != NULL){ 
	      for(i=0;i<nco;i++) str[n-1].modelnrnz[i]=nrnz[i]; 
	      free(nrnz); 
	   }	   
	  
         if(debugon) printPfPassive(dbgout, str[n-1]); 
      } 
      cur = cur->next; 
   } 
   *np = n;	// Number of Tags Found 
   return str; 
}	 
 
	 
// PF Supplies 
// Assume multiple tags per document 
 
PFSUPPLIES *parsePfSupplies(xmlDocPtr doc, xmlNodePtr cur, PFSUPPLIES *str, int *np){ 
 
   int n = 0; 
   xmlChar *att;	// General Input of tag attribute values 
    
   *np = 0;  
   cur = cur->xmlChildrenNode; 
   while(cur != NULL){ 
      if(debugon)fprintf(dbgout,"parsePfSupplies: %s\n", (char *)cur->name);     
      if((!xmlStrcmp(cur->name, (const xmlChar *)"instance"))){ 
         n++; 
         str = (PFSUPPLIES *)realloc((void *)str, n*sizeof(PFSUPPLIES)); 
	  
	   if(debugon)fprintf(dbgout,"parsePfSupplies#%d: %p\n", n, str); 
	   initPfSupplies(&str[n-1]); 
	  
// Attributes 
 
         if((att = xmlGetProp(cur, "id")) != NULL){ 
	      if(strlen(att) > 0) strcpy(str[n-1].id, att); 
	      if(debugon)fprintf(dbgout,"Pf Supplies ID: %s\n", str[n-1].id); 
	      xmlFree(att); 
	   }    
	 	  
         parseInstance(cur, &str[n-1].instance); 
 
// Child Tags	 
	  
	   parseFloat(doc, cur, "abs_error", &str[n-1].aerr); 
	   parseFloat(doc, cur, "rel_error", &str[n-1].rerr); 
	  
         if(debugon) printPfSupplies(dbgout, str[n-1]); 
      } 
      cur = cur->next; 
   } 
   *np = n;	// Number of Tags Found 
   return str; 
}	 
 
 
	 
// PF Circuits 
// Assume multiple tags per document 
 
PFCIRCUIT *parsePfCircuits(xmlDocPtr doc, xmlNodePtr cur, PFCIRCUIT *str, int *np){ 
 
   int n = 0; 
   xmlChar *att;	// General Input of tag attribute values 
    
   *np = 0;  
   cur = cur->xmlChildrenNode; 
   while(cur != NULL){ 
      if(debugon)fprintf(dbgout,"parsePfCircuits: %s\n", (char *)cur->name);     
      if((!xmlStrcmp(cur->name, (const xmlChar *)"instance"))){ 
         n++; 
         str = (PFCIRCUIT *)realloc((void *)str, n*sizeof(PFCIRCUIT)); 
	  
	   if(debugon)fprintf(dbgout,"parsePfCircuits#%d: %p\n", n, str); 
	   initPfCircuits(&str[n-1]); 
	  
// Attributes 
 
         if((att = xmlGetProp(cur, "id")) != NULL){ 
	      if(strlen(att) > 0) strcpy(str[n-1].id, att); 
	      if(debugon)fprintf(dbgout,"Pf Circuits ID: %s\n", str[n-1].id); 
	      xmlFree(att); 
	   }    
	 	  
         parseInstance(cur, &str[n-1].instance); 
 
// Child Tags	 
	  
         str[n-1].coil = parseIntArray(doc, cur, "coil_connect", &str[n-1].nco); 
	   parseInt(doc, cur, "supply_connect", &str[n-1].supply); 
	  
         if(debugon) printPfCircuits(dbgout, str[n-1]); 
      } 
      cur = cur->next; 
   } 
   *np = n;	// Number of Tags Found 
   return str; 
}	 
 
 
	 
// Plasma Current  
// Assume Single tag per document 
 
PLASMACURRENT *parsePlasmaCurrent(xmlDocPtr doc,xmlNodePtr cur,PLASMACURRENT *str){ 
 
   xmlChar *att;	// General Input of tag attribute values 
    
   cur = cur->xmlChildrenNode; 
   while(cur != NULL){ 
      if(debugon)fprintf(dbgout,"parsePlasmaCurrent: %s\n", (char *)cur->name);     
      if((!xmlStrcmp(cur->name, (const xmlChar *)"instance"))){ 
         str = (PLASMACURRENT *)realloc((void *)str, sizeof(PLASMACURRENT)); 
	  
	   if(debugon)fprintf(dbgout,"parsePlasmaCurrent# %p\n", str); 
	   initPlasmaCurrent(str); 
	  
// Attributes  
	 	 	  
	   parseInstance(cur, &(str->instance) ); 
 
// Child Tags	  
	    
	   parseFloat(doc, cur, "abs_error",  &str->aerr); 
	   parseFloat(doc, cur, "rel_error",  &str->rerr); 
 
         if(debugon) printPlasmaCurrent(dbgout, *str); 
      } 
      cur = cur->next; 
   } 
   return str; 
} 
	 
// Diamagnetic Flux  
// Assume Single tag per document 
 
DIAMAGNETIC *parseDiaMagnetic(xmlDocPtr doc,xmlNodePtr cur,DIAMAGNETIC *str){ 
 
   xmlChar *att;	// General Input of tag attribute values 
    
   cur = cur->xmlChildrenNode; 
   while(cur != NULL){ 
      if(debugon)fprintf(dbgout,"parseDiaMagnetic: %s\n", (char *)cur->name);     
      if((!xmlStrcmp(cur->name, (const xmlChar *)"instance"))){ 
         str = (DIAMAGNETIC *)realloc((void *)str, sizeof(DIAMAGNETIC)); 
	  
	   if(debugon)fprintf(dbgout,"parseDiaMagnetic# %p\n", str); 
	   initDiaMagnetic(str); 
	  
// Attributes  
	 	 	  
	   parseInstance(cur, &(str->instance) ); 
 
// Child Tags	  
	    
	   parseFloat(doc, cur, "abs_error",  &str->aerr); 
	   parseFloat(doc, cur, "rel_error",  &str->rerr); 
 
         if(debugon) printDiaMagnetic(dbgout, *str); 
      } 
      cur = cur->next; 
   } 
   return str; 
} 
 
// Toroidal Field 
// Assume Single tag per document 
 
TOROIDALFIELD *parseToroidalField(xmlDocPtr doc,xmlNodePtr cur,TOROIDALFIELD *str){ 
 
   xmlChar *att;	// General Input of tag attribute values 
    
   cur = cur->xmlChildrenNode; 
   while(cur != NULL){ 
      if(debugon)fprintf(dbgout,"parseToroidalField: %s\n", (char *)cur->name);     
      if((!xmlStrcmp(cur->name, (const xmlChar *)"instance"))){ 
         str = (TOROIDALFIELD *)realloc((void *)str, sizeof(TOROIDALFIELD)); 
	  
	   if(debugon)fprintf(dbgout,"parseToroidalField# %p\n", str); 
	   initToroidalField(str); 
	  
// Attributes  
	 	  
         parseInstance(cur, &(str->instance)); 
 
// Child Tags	  
	    
	   parseFloat(doc, cur, "abs_error",  &str->aerr); 
	   parseFloat(doc, cur, "rel_error",  &str->rerr); 
 
         if(debugon) printToroidalField(dbgout, *str); 
      } 
      cur = cur->next; 
   } 
   return str; 
}		      	      		      	       
	    
	 
// Limiter Data 
// Assume Single tag per document 
 
LIMITER *parseLimiter(xmlDocPtr doc,xmlNodePtr cur,LIMITER *str){ 
 
   int nco = 0; 
   xmlChar *att;	// General Input of tag attribute values 
    
   cur = cur->xmlChildrenNode; 
   while(cur != NULL){ 
      if(debugon)fprintf(dbgout,"parseLimiter: %s\n", (char *)cur->name);     
      if((!xmlStrcmp(cur->name, (const xmlChar *)"instance"))){ 
         str = (LIMITER *)realloc((void *)str, sizeof(LIMITER)); 
	  
	   if(debugon)fprintf(dbgout,"parseLimiter# %p\n", str); 
	   initLimiter(str); 
	  
// Attributes  
	 	 	 	 	 	  
	   if((att = xmlGetProp(cur, "factor")) != NULL){ 
	      if(strlen(att) > 0) str->factor = atof(att); 
	      if(debugon)fprintf(dbgout,"Limiter Coordinates Factor: %f\n", str->factor); 
	      xmlFree(att); 
	   }    
 
// Child Tags	  
	    
         str->r = (float *)parseFloatArray(doc, cur, "r", &str->nco); 
         str->z = (float *)parseFloatArray(doc, cur, "z", &nco); 
 
         if(debugon) printLimiter(dbgout, *str); 
      } 
      cur = cur->next; 
   } 
   return str; 
}		      	   
 
	    
	      	       
 
int parseEfitXML(char *xmlfile, EFIT *efit){ 
 
   int ninst; 
 
   xmlDocPtr  doc; 
   xmlNodePtr cur;   
       
   xmlChar *att;	// General Input of tag attribute values 
    
   if((doc = xmlParseFile(xmlfile)) == NULL){ 
      addIdamError(&idamerrorstack, CODEERRORTYPE, "parseHData", 1, "XML Not Parsed - Problem with File"); 
      return 1; 
   }	  
 
   if((cur = xmlDocGetRootElement(doc)) == NULL){       
      addIdamError(&idamerrorstack, CODEERRORTYPE, "parseHData", 1, "Empty XML Document"); 
      xmlFreeDoc(doc); 
      return 1; 
   }  
 
// Search for the <itm> or <device> tags 
 
   if(xmlStrcmp(cur->name, (const xmlChar *)"itm")){ 
      if(xmlStrcmp(cur->name, (const xmlChar *)"device")){          
         addIdamError(&idamerrorstack, CODEERRORTYPE, "parseHData", 1, "XML Document has neither an ITM nor a DEVICE tag"); 
         xmlFreeDoc(doc); 
	 return 1; 
      } else { 
         if((att = xmlGetProp(cur, "name")) != NULL){ 
	      if(strlen(att) > 0) strcpy(efit->device,(char *)att); 
	      if(debugon)fprintf(dbgout,"Device Name: %s\n", efit->device); 
	      xmlFree(att); 
	   }  
         if((att = xmlGetProp(cur, "pulse")) != NULL){ 
	      if(strlen(att) > 0) efit->exp_number = atoi((char *)att); 
	      if(debugon)fprintf(dbgout,"Pulse Number: %d\n", efit->exp_number); 
	      xmlFree(att); 
	   }      
      } 
   }     
 
   cur = cur->xmlChildrenNode; 
   while(cur != NULL){ 
    
      if(debugon)fprintf(dbgout,"parseHData: %s\n",(char *)cur->name); 
   
      if((!xmlStrcmp(cur->name, (const xmlChar *)"magprobes"))){  
                 
	 if((att = xmlGetProp(cur, "number")) != NULL){ 
	    if(strlen(att) > 0) efit->nmagprobes = atoi(att); 
	    if(debugon)fprintf(dbgout,"No. Mag Probes: %d\n", efit->nmagprobes); 
	    xmlFree(att); 
	 }    
	  
 	 ninst = 0; 
	 efit->magprobe = parseMagProbe(doc, cur, efit->magprobe, &ninst); 
	 if(ninst != efit->nmagprobes) { 
	    xmlFreeDoc(doc); 
            addIdamError(&idamerrorstack, CODEERRORTYPE, "parseHData", 1, "Inconsistent Number of Magnetic Probes"); 
            return 1; 
	 }    
  
      } 
            
      if((!xmlStrcmp(cur->name, (const xmlChar *)"flux"))){  
                 
	 if((att = xmlGetProp(cur, "number")) != NULL){ 
	    if(strlen(att) > 0) efit->nfluxloops = atoi(att); 
	    if(debugon)fprintf(dbgout,"No. Flux Loops: %d\n", efit->nfluxloops); 
	    xmlFree(att); 
	 }    
 	  
	 ninst = 0; 
	 efit->fluxloop = parseFluxLoop(doc, cur, efit->fluxloop, &ninst); 
	 if(ninst != efit->nfluxloops) { 
	    xmlFreeDoc(doc); 
            addIdamError(&idamerrorstack, CODEERRORTYPE, "parseHData", 1, "Inconsistent Number of Flux Loops"); 
            return 1; 
	 }  
  
      } 
       
                  
      if((!xmlStrcmp(cur->name, (const xmlChar *)"pfpassive"))){  
                 
	 if((att = xmlGetProp(cur, "number")) != NULL){ 
	    if(strlen(att) > 0) efit->npfpassive = atoi(att); 
	    if(debugon)fprintf(dbgout,"No. PF Passive Elements: %d\n", efit->npfpassive); 
	    xmlFree(att); 
	 }    
 	  
	 ninst = 0; 
	 efit->pfpassive = parsePfPassive(doc, cur, efit->pfpassive, &ninst); 
	 if(ninst != efit->npfpassive) { 
	    xmlFreeDoc(doc); 
            addIdamError(&idamerrorstack, CODEERRORTYPE, "parseHData", 1, "Inconsistent Number of PF Passive Elements"); 
            return 1; 
	 }  
  
      }       
                       
      if((!xmlStrcmp(cur->name, (const xmlChar *)"pfsupplies"))){  
                 
	 if((att = xmlGetProp(cur, "number")) != NULL){ 
	    if(strlen(att) > 0) efit->npfsupplies = atoi(att); 
	    if(debugon)fprintf(dbgout,"No. PF Supplies: %d\n", efit->npfsupplies); 
	    xmlFree(att); 
	 }    
 	  
	 ninst = 0; 
	 efit->pfsupplies = parsePfSupplies(doc, cur, efit->pfsupplies, &ninst); 
	 if(ninst != efit->npfsupplies) { 
	    xmlFreeDoc(doc); 
            addIdamError(&idamerrorstack, CODEERRORTYPE, "parseHData", 1, "Inconsistent Number of PF Supplies"); 
            return 1; 
	 }  
  
      } 
                                
      if((!xmlStrcmp(cur->name, (const xmlChar *)"pfcoils"))){  
                 
	 if((att = xmlGetProp(cur, "number")) != NULL){ 
	    if(strlen(att) > 0) efit->npfcoils = atoi(att); 
	    if(debugon)fprintf(dbgout,"No. PF Coils: %d\n", efit->npfcoils); 
	    xmlFree(att); 
	 }    
 	  
	 ninst = 0; 
	 efit->pfcoils = parsePfCoils(doc, cur, efit->pfcoils, &ninst); 
	 if(ninst != efit->npfcoils) { 
	    xmlFreeDoc(doc); 
            addIdamError(&idamerrorstack, CODEERRORTYPE, "parseHData", 1, "Inconsistent Number of PF Coils"); 
            return 1; 
	 }  
  
      } 
       
      if((!xmlStrcmp(cur->name, (const xmlChar *)"plasmacurrent"))){                  
	 efit->plasmacurrent  = parsePlasmaCurrent(doc, cur, efit->plasmacurrent); 
	 efit->nplasmacurrent = 1;	  
      } 
      
      if((!xmlStrcmp(cur->name, (const xmlChar *)"diamagneticflux"))){                  
	 efit->diamagnetic  = parseDiaMagnetic(doc, cur, efit->diamagnetic); 
	 efit->ndiamagnetic = 1;	  
      }       
       
      if((!xmlStrcmp(cur->name, (const xmlChar *)"toroidalfield"))){                  
	 efit->toroidalfield  = parseToroidalField(doc, cur, efit->toroidalfield); 
	 efit->ntoroidalfield = 1;	  
      } 
 
        
      if((!xmlStrcmp(cur->name, (const xmlChar *)"pfcircuits"))){  
                 
	 if((att = xmlGetProp(cur, "number")) != NULL){ 
	    if(strlen(att) > 0) efit->npfcircuits = atoi(att); 
	    if(debugon)fprintf(dbgout,"No. PF Circuits: %d\n", efit->npfcircuits); 
	    xmlFree(att); 
	 }    
  	  
	 ninst = 0; 
	 efit->pfcircuit = parsePfCircuits(doc, cur, efit->pfcircuit, &ninst); 
	 if(ninst != efit->npfcircuits) { 
	    xmlFreeDoc(doc); 
            addIdamError(&idamerrorstack, CODEERRORTYPE, "parseHData", 1, "Inconsistent Number of PF Circuits"); 
            return 1; 
	 }  
      }       
       
      if((!xmlStrcmp(cur->name, (const xmlChar *)"limiter"))){                 	  
	 efit->limiter  = parseLimiter(doc, cur, efit->limiter); 
	 efit->nlimiter = 1; 
      } 
         
      cur = cur->next; 
   } 
 
   xmlFreeDoc(doc); 
    
   return 0; 
} 

