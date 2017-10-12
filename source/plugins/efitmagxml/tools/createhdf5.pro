
pro createhdf5

; Script to read a Legacy EFIT++ magnetics xml file and create a HDF5 file with Magnetics, PF_Active, TF IDS abstractions
; HDF5 file is organised similarly to the IMAS data model

   rc  = resetProperties()		; Reset all properties to their default values

   rc = setProperty('debug')		; Client debug option
   rc = setProperty('verbose')
   
   rc = putIdamServerPort(56565L)	 
   rc = putIdamServerHost('idam0')
   
   verbose = 1
 
;----------------------------------------------------------------------------------------------------------------------------------
; Target Data
  
   device = 'MAST'

   shot     = 29891      
   filename = 'livedisplay29891.nc'

   ;shot     = 13500
   ;filename = 'livedisplay13500.nc'

;----------------------------------------------------------------------------------------------------------------------------------
; Location of the XML file

   xmlfile = '/home/dgm/ITER/efit++/MAST/magnetics/idam.xml'

;----------------------------------------------------------------------------------------------------------------------------------
; Open the HDF5 file

   fileId=0L
      
   rc = putdata( filename, stepId='create', directory='/home/dgm/IDAM/test/source/plugins/efitxml', $
                 fileId=fileId, conventions='Fusion-1.0', $
                 class='analysed data', title='Livedisplay data for MAST', shot=13500L, pass=0L, $
                 date='10 March 2017', time='14:44', status=1L, $
                 comment='THIS IS TEST DATA - USE ONLY FOR DEVELOPMENT', code='createhdf5.pro', version=1L,$
                 debug=debug, verbose=verbose)
 
;----------------------------------------------------------------------------------------------------------------------------------
; Add meta data

   rc = putdata( 'first data generation and initial testing', stepId='Attribute', group='/', name='test', debug=debug, verbose=verbose)

;----------------------------------------------------------------------------------------------------------------------------------
; Count of Flux Loops

; Add a unitary dimension

   rc = putdata( 1L, stepId='Dimension', group='/magnetics/flux_loop', name='one', debug=debug, verbose=verbose)

; Count of Flux Loops

   a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Count)','', /noecho)  
   fcount = a.data[0]
 
   rc = putdata( a.data, stepId='Variable', group='/magnetics/flux_loop', name='shape_of', dimension='one', debug=debug, verbose=verbose)   
        
; Add names and identifiers as group attributes
   
   for i=1, fcount do begin 
     group = '/magnetics/flux_loop/'+strtrim(i,2) 
     a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Name, /UnitStartIndex, id='+strtrim(i,2)+')','', /noecho)
 
     if a.erc eq 0 then begin     
	rc = putdata( string(a.data[0]), stepId='Attribute', group=group, name='name', debug=debug, verbose=verbose)
     endif    
     a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Identifier, /UnitStartIndex, id='+strtrim(i,2)+')','', /noecho)
 
     if a.erc eq 0 then begin     
        rc = putdata( string(a.data[0]), stepId='Attribute', group=group, name='identifier', debug=debug, verbose=verbose)
     endif
   endfor
   
   for i=1, fcount do begin

; Count of Coordinates
 
      a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Position, /count, /UnitStartIndex, id='+strtrim(i,2)+')','', /noecho)
      pcount = a.data[0]

      group = '/magnetics/flux_loop/'+strtrim(i,2)+'/position'
      rc = putdata( pcount, stepId='Dimension', group=group, name='shape_of', debug=debug, verbose=verbose)   
      rc = putdata( a.data, stepId='Variable', group=group, name='shape_of', dimension='shape_of', debug=debug, verbose=verbose)   
     
      for j=1, pcount do begin
        group = '/magnetics/flux_loop/'+strtrim(i,2)+'/position/'+strtrim(j,2)
        a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Position, /R, /UnitStartIndex, id='+strtrim(i,2)+', index='+strtrim(j,2)+')' ,'', /noecho)
        rc = putdata( a.data, stepId='Variable', group=group, name='r', dimension='shape_of', debug=debug, verbose=verbose)
 
        a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Position, /Z, /UnitStartIndex, id='+strtrim(i,2)+', index='+strtrim(j,2)+')' ,'', /noecho)
        rc = putdata( a.data, stepId='Variable', group=group, name='z', dimension='shape_of', debug=debug, verbose=verbose)

        a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Position, /Phi, /UnitStartIndex, id='+strtrim(i,2)+', index='+strtrim(j,2)+')' ,'', /noecho)
        rc = putdata( a.data, stepId='Variable', group=group, name='phi', dimension='shape_of', debug=debug, verbose=verbose)

      endfor
   endfor
   
; Measurement Data

   for i=1, fcount do begin

; Signal name

      a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Signal, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)
 
      datascaling = 1.0d0
      timescaling = 1.0d0
      
      signal = strtrim(string(a.data),2)
      print, signal

      if(a.erc eq 0 and signal ne 'noData') then begin

; Data Scaling Factor

         b = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /DataScaling, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)
         
	 if b.erc eq 0 then datascaling = double(b.data[0])

; Time Scaling Factor

         c = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /TimeScaling, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)      
         if c.erc eq 0 then timescaling = double(c.data[0])

      endif
      
      if device eq 'MAST' then begin
         if b.erc eq 0 and signal ne 'noData' then begin
            d = getdata(signal, shot, /noecho)  
            if d.erc eq 0 then begin
	       if(datascaling ne 1.0) then data = double(d.data) * datascaling $
	       else data = double(d.data)
	       print, datascaling,total(data)
	       if(datascaling ne 1.0) then time = double(d.data) * timescaling $
	       else time = double(d.time)
	       print, timescaling,total(time)
	       
	       group = '/magnetics/flux_loop/'+strtrim(i,2)+'/flux'	       
	       rc = putdata( n_elements(data), stepId='Dimension', group=group, name='time', debug=debug, verbose=verbose)
	       print, i, signal, rc
	       rc = putdata( data, stepId='Variable', group=group, name='data', dimension='time', debug=debug, verbose=verbose)
	       print, i, signal, rc
	       rc = putdata( time, stepId='Variable', group=group, name='time', dimension='time', debug=debug, verbose=verbose)
	       print, i, signal, rc
	    endif   
         endif

      endif

   endfor

;----------------------------------------------------------------------------------------------------------------------------------
; Count of Magnetic Probes

; Add a unitary dimension

   rc = putdata( 1L, stepId='Dimension', group='/magnetics/bpol_probe', name='one', debug=debug, verbose=verbose)

; Count of Probes
         
   a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Count)','', /noecho)
   fcount = a.data[0]

   rc = putdata( a.data, stepId='Variable', group='/magnetics/bpol_probe', name='shape_of', dimension='one', debug=debug, verbose=verbose)   
    
   for i=1, fcount do begin
     group = '/magnetics/bpol_probe/'+strtrim(i,2)
     
     a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Name, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)
     if a.erc eq 0 then begin
	rc = putdata( string(a.data[0]), stepId='Attribute', group=group, name='name', debug=debug, verbose=verbose)
     endif
     
     a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Identifier, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)
     if a.erc eq 0 then begin
	rc = putdata( string(a.data[0]), stepId='Attribute', group=group, name='identifier', debug=debug, verbose=verbose)
     endif

   endfor
   
   for i=1, fcount do begin
      group = '/magnetics/bpol_probe/'+strtrim(i,2)+'/position'
      
      a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Position, /R, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)
      if a.erc eq 0 then rc = putdata( a.data, stepId='Variable', group=group, name='r', dimension='one', debug=debug, verbose=verbose)

      a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Position, /Z, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)
      if a.erc eq 0 then rc = putdata( a.data, stepId='Variable', group=group, name='z', dimension='one', debug=debug, verbose=verbose)
      
      a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Position, /Phi, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)
      if a.erc eq 0 then rc = putdata( a.data, stepId='Variable', group=group, name='phi', dimension='one', debug=debug, verbose=verbose)

   endfor

; Measurement Data

   for i=1, fcount do begin

; Signal name

      a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Signal, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)

      datascaling = 1.0d0
      timescaling = 1.0d0

      signal = strtrim(string(a.data),2)
      print, signal

      if(a.erc eq 0 and signal ne 'noData') then begin

; Data Scaling Factor

         b = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /DataScaling, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)
         if b.erc eq 0 then datascaling = double(b.data[0])

; Time Scaling Factor

         c = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /TimeScaling, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)      
         if c.erc eq 0 then timescaling = double(c.data[0])

      endif
      
      if device eq 'MAST' then begin
         if b.erc eq 0 and signal ne 'noData' then begin
            d = getdata(signal, shot, /noecho)  
            if d.erc eq 0 then begin
	       if(datascaling ne 1.0) then data = double(d.data) * datascaling $
	       else data = double(d.data)
	       print, datascaling,total(data)
	       if(timescaling ne 1.0) then time = double(d.time) * timescaling $
	       else time = double(d.time)
	       print, timescaling,total(time)
	       
	       group = '/magnetics/bpol_probe/'+strtrim(i,2)+'/field'
	       rc = putdata( n_elements(data), stepId='Dimension', group=group, name='time', debug=debug, verbose=verbose)
	       print, i, signal, rc
	       rc = putdata( data, stepId='Variable', group=group, name='data', dimension='time', debug=debug, verbose=verbose)
	       print, i, n_elements(data), rc
	       rc = putdata( time, stepId='Variable', group=group, name='time', dimension='time', debug=debug, verbose=verbose)
	       print, i, '   ', rc
	    endif   
         endif

      endif

   endfor

;----------------------------------------------------------------------------------------------------------------------------------
; PF_Active Coil Information

; Add a unitary dimension

   rc = putdata( 1L, stepId='Dimension', group='/pf_active/coil', name='one', debug=debug, verbose=verbose)

; Count of PF Active Coils
         
   a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Count)','', /noecho)
   coilcount = a.data[0]
   
   rc = putdata( a.data, stepId='Variable', group='/pf_active/coil', name='shape_of', dimension='one', debug=debug, verbose=verbose)      

; Add names and identifiers as group attributes

   for i=1, coilcount do begin
     group = '/pf_active/coil/'+strtrim(i,2) 
     
     a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Name, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)
     if a.erc eq 0 then begin     
	rc = putdata( string(a.data[0]), stepId='Attribute', group=group, name='name', debug=debug, verbose=verbose)
     endif    
     
     a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Identifier, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)
     if a.erc eq 0 then begin     
	rc = putdata( string(a.data[0]), stepId='Attribute', group=group, name='identifier', debug=debug, verbose=verbose)
     endif    
     
   endfor
   
   for i=1, coilcount do begin

; Count of Elements/Coordinates
 
      a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Element, /count, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)
      elementcount = a.data[0]
 
      group = '/pf_active/coil/'+strtrim(i,2)+'/element'
      rc = putdata( a.data, stepId='Variable', group=group, name='shape_of', dimension='one', debug=debug, verbose=verbose)   
  
; Individual element coordinates

      for j=1, elementcount do begin
         group = '/pf_active/coil/'+strtrim(i,2)+'/element/'+strtrim(j,2)+'/geometry/rectangle'
	  
         a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Element, /Geometry, /Rectangle, /R, /UnitStartIndex, id='+strtrim(i,2)+', index='+strtrim(j,2)+')','', /noecho)
         rc = putdata( a.data, stepId='Variable', group=group, name='r', dimension='one', debug=debug, verbose=verbose)         
	 a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Element, /Geometry, /Rectangle, /Z, /UnitStartIndex, id='+strtrim(i,2)+', index='+strtrim(j,2)+')','', /noecho)
         rc = putdata( a.data, stepId='Variable', group=group, name='z', dimension='one', debug=debug, verbose=verbose)
         a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Element, /Geometry, /Rectangle, /Width, /UnitStartIndex, id='+strtrim(i,2)+', index='+strtrim(j,2)+')','', /noecho)
         rc = putdata( a.data, stepId='Variable', group=group, name='width', dimension='one', debug=debug, verbose=verbose)
         a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Element, /Geometry, /Rectangle, /Height, /UnitStartIndex, id='+strtrim(i,2)+', index='+strtrim(j,2)+')','', /noecho)
         rc = putdata( a.data, stepId='Variable', group=group, name='height', dimension='one', debug=debug, verbose=verbose)
      endfor
   endfor

; Measurement Data

   for i=1, coilcount do begin

; Signal name

      a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Signal, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)

      datascaling = 1.0d0
      timescaling = 1.0d0

      signal = strtrim(string(a.data),2)
      print, signal

; Data Scaling Factor

      b = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /DataScaling, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)

      datascaling = 1.0d0
      if b.erc eq 0 then datascaling = double(b.data[0])

; Time Scaling Factor

      c = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /TimeScaling, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)
       
      timescaling = 1.0d0
      if c.erc eq 0 then timescaling = double(c.data[0])
      
      if device eq 'MAST' then begin
         if b.erc eq 0 and signal ne 'noData' then begin
            d = getdata(signal, shot, /noecho)  
            if d.erc eq 0 then begin
	       if(datascaling ne 1.0) then data = double(d.data) * datascaling $
	       else data = double(d.data)
	       print, datascaling,total(data)
	       if(timescaling ne 1.0) then time = double(d.time) * timescaling $
	       else time = double(d.time)
	       print, timescaling, total(time)
	       
	       group = '/pf_active/coil/'+strtrim(i,2)+'/current'
	       rc = putdata( n_elements(data), stepId='Dimension', group=group, name='time', debug=debug, verbose=verbose)
	       print, i, '   ', coilcount, '   ', signal, '   ', rc
	       rc = putdata( data, stepId='Variable', group=group, name='data', dimension='time', debug=debug, verbose=verbose)
	       print, i, '   ', coilcount, '   ', group, '   ', rc
	       rc = putdata( time, stepId='Variable', group=group, name='time', dimension='time', debug=debug, verbose=verbose)
	       print, i, '   ', coilcount, '   ', n_elements(data), '   ', rc

	    endif   
         endif

      endif

   endfor
   
;----------------------------------------------------------------------------------------------------------------------------------
; Count of Toroidal Field

if(0) then begin            
      if device eq 'MAST' then begin
         
	 a = getdata('LIVEDISPLAY::rb()', shot, /noecho) 
	 stop
	 
	 datascaling = 1.0d0
         timescaling = 1.0d0
         signal = 'amc_tf current'
	 radius = 0.7
	 group='/tf/b_field_tor_vacuum_r
            d = getdata(signal, shot, /noecho)  
            if d.erc eq 0 then begin
	       if(datascaling ne 1.0) then data = double(d.data) * datascaling $
	       else data = double(d.data)
	       print, datascaling,total(data)
	       if(datascaling ne 1.0) then time = double(d.data) * timescaling $
	       else time = double(d.time)
	       print, timescaling,total(time)
	       
	       group = '/magnetics/flux_loop/'+strtrim(i,2)+'/flux'	       
	       rc = putdata( n_elements(data), stepId='Dimension', group=group, name='time', debug=debug, verbose=verbose)
	       print, i, signal, rc
	       rc = putdata( data, stepId='Variable', group=group, name='data', dimension='time', debug=debug, verbose=verbose)
	       print, i, signal, rc
	       rc = putdata( time, stepId='Variable', group=group, name='time', dimension='time', debug=debug, verbose=verbose)
	       print, i, signal, rc
	    endif   

      endif

 
endif
;----------------------------------------------------------------------------------------------------------------------------------
   

;----------------------------------------------------------------------------------------------------------------------------------
; Close the file

      rc = putdata( stepId='close', fileId=fileId, debug=debug, verbose=verbose)
      print,'       Close rc = ', rc
      
   
  return
end
