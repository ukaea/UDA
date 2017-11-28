#include "makeServerRequestBlock.h"

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <strings.h>

#include <clientserver/udaErrors.h>
#include <clientserver/stringUtils.h>
#include <clientserver/initStructs.h>

// Deconstruct the text pass parameter for name valaue placeholder substitution 
// and for additional name-value pairs and keywords

// patterns	a, b, c					Name value substitution or keywords
//		name=a, name=b, name=c 			Additional Name Value Pairs
//		a,b,c, name=value, name=value, d, e	Name value pairs and keywords MUST be included after all substitution values

int nameValueSubstitution(NAMEVALUELIST* nameValueList, char *tpass)
{
   int i, err=0;

// Identify placeholder name-value pairs and additional name-values

   if(!tpass && strlen(tpass) == 0) return 0;	// nothing to substitute or no name value pairs
   
   NAMEVALUELIST newNameValueList;
   initNameValueList(&newNameValueList);
   unsigned short strip = 1;        // Remove enclosing quotes from name value pairs

   if (nameValuePairs(tpass, &newNameValueList, strip) == -1) {
      err = 999;
      addIdamError(CODEERRORTYPE, "nameValueSubstitution", err,
         "Name Value pair syntax is incorrect!");
      return err;
   }	 
  
// How many placeholders?
 
   int placeholderCount = 0;
   int* placeholderIndex = NULL;
   int* tpassIndex = NULL;
   
   if(nameValueList->pairCount > 0){	// Count how many Placeholders
      do{
         int* placeholderIndex = (int *)malloc(nameValueList->pairCount*sizeof(int));
         int* tpassIndex = (int *)malloc(nameValueList->pairCount*sizeof(int));
   
         for (i = 0; i < nameValueList->pairCount; i++)
         {
            if(nameValueList->nameValue[i].value != NULL && nameValueList->nameValue[i].value[0] == '$'){		// Is it a placeholder? (Not a keyword and begiining '$')
               placeholderIndex[placeholderCount] = i;									// Identify which pair
               tpassIndex[placeholderCount] = placeholderCount;								// Ordering: Default substitution value to use
               if(nameValueList->nameValue[i].value[1] != '\0' && IsNumber(&nameValueList->nameValue[i].value[1]))	// Is the placeholder numbered? 
                  tpassIndex[placeholderCount] = atoi(&nameValueList->nameValue[i].value[1])-1;				// Use a specific substitution value 
               placeholderCount++;
	       UDA_LOG(UDA_LOG_DEBUG, "[%d] value: %s, placeholderCount %d\n", i, nameValueList->nameValue[i].value, placeholderCount);   
            } else 
	       UDA_LOG(UDA_LOG_DEBUG, "[%d] placeholderCount %d\n", i, placeholderCount);     
         }
   
         if(placeholderCount == 0) break;		// There are no placeholders to process - add new name-value pairs and keywords

// Placeholder substitutions (must be the specified first in tpass)

         if(placeholderCount > newNameValueList.pairCount){	// Too many placeholders for the available substitutions
            err = 999;
	    //char work[1024];
	    //sprintf(work, "Inconsistent count of placeholders [%d] and available substitutions [%d]!\ntpass: %s", placeholderCount, newNameValueList.pairCount, tpass); 
	    //addIdamError(CODEERRORTYPE, "nameValueSubstitution", err, work);
	    addIdamError(CODEERRORTYPE, "nameValueSubstitution", err, "Inconsistent count of placeholders and available substitutions!\n");
            break;
	 }

	 for (i = 0; i < placeholderCount; i++){
	    if(tpassIndex[i] < 0 || tpassIndex[i] > placeholderCount){
               err = 999;
	       addIdamError(CODEERRORTYPE, "nameValueSubstitution", err, "Placeholder numbering is Inconsistent with Placeholder Count!\n");
	       break;
	    }
	     
            nameValueList->nameValue[placeholderIndex[i]].value = newNameValueList.nameValue[tpassIndex[i]].value;
	    
	    UDA_LOG(UDA_LOG_DEBUG, "Placeholder: [%d] %s, Substitution Value %s\n", i, nameValueList->nameValue[placeholderIndex[i]].name,
	    	 nameValueList->nameValue[placeholderIndex[i]].value); 		
	 }       
      } while(0);
   }   
   
   if(tpassIndex) free(tpassIndex);
   if(placeholderIndex) free(placeholderIndex);
   
   if(err != 0) return err;
   
// How many new name value pairs and keywords?

   int count = newNameValueList.pairCount - placeholderCount;
   if(count == 0) return 0;
   
// Add new name value pairs and keywords

   if (nameValueList->pairCount + count > nameValueList->listSize) {
      nameValueList->nameValue = (NAMEVALUE*) realloc((void*) nameValueList->nameValue,
                                                      (nameValueList->listSize + count) * sizeof(NAMEVALUE));
      nameValueList->listSize = nameValueList->listSize + count;
   }
   
   for (i = 0; i < count; i++){ 
      nameValueList->nameValue[nameValueList->pairCount  ].name  = newNameValueList.nameValue[placeholderCount+i].name; 		
      nameValueList->nameValue[nameValueList->pairCount++].value = newNameValueList.nameValue[placeholderCount+i].value; 		
   }
   
   return 0;
}
