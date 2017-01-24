/** @file */
//---------------------------------------------------------------------------------------------
// User Accessor functions to General Data Structures

//---------------------------------------------------------------------------------------------
// Locating Data: The whole sub-tree is in scope
//
// Two types: 
// A) Return the Tree Node containing a Data Structure with a named structure element. This may be
//    either a structure itself or an atomic typed element.
//
// B) Return the Tree Node containing a named Data Structure (node will be a child of the node located using A). 
//    The hierarchical name must consist only of structure types. 
//
// Hierarchical names can have one of two forms: a.b.c or a/b/c. The latter can also begin with an optional '/'.  
//
//
// 
//

/** Find (search type A) and return a Pointer to the Data Tree Node with a data structure that contains a named 
* Structure element. The name of the element is also returned.  
*
* This is a private function with the whole sub-tree in scope.
*
* @param ntree A pointer to a parent tree node. If NULL the root node is assumed. 
* @param target The name of the Structure element (case sensitive) using a hierachical naming syntax a.b.c or a/b/c.
* This element may be either a structure itself or an atomic typed element.
* @param lastname Returns the name of the Structure element, i.e., the name of the last node in the name hierarchy.
* @return the Data Tree Node containing the named Structure element.
*/ 

NTREE *findNTreeStructureComponent2(NTREE *ntree, char *target, char **lastname){
   int i;
   NTREE *child = NULL;
   char *p;
   
   if(ntree == NULL) ntree = fullNTree;

// Is the hierarchical name of the form: a.b.c or a/b/c

   if(((p=strchr(target,'.')) != NULL) || (p=strchr(target,'/')) != NULL){
      int ntargets;
      char **targetlist = NULL;
      child = ntree;
      
      targetlist = parseTarget(target, &ntargets);	// Deconstruct the Name and search for each hierarchy group
      
      for(i=0;i<ntargets;i++){				// Drill Down to requested named structure element
 
         if(i<ntargets-1){
	    child = findNTreeStructure2(child, targetlist[i], lastname); 		// Tree Branch Nodes
	 } else {
	    child = findNTreeStructureComponent2(child, targetlist[i], lastname);	// Confirm target is a structure element
	 }

         if(child == NULL){
	        addIdamError(&idamerrorstack, CODEERRORTYPE, "findNTreeStructureComponent2", 999, 
		"Unable to locate a data tree branch with a specific structure element!");
	     child = NULL;
	     break;
         }
      }
            
      *lastname = targetlist[ntargets-1];			// Preserve the last element name
      
      addMalloc((void *)targetlist[ntargets-1], strlen(targetlist[ntargets-1])+1, sizeof(char), "char");			
      for(i=0;i<ntargets-1;i++) free((void *)targetlist[i]);	// Free all others
      free((void *)targetlist);					// Free the list

      return(child);	// Always the last node you look in !
   }

// Search using a single hierarchical group name

   *lastname = target;

   for(i=0;i<ntree->userdefinedtype->fieldcount;i++){	       
      if(!strcmp(ntree->userdefinedtype->compoundfield[i].name, target)) return(ntree);
   }
   
// Search Child nodes

   for(i=0;i<ntree->branches;i++){
      if((child = findNTreeStructureComponent(ntree->children[i], target)) != NULL) return(child);
   }
   
   addIdamError(&idamerrorstack, CODEERRORTYPE, "findNTreeStructureComponent2", 999, 
		"Unable to locate a data tree branch with a specific structure element!");

   return NULL;
}

/** Find (search type B) and return a Pointer to the named Data Tree Node with a data structure of the same name.
* The name of the structure is also returned.  
*
* This is a private function with the whole sub-tree in scope.
*
* @param ntree A pointer to a parent tree node. If NULL the root node is assumed. 
* @param target The name of the Structure (case sensitive) using a hierachical naming syntax a.b.c or a/b/c.
* @param lastname Returns the name of the Structure, i.e., the name of the last node in the name hierarchy.
* @return the Data Tree Node with the structure name.
*/ 

NTREE *findNTreeStructure2(NTREE *ntree, char *target, char **lastname){
   int i;
   NTREE *child = NULL;
   NTREE *test  = NULL;
   char *p;
   if(ntree == NULL) ntree = fullNTree;

// Is the hierarchical name of the form: a.b.c or a/b/c

   if(((p=strchr(target,'.')) != NULL) || (p=strchr(target,'/')) != NULL){
      int ntargets;
      char **targetlist = NULL;
      child = ntree;
      
      targetlist = parseTarget(target, &ntargets);	// Deconstruct Name and search for each hierarchy group
      
      for(i=0;i<ntargets;i++){				// Drill Down to requested named structure element
         if(i<ntargets-1){
	    child = findNTreeStructure2(child, targetlist[i], lastname);
	 } else {
	    if((test = findNTreeStructure2(child, targetlist[i], lastname)) == NULL){	// Last element may not be a structure
	       if((test = findNTreeStructureComponent2(child, targetlist[i], lastname)) == NULL) child = NULL;   
	    } else {
	       child = test;
	    }	 
	 }
	 if(child == NULL){
             addIdamError(&idamerrorstack, CODEERRORTYPE, "findNTreeStructure2", 999, "Unable to locate a named structure data tree branch!");
	     child = NULL;
	     break;
         }
      }
            
      *lastname = targetlist[ntargets-1];			// Preserve the last element name
      
      addMalloc((void *)targetlist[ntargets-1], strlen(targetlist[ntargets-1])+1, sizeof(char), "char");			
      for(i=0;i<ntargets-1;i++) free((void *)targetlist[i]);	// Free all others
      free((void *)targetlist);					// Free the list
      
      return(child);	// Always the last node you look in !
   }

// Search using a single hierarchical group name

   *lastname = target;

   if(!strcmp(ntree->name, target)) return(ntree);		// Test the current Tree Node
   
// Search Child nodes

   for(i=0;i<ntree->branches;i++){
      if(!strcmp(ntree->children[i]->name, target)) return(ntree->children[i]);
   }
   
   addIdamError(&idamerrorstack, CODEERRORTYPE, "findNTreeStructure2", 999, "Unable to locate a named structure data tree branch!");

   return NULL;
}

/** Find (search type A) and return a Pointer to the Data Tree Node with a data structure that contains a named element. 
*
* This is a public function with the whole sub-tree in scope.
*
* @param ntree A pointer to a parent tree node. If NULL the root node is assumed. 
* @param target The name of the structure element (case sensitive) using a hierachical naming syntax a.b.c or a/b/c.
* This element may be either a structure itself or an atomic typed element.
* @return the Data Tree Node.
*/ 

NTREE *findNTreeStructureComponent(NTREE *ntree, char *target){
   char *lastname;
   return(findNTreeStructureComponent2(ntree, target, &lastname));
}

/** Find (search type B) and return a Pointer to the named Data Tree Node with a data structure of the same name.
*
* This is a public function with the whole sub-tree in scope.
*
* @param ntree A pointer to a parent tree node. If NULL the root node is assumed. 
* @param target The name of the Structure (case sensitive) using a hierachical naming syntax a.b.c or a/b/c.
* @return the Data Tree Node.
*/ 

NTREE *findNTreeStructure(NTREE *ntree, char *target){
   char *lastname;
   return(findNTreeStructure2(ntree, target, &lastname));
}


/** Find and return a Pointer to a Data Tree Node with a data structure located at a specific memory location.
*
* This is a public function with the whole sub-tree in scope.
*
* @param ntree A pointer to a parent tree node. If NULL the root node is assumed. 
* @param data The heap address of the data.
* @return the Data Tree Node.
*/ 

NTREE *findNTreeStructureMalloc(NTREE *ntree, void *data){
   int i;
   NTREE *next;
   if(ntree == NULL) ntree = fullNTree; 
   if(data == ntree->data) return ntree;
   for(i=0;i<ntree->branches;i++) if((next = findNTreeStructureMalloc(ntree->children[i], data)) != NULL) return(next);
   return NULL;
}

/** Locate a tree node with structured data having the specified Structure Definition name. 
*
* This is a public function with the whole sub-tree in scope.
*
* @param tree A pointer to a parent tree node. If NULL the root node is assumed.  
* @param target The name of the Structure Definition.
* @return A pointer to the First tree node found with the targeted structure definition.
*/

NTREE *findNTreeStructureDefinition(NTREE *tree, char *target){
   int i;
   NTREE *next;
   if(tree == NULL) tree = fullNTree;
   if(!strcmp(tree->userdefinedtype->name, target)) return(tree);
   for(i=0;i<tree->branches;i++) if((next = findNTreeStructureDefinition(tree->children[i], target)) != NULL) return(next);
   return NULL;
}


/** Locate a tree node with structured data having the specified Structure Definition name. 
*
* This is a public function with the whole sub-tree in scope.
*
* @param tree A pointer to a parent tree node. If NULL the root node is assumed.  
* @param target The name of the Structure Definition.
* @return A pointer to the First tree node found with the targeted structure definition.
*/

NTREE *findNTreeStructureComponentDefinition(NTREE *tree, char *target){
   int i;
   NTREE *next;
   if(tree == NULL) tree = fullNTree;
   for(i=0;i<tree->userdefinedtype->fieldcount;i++)
      if(tree->userdefinedtype->compoundfield[i].atomictype == TYPE_UNKNOWN && !strcmp(tree->userdefinedtype->compoundfield[i].type, target)) return(tree);
   for(i=0;i<tree->branches;i++) if((next = findNTreeStructureComponentDefinition(tree->children[i], target)) != NULL) return(next);
   return NULL;
}


/** Locate a tree node with structured data having a Specific Structure Class. 
*
* This is a public function with the whole sub-tree in scope.
*
* @param tree A pointer to a parent tree node. If NULL the root node is assumed.  
* @param class The Structure Class, e.g., TYPE_VLEN.
* @return A pointer to the First tree node found with the targeted structure class.
*/

NTREE *idam_findNTreeStructureClass(NTREE *tree, int class){
   int i;
   NTREE *next;
   if(tree == NULL) tree = fullNTree;
   if(tree->userdefinedtype->idamclass == class) return(tree);
   for(i=0;i<tree->branches;i++) if((next = idam_findNTreeStructureClass(tree->children[i], class)) != NULL) return(next);
   return NULL;
}


/** Identify the largest count of a Variable Length Array with a given structure type. 
*
* This is a public function with the whole sub-tree in scope.
*
* @param tree A pointer to a parent tree node. If NULL the root node is assumed.  
* @param target The name of the VLEN Structure Definition.
* @return An integer returning the maximum count value.
*/

int idam_maxCountVlenStructureArray(NTREE *tree, char *target, int reset){

   struct VLENTYPE {
      unsigned int len;
      void *data;
   };
   typedef struct VLENTYPE VLENTYPE ; 

   int i;
   static int count = 0;
   NTREE *next;
   if(reset) count = 0;
   if(tree == NULL) tree = fullNTree;
   if(tree->userdefinedtype->idamclass == TYPE_VLEN && !strcmp(tree->userdefinedtype->name, target)){
      VLENTYPE *vlen = (VLENTYPE *)tree->data;
      if(vlen->len > count) count = vlen->len;
   }
   for(i=0;i<tree->branches;i++) count = idam_maxCountVlenStructureArray(tree->children[i], target, 0);
   return(count);
}


/** Regularise a specific VLEN structure. 
*
* This is a public function with the whole sub-tree in scope.
*
* @param tree A pointer to a parent tree node. If NULL the root node is assumed.  
* @param target The name of the VLEN Structure Definition.
* @param count The maximum count size for the VLEN data arrays.
* @return An integer returning an error code: 0 => OK.
*/

int idam_regulariseVlenStructures(NTREE *tree, char *target, int count){
/* 
   struct pfcoilType {
      char *name[1];
      char *turns[1];
      char *polarity[1];
   };
   typedef struct pfcoilType pfcoilType ;
*/   
   
   struct VLENTYPE {
      unsigned int len;
      void *data;
   };
   typedef struct VLENTYPE VLENTYPE ; 

   int i, rc=0, size=0,resetBranches=0;
   void *old=NULL, *newnew=NULL;
   NTREE *next;
   if(tree == NULL) tree = fullNTree;
   if(tree->userdefinedtype->idamclass == TYPE_VLEN && !strcmp(tree->userdefinedtype->name, target)){
      VLENTYPE *vlen = (VLENTYPE *)tree->data;

//pfcoilType *x = (pfcoilType *)vlen->data; 

// VLEN stuctures have only two fields: len and data
// Need the size of the data component

      if(vlen->len == 0)    return 1;			// No data!
      if(vlen->len > count) return 1;			// Incorrect count value
      
// VLEN Memory is contiguous so re-allocate: regularise by expanding to a consistent array size (No longer a VLEN!)       

      old = (void *)vlen->data;
      USERDEFINEDTYPE *child = findUserDefinedType(tree->userdefinedtype->compoundfield[1].type, 0);
      vlen->data = (void *)realloc((void *)vlen->data, count*child->size);				// Expand Heap to regularise
      newnew = (void *)vlen->data;
      size   = child->size;      
      changeMalloc(old, vlen->data, count, child->size, child->name);
      tree->data = (void *)vlen;		

// Write new data array to Original Tree Nodes

      for(i=0;i<vlen->len;i++) tree->children[i]->data = newnew+i*size;

//vlen = (VLENTYPE *)tree->data;      
//pfcoilType *X = (pfcoilType *)vlen->data;      
//for(i=0;i<count;i++){
//pfcoilType *Y = (pfcoilType *)tree->children[i]->data;
//rc = 0;
//}      

      resetBranches=vlen->len;		// Flag requirement to add extra tree nodes       
 
   }
   for(i=0;i<tree->branches;i++) if((rc = idam_regulariseVlenStructures(tree->children[i], target, count)) != 0) return rc;

// Update branch count and add new Child nodes with New data array
   
   if(resetBranches > 0){
      tree->branches = count; 		// Only update once all True children have been regularised 
      old = (void *)tree->children;
      tree->children = (NTREE **)realloc((void *)tree->children, count*sizeof(void *));
      for(i=resetBranches;i<count;i++){
         tree->children[i] = (NTREE *)malloc(sizeof(NTREE));
	 addMalloc((void *)tree->children[i], 1, sizeof(NTREE), "NTREE");
         memcpy(tree->children[i], tree->children[0], sizeof(NTREE)); 
      }	 
      changeMalloc(old, (void *)tree->children, count, sizeof(NTREE), "NTREE");

// Update All new Child Nodes with array element addresses

      for(i=resetBranches;i<count;i++) memcpy(newnew+i*size, newnew, size);			// write extra array items: use the first array element 
      for(i=resetBranches;i<count;i++) tree->children[i]->data = newnew+i*size;
      
   }
   
      
   return rc;
}


/** Regularise the Shape of All VLEN structured data arrays in the data tree: necessary for accessing in some languages, e.g. IDL. 
*
* This is a public function with the whole sub-tree in scope.
*
* @param tree A pointer to a parent tree node. If NULL the root node is assumed.  
* @return An integer returning an error code: 0 => OK.
*/

int idam_regulariseVlenData(NTREE *tree){
   int rc = 0, count = 0;
   NTREE *nt = NULL;
   if(tree == NULL) tree = fullNTree;
   do{
      if((nt = idam_findNTreeStructureClass(tree, TYPE_VLEN)) != NULL){
         count = idam_maxCountVlenStructureArray(tree, nt->userdefinedtype->name, 1); 
         //printf("max count = %d\n", count); 
         if(count > 0) rc = idam_regulariseVlenStructures(tree, nt->userdefinedtype->name, count);
	 if(rc != 0) return rc;
         nt->userdefinedtype->idamclass = TYPE_COMPOUND;   // Change the class to 'regular compound structure'   
      } 
   } while(nt != NULL);
   return 0;
}



//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
// User Accessor functions to Node Data (only the current node is in scope)   

/** Return the Count of data array elements attached to this tree node. 
*
* This is a public function with the current tree node only in scope.
*
* @param ntree A pointer to a tree node. If NULL the root node is assumed. 
* @return the Count of structured data array elements.
*/ 

int getNodeStructureDataCount(NTREE *ntree){
   int count, size;
   char *type;
   if(ntree == NULL) ntree = fullNTree;   
   findMalloc((void *)&ntree->data, &count, &size, &type);   
   return count;
}

/** Return the Size (bytes) of the structured data array attached to this tree node. 
*
* This is a public function with the current tree node only in scope.
*
* @param ntree A pointer to a tree node. If NULL the root node is assumed. 
* @return the Size (bytes) of the structured data array.
*/ 

int getNodeStructureDataSize(NTREE *ntree){
   int count, size;
   char *type;
   if(ntree == NULL) ntree = fullNTree;
   findMalloc((void *)&ntree->data, &count, &size, &type);   
   return size;
}

/** Return the rank of the structured data array attached to this tree node. 
*
* This is a public function with the current tree node only in scope.
*
* @param ntree A pointer to a tree node. If NULL the root node is assumed. 
* @return The rank of the structured data array.
*/ 

int getNodeStructureDataRank(NTREE *ntree){
   int count, size, rank;
   int *shape;
   char *type;
   if(ntree == NULL) ntree = fullNTree;
   findMalloc2((void *)&ntree->data, &count, &size, &type, &rank, &shape);   
   return rank;
}

/** Return the shape of the structured data array attached to this tree node. 
*
* This is a public function with the current tree node only in scope.
*
* @param ntree A pointer to a tree node. If NULL the root node is assumed. 
* @return A pointer to the integer shape array of the structured data array.
*/ 

int *getNodeStructureDataShape(NTREE *ntree){
   int count, size, rank;
   int *shape;
   char *type;
   if(ntree == NULL) ntree = fullNTree;
   findMalloc2((void *)&ntree->data, &count, &size, &type, &rank, &shape);   
   return shape;
}

/** Return a pointer to the structured data type name of the data array attached to this tree node. 
*
* This is a public function with the current tree node only in scope.
*
* @param ntree A pointer to a tree node. If NULL the root node is assumed. 
* @return the data type name of the structured data array.
*/ 

char *getNodeStructureDataDataType(NTREE *ntree){
   int count, size;
   char *type=NULL;
   if(ntree == NULL) ntree = fullNTree;
   findMalloc((void *)&ntree->data, &count, &size, &type);   
   return type;   
}

 
/** Return a pointer to the data attached to this tree node.  
*
* This is a public function with the current tree node only in scope.
*
* @param ntree A pointer to a tree node. If NULL the root node is assumed. 
* @return A void pointer to the data .
*/ 

void *getNodeStructureData(NTREE *ntree){
   if(ntree == NULL) ntree = fullNTree;   
   return(void *) ntree->data;   
}


//----------------------------------------------------------------------------------------------------------
// Sundry utility functions

/** Print the Contents of a tree node to a specified File Descriptor. The arguments for this function are available from the Structure
* Definition structure. 
*
* This is a public function with the current tree node only in scope.
*
* @param fd The File Descriptor, e.g., stdout
* @param image A text block containing null terminated strings that forms an text image of the structure definition.
* @param imagecount The number of bytes in the image text block.
* @return Void
*/

void printImage(FILE *fd, char *image, int imagecount){ 
   int next = 0;
   if(image == NULL || imagecount == '\0') return;
   while(next < imagecount){
      fprintf(fd, "%s", &image[next]);
      next = next + strlen(&image[next])+1;
   }       
}   
