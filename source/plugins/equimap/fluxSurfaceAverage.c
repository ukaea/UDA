

int fluxSurfaceAverage(){

   int i,j,k,m,n;
   int offset = 0;
   int err = 0;
   static int scrunched = 0;
   
   if(scrunched) return 0;		// Data is available

//-------------------------------------------------------------------------------------------------
// Locate contours of each flux surface and Scrunch

   KW_RESULT kw;
   
   CONTOURS_IN contourIn;
   CONTOURS_OUT *contourOut;
   
   SCRUNCH_IN scrunchIn;
   SCRUNCH_OUT *scrunchOut;
   
   METRIC_IN metricIn;
   METRIC_OUT *metricOut;
   
   kw.verbose       = 0;
   kw.debug         = 0;
   kw.interpolation = LINEAR;
   kw.errout        = NULL;
   kw.dbgout        = NULL;
   
   equimapdata.fluxAverages = (FLUXAVERAGES *)malloc(equimapdata.timeCount*sizeof(FLUXAVERAGES));
   
   scrunchIn.nmoms  = 8;
   scrunchIn.lasym0 = 1;
   scrunchIn.lreg   = 1; 

   for(j=0; j<equimapdata.timeCount; j++){
   
      equimapdata.fluxAverages[j].contours = (CONTOURS_OUT *)malloc((equimapdata.rhoCount+1)*sizeof(CONTOURS_OUT));
      equimapdata.fluxAverages[j].scrunch  = (SCRUNCH_OUT *) malloc((equimapdata.rhoCount+1)*sizeof(SCRUNCH_OUT));

// Scrunch the LCFS, taking into account the X-point locations, with a high number of moments

// Extend the coarse grid with the LCFS contour (Scrunched LCFS locii)  
   
// Locate the flux contours within the refined psi map
// Methods: bi-cubic spline, non-linear or linear interpolation   
   
      contourIn.nr = equimapdata.efitdata[j].psiCountSR[0];
      contourIn.nz = equimapdata.efitdata[j].psiCountSR[1]; 
      contourIn.r  = equimapdata.efitdata[j].rgridSR;
      contourIn.z  = equimapdata.efitdata[j].zgridSR;
      contourIn.psirz = (float *)malloc(contourIn.nr*contourIn.nz*sizeof(float));
      contourIn.psiZR = equimapdata.efitdata[j].psigSR;
      contourIn.nlcfs = equimapdata.efitdata[j].nlcfs;
      contourIn.rlcfs = equimapdata.efitdata[j].rlcfs;
      contourIn.zlcfs = equimapdata.efitdata[j].zlcfs;
      contourIn.psi_mag = equimapdata.efitdata[j].psi_mag;
      contourIn.psi_bnd = equimapdata.efitdata[j].psi_bnd;
      
	 
      offset = 0;
      for(m=0;m<contourIn.nz;m++)	//  m*contourIn.nr + n
         for(n=0;n<contourIn.nr;n++) contourIn.psirz[offset++] = equimapdata.efitdata[j].psigSR[m][n]; // Psi[z][r]	    
   
      for(i=0; i<equimapdata.rhoCount; i++){
      
	 contourIn.psi = equimapdata.efitdata[j].mappsi[i];		// Psi mapped to the required flux label       
	 	 
	 contourOut = &equimapdata.fluxAverages[j].contours[i];
	       
	 err = mastcontours(&contourIn, contourOut, kw);		// Locate the Contour  
	 	 
	 if(err != 0){
	    // ERROR
	 }

// Scrunch the contours 
	 
	 scrunchIn.n_mb = contourOut->ncontour;
	 scrunchIn.rmb  = contourOut->rcontour;
	 scrunchIn.zmb  = contourOut->zcontour;
	 
	 scrunchOut = &equimapdata.fluxAverages[j].scrunch[i];
	 
	 err = mastscrunch(scrunchIn, scrunchOut, kw);			// Fit the Contour with a moments representation
	 
	 if(err != 0){
	    // ERROR
	 }	 
      
      }
      free(contourIn.psirz);
   }
  
//-------------------------------------------------------------------------------------------------
// Flux Surface metrics

   metricIn.nmoms = 8;				// Number of Moments
   metricIn.nrho  = equimapdata.rhoCount;	// Number of (volume) flux surfaces
   metricIn.rho   = equimapdata.rho;		// Flux surface label 
   metricIn.ntheta = 101;			// Number of Angular divisions
   metricIn.dtheta = 2.0*M_PI/(float)(metricIn.ntheta-1); 		// Angular increment
   metricIn.theta = (float *)malloc(metricIn.ntheta*sizeof(float));	// Equidistant Poloidal Angles from the mid-plane measured anti-clockwise;

   metricIn.theta[0] = 0.0;
   for(j=1; j<metricIn.ntheta; j++) metricIn.theta[j] = metricIn.theta[j-1] + metricIn.dtheta;

   metricIn.rcos = (float **)malloc(metricIn.nrho*sizeof(float *));
   metricIn.rsin = (float **)malloc(metricIn.nrho*sizeof(float *));
   metricIn.zcos = (float **)malloc(metricIn.nrho*sizeof(float *));
   metricIn.zsin = (float **)malloc(metricIn.nrho*sizeof(float *));
   
   for(i=0; i<metricIn.nrho; i++){
      metricIn.rcos[i] = (float *)malloc((metricIn.nmoms+1)*sizeof(float));
      metricIn.rsin[i] = (float *)malloc((metricIn.nmoms+1)*sizeof(float));
      metricIn.zcos[i] = (float *)malloc((metricIn.nmoms+1)*sizeof(float));
      metricIn.zsin[i] = (float *)malloc((metricIn.nmoms+1)*sizeof(float));   
   }
 
   for(j=0; j<equimapdata.timeCount; j++){
         	
      for(i=0; i<metricIn.nrho; i++){
         for(k=0; k<=metricIn.nmoms; k++){		// [nmoms+1][nrho]
	     metricIn.rcos[i][k] = equimapdata.fluxAverages[j].scrunch[i].rcos[k];
	     metricIn.rsin[i][k] = equimapdata.fluxAverages[j].scrunch[i].rsin[k];
	     metricIn.zcos[i][k] = equimapdata.fluxAverages[j].scrunch[i].zcos[k];
	     metricIn.zsin[i][k] = equimapdata.fluxAverages[j].scrunch[i].zsin[k];
         }
      }
      
      metricOut = &equimapdata.fluxAverages[j].metrics;
      	 	 
      metrics(metricIn, metricOut); 
      
      if(err != 0){
	    // ERROR
      }
   }
   
   for(i=0; i<metricIn.nrho; i++){
      if(metricIn.rcos[i] != NULL) free((void *)metricIn.rcos[i]);
      if(metricIn.rsin[i] != NULL) free((void *)metricIn.rsin[i]);
      if(metricIn.zcos[i] != NULL) free((void *)metricIn.zcos[i]);
      if(metricIn.zsin[i] != NULL) free((void *)metricIn.zsin[i]);
   }
   if(metricIn.rcos != NULL) free((void *)metricIn.rcos);
   if(metricIn.rsin != NULL) free((void *)metricIn.rsin);
   if(metricIn.zcos != NULL) free((void *)metricIn.zcos);
   if(metricIn.zsin != NULL) free((void *)metricIn.zsin);
   
   if(metricIn.theta != NULL) free((void *)metricIn.theta);
   
   scrunched = 1;
   return 0;               
}
