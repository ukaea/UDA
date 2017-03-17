
pro testhdf5

; Script to read a Legacy EFIT++ magnetics xml file and verify identical to data in a HDF5 file
; Data are access using IMAS name abstractions (Magnetics, PF_Active, TF IDS)

   rc  = resetProperties()		; Reset all properties to their default values

   rc = setProperty('debug')		; Client debug option
   rc = setProperty('verbose')
   
   rc = putIdamServerPort(56565L)	 
   rc = putIdamServerHost('idam0')
 
;----------------------------------------------------------------------------------------------------------------------------------
; Target Data
  
   device = 'MAST'
   ;shot   = 13500L
   shot   = 29891L

;----------------------------------------------------------------------------------------------------------------------------------
; Location of the XML file

   xmlfile = '/home/dgm/ITER/efit++/MAST/magnetics/idam.xml'
   
   ;hdf5source = 'netcdf::/home/dgm/IDAM/test/source/plugins/efitxml/livedisplay13500.nc'
   ;hdf5source = 'netcdf::/home/dgm/IDAM/test/source/plugins/efitxml/livedisplay29891.nc'
   hdf5source = 'hdf5::/home/dgm/IDAM/test/source/plugins/efitxml/livedisplay29891.nc'

;----------------------------------------------------------------------------------------------------------------------------------
; Count of Flux Loops
if(1) then begin
   a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Count)','', /noecho)  
   fcount = a.data[0]

   b = getdata('/magnetics/flux_loop/shape_of', hdf5source, /noecho)   

; compare

   if fcount ne b.data[0] then begin
      print, 'ERROR: Inconsistent Flux Loop Count!'
      stop
   endif 
   
   print, 'Consistent count of flux loops'  
   
;----------------------------------------------------------------------------------------------------------------------------------
; Names and identifiers 
   
   for i=1, fcount do begin 
     a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Name, /UnitStartIndex, id='+strtrim(i,2)+')','', /noecho)

     attribute = '/magnetics/flux_loop/'+strtrim(i,2)+'/name' 
     b = getdata(attribute, hdf5source, /noecho)   

     if string(a.data) ne string(b.data) then begin
        print, 'ERROR: Inconsistent Flux Loop Name!'
	help, a
	help, b
	stop
      endif	
   endfor
   
   for i=1, fcount do begin    
     a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Identifier, /UnitStartIndex, id='+strtrim(i,2)+')','', /noecho)

     attribute = '/magnetics/flux_loop/'+strtrim(i,2)+'/identifier' 
     b = getdata(attribute, hdf5source, /noecho)   

     if string(a.data) ne string(b.data) then begin
        print, 'ERROR: Inconsistent Flux Loop Identifier!'
	stop
     endif 	
 
   endfor

   print, 'Consistent flux loop names and identifiers'  
   
;----------------------------------------------------------------------------------------------------------------------------------
; Coordinates

   for i=1, fcount do begin
 
      a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Position, /count, /UnitStartIndex, id='+strtrim(i,2)+')','', /noecho)
      pcount = a.data[0]

      b = getdata('/magnetics/flux_loop/'+strtrim(i,2)+'/position/shape_of', hdf5source, /noecho) 

      if a.data ne b.data then begin
        print, 'ERROR: Inconsistent Flux Loop Coordinate Count!'
	stop
      endif 	

      print, 'Consistent flux loop coordinate count'  
     
      for j=1, pcount do begin
        a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Position, /R, /UnitStartIndex, id='+strtrim(i,2)+', index='+strtrim(j,2)+')' ,'', /noecho)
        b = getdata( '/magnetics/flux_loop/'+strtrim(i,2)+'/position/'+strtrim(j,2)+'/r', hdf5source, /noecho) 
        if a.data ne b.data then begin
           print, 'ERROR: Inconsistent Flux Loop R Coordinate !'
	   stop
        endif 	

        a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Position, /Z, /UnitStartIndex, id='+strtrim(i,2)+', index='+strtrim(j,2)+')' ,'', /noecho)
        b = getdata( '/magnetics/flux_loop/'+strtrim(i,2)+'/position/'+strtrim(j,2)+'/z', hdf5source, /noecho) 
        if a.data ne b.data then begin
           print, 'ERROR: Inconsistent Flux Loop Z Coordinate !'
	   stop
        endif 	

        a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Position, /Phi, /UnitStartIndex, id='+strtrim(i,2)+', index='+strtrim(j,2)+')' ,'', /noecho)
        b = getdata( '/magnetics/flux_loop/'+strtrim(i,2)+'/position/'+strtrim(j,2)+'/phi', hdf5source, /noecho) 
        if a.data ne b.data then begin
           print, 'ERROR: Inconsistent Flux Loop Phi Coordinate !'
	   stop
        endif 	

      endfor
      
      print, 'Consistent flux loop coordinates'  

   endfor

;----------------------------------------------------------------------------------------------------------------------------------   
; Measurement Data

   for i=1, fcount do begin

; Signal name

      a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Signal, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)
 
      datascaling = 1.0d0
      timescaling = 1.0d0
      
      signal = strtrim(string(a.data),2)

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
	       if(datascaling ne 1.0) then time = double(d.data) * timescaling $
	       else time = double(d.time)
	       
	       e = getdata('/magnetics/flux_loop/'+strtrim(i,2)+'/flux/data', hdf5source, /noecho)
	       f = getdata('/magnetics/flux_loop/'+strtrim(i,2)+'/flux/time', hdf5source, /noecho)
	       
	       count1 = n_elements(data)
	       count2 = n_elements(time)
	       count3 = n_elements(e.data)
	       count4 = n_elements(f.data)
	       
	       if count1 ne count2 or count2 ne count3 or count3 ne count4 then begin
	          print, 'Error: Inconsistent count of flux loop measurement data!'
		  stop
	       endif
	       
	       for j = 0, count1-1 do begin
	          if data[j] ne e.data[j] or time[j] ne f.data[j] then begin
	             print, 'Error: Inconsistent values of flux loop measurement data!'
		     stop
		  endif
	       endfor	      	   
	       
	    endif   
         endif

      endif

   endfor
   
   print, 'Consistent flux loop measurement values'
   
endif
;----------------------------------------------------------------------------------------------------------------------------------
; Count of Magnetic Probes
if(1) then begin         
   a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Count)','', /noecho)
   fcount = a.data[0]

   b = getdata('/magnetics/bpol_probe/shape_of', hdf5source, /noecho)   

; compare

   if fcount ne b.data[0] then begin
      print, 'ERROR: Inconsistent Magnetic Probe Count!'
      stop
   endif 
   
   print, 'Consistent count of Magnetic Probe'  
   
;----------------------------------------------------------------------------------------------------------------------------------
; Names and identifiers 
    
   for i=1, fcount do begin
     
     a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Name, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)

     attribute = '/magnetics/bpol_probe/'+strtrim(i,2)+'/name' 
     b = getdata(attribute, hdf5source, /noecho)   

     if string(a.data) ne string(b.data) then begin
        print, 'ERROR: Inconsistent MagProbe Name!'
	help, a
	help, b
	stop
      endif	
   endfor
    
   for i=1, fcount do begin
     
     a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Identifier, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)

     attribute = '/magnetics/bpol_probe/'+strtrim(i,2)+'/identifier' 
     b = getdata(attribute, hdf5source, /noecho)   

     if string(a.data) ne string(b.data) then begin
        print, 'ERROR: Inconsistent MagProbe identifier!'
	help, a
	help, b
	stop
      endif	
   endfor

   print, 'Consistent MagProbe names and identifiers'  
   
;----------------------------------------------------------------------------------------------------------------------------------
; Coordinates
         
   for i=1, fcount do begin      
      a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Position, /R, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)
      b = getdata( '/magnetics/bpol_probe/'+strtrim(i,2)+'/position/r', hdf5source, /noecho) 
      if a.data ne b.data then begin
         print, 'ERROR: Inconsistent MagProbe R Coordinate !'
	 stop
      endif 	

      a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Position, /Z, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)
      b = getdata( '/magnetics/bpol_probe/'+strtrim(i,2)+'/position/z', hdf5source, /noecho) 
      if a.data ne b.data then begin
         print, 'ERROR: Inconsistent MagProbe Z Coordinate !'
	 stop
      endif 	

      a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Position, /Phi, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)
      b = getdata( '/magnetics/bpol_probe/'+strtrim(i,2)+'/position/phi', hdf5source, /noecho) 
      if a.data ne b.data then begin
         print, 'ERROR: Inconsistent MagProbe Phi Coordinate !'
	 stop
      endif 	
   endfor
   
   print, 'Consistent MagProbe coordinates'

;----------------------------------------------------------------------------------------------------------------------------------
; Measurement Data

   for i=1, fcount do begin

; Signal name

      a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Signal, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)

      datascaling = 1.0d0
      timescaling = 1.0d0

      signal = strtrim(string(a.data),2)

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
	       group = 'magnetics/bpol_probe/'+strtrim(i,2)+'/field/data'
	       if(timescaling ne 1.0) then time = double(d.time) * timescaling $
	       else time = double(d.time)
	       	       
	       e = getdata('/magnetics/bpol_probe/'+strtrim(i,2)+'/field/data', hdf5source, /noecho)
	       f = getdata('/magnetics/bpol_probe/'+strtrim(i,2)+'/field/time', hdf5source, /noecho)
	       
	       count1 = n_elements(data)
	       count2 = n_elements(time)
	       count3 = n_elements(e.data)
	       count4 = n_elements(f.data)
	       
	       if count1 ne count2 or count2 ne count3 or count3 ne count4 then begin
	          print, 'Error: Inconsistent count of MagProbe measurement data!'
		  stop
	       endif
	       
	       for j = 0, count1-1 do begin
	          if data[j] ne e.data[j] or time[j] ne f.data[j] then begin
	             print, 'Error: Inconsistent values of MagProbe measurement data!'
		     stop
		  endif
	       endfor	      	   	       
	       
	    endif   
         endif

      endif

   endfor
   
   print, 'Consistent MagProbe measurement values'
endif

;----------------------------------------------------------------------------------------------------------------------------------
; Count of PF Active Coils
         
   a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Count)','', /noecho)
   coilcount = a.data[0]

   b = getdata('/pf_active/coil/shape_of', hdf5source, /noecho)   

; compare

   if coilcount ne b.data[0] then begin
      print, 'ERROR: Inconsistent PF Active Coil Count!'
      stop
   endif 
   
   print, 'Consistent count of PF Active Coils'  

;----------------------------------------------------------------------------------------------------------------------------------
; Names and identifiers 
   
   for i=1, coilcount do begin
     
     a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Name, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)

     attribute = '/pf_active/coil/'+strtrim(i,2)+'/name' 
     b = getdata(attribute, hdf5source, /noecho)   

     if string(a.data) ne string(b.data) then begin
        print, 'ERROR: Inconsistent PFActive Name!'
	help, a
	help, b
	stop
      endif	
   endfor

   for i=1, coilcount do begin
     
     a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Name, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)

     attribute = '/pf_active/coil/'+strtrim(i,2)+'/identifier' 
     b = getdata(attribute, hdf5source, /noecho)   

     if string(a.data) ne string(b.data) then begin
        print, 'ERROR: Inconsistent PFActive identifier!'
	help, a
	help, b
	stop
      endif	
   endfor

   print, 'Consistent PFActive names and identifiers'  
   
;----------------------------------------------------------------------------------------------------------------------------------
; Coordinates
        
   for i=1, coilcount do begin

; Count of Elements/Coordinates
 
      a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Element, /count, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)
      elementcount = a.data[0]

      b = getdata('/pf_active/coil/'+strtrim(i,2)+'/element/shape_of', hdf5source, /noecho) 

      if a.data ne b.data then begin
        print, 'ERROR: Inconsistent PFActive Coordinate Count!'
	stop
      endif 	

      print, 'Consistent PFActive coordinate count'  
       
; Individual element coordinates

      for j=1, elementcount do begin
	  
         a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Element, /Geometry, /Rectangle, /R, /UnitStartIndex, id='+strtrim(i,2)+', index='+strtrim(j,2)+')','', /noecho)
         b = getdata('/pf_active/coil/'+strtrim(i,2)+'/element/'+strtrim(j,2)+'/geometry/rectangle/r', hdf5source, /noecho) 
         if a.data ne b.data then begin
           print, 'ERROR: Inconsistent PFActive R Coordinate !'
	   stop
         endif 	
         a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Element, /Geometry, /Rectangle, /Z, /UnitStartIndex, id='+strtrim(i,2)+', index='+strtrim(j,2)+')','', /noecho)
         b = getdata('/pf_active/coil/'+strtrim(i,2)+'/element/'+strtrim(j,2)+'/geometry/rectangle/z', hdf5source, /noecho) 
         if a.data ne b.data then begin
           print, 'ERROR: Inconsistent PFActive Z Coordinate !'
	   stop
         endif 	
         a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Element, /Geometry, /Rectangle, /Width, /UnitStartIndex, id='+strtrim(i,2)+', index='+strtrim(j,2)+')','', /noecho)
         b = getdata('/pf_active/coil/'+strtrim(i,2)+'/element/'+strtrim(j,2)+'/geometry/rectangle/width', hdf5source, /noecho) 
         if a.data ne b.data then begin
           print, 'ERROR: Inconsistent PFActive Width Coordinate !'
	   stop
         endif 	
         a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Element, /Geometry, /Rectangle, /Height, /UnitStartIndex, id='+strtrim(i,2)+', index='+strtrim(j,2)+')','', /noecho)
         b = getdata('/pf_active/coil/'+strtrim(i,2)+'/element/'+strtrim(j,2)+'/geometry/rectangle/height', hdf5source, /noecho) 
         if a.data ne b.data then begin
           print, 'ERROR: Inconsistent PFActive Height Coordinate !'
	   stop
         endif 	
      endfor
      print, 'Consistent PFActive coordinates'  

   endfor

;----------------------------------------------------------------------------------------------------------------------------------   
; Measurement Data

   for i=1, coilcount do begin

; Signal name

      a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Signal, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)

      datascaling = 1.0d0
      timescaling = 1.0d0

      signal = strtrim(string(a.data),2)

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
	       if(timescaling ne 1.0) then time = double(d.time) * timescaling $
	       else time = double(d.time)

	       e = getdata('/pf_active/coil/'+strtrim(i,2)+'/current/data', hdf5source, /noecho)
	       f = getdata('/pf_active/coil/'+strtrim(i,2)+'/current/time', hdf5source, /noecho)
	       
	       count1 = n_elements(data)
	       count2 = n_elements(time)
	       count3 = n_elements(e.data)
	       count4 = n_elements(f.data)
	       
	       if count1 ne count2 or count2 ne count3 or count3 ne count4 then begin
	          print, 'Error: Inconsistent count of PFActive measurement data!'
		  stop
	       endif
	       
	       for j = 0, count1-1 do begin
	          if data[j] ne e.data[j] or time[j] ne f.data[j] then begin
	             print, 'Error: Inconsistent values of PFActive measurement data!'
		     stop
		  endif
	       endfor	      	   

	    endif   
         endif

      endif

   endfor
   
   print, 'Consistent PFActive measurement values'
   
  return
end
