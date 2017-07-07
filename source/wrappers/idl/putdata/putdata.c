//! $LastChangedRevision: 87 $
//! $LastChangedDate: 2008-12-19 15:34:15 +0000 (Fri, 19 Dec 2008) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/idl/putdata.c $

// putdata.c
//
// IDL DLM C Code Wrapper to netCDF4 API functions for writing netCDF4 data files
//
// Change Control
//
// To DO.... 
//
// States
//
//	Create		File must be closed
//	Update		File can be either Open or Closed
//	Close		File must be Open
//	Delete		File must be Open
//
// 30Jan2009	DGMuir	Original Version
// 19May2009	DGMuir	Added compression and chunksize
//			Changed open to create
//			Added errors, removed filename
// 28May2009	DGMuir	Compliance Tests added
//-------------------------------------------------------------------------------------------------------

#include "putdata.h"		// IDL API Header

#include <clientserver/stringUtils.h>
#include "comp.c"
   
// Function Prototypes
 
void userhelp(FILE *fh, char *name);
   
extern void putdata_exit_handler(void);
extern int  putdata_Startup(void);
      
extern IDL_VPTR IDL_CDECL putdata(  int argc, IDL_VPTR argv[], char *argk);
   
static IDL_SYSFUN_DEF2 putdata_functions[] = {
  {{(IDL_FUN_RET) putdata}, "PUTDATA", 0,4,IDL_SYSFUN_DEF_F_KEYWORDS,0}
};

int putdata_startup(void){ 

// TRUE for Functions, FALSE for Procedures
 
    if (!IDL_SysRtnAdd(putdata_functions, TRUE, ARRLEN(putdata_functions))) {return IDL_FALSE;}

// Register the exit handler 

    IDL_ExitRegister(putdata_exit_handler);
    
    return(IDL_TRUE);
}

int IDL_Load(void){ 
    if (!IDL_SysRtnAdd(putdata_functions, TRUE, ARRLEN(putdata_functions))) {return IDL_FALSE;}
    return(IDL_TRUE);
}

// Called when IDL is shutdown
 
void putdata_exit_handler(void){
   // Nothing to do!
}

void freeMem(UCHAR *memPtr){
   free((void *)memPtr);
}  


void initKW(KW_RESULT *kw){
      
   kw->create     = 0;
   kw->close      = 0;
   kw->update     = 0;
   kw->delete     = 0;
   
   kw->notstrict    = 0;
   kw->nocompliance = 0;

   kw->verbose    = 0;
   kw->debug      = 0; 
   kw->fileid	  = 0;
   kw->length	  = 0;
   kw->resolution = 0;
   kw->channels   = 0;
   kw->channel    = 0;
   kw->compression= 0;
   kw->chunksize  = 0;
   kw->scale	  = 0.0;
   kw->offset	  = 0.0;
   kw->range[0]   = 0.0;
   kw->range[1]   = 0.0;
   
   kw->pass       = 0;

   
   kw->is_dimensions = 0;   
   kw->is_stepId     = 0;   
   kw->is_group      = 0;   
   kw->is_label      = 0;   
   kw->is_name       = 0;   
   kw->is_errors     = 0;   
   kw->is_units      = 0; 
   kw->is_length     = 0;
   kw->is_device     = 0;
   kw->is_type       = 0;
   kw->is_serial     = 0;
   kw->is_resolution = 0;
   kw->is_range      = 0;
   kw->is_channel    = 0;
   kw->is_channels   = 0;
   kw->is_scale      = 0;
   kw->is_offset     = 0; 
   kw->is_varname    = 0;
   kw->is_compression= 0;
   kw->is_chunksize  = 0;
     
   kw->is_conventions= 0;
   kw->is_code       = 0;   
   kw->is_date       = 0;   
   kw->is_exp_number = 0;   
   kw->is_fileid     = 0;   
   kw->is_format     = 0;   
   kw->is_directory  = 0;   
   kw->is_pass       = 0;   
   kw->is_status     = 0;   
   kw->is_time       = 0;   
   kw->is_version    = 0;   
   kw->is_xml	     = 0;
   kw->is_class	     = 0;
   kw->is_title	     = 0;
   kw->is_comment    = 0;  
   
   return;
}    

void printKW(FILE *fd, KW_RESULT kw){

   fprintf(fd,"debug?   %d\n", (int)kw.debug);
   fprintf(fd,"verbose? %d\n", (int)kw.verbose);

   fprintf(fd,"create?  %d\n", (int)kw.create);
   fprintf(fd,"close?   %d\n", (int)kw.close);
   fprintf(fd,"update?  %d\n", (int)kw.update);
   fprintf(fd,"delete?  %d\n", (int)kw.delete);
   
   fprintf(fd,"Not Strict?             %d\n", (int)kw.notstrict);
   fprintf(fd,"No Compliance Testing?  %d\n", (int)kw.nocompliance);
   
   if(kw.is_exp_number)  fprintf(fd,"exp_number  %d\n", (int)kw.exp_number);
   if(kw.is_pass)	 fprintf(fd,"pass        %d\n", (int)kw.pass);
   if(kw.is_status)      fprintf(fd,"status      %d\n", (int)kw.status);
   if(kw.is_version)     fprintf(fd,"version     %d\n", (int)kw.version);
   if(kw.is_length)      fprintf(fd,"length      %d\n", (int)kw.length);
   if(kw.unlimited)      fprintf(fd,"unlimited?  %d\n", (int)kw.unlimited);
   if(kw.is_channels)    fprintf(fd,"channels    %d\n", (int)kw.channels);
   if(kw.is_channel)     fprintf(fd,"channel     %d\n", (int)kw.channel);
   if(kw.is_resolution)  fprintf(fd,"resolution  %d\n", (int)kw.resolution);
   if(kw.is_compression) fprintf(fd,"compression %d\n", (int)kw.compression);
   if(kw.is_chunksize)   fprintf(fd,"chunksize   %d\n", (int)kw.chunksize);
   if(kw.is_scale)       fprintf(fd,"scale       %f\n", (float)kw.scale);
   if(kw.is_offset)      fprintf(fd,"offset      %f\n", (float)kw.offset);
   if(kw.is_range)       fprintf(fd,"range       %f : %f \n", (float)kw.range[0],(float)kw.range[1]);
   if(kw.is_fileid)      fprintf(fd,"fileId      %d\n", (int)IDL_LongScalar(kw.fileid));
    
   if(kw.is_conventions) fprintf(fd,"conventions %s\n", IDL_STRING_STR(&kw.conventions));
   if(kw.is_class)       fprintf(fd,"class       %s\n", IDL_STRING_STR(&kw.class));
   if(kw.is_title)       fprintf(fd,"title       %s\n", IDL_STRING_STR(&kw.title));
   if(kw.is_comment)     fprintf(fd,"comment     %s\n", IDL_STRING_STR(&kw.comment));
   if(kw.is_code)	 fprintf(fd,"code        %s\n", IDL_STRING_STR(&kw.code));
   if(kw.is_format)      fprintf(fd,"format      %s\n", IDL_STRING_STR(&kw.format));
   if(kw.is_directory)   fprintf(fd,"directory   %s\n", IDL_STRING_STR(&kw.directory));
   if(kw.is_date)	 fprintf(fd,"date        %s\n", IDL_STRING_STR(&kw.date));
   if(kw.is_time)	 fprintf(fd,"time        %s\n", IDL_STRING_STR(&kw.time));
   if(kw.is_xml)	 fprintf(fd,"xml         %s\n", IDL_STRING_STR(&kw.xml));
   if(kw.is_stepId)	 fprintf(fd,"stepId      %s\n", IDL_STRING_STR(&kw.stepId));

   if(kw.is_group)	 fprintf(fd,"group       %s\n", IDL_STRING_STR(&kw.group));
   if(kw.is_name)	 fprintf(fd,"name        %s\n", IDL_STRING_STR(&kw.name));
   if(kw.is_errors)	 fprintf(fd,"errors      %s\n", IDL_STRING_STR(&kw.errors));
   if(kw.is_dimensions)	 fprintf(fd,"dimensions  %s\n", IDL_STRING_STR(&kw.dimensions));
   if(kw.is_device)	 fprintf(fd,"device      %s\n", IDL_STRING_STR(&kw.device));
   if(kw.is_type)	 fprintf(fd,"type        %s\n", IDL_STRING_STR(&kw.type));
   if(kw.is_id)	         fprintf(fd,"id          %s\n", IDL_STRING_STR(&kw.id));
   if(kw.is_serial)	 fprintf(fd,"serial      %s\n", IDL_STRING_STR(&kw.serial));
   if(kw.is_varname)	 fprintf(fd,"varname     %s\n", IDL_STRING_STR(&kw.varname));
   if(kw.is_units)	 fprintf(fd,"units       %s\n", IDL_STRING_STR(&kw.units));
   if(kw.is_label)	 fprintf(fd,"label       %s\n", IDL_STRING_STR(&kw.label));
			
   return;
}


//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
// API Function


IDL_VPTR IDL_CDECL putdata(int argc, IDL_VPTR argv[], char *argk){   
//
//-------------------------------------------------------------------------
// Change History:
//
// 30Jan2009 DGMuir	Original Version
//-------------------------------------------------------------------------	      

   int i, nparams, fileid, err;
   int ncgrpid;
   char *stepId;
      
//---------------------------------------------------------------------------            
// Maintain Alphabetical Order of Keywords
// Keywords are IDL LONG, IDL DOUBLE and IDL_STRING types only.


   static IDL_KW_ARR_DESC_R rangeDesc = {IDL_KW_OFFSETOF(range), 2,2,IDL_KW_OFFSETOF(rangeCount)};
 
   static IDL_KW_PAR kw_pars[] = 
   {IDL_KW_FAST_SCAN,   
     //{"CHANNEL",	IDL_TYP_LONG,    1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_channel),		IDL_KW_OFFSETOF(channel)},
     {"CHANNELS",	IDL_TYP_LONG,    1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_channels),		IDL_KW_OFFSETOF(channels)},
     {"CHUNKSIZE",	IDL_TYP_LONG,    1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_chunksize),		IDL_KW_OFFSETOF(chunksize)},
     {"CLASS",		IDL_TYP_STRING,  1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_class),		IDL_KW_OFFSETOF(class)},
     {"CLOSE",		IDL_TYP_LONG,    0, IDL_KW_ZERO, 0, 					IDL_KW_OFFSETOF(close)},
     {"CODE",		IDL_TYP_STRING,  1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_code),		IDL_KW_OFFSETOF(code)},
     {"COMMENT",	IDL_TYP_STRING,  1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_comment),		IDL_KW_OFFSETOF(comment)},
     {"COMPRESSION",	IDL_TYP_LONG,    1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_compression),	IDL_KW_OFFSETOF(compression)},
     {"CONVENTIONS",    IDL_TYP_STRING,  1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_conventions),	IDL_KW_OFFSETOF(conventions)},
     {"CREATE",		IDL_TYP_LONG,    0, IDL_KW_ZERO, 0, 					IDL_KW_OFFSETOF(create)},
     {"DATE",		IDL_TYP_STRING,  1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_date),		IDL_KW_OFFSETOF(date)},
     {"DEBUG",		IDL_TYP_LONG,    1, IDL_KW_ZERO, 0, 					IDL_KW_OFFSETOF(debug)},
     {"DELETE",		IDL_TYP_LONG,    0, IDL_KW_ZERO, 0, 					IDL_KW_OFFSETOF(delete)},
     {"DEVICE",		IDL_TYP_STRING,  1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_device),		IDL_KW_OFFSETOF(device)},
     {"DIMENSIONS",	IDL_TYP_STRING,  1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_dimensions),	IDL_KW_OFFSETOF(dimensions)},
     {"DIRECTORY",	IDL_TYP_STRING,  1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_directory),		IDL_KW_OFFSETOF(directory)},
     {"ERRORS",		IDL_TYP_STRING,  1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_errors),		IDL_KW_OFFSETOF(errors)},
     {"EXP_NUMBER",	IDL_TYP_LONG,    1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_exp_number),        IDL_KW_OFFSETOF(exp_number)},
     {"FILEID",		IDL_TYP_UNDEF,   1, IDL_KW_OUT|IDL_KW_ZERO, IDL_KW_OFFSETOF(is_fileid),	IDL_KW_OFFSETOF(fileid)},
     {"FORMAT",		IDL_TYP_STRING,  1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_format),		IDL_KW_OFFSETOF(format)},
     {"GROUP",		IDL_TYP_STRING,  1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_group),		IDL_KW_OFFSETOF(group)},
     {"ID",		IDL_TYP_STRING,  1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_id),		IDL_KW_OFFSETOF(id)},
     {"LABEL",		IDL_TYP_STRING,  1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_label),		IDL_KW_OFFSETOF(label)},
     {"LENGTH",		IDL_TYP_LONG,    1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_length),		IDL_KW_OFFSETOF(length)},
     {"NAME",		IDL_TYP_STRING,  1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_name),		IDL_KW_OFFSETOF(name)},
     {"NOCOMPLIANCE",	IDL_TYP_LONG,    1, IDL_KW_ZERO, 0, 					IDL_KW_OFFSETOF(nocompliance)},
     {"NOTSTRICT",	IDL_TYP_LONG,    1, IDL_KW_ZERO, 0, 					IDL_KW_OFFSETOF(notstrict)},
     {"OFFSET",		IDL_TYP_DOUBLE,  1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_offset),		IDL_KW_OFFSETOF(offset)},
     {"PASS",		IDL_TYP_LONG,    1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_pass),   		IDL_KW_OFFSETOF(pass)},
     {"PULSE",		IDL_TYP_LONG,    1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_exp_number),        IDL_KW_OFFSETOF(exp_number)},
     {"RANGE",		IDL_TYP_DOUBLE,  1, IDL_KW_ARRAY,IDL_KW_OFFSETOF(is_range),		IDL_CHARA(rangeDesc)},
     {"RESOLUTION",	IDL_TYP_LONG,    1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_resolution),	IDL_KW_OFFSETOF(resolution)},
     {"SCALE",		IDL_TYP_DOUBLE,  1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_scale),		IDL_KW_OFFSETOF(scale)},
     {"SERIAL",		IDL_TYP_STRING,  1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_serial),		IDL_KW_OFFSETOF(serial)},
     {"SHOT",		IDL_TYP_LONG,    1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_exp_number),        IDL_KW_OFFSETOF(exp_number)},
     {"STATUS",		IDL_TYP_LONG,    1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_status),   		IDL_KW_OFFSETOF(status)},
     {"STEPID",		IDL_TYP_STRING,  1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_stepId),		IDL_KW_OFFSETOF(stepId)},
     {"TIME",		IDL_TYP_STRING,  1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_time),		IDL_KW_OFFSETOF(time)},
     {"TITLE",		IDL_TYP_STRING,  1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_title),		IDL_KW_OFFSETOF(title)},
     {"TYPE",		IDL_TYP_STRING,  1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_type),		IDL_KW_OFFSETOF(type)},
     {"UNITS",		IDL_TYP_STRING,  1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_units),		IDL_KW_OFFSETOF(units)},
     {"UNLIMITED",	IDL_TYP_STRING,  1, IDL_KW_ZERO, 0,					IDL_KW_OFFSETOF(unlimited)},
     {"UPDATE",		IDL_TYP_LONG,    0, IDL_KW_ZERO, 0, 					IDL_KW_OFFSETOF(update)},
     {"VARNAME",	IDL_TYP_STRING,  1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_varname),		IDL_KW_OFFSETOF(varname)},
     {"VERBOSE",	IDL_TYP_LONG,    1, IDL_KW_ZERO, 0, 					IDL_KW_OFFSETOF(verbose)},
     {"VERSION",	IDL_TYP_LONG,    1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_version),   	IDL_KW_OFFSETOF(version)},
     {"XML",		IDL_TYP_STRING,  1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_xml),		IDL_KW_OFFSETOF(xml)},     			    			
     {NULL}
   };  
      
   KW_RESULT kw;
      
   initKW(&kw);		// Initialise the Keyword Structure
       
//---------------------------------------------------------------------------      
// Get Passed Keywords and Parameters

   nparams = IDL_KWProcessByOffset(argc,argv,argk,kw_pars,(IDL_VPTR *)0,1,&kw);
   
//---------------------------------------------------------------------------      
// Extract Keywords 
            
   if(kw.is_stepId){
      stepId = IDL_STRING_STR(&kw.stepId); 
      
      do{
         if(!strcasecmp(stepId, "create")){
	    kw.create = 1;
	    break;
	 }   
         if(!strcasecmp(stepId, "close")){
	    kw.close = 1;
	    break;
	 }   
         if(!strcasecmp(stepId, "update")){
	    kw.update = 1;
	    break;
	 }   
         if(!strcasecmp(stepId, "delete")){
	    kw.delete = 1;
	    break;
	 }
      } while(0);
      	        
   } else {
      if(kw.verbose) fprintf(stderr,"Please Identify the file operation step, e.g, dimension\n");
      IDL_KW_FREE;
      return(IDL_GettmpLong(-1));   
   }     
         
//---------------------------------------------------------------------------      
// Previous File ID? (i.e. the file is open)  

   if(kw.is_fileid && !kw.create && !kw.update){
   
      fileid = IDL_LongScalar(kw.fileid);
      
      if(fileid != ncfileid){				// Different File ID specified: Check the file is Open 	    
	 if(ncfilecount == 0 ){				// Error ... No File currently Open  
            if(kw.verbose) fprintf(stderr,"A File ID has been passed but there are No Files currently Open!\n");
            IDL_KW_FREE;
            return(IDL_GettmpLong(-1));   
	 }
         if(kw.debug && ncfileids != NULL) for(i=0;i<ncfilecount;i++)fprintf(stdout,"*[%d] %d\n", i, ncfileids[i]);	    	       
	 for(i=0;i<ncfilecount;i++){
            if(ncfileids[i] == fileid){
	       ncfileid   = fileid;		// Identify the Open File ID and Assign this as the current file
	       compliance = complianceSet[i];	// Compliance tests so far ...
	       break;
            }
	 }
	 if(ncfileid != fileid){	       
            if(kw.verbose) fprintf(stderr,"The passed File ID does not correspond to an Open file!\n");
            IDL_KW_FREE;
            return(IDL_GettmpLong(-1));   
	 }              	                 
      }
   } 
   
//--------------------------------------------------------------------------      
// Test Compression

   if(kw.debug && kw.compression==99){
      compTest();
      IDL_KW_FREE;
      return(IDL_GettmpLong(0));   
   }
    

//--------------------------------------------------------------------------      
// Create an Error Trap

   err = 0;
   
   do {  
            
//---------------------------------------------------------------------------      
// Create or Close the File?

      if(kw.create || kw.close || kw.update || kw.delete){
      
         if(!kw.close){
      
            err = opennetcdf(nparams, argv, &kw);	// Open the file		

// Return the fileid if a keyword was passed
	 
	    if(kw.is_fileid && ncfileid != INT_MAX) IDL_StoreScalar(kw.fileid, IDL_TYP_LONG, (IDL_ALLTYPES *) &ncfileid);	      
	 
            if(kw.debug){   
               fprintf(stdout,"Number of Arguments   %d\n", argc);
               fprintf(stdout,"Number of Parameters  %d\n", nparams);
               fprintf(stdout,"Number of Keywords    %d\n", argc-nparams);
               fprintf(stdout,"File ID               %d\n", ncfileid);
               fprintf(stdout,"File Count            %d\n", ncfilecount);
               if(ncfileids != NULL) for(i=0;i<ncfilecount;i++)fprintf(stdout,"B[%d] %d   %d\n", i, ncfileids[i], complianceSet[i]);
            }
   
         } else {
            err = closenetcdf(&kw);		// Close the File	 
         }
      
         break;   
      }                 
         
//--------------------------------------------------------------------------      
// Devices: A Group Named after a Device

      if(!strcasecmp(stepId, "device")){      
         err = putDevice(nparams, argv, &kw);
	 break;
      }	        
              
//--------------------------------------------------------------------------      
// Identify/Create a Group: Used by following steps 

      if((err = putGroup(&kw, &ncgrpid)) != NC_NOERR) break;
                                                
//--------------------------------------------------------------------------      
// Dimensions

      if(!strcasecmp(stepId, "dimension")){      
         err = putDimension(nparams, argv, &kw, ncgrpid);  	 	 
         break;
      }         
      
//--------------------------------------------------------------------------      
// Coordinate Variables

      if(!strcasecmp(stepId, "coordinate")){      
         err = putCoordinate(nparams, argv, &kw, ncgrpid);  	 	 
         break;
      }

//--------------------------------------------------------------------------      
// Data Variables
   
      if(!strcasecmp(stepId, "variable")){
         err = putVariable(nparams, argv, &kw, ncgrpid);
	 break;
      } 
        
//--------------------------------------------------------------------------      
// Attributes: Additional attributes can be placed within any group level

      if(!strcasecmp(stepId, "attribute")){
         err = putAttribute(nparams, argv, &kw, ncgrpid);
         break;
      }
//--------------------------------------------------------------------------      
// End of Error Trap  
   
   } while(0); 

            
//--------------------------------------------------------------------------      
// Update the Compliance Set
    
   for(i=0;i<ncfilecount;i++) if(ncfileids[i] == fileid){complianceSet[i] = compliance; break;}
   
            
//--------------------------------------------------------------------------      
// Cleanup Keywords 

   IDL_KW_FREE;
   return(IDL_GettmpLong(err)); 
} 

#include "./putOpenClose.c"
#include "./putGroup.c"
#include "./putCoordinate.c"
#include "./putAttribute.c"
#include "./putDevice.c"
#include "./putVariable.c"
