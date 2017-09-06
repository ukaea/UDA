pro printscript, database, crud, abstract, plugin, device

   if database eq 'mongodb' then begin

; mongodb pattern
; {'range_end': 0, 'range_start': 0, 'signal_name': $plugin, 'creation_date': '{ "$date": 2016-09-07}', 'signal_alias': $abstract, 
;  'description': '', 'source_alias': 'imas', 'signal_class': 'magnetics', 'generic_name': '', 'source_device': $device, 'type': 'P'}

      pattern = strarr(4)
      pattern[0] = "{'range_end': 0, 'range_start': 0, 'signal_name': '" 
      pattern[1] = "', 'creation_date': '{ "+'"$date"'+": 2016-09-07}', 'signal_alias': '" 
      pattern[2] = "', 'description': '', 'source_alias': 'imas', 'signal_class': 'magnetics', 'generic_name': '', 'source_device': '"
      pattern[3] = "', 'type': 'P'}"
      
      print, pattern[0]+plugin+pattern[1]+strupcase(abstract)+pattern[2]+device+pattern[3]
   
   endif else begin

      ;print, 'Verify order of signal_alias, signal_name'
      ;stop
      ;return
 
      if crud eq 'create' then begin
     
;        INSERT INTO Signal_Desc (signal_desc_id, signal_name, signal_alias, generic_name, source_alias, signal_class,
;        description, type, signal_alias_type) VALUES (
;        nextval('signal_desc_id_seq'),
;        '/MAGNETICS/BPOL_PROBE/3/FIELD/DATA', 'imas::source(signal="BPME(3)", source=magn, format=ppf)', 
;        '', 'imas', 'magnetics', 'Internal Discrete Coil, Oct.3', 'P', 0 );
   
         pattern = strarr(3)
         pattern[0] = "INSERT INTO Signal_Desc (signal_desc_id, signal_name, signal_alias, generic_name, source_alias, signal_class, "+$
                      "description, type, signal_alias_type) VALUES ( nextval('signal_desc_id_seq'), '"
         pattern[1] = "', '" 
         pattern[2] = "', '', 'imas', 'magnetics', '', 'P', 0 );"

         print, pattern[0]+plugin+pattern[1]+strupcase(abstract)+pattern[2]
      endif

      if crud eq 'delete' then begin

;        Clean-up: 
; DELETE FROM Signal_Desc WHERE type='P' and source_alias='imas' and signal_class='magnetics' 
; DELETE FROM Signal_Desc WHERE type='P' and source_alias='imas' and signal_alias ilike '/EQUILIBRIUM/EQUILIBRIUM/TIME_SLICE(:)/GLOBAL_QUANTITIES/%'
; DELETE FROM Signal_Desc WHERE type='P' and source_alias='imas' and signal_class='magnetics' and signal_alias ilike '/MAGNETICS/BPOL_PROBE[%]/FIELD/DATA'
; DELETE FROM Signal_Desc WHERE type='P' and source_alias='imas' and signal_class='magnetics' and signal_alias ilike '/MAGNETICS/FLUX_LOOP[%]/FLUX/DATA'
; DELETE FROM Signal_Desc WHERE type='P' and source_alias='imas' and signal_class='magnetics' and signal_alias ilike 'MAGNETICS/BPOL_PROBE/%'
; DELETE FROM Signal_Desc WHERE type='P' and source_alias='imas' and signal_class='magnetics' and signal_alias ilike 'MAGNETICS/FLUX_LOOP/%'

;        DELETE FROM Signal_Desc WHERE type='P' and source_alias='imas' and signal_class='magnetics' and (signal_alias = '' or generic_name = '') 

         pattern = strarr(3)
         pattern[0] = "DELETE FROM Signal_Desc WHERE type='P' and source_alias='imas' and signal_class='magnetics' and (signal_alias = '"
         pattern[1] = "' or generic_name = '"
         pattern[2] = "')"
         print, pattern[0]+abstract+pattern[1]+abstract+pattern[2]

      endif

      if crud eq 'update' then begin

;        UPDATE Signal_Desc SET plugin = '' WHERE type='P' and source_alias='imas' and signal_class='magnetics' and (signal_alias = '' or generic_name = '');

         pattern = strarr(4)
         pattern[0] = "UPDATE Signal_Desc SET plugin = '"
         pattern[1] = "' WHERE type='P' and source_alias='imas' and signal_class='magnetics' and (signal_alias = '"
         pattern[2] = "' or generic_name = '"
         pattern[3] = "')"
         print, pattern[0]+plugin+pattern[1]+abstract+pattern[2]+abstract+pattern[3]

      endif
   
   endelse

return 
end

pro createdb

; Script to read a legacy EFIT++ magnetics xml file and create the SQL to write Magnetics, PF_Active, TF IDS abstractions
; XML is compliant with a legacy EFIT++ standard
; All coordinate data have SI units

   rc  = resetProperties()		; Reset all properties to their default values

   rc = setProperty('debug')		; Client debug option
   rc = setProperty('verbose')
   
   rc = putIdamServerPort(56565L)	 
   rc = putIdamServerHost('idam0')

;----------------------------------------------------------------------------------------------------------------------------------
; Target database

   database = 'postgres'
   ;database = 'mongodb'

;----------------------------------------------------------------------------------------------------------------------------------
; Database CRUD operation

   crud = 'create' ; 'update'  ; 'delete' 
  
;----------------------------------------------------------------------------------------------------------------------------------
; Location of the XML file

   ;xmlfile = '/home/dgm/ITER/efit++/MAST/magnetics/idam.xml'		; MAST
   xmlfile = '/home/dgm/IDAM/test/source/plugins/efitxml/jet.xml'	; JET

;----------------------------------------------------------------------------------------------------------------------------------
; Begin the SQL transaction block

   if database eq 'postgres' then print, 'BEGIN;'
   
;----------------------------------------------------------------------------------------------------------------------------------
; Device Name
         
   a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /Device)','', /noecho)
   
   if a.erc ne 0 then begin
      print, "ERROR parsing XML file'
      return
   endif
      
   device = strupcase(string(a.data))
   ;print, device

;----------------------------------------------------------------------------------------------------------------------------------
; Count of Flux Loops
         
   a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Count)','', /noecho)
   
   abstract = 'magnetics/flux_loop/shape_of'
   plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Count)'     
   printscript, database, crud, abstract, plugin, device 


   fcount = a.data[0]
   ;print, 'flux loop count = ', fcount
   for i=1, fcount do begin
   
     abstract = 'magnetics/flux_loop/'+strtrim(i,2)+'/name'
     plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Name, /UnitStartIndex, id='+strtrim(i,2)+')'      
     printscript, database, crud, abstract, plugin, device 

   endfor
   
   for i=1, fcount do begin
   
     abstract = 'magnetics/flux_loop/'+strtrim(i,2)+'/identifier'
     plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Identifier, /UnitStartIndex, id='+strtrim(i,2)+')'      
     printscript, database, crud, abstract, plugin, device 

   endfor
   
   for i=1, fcount do begin

; Count of Coordinates
 
      a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Position, /count, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)

      abstract = 'magnetics/flux_loop/'+strtrim(i,2)+'/position/shape_of'
      plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Position, /count, /UnitStartIndex, id='+strtrim(i,2)+')' 
      printscript, database, crud, abstract, plugin, device 
     
      pcount = a.data[0]
      for j=1, pcount do begin
  
        abstract = 'magnetics/flux_loop/'+strtrim(i,2)+'/position/'+strtrim(j,2)+'/r'
        plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Position, /R, /UnitStartIndex, id='+strtrim(i,2)+', index='+strtrim(j,2)+')'       
        printscript, database, crud, abstract, plugin, device 

        abstract = 'magnetics/flux_loop/'+strtrim(i,2)+'/position/'+strtrim(j,2)+'/z'
        plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Position, /Z, /UnitStartIndex, id='+strtrim(i,2)+', index='+strtrim(j,2)+')'       
        printscript, database, crud, abstract, plugin, device 

        abstract = 'magnetics/flux_loop/'+strtrim(i,2)+'/position/'+strtrim(j,2)+'/phi'
        plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Position, /Phi, /UnitStartIndex, id='+strtrim(i,2)+', index='+strtrim(j,2)+')'       
        printscript, database, crud, abstract, plugin, device 

      endfor
   endfor

; Measurement Data

   priorsignal = string('')

   for i=1, fcount do begin

; Signal name
; Add an extra ignorable parameter to avoid problem with uniqueness constraint when the signal name is NoData

      a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Signal, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)
 
      datascaling = 1.0d0
      timescaling = 1.0d0
      
      signal = strtrim(string(a.data),2)

      if(a.erc eq 0 and signal ne 'noData') then begin

; Data Scaling Factor

         b = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /DataScaling, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)
         if b.erc eq 0 then datascaling = b.data

; Time Scaling Factor

         c = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /TimeScaling, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)      
         if c.erc eq 0 then timescaling = c.data

      endif
      
      if device eq 'MAST' then begin
         abstract = 'magnetics/flux_loop/'+strtrim(i,2)+'/flux/data'
         if b.erc eq 0 and datascaling ne 1.0d0 then begin
            plugin = 'IMAS::source(signal="'+signal+'", /Data, format=MAST, datascaling='+strtrim(string(datascaling),2)  
         endif else begin
            plugin = 'IMAS::source(signal="'+signal+'", /Data, format=MAST'  
         endelse

	 if(signal ne priorsignal and signal ne 'noData') then begin 
            plugin = plugin + ')'
         endif else begin
            plugin = plugin + ', dummy='+strtrim(i,2)+')' 
         endelse 
	 printscript, database, crud, abstract, plugin, device 

         abstract = 'magnetics/flux_loop/'+strtrim(i,2)+'/flux/time'
         if c.erc eq 0 and timescaling ne 1.0d0 then begin
            plugin   = 'IMAS::source(signal="'+signal+'", /Time, format=MAST, timescaling='+strtrim(string(timescaling),2)        
         endif else begin
            plugin   = 'IMAS::source(signal="'+signal+'", /Time, format=MAST'       
         endelse       
	 if signal ne priorsignal and signal ne 'noData' then begin 
            plugin = plugin + ')'
         endif else begin
            plugin = plugin + ', dummy='+strtrim(i,2)+')' 
         endelse 
	 printscript, database, crud, abstract, plugin, device 
      endif
      
      if device eq 'JET' then begin
	 
	 ppf      = 'magn'	; for all magnetics data - not specified in the XML
	 owner    = 'jetppf'	; default public production data
	 sequence = '0'		; latest / most recent
      
         abstract = 'magnetics/flux_loop/'+strtrim(i,2)+'/flux/data'
	 
         if b.erc eq 0 and datascaling ne 1.0d0 then begin
            plugin = 'IMAS::source(signal="'+signal+'", source='+ppf+', owner='+owner+', pass='+sequence+', /Data, format=PPF, datascaling='+strtrim(string(datascaling),2)  
         endif else begin
            plugin = 'IMAS::source(signal="'+signal+'", source='+ppf+', owner='+owner+', pass='+sequence+', /Data, format=PPF'  
         endelse
         plugin = plugin + ', dummy='+strtrim(i,2)+')'
	 
	 printscript, database, crud, abstract, plugin, device 

         abstract = 'magnetics/flux_loop/'+strtrim(i,2)+'/flux/time'
	 
         if c.erc eq 0 and timescaling ne 1.0d0 then begin
            plugin   = 'IMAS::source(signal="'+signal+'", source='+ppf+', owner='+owner+', pass='+sequence+', /Time, format=PPF, timescaling='+strtrim(string(timescaling),2)        
         endif else begin
            plugin   = 'IMAS::source(signal="'+signal+'", source='+ppf+', owner='+owner+', pass='+sequence+', /Time, format=PPF'       
         endelse       
	 plugin = plugin + ', dummy='+strtrim(i,2)+')'
	 printscript, database, crud, abstract, plugin, device 
      endif
      
      priorsignal = signal

   endfor

;----------------------------------------------------------------------------------------------------------------------------------
; Count of Magnetic Probes
         
   a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Count)','', /noecho)
   
   abstract = 'magnetics/bpol_probe/shape_of'
   plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Count)'     
   printscript, database, crud, abstract, plugin, device 


   fcount = a.data[0]
   ;print, 'Magnetic Probe count = ', fcount
   for i=1, fcount do begin
   
     abstract = 'magnetics/bpol_probe/'+strtrim(i,2)+'/name'
     plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Name, /UnitStartIndex, id='+strtrim(i,2)+')'      
     printscript, database, crud, abstract, plugin, device 

   endfor
   
   for i=1, fcount do begin
   
     abstract = 'magnetics/bpol_probe/'+strtrim(i,2)+'/identifier'
     plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Identifier, /UnitStartIndex, id='+strtrim(i,2)+')'      
     printscript, database, crud, abstract, plugin, device 

   endfor
   
   for i=1, fcount do begin
  
      abstract = 'magnetics/bpol_probe/'+strtrim(i,2)+'/position/r'
      plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Position, /R, /UnitStartIndex, id='+strtrim(i,2)+')'       
      printscript, database, crud, abstract, plugin, device 

      abstract = 'magnetics/bpol_probe/'+strtrim(i,2)+'/position/z'
      plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Position, /Z, /UnitStartIndex, id='+strtrim(i,2)+')'       
      printscript, database, crud, abstract, plugin, device 

      abstract = 'magnetics/bpol_probe/'+strtrim(i,2)+'/position/phi'
      plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Position, /Phi, /UnitStartIndex, id='+strtrim(i,2)+')'       
      printscript, database, crud, abstract, plugin, device 

   endfor

; Measurement Data

   priorsignal = string('')

   for i=1, fcount do begin

; Signal name
; Add an extra ignorable unique parameter to avoid problem with uniqueness constraint when the signal name is NoData

      a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Signal, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)

      datascaling = 1.0d0
      timescaling = 1.0d0

      signal = strtrim(string(a.data),2)

      if(a.erc eq 0 and signal ne 'noData') then begin

; Data Scaling Factor

         b = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /DataScaling, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)
         if b.erc eq 0 then datascaling = b.data

; Time Scaling Factor

         c = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /TimeScaling, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)      
         if c.erc eq 0 then timescaling = c.data

      endif
      
      if device eq 'MAST' then begin
         abstract = 'magnetics/bpol_probe/'+strtrim(i,2)+'/field/data'
         if b.erc eq 0 and datascaling ne 1.0d0 then begin
            plugin = 'IMAS::source(signal="'+signal+'", /Data, format=MAST, datascaling='+strtrim(string(datascaling),2)   
         endif else begin
            plugin = 'IMAS::source(signal="'+signal+'", /Data, format=MAST'  
         endelse    
	 if signal ne priorsignal and signal ne 'noData' then begin 
            plugin = plugin + ')'
         endif else begin
            plugin = plugin + ', dummy='+strtrim(i,2)+')' 
         endelse 
         printscript, database, crud, abstract, plugin, device 

         abstract = 'magnetics/bpol_probe/'+strtrim(i,2)+'/field/time'
         if c.erc eq 0 and timescaling ne 1.0d0 then begin
            plugin   = 'IMAS::source(signal="'+signal+'", /Time, format=MAST, timescaling='+strtrim(string(timescaling),2)        
         endif else begin
            plugin   = 'IMAS::source(signal="'+signal+'", /Time, format=MAST'       
         endelse    
	 if signal ne priorsignal and signal ne 'noData' then begin 
            plugin = plugin + ')'
         endif else begin
            plugin = plugin + ', dummy='+strtrim(i,2)+')' 
         endelse 
         printscript, database, crud, abstract, plugin, device 
      endif
      
      if device eq 'JET' then begin
	 
	 ppf      = 'magn'	; for all magnetics data - not specified in the XML
	 owner    = 'jetppf'	; default public production data
	 sequence = '0'		; latest / most recent
      
         abstract = 'magnetics/bpol_probe/'+strtrim(i,2)+'/field/data'
	 
         if b.erc eq 0 and datascaling ne 1.0d0 then begin
            plugin = 'IMAS::source(signal="'+signal+'", source='+ppf+', owner='+owner+', pass='+sequence+', /Data, format=PPF, datascaling='+strtrim(string(datascaling),2)   
         endif else begin
            plugin = 'IMAS::source(signal="'+signal+'", source='+ppf+', owner='+owner+', pass='+sequence+', /Data, format=PPF'  
         endelse    
	 plugin = plugin + ', dummy='+strtrim(i,2)+')'
         printscript, database, crud, abstract, plugin, device 

         abstract = 'magnetics/bpol_probe/'+strtrim(i,2)+'/field/time'
	 
         if c.erc eq 0 and timescaling ne 1.0d0 then begin
            plugin   = 'IMAS::source(signal="'+signal+'", source='+ppf+', owner='+owner+', pass='+sequence+', /Time, format=PPF, timescaling='+strtrim(string(timescaling),2)        
         endif else begin
            plugin   = 'IMAS::source(signal="'+signal+'", source='+ppf+', owner='+owner+', pass='+sequence+', /Time, format=PPF'       
         endelse    
	 plugin = plugin + ', dummy='+strtrim(i,2)+')'
         printscript, database, crud, abstract, plugin, device 
      endif

      priorsignal = signal

   endfor

;----------------------------------------------------------------------------------------------------------------------------------
; PF_Active Coil Information

; Count of PF Active Coils
         
   a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Count)','', /noecho)
   
   abstract = 'pf_active/coil/shape_of'
   plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Count)'     
   printscript, database, crud, abstract, plugin, device 


   coilcount = a.data[0]
   ;print, 'PF Active Coil count = ', coilcount

   for i=1, coilcount do begin
   
     abstract = 'pf_active/coil/'+strtrim(i,2)+'/name'
     plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Name, /UnitStartIndex, id='+strtrim(i,2)+')'      
     printscript, database, crud, abstract, plugin, device 

   endfor
   
   for i=1, coilcount do begin
   
     abstract = 'pf_active/coil/'+strtrim(i,2)+'/identifier'
     plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Identifier, /UnitStartIndex, id='+strtrim(i,2)+')'      
     printscript, database, crud, abstract, plugin, device 

   endfor

; Array of element coordinates
; Not in the IMAS data model
; Use * character to mean 'All' elements (This will be escaped in the SQL query so will not operate as a special wildcard character)

   for i=1, coilcount do begin
 
      abstract = 'pf_active/coil/'+strtrim(i,2)+'/element/*/geometry/rectangle/r'
      plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Element, /Geometry, /Rectangle, /R, /UnitStartIndex, id='+strtrim(i,2)+')'       
      printscript, database, crud, abstract, plugin, device 
 
      abstract = 'pf_active/coil/'+strtrim(i,2)+'/element/*/geometry/rectangle/z'
      plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Element, /Geometry, /Rectangle, /Z, /UnitStartIndex, id='+strtrim(i,2)+')'       
      printscript, database, crud, abstract, plugin, device 
 
      abstract = 'pf_active/coil/'+strtrim(i,2)+'/element/*/geometry/rectangle/width'
      plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Element, /Geometry, /Rectangle, /Width, /UnitStartIndex, id='+strtrim(i,2)+')'       
      printscript, database, crud, abstract, plugin, device 
 
      abstract = 'pf_active/coil/'+strtrim(i,2)+'/element/*/geometry/rectangle/height'
      plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Element, /Geometry, /Rectangle, /Height, /UnitStartIndex, id='+strtrim(i,2)+')'       
      printscript, database, abstract, plugin, device
 
   endfor
   
   for i=1, coilcount do begin

; Count of Elements/Coordinates
 
      a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Element, /count, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)

      abstract = 'pf_active/coil/'+strtrim(i,2)+'/element/shape_of'
      plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Element, /count, /UnitStartIndex, id='+strtrim(i,2)+')' 
      printscript, database, crud, abstract, plugin, device 
     
      elementcount = a.data[0]
      ;print, 'PF Active Coil Element count = ', elementcount
 
; Individual element coordinates

      for j=1, elementcount do begin 

         abstract = 'pf_active/coil/'+strtrim(i,2)+'/element/'+strtrim(j,2)+'/geometry/rectangle/r'
         plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Element, /Geometry, /Rectangle, /R, /UnitStartIndex, id='+strtrim(i,2)+', index='+strtrim(j,2)+')'       
         printscript, database, crud, abstract, plugin, device 

         abstract = 'pf_active/coil/'+strtrim(i,2)+'/element/'+strtrim(j,2)+'/geometry/rectangle/z'
         plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Element, /Geometry, /Rectangle, /Z, /UnitStartIndex, id='+strtrim(i,2)+', index='+strtrim(j,2)+')'       
         printscript, database, crud, abstract, plugin, device 

         abstract = 'pf_active/coil/'+strtrim(i,2)+'/element/'+strtrim(j,2)+'/geometry/rectangle/width'
         plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Element, /Geometry, /Rectangle, /Width, /UnitStartIndex, id='+strtrim(i,2)+', index='+strtrim(j,2)+')'       
         printscript, database, crud, abstract, plugin, device 

         abstract = 'pf_active/coil/'+strtrim(i,2)+'/element/'+strtrim(j,2)+'/geometry/rectangle/height'
         plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Element, /Geometry, /Rectangle, /Height, /UnitStartIndex, id='+strtrim(i,2)+', index='+strtrim(j,2)+')'       
         printscript, database, crud, abstract, plugin, device 

      endfor
   endfor

; Measurement Data

   priorsignal = string('')		; check duplicate use of 'amc_sol current' is detected

   for i=1, coilcount do begin

; Signal name

      a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /Signal, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)

      datascaling = 1.0d0
      timescaling = 1.0d0

      signal = strtrim(string(a.data),2)

; Data Scaling Factor

      b = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /DataScaling, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)

      datascaling = 1.0d0
      if b.erc eq 0 then datascaling = b.data

; Time Scaling Factor

      c = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /PFActive, /TimeScaling, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)
       
      timescaling = 1.0d0
      if c.erc eq 0 then timescaling = c.data
     
      if device eq 'MAST' then begin
         abstract = 'pf_active/coil/'+strtrim(i,2)+'/current/data'
         if b.erc eq 0 and datascaling ne 1.0d0 then begin
            plugin = 'IMAS::source(signal="'+signal+'", /Data, format=MAST, datascaling='+strtrim(string(datascaling),2)   
         endif else begin
            plugin = 'IMAS::source(signal="'+signal+'", /Data, format=MAST'  
         endelse    
	 if signal ne priorsignal and signal ne 'noData' then begin 
            plugin = plugin + ')'
         endif else begin
            plugin = plugin + ', dummy='+strtrim(i,2)+')' 
         endelse 
         printscript, database, crud, abstract, plugin, device 

         abstract = 'pf_active/coil/'+strtrim(i,2)+'/current/time'
         if c.erc eq 0 and timescaling ne 1.0d0 then begin
            plugin   = 'IMAS::source(signal="'+signal+'", /Time, format=MAST, timescaling='+strtrim(string(timescaling),2)        
         endif else begin
            plugin   = 'IMAS::source(signal="'+signal+'", /Time, format=MAST'       
         endelse    
	 if signal ne priorsignal and signal ne 'noData' then begin 
            plugin = plugin + ')'
         endif else begin
            plugin = plugin + ', dummy='+strtrim(i,2)+')' 
         endelse 
         printscript, database, crud, abstract, plugin, device 
      endif
     
      if device eq 'JET' then begin

	 ppf      = 'magn'	; for all magnetics data - not specified in the XML
	 owner    = 'jetppf'	; default public production data
	 sequence = '0'		; latest / most recent
      
         abstract = 'pf_active/coil/'+strtrim(i,2)+'/current/data'
	 
         if b.erc eq 0 and datascaling ne 1.0d0 then begin
            plugin = 'IMAS::source(signal="'+signal+'", source='+ppf+', owner='+owner+', pass='+sequence+', /Data, format=PPF, datascaling='+strtrim(string(datascaling),2)   
         endif else begin
            plugin = 'IMAS::source(signal="'+signal+'", source='+ppf+', owner='+owner+', pass='+sequence+', /Data, format=PPF'  
         endelse    
	 plugin = plugin + ', dummy='+strtrim(i,2)+')'
         printscript, database, crud, abstract, plugin, device 

         abstract = 'pf_active/coil/'+strtrim(i,2)+'/current/time'
	 
         if c.erc eq 0 and timescaling ne 1.0d0 then begin
            plugin   = 'IMAS::source(signal="'+signal+'", source='+ppf+', owner='+owner+', pass='+sequence+', /Time, format=PPF, timescaling='+strtrim(string(timescaling),2)        
         endif else begin
            plugin   = 'IMAS::source(signal="'+signal+'", source='+ppf+', owner='+owner+', pass='+sequence+', /Time, format=PPF'       
         endelse    
	 plugin = plugin + ', dummy='+strtrim(i,2)+')'
         printscript, database, crud, abstract, plugin, device 
      endif

      priorsignal = signal

   endfor

;----------------------------------------------------------------------------------------------------------------------------------
; End the SQL transaction block

   if database eq 'postgres' then print, 'END;'
      
   
  return
end
