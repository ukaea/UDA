;------------------------------------------------------
; FUNCTION: getgeomsignaldata
; AUTHOR: L. Kogan
; Date: April 2016
;
; Function to retrieve signal geometry information, 
; ie. which components were used to make the signals,
; whether any calibrations were applied, polarity of the 
; signals etc.
;
; Arguments:
;   - namearg : signal name or group to retrieve
;     (eg. /magnetics/mirnov)
;   - sourcearg : shot number for which to retrieve info
;   - versignal : version of the signal file to retrieve. If not set
;                 then the latest version is returned.
;   - addgeom : add the geometry information for the components
;               associated with these signals (eg. R, Z, phi locations)
; 
; Keywords:
;  - Only affects /magnetics/pickup signals
;     - /POLOIDAL : Set this to return the poloidal projection of the
;                   lengths of the pickup coils, and the fraction
;                   of the signal that is measured in R, Z and Phi
;                   directions;   
; 
; Dependencies:
;   - IDAM GEOMETRY plugin
;   - getstruct
;   - getgeomdata
;------------------------------------------------------

;------------------------------------------------------
; Walk signal tree and remove signals which aren't available 
; signaldata: signal data struct
; available: array of signal names that are available
;------------------------------------------------------
function walktreetoremoveunavailable, signaldata, available=available

  forward_function walktreetoremoveunavailable

  alldata = {}

  tag_names_data = tag_names(signaldata)

  ind_dim = where(strmatch(tag_names_data, 'DIMENSIONS') eq 1, count_dim)
  ind_data = where(strmatch(tag_names_data, 'DATA') eq 1, count_data)

  if count_dim gt 0 and count_data gt 0 then begin
     tag_names_here = tag_names((signaldata.data)[0])
     ind_sig_alias = where(strmatch(tag_names_here, 'SIGNAL_ALIAS') eq 1, count_alias)


     if count_alias gt 0 then begin
        sig_alias = (signaldata.data)[0].signal_alias[0]

        ind_match = where(available eq sig_alias, count_match)

        if count_match gt 0 then return, {dimension: signaldata.dimensions, data: signaldata.data} $
        else return, -1
     endif
  endif else begin
     for i = 0, n_elements(tag_names_data)-1 do begin
        if is_structure(signaldata.(i)) then begin
           thisdata = walktreetoremoveunavailable(signaldata.(i), available=available)
        endif else thisdata = signaldata.(i)
        if is_structure(thisdata) then alldata = create_struct(alldata, tag_names_data[i], thisdata)
     endfor
  endelse

  return, alldata

end

;------------------------------------------------------
; Construct the signal data struct, remove any unavailable
; signals if requested & put into the correct structure
; (getting rid of excess data elements)
; datastruct: signal data structure
; keepall: set keyword to keep all signals, even if they are unavailable
;------------------------------------------------------
function constructstruct, datastruct, signal_type, keepall=keepall

  signal_alias_available = datastruct.data.signal_alias_available

  tag_names_first = tag_names(datastruct.data.data)
  ind_signal_alias = where(tag_names_first eq 'SIGNAL_ALIAS', count_alias)

  if not keyword_set(keepall) and signal_type ne 'element' and count_alias eq 0 then begin
     data = walktreetoremoveunavailable(datastruct.data.data, available=signal_alias_available)
  endif else data = datastruct.data.data

  newstruct = { erc: datastruct.erc,       $
                errmsg: datastruct.errmsg, $
                signal: datastruct.signal, $
                source: datastruct.source, $
                handle: datastruct.handle, $
                data: data }

  return, newstruct

end

;------------------------------------------------------
; Extract array element 
;------------------------------------------------------
function getgeomsignal, geomdata, geomcommand

  if geomcommand[0] eq '' then err = execute('array = geomdata.data') $
  else err = execute('array = geomdata.data.'+geomcommand[0]+'.data')

  if err eq 0 then return, []

  return, array

end

;------------------------------------------------------
; Loop recursively over the signal structure to insert appropriate
; geometry component data in the correct locations
;------------------------------------------------------
function walktreetoaddgeom, signaldata, geomdata, geomsignals=geomsignals, geomcommands=geomcommands

  forward_function walktreetoaddgeom

  alldata = {}

  tag_names_data = tag_names(signaldata)

  ind_name = where(strmatch(tag_names_data, 'COMP_NAMES') eq 1, count_name)

  if count_name gt 0 then begin

     ; This is signal level     
     all_structs = list()

     for i = 0, n_elements(signaldata)-1 do begin
        struct_here = signaldata[i]

        comp_names = signaldata[i].comp_names
        comp_names = strtrim(strsplit(comp_names, ',', /extract), 2)
        
        geomstructs = list()

        for j = 0, n_elements(comp_names)-1 do begin
           ind_geom = where(strmatch(geomsignals, comp_names[j]) eq 1, count_geom)
           if count_geom eq 0 then continue

           geomsignal = getgeomsignal(geomdata, geomcommands[ind_geom])

           if not undefined(geomsignal) then geomstructs.add, geomsignal
        endfor

        if n_elements(geomstructs) gt 0 then struct_here = create_struct(struct_here, 'geomsignals', geomstructs)

        all_structs.add, struct_here
     endfor

     return, all_structs

  endif else begin
     for i = 0, n_elements(tag_names_data)-1 do begin
        if is_structure(signaldata.(i)) then begin
           thisdata = walktreetoaddgeom(signaldata.(i), geomdata, geomsignals=geomsignals, geomcommands=geomcommands)
        endif else thisdata = signaldata.(i)
        alldata = create_struct(alldata, tag_names_data[i], thisdata)
     endfor
  endelse

  return, alldata

end

;------------------------------------------------------
; Check for lowest group level which contains all geomsignals
; (ie. if geomsignals is [/magnetics/pickup/outerVessel/X/X/X,
; /magnetics/pickup/centreColumn/X/X/X] then the matching group is
; /magnetics/pickup. 
;------------------------------------------------------
function findmatch, geomsignals
  forward_function findmatch

  if n_elements(geomsignals) eq 1 then return, geomsignals[0]

  ; Check there is a '/' left
  ind_slash = strpos(geomsignals, '/')
  if ind_slash[0] lt 0 then return, ""

  ; First remove leading '/'
  signals = strmid(geomsignals, 1)

  ; Next, find next '/'
  ind_first = strpos(signals, '/') 
  signals_first = strmid(signals, 0, transpose(ind_first))
  signals_end = strmid(signals, transpose(ind_first))
  
  lead_uniq = signals_first[uniq(signals_first, sort(signals_first))]

  if n_elements(lead_uniq) gt 1 then return, ""

  return, '/'+lead_uniq+findmatch(signals_end)

end

;------------------------------------------------------
; Loop over signal structure to extract the component names
;------------------------------------------------------
function walktreeforgeomsignals, signalstruct
  forward_function walktreeforgeomsignals

  allsignals = []

  
  tag_names_data = tag_names(signalstruct)

  ind_name = where(strmatch(tag_names_data, 'COMP_NAMES') eq 1, count_name)

  if count_name gt 0 then begin
     coilnames = strsplit(signalstruct[0].comp_names, ',', /extract)

     if n_elements(signalstruct) gt 1 then begin
        for i=0, n_elements(signalstruct)-1 do coilnames = [coilnames, strsplit(signalstruct[1].comp_names, ',', /extract)]
     endif

     coilnames = strtrim(coilnames, 2)

     allsignals = [allsignals, coilnames]
     
  endif else begin
     for i = 0, n_elements(tag_names_data)-1 do begin
        if is_structure(signalstruct.(i)) then begin
           signals_here = walktreeforgeomsignals(signalstruct.(i))
        endif else signals_here = []
        allsignals = [allsignals, signals_here]
     endfor
  endelse
  
  return, allsignals
end

;------------------------------------------------------
; Retrieve signal information and add corresponding geometry info
; for the components used for the signal.
;------------------------------------------------------
function getgeomsignaldata, namearg, sourcearg,  $
                            versignal=versignal, $
                            addgeom=addgeom,     $
                            keepall=keepall,     $
                            _extra=_extra

  ; Check we have been given a signal and source
  errmsg=''
  if not_string(namearg) then begin
     errmsg='Signal not defined'
     goto, fatalerror
  endif

  if undefined(sourcearg) then begin
    errmsg='Source Argument Undefined'
    goto, fatalerror
  endif

  if not_integer(sourcearg) then begin
     errmsg='Source argument must a pulse number.'
     goto, fatalerror
  endif

  ; If there is a / at the end, remove it
  if strpos(namearg[0], '/', /reverse_search) eq (strlen(namearg[0])-1) then begin
     namearg = strmid(namearg[0], 0, strpos(namearg[0], '/', /reverse_search))
  endif

  ; If there is not an / at the beginning, add one
  if strpos(namearg[0], '/') ne 0 then begin
     namearg = '/'+namearg[0]
  endif

  ; Map for different signals? (eg. pickup, fluxloops etc)

  ; Get data
  workname = 'GEOM::getSignalFile(signal='+namearg[0]+')'
  worksource = strtrim(sourcearg[0], 2)
  signalstruct = getstruct(workname, worksource)  

  if not_structure(signalstruct) and is_string(signalstruct) then begin
     errmsg = signalstruct
     goto, errorcatch
  endif else if not_structure(signalstruct) then begin
     errmsg = 'Error retrieving '+workname+' from IDAM'
     goto, fatalerror
  endif

  if signalstruct.erc ne 0 then begin
     errmsg = signalstruct.errmsg
     goto, errorcatch
  endif

  ; If they don't want to keep all signals, only those that
  ; are available, then remove any signals that are unavailable for
  ; this shot.
  ; Otherwise, get rid of the data.data thing!
  signalstruct = constructstruct(signalstruct, signalstruct.data.signal_type, keepall=keepall)

  ; If they haven't asked for any geometry info, just return signal struct
  if ~keyword_set(addgeom) then return, signalstruct

  ; Extract geom signals  
  geomsignals = walktreeforgeomsignals(signalstruct)

  if undefined(geomsignals) or is_structure(geomsignals) then begin
     errmsg = 'No geom info was found for the signals retrieved'
     goto, fatalerror
  endif

  geomsignals = geomsignals[uniq(geomsignals, sort(geomsignals))]

  ; Find last matching group for geom signals
  matching_group = findmatch(geomsignals)

  if matching_group eq "" then return, signalstruct

  ; Retrieve geom info for this group
  geomdata = getgeomdata(matching_group, sourcearg, _extra=_extra)
  
  if not_structure(geomstruct) and is_string(geomstruct) then begin
     errmsg = 'Error retrieving geom data '+matching_group+' from IDAM, ERROR: '+geomstruct
     goto, geomerrorcatch
  endif else if not_structure(signalstruct) then begin
     errmsg = 'Error retrieving geom data '+matching_group+' from IDAM'
     goto, geomerrorcatch
  endif
  
  ; Convert geom signals into commands
  ; etc needed to extract these geometry
  ; signals from the structure
  geomcommands = strmid(geomsignals, strlen(matching_group))
  ind_first = strpos(geomcommands, '/')

  if n_elements(geomcommands) gt 1 then geomcommands = strmid(geomcommands, transpose(ind_first)) $
  else geomcommands = strmid(geomcommands, ind_first)

  for i = 0, n_elements(geomcommands)-1 do geomcommands[i] = strjoin(strsplit(geomcommands[i], '/', /extract), '.')

  ; Loop over signal data, adding in geom signals
  signalstruct = walktreetoaddgeom(signalstruct, geomdata, geomsignals=geomsignals, geomcommands=geomcommands)

  return, signalstruct

geomerrorcatch:
  if ~keyword_set(noecho) then print, 'GETGEOMSIGNALDATA: '+errmsg
  return, signalstruct

errorcatch:

  if ~keyword_set(noecho) then print, '['+worksource+', '+namearg[0]+'] '+errmsg
  return, {erc:-1, errmsg:errmsg}

fatalerror:
  print, 'GETGEOMSIGNALDATA: '+errmsg
  print, 'GETGEOMSIGNALDATA: Calling format - result=GETGEOMSIGNALDATA(signal, source)'

  return, {erc: -1, errmsg: errmsg}

end
