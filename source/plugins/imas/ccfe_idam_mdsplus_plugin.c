// IDAM plugin interface library to IMAS mdsplus components

/*-----------------------------------------------------------------------------------------------------
Approach: static functions, variables etc. retain their original name
	  public functions are prefixed with 'imas_' when their name begins 'mds' otherwise 'imas_mds' to manage the namespace.
	  These public functions are called by the imas_mds plugin with arguments passed from the
	  client through IDAM. The plugin method names are the same as the original set of public functions and call the new set
	  with the prefixed names.
	  Only those functions called by the plugin will be public functions - existing public functions not called will be changed to static
	  Application build will identify if there are missing public symbols.
	  The imas plugin built for HDF5 will be reused for mdsplus (including bugs!) as far as feasible
	  Start with the low_level_ual.c and work down the list of mds functions ..... 
	  	  
ToDO:	  all 'printf(' statements need to be changed to 'if(debgon)idamLog(LOG_DEBUG,' as this can interfere with data serialisation
	  check internalFlush is external? Where called?	  	  	  		

Change History

20Aug2015 DGMuir Copied from git IMAS repo - master/3/UAL/lowlevel/ual_low_level_mdsplus.c
		 
		 Pointer errmsg is undefined - create locate string and assign errmsg to it.
		 errmsg was declared extern - changed to static
	
-----------------------------------------------------------------------------------------------------*/

#include "ccfe_idam_mdsplus_plugin.h"

//extern char *errmsg;
char ErrMsg[MAX_STRINGS];
char *errmsg = &ErrMsg[0];
