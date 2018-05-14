;------------------------------------------------------
; FUNCTION: getgeomdata
; AUTHOR: L. Kogan
; DATE: March 2016
;
; Function to retrieve geometry data (and calibration for the geometry
; data) from IDAM, perform any required manipulations and return
; a structure containing the data.
;
; Arguments:
;  - namearg : Name of the signal (eg. /magnetics/pickup)
;  - sourcearg : Either the shot number or file name 
;
; Keywords:
;
;  - ADDSIGNALS : Set this keyword to include information about the
;                 signals associated with the geometry components.
;  - VERSIGNAL : Set this to a version number for the signal file, 
;                to retrieve a specfic version. Otherwise the latest
;                version for the shot number requested is retrieved.
;  - TORANGLE : Give the toroidal angle at which to slice the 3D
;               model. For components which have no toroidal depedence
;               this is not needed. If keyword THREED is set then this
;               keyword is ignored.
;  - THREED : If set the 3D model is returned, otherwise the 2D model
;    is returned, sliced at the angle specified in torangle
;
;  - Only affects /magnetics/pickup signals
;     - /POLOIDAL : Set this to return the poloidal projection of the
;                   lengths of the pickup coils, and the fraction
;                   of the signal that is measured in R, Z and Phi
;                   directions
;
;  Dependencies:
;   - IDAM GEOMETRY plugin
;   - getstruct
;   - applygeommanipulation
;   - addsignaldata
;------------------------------------------------------

;------------------------------------------------------
; Recursively loop over the configdata and caldata structures
; and calibrate any variables with variables with matching
; names in the two structures.
; If caltype = 1 then the calibration is relative and
;   data = configuration + calibration
; If caltype = 2 then the calibration is absolute and
;   data = calibration
;------------------------------------------------------
function calibrationloop, configdata, caldata, caltype=caltype
  forward_function calibrationloop

  if undefined(caltype) then caltype = 'RELATIVE'

  tag_names_config = tag_names(configdata)
  tag_names_cal = tag_names(caldata)

  ; First, loop over all config
  ; elements. If they are
  ; structures, don't add to new structure
  new_struct = {}

  for i = 0, n_elements(tag_names_config)-1 do begin
     ; Is it in cal?
     ind_match = where(strmatch(tag_names_cal, tag_names_config[i]) eq 1, count_cal)

     if count_cal eq 0 then new_struct = create_struct(new_struct, tag_names_config[i], configdata.(i))
  endfor

  ; Next, loop over cal, looking for
  ; structures in config and anything in
  ; cal that isn't in config
  for i = 0, n_elements(tag_names_cal)-1 do begin
     ind_match = where(strmatch(tag_names_config, tag_names_cal[i]) eq 1, count_config)

     if count_config eq 0 then begin
        new_struct = create_struct(new_struct, tag_names_cal[i], caldata.(i))
     endif else if is_structure(configdata.(ind_match[0])) then begin
        toadd = []
        for j = 0, n_elements(configdata.(ind_match[0]))-1 do begin
           toadd = [toadd, calibrationloop(configdata.(ind_match[0])[j], caldata.(i)[j], caltype=caltype)]
        endfor

        new_struct = create_struct(new_struct, tag_names_config[ind_match[0]], toadd)
     endif else begin
        if is_number(caldata.(i)) and tag_names_cal[i] ne 'TYPE' and tag_names_cal[i] ne 'STATUS' then begin
           if caltype eq 'RELATIVE' then begin
              ; Relative calibration: add
              calibrated_data = configdata.(ind_match[0]) + caldata.(i)
           endif else begin
              ; Absolute calibration: replace
              calibrated_data =caldata.(i)
           endelse
           new_struct = create_struct(new_struct, tag_names_config[ind_match[0]], calibrated_data)
        endif else new_struct = create_struct(new_struct, tag_names_config[ind_match[0]], configdata.(ind_match[0]))
     endelse
  endfor

  return, new_struct

end

;------------------------------------------------------
; Recursively loop over configuration and calibration structures.
; Look for variables in calibration with "_CAL" at the end, where
; the rest of the name matches that in the configuration data.
; These variables need to be calibrated.
;------------------------------------------------------
function combineconfigcalloop, configdata, caldata
  forward_function combineconfigcalloop

  if not_structure(configdata) or not_structure(caldata) then return, {}


  tag_names_config = tag_names(configdata)
  tag_names_cal = tag_names(caldata)

  ind_match = where(strmatch(tag_names_cal, 'CALIBRATION') eq 1, count_match)
  if count_match gt 0 then is_calibration = (caldata.calibration eq 'True') $    
  else is_calibration = 0 

  combineddata = {}

  ; First, add all config tags that aren't going to be calibrated
  for i = 0, n_elements(tag_names_config)-1 do begin
     ind_match = where(strmatch(tag_names_cal, tag_names_config[i]) eq 1, count_cal) 
     
     if count_cal eq 0 and is_calibration then combineddata = create_struct(combineddata, tag_names_config[i], configdata.data.(i)) $
     else if count_cal eq 0 then combineddata = create_struct(combineddata, tag_names_config[i],configdata.(i))
  endfor

  if is_calibration then begin
     if tag_exists(caldata.data, 'TYPE') then caltype = caldata.data[0].type $
     else caltype = 'RELATIVE'

     toadd = calibrationloop(configdata.data[0], caldata.data[0], caltype=caltype)

     combineddata = create_struct(combineddata, 'data', toadd)
  endif else begin
     for i = 0, n_elements(tag_names_cal)-1 do begin     
        ind_match = where(strmatch(tag_names_config, tag_names_cal[i]) eq 1, count_config)
        if count_config gt 0 then begin
           toadd = combineconfigcalloop(configdata.(ind_match[0]), caldata.(i))
           if exists(toadd) then combineddata = create_struct(combineddata, tag_names_config[ind_match[0]], toadd)
        endif
     endfor
  endelse

  return, combineddata

end

;------------------------------------------------------
; Combine configuration and calibration data.
; If signal is a group, then need to search for calibratable elements.
; If signal is an array, then need to calibrate each element.
; If signal asked for was an element of an array, need to find correct element and calibrate.
;------------------------------------------------------
function combineconfigcal, configstruct, calstruct, signal, signal_type

  if (signal_type eq 'element') then begin     

     if tag_exists(calstruct, 'TYPE') then caltype = calstruct.type $
     else caltype = 'RELATIVE'

     combineddata = calibrationloop(configstruct, calstruct, caltype=caltype)

;     combineddata = data_structs

     return, combineddata
  endif else begin
     return, combineconfigcalloop(configStruct, calStruct)
  endelse

  return, -1

end

;------------------------------------------------------
; Reads in geometry data and calibration, makes the
; calibration and returns the information.
; namearg: signal that is requested
; sourcearg: either shotnumber or filename
;------------------------------------------------------
function getgeomdata, namearg, sourcearg,     $
                      filecal=filecal,        $
                      vercal=vercal,          $
                      verconfig=verconfig,    $
                      addsignals=addsignals,  $
                      versignal=versignal,    $
                      torangle=torangle,      $
                      threed=threed,          $
                      nocal=nocal,            $
                      debug=debug,            $
                      _extra=_extra 

  ; Check we have been given a signal and source
  errmsg=''
  if not_string(namearg) then begin
     errmsg='Signal not defined'
     goto, fatalerror
  endif

  isfile = 0B
  if undefined(sourcearg) then begin
    errmsg='Source Argument Undefined. Please give either a pulse number or for local files a filepath to .nc file'
    goto, fatalerror
  endif else if is_string(sourcearg) and (strpos(sourcearg[0], '.nc', /reverse_search) ne -1) then begin
     isfile = 1B
  endif

  if (exists(filecal) and not_string(filecal)) then begin
     errmsg='filecal must be a string (containing filepath and name), if it is set.'
     goto, fatalerror
  endif

  ; If there is a / at the end, remove it
  if strpos(namearg[0], '/', /reverse_search) eq (strlen(namearg[0])-1) then begin
     namearg = strmid(namearg[0], 0, strpos(namearg[0], '/', /reverse_search))
  endif

  ; If there is not an / at the beginning, add one
  ;if strpos(namearg[0], '/') ne 0 then begin
  ;   namearg = '/'+namearg[0]
  ;endif
  if not isfile then begin
    ; Signal map: if asking for very top
    ; levels may need to read in multiple files
     filenames_call = 'GEOM::getConfigFilenames(signal='+strtrim(namearg[0])+')'
     multiple_names = getstruct(filenames_call, sourcearg[0])

     if multiple_names.erc eq 0 then begin
        allsignals = multiple_names.data.geomgroups
        allsignals = allsignals[uniq(allsignals, sort(allsignals))]

        if n_elements(allsignals) gt 1 then begin
           allsignals = multiple_names.data.geomgroups 
           allsignals = strmid(allsignals, 0, transpose(strpos(allsignals, '/', /reverse_search)))
        endif else allsignals = namearg[0]
     endif else begin
        errmsg='Signal was not recognized'
        goto, fatalerror
     endelse
  endif else begin
     allsignals = namearg[0]
  endelse

  datastruct = {}
  signalinfostruct = {}

  idamcalls = []
  idamhandles = []

  warning = ''

  ; Source : shot number if given.
  if exists(sourcearg) and not isfile then worksource = strtrim(sourcearg[0], 2) $
  else worksource = ''

  for i = 0, n_elements(allsignals)-1 do begin
     configname = strtrim(allsignals[i], 2)

     ; Retrieve configuration data
     workname = 'GEOM::get(signal='+strtrim(allsignals[i])+', '
                                            
     if isfile then workname = workname+'file='+strtrim(sourcearg[0],2)+', '
     if exists(verconfig) then workname = workname+'version='+strtrim(verconfig[0], 2)+', '
     if exists(torangle) and not keyword_set(threed) then workname = workname+'tor_angle='+strtrim(torangle[0], 2)+', '
     if keyword_set(threed) then workname = workname+'three_d=True, '
     if not isfile then workname = workname+'config=1)' $
     else workname = workname+')'
    
     idamcalls = [idamcalls, workname]
     idamhandles = [idamhandles, workname]

     if keyword_set(debug) then print, 'getgeomdata : config data call : ', workname

     configstruct = getstruct(workname, worksource)
     if not_structure(configstruct) and is_string(configstruct) then begin
        errmsg = configstruct
        goto, errorcatch
     endif else if not_structure(configstruct) then begin
        errmsg = 'Error retrieving '+workname+' from IDAM'
        goto, fatalerror
     endif
     
     rc=freeidam(configstruct.handle)

     if configstruct.erc ne 0 then begin
        errmsg = configstruct.errmsg
        goto, errorcatch
     endif     

     signal_type = configstruct.data.signal_type
     combinedstruct = configstruct.data.data

     if ~keyword_set(nocal) then begin
        ; Adjust call for retrieving calibration data
        workname = 'GEOM::get(signal='+strtrim(allsignals[i])+', '
        if exists(filecal) then workname = workname+'file='+strtrim(filecal[0],2)+', '
        if exists(verconfig) then workname = workname+'version='+strtrim(verconfig[0], 2)+', '
        if exists(vercal) then workname = workname+'version_cal='+strtrim(vercal[0], 2)+', '
        if exists(torangle) then workname = workname+'tor_angle='+strtrim(torangle[0], 2)+', '
        if keyword_set(threed) then workname = workname+'three_d=True, '
        if undefined(filecal) then workname = workname+'cal=1)' $
        else workname = workname + ')'

        ; Retrieve calibration data
        if worksource ne '' or exists(filecal) then begin
           idamcalls = [idamcalls, workname]
           idamhandles = [idamhandles, workname]

           if keyword_set(debug) then print, 'getgeomdata : cal data call : ', workname

           calstruct = getstruct(workname, worksource)
           rc=freeidam(calstruct.handle)
     
           if calstruct.erc eq 0 then begin
              if tag_exists(calstruct, 'data') then begin
                 ; Combine configuration and
                 ; calibration data
                 combinedstruct = combineconfigcal(configstruct.data.data, calstruct.data.data, allsignals[i], signal_type)
              endif
           endif else begin
              warning = warning+':: No calibration data was found for '+allsignals[i]
           endelse
        endif else begin
           warning = warning+':: No calibration data was requrested for '+allsignals[i]
        endelse
     endif else begin
        warning = warning+':: nocal keyword was set so no calibration was applied'
     endelse

     ; Apply any transformations etc. that
     ; have been asked for. (eg. projection to poloidal plane)
     combinedstruct = applygeommanipulation(combinedstruct, allsignals[i], _extra=_extra)

     if n_elements(allsignals) gt 1 then begin
        tag_name_signal = strmid(allsignals[i], strpos(allsignals[i], '/', /reverse_search)+1)
        datastruct = create_struct(datastruct, tag_name_signal, combinedstruct)
     endif else begin
        datastruct = combinedstruct
     endelse

     if keyword_set(addsignals) then begin
        workname = 'GEOM::getSignalFilename(geomsignal="'+allsignals[i]+'"'       
        
        if exists(versignal) and is_integer(versignal) then workname = workname+', version='+strtrim(versignal, 2)
        workname = workname+')'    

        siginfo_struct = getstruct(workname, worksource)

        if siginfo_struct.erc eq 0 and signalinfostruct eq !NULL then begin
           signalinfostruct = siginfo_struct.data
        endif else if siginfo_struct.erc eq 0 then begin
           signalinfostruct.filenames = [signalinfostruct.filenames, siginfo_struct.filenames]
           signalinfostruct.geom_alias = [signalinfostruct.geom_alias, siginfo_struct.geom_alias]
           signalinfostruct.signal_alias = [signalinfostruct.signal_alias, siginfo_struct.signal_alias]
           signalinfostruct.var_name = [signalinfostruct.var_name, siginfo_struct.var_name]
        endif
     endif
  endfor

  datastruct = {erc: 0, errmsg: '', warnmsg: warning, signal: idamcalls, source: sourcearg, handle: idamhandles, data: datastruct}

  ; If asked for, also retrieve mapping to signal data.
  if keyword_set(addsignals) and signalinfostruct ne !NULL then begin
     init_signal = namearg[0]
     addsignaldata, datastruct, signalinfostruct, init_signal, sourcearg, versignal=versignal, debug=debug
  endif

  return, datastruct

errorcatch:

  if ~keyword_set(noecho) then print, '['+worksource+', '+namearg[0]+'] '+errmsg
  return, {erc:-1, errmsg:errmsg}

fatalerror:
  print, 'GETGEOMDATA: '+errmsg
  print, 'GETGEOMDATA: Calling format - result=GETGEOMDATA(signal, source)'

  return, {erc: -1, errmsg: errmsg}

end
