; Create IDAM abstractions for IMAS PF_ACTIVE IDS
; Create the IDAM Database entries with PF_ACTIVE Data

; cached
; PF_ACTIVE/COIL[1]/ELEMENT[1]/GEOMETRY/RECTANGLE/R
; PF_ACTIVE/COIL[1]/ELEMENT[1]/GEOMETRY/RECTANGLE/Z
; PF_ACTIVE/COIL[1]/ELEMENT[1]/GEOMETRY/RECTANGLE/WIDTH
; PF_ACTIVE/COIL[1]/ELEMENT[1]/GEOMETRY/RECTANGLE/HEIGHT
; PF_ACTIVE/COIL[1]/ELEMENT[1]/TURNS_WITH_SIGN

; dynamic
; PF_ACTIVE/COIL[1]/ELEMENT[1]/CURRENT/DATA
; PF_ACTIVE/COIL[1]/ELEMENT[1]/CURRENT/TIME

; Database Schema for PF Coil description

;--------------------------------------------------------------------------------------
; Read a CSV file with PF_ACTIVE data and extract the Id, Name and signal
; Uniqueness constraint: identifier, name, signal_alias, range_start, range_end

pro readcsv, file, database=database, count=count

   count = 0L
   result = READ_CSV(file, count=count, HEADER=header)

   print, 'count = ', count
   print, header
   
   if database eq 'mongodb' then begin

      for j=0, count-1 do begin   
         print, "{'range_end': 0, 'range_start': 0, " +$
                "'identifier': '"+strtrim(string(result.field02[j]),2)+"', " +$
                "'name': '"+result.field03[j]+"', " +$
	        "'signal': '"+result.field04[j]+"', " +$
	        "'device': 'MAST'"
      endfor
         
   endif else begin
   
      dbname = 'pf_active_ids'

      for j=0, count-1 do begin
         print, "INSERT INTO "+dbname+" ("+dbname+"_id, identifier, name, signal_alias, range_start, range_end) VALUES ("+$
                "nextval('"+dbname+"_id_seq'), '"+strtrim(string(result.field02[j]),2)+"', '"+result.field03[j]+"', '"+result.field04[j]+"', 0 ,0);"   
      endfor

   endelse
   
   return
end

pro printscript, database, abstract, plugin, device

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
      
;INSERT INTO Signal_Desc (signal_desc_id, signal_name, signal_alias, generic_name, source_alias, signal_class,
;        description, type, signal_alias_type) VALUES (
;        nextval('signal_desc_id_seq'),
;        '/MAGNETICS/BPOL_PROBE[1]/FIELD/DATA', 'imas::source(signal="BPME(1)", source=magn, format=ppf)', 
;        '', 'imas', 'magnetics', 'Internal Discrete Coil, Oct.3', 'P', 0 );
   
      pattern = strarr(3)
      pattern[0] = "INSERT INTO Signal_Desc (signal_desc_id, signal_name, signal_alias, generic_name, source_alias, signal_class, "+$
                   "description, type, signal_alias_type) VALUES ( nextval('signal_desc_id_seq'), '"
      pattern[1] = "', '" 
      pattern[2] = "', '', 'imas', 'magnetics', '', 'P', 0 );"

      print, pattern[0]+plugin+pattern[1]+strupcase(abstract)+pattern[2]

   
   endelse

return 
end

pro createdb

; Script to read an EFIT++ magnetics xml file and create the SQL to write Magnetics IDS abstractions

   rc  = resetProperties()		; Reset all properties to their default values

   rc = setProperty('debug')		; Client debug option
   rc = setProperty('verbose')
   
   rc = putIdamServerPort(56565L)	 
   rc = putIdamServerHost('idam0')

;----------------------------------------------------------------------------------------------------------------------------------
; Target database

   ;database = 'postgres'
   database = 'mongodb'
   
;----------------------------------------------------------------------------------------------------------------------------------
; Location of the XML file

   csvfile = "/home/dgm/IDAM/test/source/plugins/livedisplay/EFIT/output_tables/pfCoils_29916_t0.24.csv"
   
;----------------------------------------------------------------------------------------------------------------------------------
; Read the CSV file for the PF Coil identifiers
   
  readcsv, csvfile, database=database, count=count
  print,'+++ count= ',count
  return

;----------------------------------------------------------------------------------------------------------------------------------
; Device Name
         
   a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /Device)','', /noecho)
   
   device = strupcase(string(a.data))
   ;print, device

;----------------------------------------------------------------------------------------------------------------------------------
; Count of Flux Loops
         
   a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Count)','', /noecho)
   
   abstract = 'magnetics/flux_loop/shape_of'
   plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Count)'     
   printscript, database, abstract, plugin, device 


   fcount = a.data[0]
   ;print, 'flux loop count = ', fcount
   for i=1, fcount do begin
   
     abstract = 'magnetics/flux_loop/'+strtrim(i,2)+'/name'
     plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Name, /UnitStartIndex, id='+strtrim(i,2)+')'      
     printscript, database, abstract, plugin, device 

   endfor
   
   for i=1, fcount do begin
   
     abstract = 'magnetics/flux_loop/'+strtrim(i,2)+'/identifier'
     plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Identifier, /UnitStartIndex, id='+strtrim(i,2)+')'      
     printscript, database, abstract, plugin, device 

   endfor
   
   for i=1, fcount do begin

; Count of Coordinates
 
      a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Position, /count, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)

      abstract = 'magnetics/flux_loop/'+strtrim(i,2)+'/position/shape_of'
      plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Position, /count, /UnitStartIndex, id='+strtrim(i,2)+')' 
      printscript, database, abstract, plugin, device 
     
      pcount = a.data[0]
      for j=1, pcount do begin
  
        abstract = 'magnetics/flux_loop/'+strtrim(i,2)+'/position/'+strtrim(j,2)+'/r'
        plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Position, /R, /UnitStartIndex, id='+strtrim(i,2)+', index='+strtrim(j,2)+')'       
        printscript, database, abstract, plugin, device 

        abstract = 'magnetics/flux_loop/'+strtrim(i,2)+'/position/'+strtrim(j,2)+'/z'
        plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Position, /Z, /UnitStartIndex, id='+strtrim(i,2)+', index='+strtrim(j,2)+')'       
        printscript, database, abstract, plugin, device 

        abstract = 'magnetics/flux_loop/'+strtrim(i,2)+'/position/'+strtrim(j,2)+'/phi'
        plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Position, /Phi, /UnitStartIndex, id='+strtrim(i,2)+', index='+strtrim(j,2)+')'       
        printscript, database, abstract, plugin, device 

      endfor
   endfor

; Measurement Data

   for i=1, fcount do begin

; Signal name
; Add an extra parameter to avoid problem with uniqueness constraint when the signal name is NoData

      a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /FluxLoop, /Signal, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)
      
      if device eq 'MAST' then begin
         abstract = 'magnetics/flux_loop/'+strtrim(i,2)+'/flux/data'
         if(strtrim(string(a.data)) ne 'noData') then begin
	    plugin = 'IMAS::source(signal="'+strtrim(string(a.data))+'", /Data, format=MAST)'      
         endif else begin
	    plugin = 'IMAS::source(signal="'+strtrim(string(a.data))+'", /Data, format=MAST, dummy='+strtrim(i,2)+')'      
         endelse
	 printscript, database, abstract, plugin, device 

         abstract = 'magnetics/flux_loop/'+strtrim(i,2)+'/flux/time'
         if(strtrim(string(a.data)) ne 'noData') then begin
            plugin = 'IMAS::source(signal="'+strtrim(string(a.data))+'", /Time, format=MAST)'     
         endif else begin
            plugin = 'IMAS::source(signal="'+strtrim(string(a.data))+'", /Time, format=MAST, dummy='+strtrim(i,2)+')'     
         endelse
	 printscript, database, abstract, plugin, device 

      endif

      if device eq 'JET' then begin
         abstract = 'magnetics/flux_loop/'+strtrim(i,2)+'/flux/data'
         if(strtrim(string(a.data)) ne 'noData') then begin
            plugin = 'IMAS::source(signal="'+strtrim(string(a.data))+'", /Data, source=magn, format=ppf)'      
         endif else begin
            plugin = 'IMAS::source(signal="'+strtrim(string(a.data))+'", /Data, source=magn, format=ppf, dummy='+strtrim(i,2)+')'       
         endelse
	 printscript, database, abstract, plugin, device 

         abstract = 'magnetics/flux_loop/'+strtrim(i,2)+'/flux/time'
         if(strtrim(string(a.data)) ne 'noData') then begin
            plugin = 'IMAS::source(signal="'+strtrim(string(a.data))+'", /Time, source=magn, format=ppf)'     
         endif else begin
            plugin = 'IMAS::source(signal="'+strtrim(string(a.data))+'", /Time, source=magn, format=ppf, dummy='+strtrim(i,2)+')'     
         endelse
	 printscript, database, abstract, plugin, device 

      endif

   endfor

;----------------------------------------------------------------------------------------------------------------------------------
; Count of Magnetic Probes
         
   a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Count)','', /noecho)
   
   abstract = 'magnetics/bpol_probe/shape_of'
   plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Count)'     
   printscript, database, abstract, plugin, device 


   fcount = a.data[0]
   ;print, 'Magnetic Probe count = ', fcount
   for i=1, fcount do begin
   
     abstract = 'magnetics/bpol_probe/'+strtrim(i,2)+'/name'
     plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Name, /UnitStartIndex, id='+strtrim(i,2)+')'      
     printscript, database, abstract, plugin, device 

   endfor
   
   for i=1, fcount do begin
   
     abstract = 'magnetics/bpol_probe/'+strtrim(i,2)+'/identifier'
     plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Identifier, /UnitStartIndex, id='+strtrim(i,2)+')'      
     printscript, database, abstract, plugin, device 

   endfor
   
   for i=1, fcount do begin

; Count of Coordinates
 
      a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Position, /count, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)

      abstract = 'magnetics/bpol_probe/'+strtrim(i,2)+'/position/shape_of'
      plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Position, /count, /UnitStartIndex, id='+strtrim(i,2)+')' 
      printscript, database, abstract, plugin, device 
     
      pcount = a.data[0]
      ;print, 'Magnetic Probe Position count = ', pcount
  
        abstract = 'magnetics/bpol_probe/'+strtrim(i,2)+'/position/r'
        plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Position, /R, /UnitStartIndex, id='+strtrim(i,2)+', index='+strtrim(j,2)+')'       
        printscript, database, abstract, plugin, device 

        abstract = 'magnetics/bpol_probe/'+strtrim(i,2)+'/position/z'
        plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Position, /Z, /UnitStartIndex, id='+strtrim(i,2)+', index='+strtrim(j,2)+')'       
        printscript, database, abstract, plugin, device 

        abstract = 'magnetics/bpol_probe/'+strtrim(i,2)+'/position/phi'
        plugin   = 'EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Position, /Phi, /UnitStartIndex, id='+strtrim(i,2)+', index='+strtrim(j,2)+')'       
        printscript, database, abstract, plugin, device 

   endfor

; Measurement Data

   for i=1, fcount do begin

; Signal name

      a = getdata('EFITMAGXML::get(xmlfile='+xmlfile+', /MagProbe, /Signal, /UnitStartIndex, id='+strtrim(i,2)+')' ,'', /noecho)
      
      if device eq 'MAST' then begin
         abstract = 'magnetics/bpol_probe/'+strtrim(i,2)+'/field/data'
         plugin = 'IMAS::source(signal="'+strtrim(string(a.data))+'", /Data, format=MAST)'      
         printscript, database, abstract, plugin, device 

         abstract = 'magnetics/bpol_probe/'+strtrim(i,2)+'/field/time'
         plugin   = 'IMAS::source(signal="'+strtrim(string(a.data))+'", /Time, format=MAST)'     
         printscript, database, abstract, plugin, device 

      endif

      if device eq 'JET' then begin	
         abstract = 'magnetics/bpol_probe/'+strtrim(i,2)+'/field/data'
         plugin = 'IMAS::source(signal="'+strtrim(string(a.data))+'", /Data, source=magn, format=ppf)'      
         printscript, database, abstract, plugin, device 

         abstract = 'magnetics/bpol_probe/'+strtrim(i,2)+'/field/time'
         plugin   = 'IMAS::source(signal="'+strtrim(string(a.data))+'", /Time, source=magn, format=ppf)'     
         printscript, database, abstract, plugin, device 

      endif

   endfor
   
   
  return
end
