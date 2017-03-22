;------------------------------------------------------
; PROCEDURE: addsignaldata
; AUTHOR: L. Kogan
; DATE: April 2016
;
; Procedure to add the signal data to the geometry data structure.
; It retrieves the appropriate signal filenames from IDAM and reads
; in signal data for those signals.
;
; Arguments:
;   - datastruct : the geometry data structure
;   - init_signal : the geometry signal requested by the user
;   - source : the shot number requested by the user
;   
; Keywords:
;   - versignal : The version number for the signal file
;------------------------------------------------------

function findmatch, geomsignals
  forward_function findmatch

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
; Get the signal from the signal structure
;------------------------------------------------------
function getsignal, datastruct, group, type, name

  command = 'array = datastruct.'+group+'.data.'+type
  if type eq '' then command = 'array = datastruct.'+group

  err = execute(command, 1, 1)

  if err eq 0 then return, []

  ind_match = where(strmatch(array.data.signal_alias, name[0]) eq 1, count_match)

  return, array.data[ind_match]

end

;------------------------------------------------------
; Recursively loop over the geometry structure, and add the
; signals to the structure
;------------------------------------------------------
function walktreeforgeomsignals, datastruct, current_type=current_type, $
                                 geomsignals=geomsignals, groups=groups, $
                                 commands=commands, signals=signals, sigdata=sigdata

  forward_function walktreeforgeomsignals

  alldata = {}

  tag_names_data = tag_names(datastruct)

  ind_name = where(strmatch(tag_names_data, 'NAME') eq 1, count_name)

  if undefined(current_type) then current_type = ''

  if count_name gt 0 then begin

     ; This is signal level
     signal_aliases = datastruct.name
     geomsignalnames = strmid(current_type, 0, strlen(current_type)-1)

     all_structs = list()
     
     for i = 0, n_elements(geomsignalnames)-1 do begin
        ind_signal = where(strmatch(geomsignals, geomsignalnames[i]+'/', /fold_case) eq 1, count_signal)
        struct_here = datastruct[i]
        if count_signal gt 0 then begin
           all_signals = []
           for j = 0, count_signal-1 do begin
              signal_here = getsignal(sigdata, groups[j], commands[j], signals[j])
              if not undefined(signal_here) then all_signals = [all_signals, signal_here]
           endfor                     

           if n_elements(all_signals) gt 0 then struct_here = create_struct(struct_here, 'signals', all_signals)
        endif 

        all_structs.add, struct_here
     endfor

     return, all_structs

  endif else begin
     current_type_here = current_type
     for i = 0, n_elements(tag_names_data)-1 do begin
        if is_structure(datastruct.(i)) then begin
           if tag_names_data[i] ne 'DATA' then current_type = current_type_here+strlowcase(tag_names_data[i])+'/'
           tag = tag_names_data[i]
           thisdata = walktreeforgeomsignals(datastruct.(i), current_type=current_type, geomsignals=geomsignals, groups=groups, commands=commands, signals=signals, sigdata=sigdata)
        endif else thisdata = datastruct.(i)
        alldata = create_struct(alldata, tag_names_data[i], thisdata)
     endfor
  endelse  

  return, alldata
end

;------------------------------------------------------
; Recursively loop over datastructure, looking for the name
; of the geometry signals
;------------------------------------------------------
pro walktreeforsignals, datastruct, current_type=current_type, $
                        signals=signals

  if not_structure(datastruct) then begin
     ind_last = strpos(current_type, '/', /reverse_search)
     current_type = strmid(current_type, 0, ind_last)
     ind_last = strpos(current_type, '/', /reverse_search)
     current_type = strmid(current_type, 0, ind_last)
     current_type = current_type+'/'
     return
  endif

  tag_names_data = tag_names(datastruct)

  ind_name = where(strmatch(tag_names_data, 'NAME') eq 1, count_name)

  if count_name gt 0 then begin
     ; This is signal level
     signal_aliases = reform(datastruct.name)
     
     current_type = strmid(current_type, 0, strlen(current_type)-1)

     if undefined(signals) then signals = current_type $
     else signals = [signals, current_type]
  endif else begin
     current_type_here = current_type
     for i = 0, n_elements(tag_names_data)-1 do begin
        if tag_names_data[i] ne 'DATA' then current_type = current_type_here+strlowcase(tag_names_data[i])+'/'
        walktreeforsignals, datastruct.(i), current_type=current_type, signals=signals
     endfor
  endelse
end

;------------------------------------------------------
; Take datastruct and add relevant signal information
; init_signal : signal name that was requested by the user
; source : shot number requested by the user
; versignal : version number for the signal file (the latest is
;             returned if this is not set) 
;------------------------------------------------------
pro addsignaldata, datastruct, signalinfostruct, init_signal, source, versignal=versignal, debug=debug

  if keyword_set(debug)then print, 'addsignaldata : Adding signals to data'

  signal_filenames = signalinfostruct.filenames
  geom_signals_aliasmap = signalinfostruct.geom_alias
  signals_aliasmap = signalinfostruct.signal_alias
  varnames_aliasmap = signalinfostruct.var_name

  if undefined(signal_filenames) then return

  signal_filenames_unique = signal_filenames[uniq(signal_filenames, sort(signal_filenames))]

  command_group = strarr(n_elements(signal_filenames))

  for i = 0, n_elements(signal_filenames_unique)-1 do begin
     if keyword_set(debug) then print, 'addsignaldata : signal filenames are ', signal_filenames_unique[i]
     
     ind_files = where(signal_filenames eq signal_filenames_unique[i])

     all_signals = signals_aliasmap[ind_files]
     all_varnames = varnames_aliasmap[ind_files]

     ; Get matching group to retrieve from signal files
     sig_group = findmatch(all_varnames)

     ; Get last element of matching group
     ; Remove slash at the end if neccessary
     if strmid(sig_group, strlen(sig_group)-1) eq '/' then sig_group = strmid(sig_group, 0, strlen(sig_group)-1)

     ind_last = strpos(sig_group, '/', /reverse_search)
     this_group = strmid(sig_group, ind_last+1)

     sig_data = getstruct(sig_group, signal_filenames_unique[i])

     if sig_data.erc eq 0 then begin
        if strnumber(strmid(this_group, 0, 1)) eq 1 then this_group = '_'+this_group

        if undefined(signal_data) then signal_data = create_struct(this_group, sig_data) $
        else signal_data = create_struct(signal_data, this_group, sig_data)
     endif

     command_group[ind_files] = this_group
  endfor

  if undefined(signal_data) then return

  ; Turn the signal info into strings
  ; that can be used to extract those
  ; signals from signal_data without looping

  ; Remove first group and end
  signal_temp = strmid(varnames_aliasmap, 1)
  ind_first = strpos(signal_temp, sig_group)+strlen(sig_group)
  signal_therest = strmid(signal_temp, transpose(ind_first))

  signals_commands = strarr(n_elements(signal_therest))
  for i = 0, n_elements(signal_therest)-1 do begin
     split_by_slash = strsplit(signal_therest[i], '/', /extract)
     number_at_beginning = stregex(split_by_slash, '^[0-9]')
     ind_number = where(number_at_beginning eq 0, count_number)

     ; If any element starts with a number, then
     ; we have to add _ to the
     ; beginning since in IDL
     ; structure elements can't start with number
     if count_number gt 0 then split_by_slash[ind_number] = '_'+split_by_slash[ind_number]

     signals_commands[i] = strjoin(split_by_slash, '.')
  endfor

  if keyword_set(debug) then print, 'addsignaldata : loop over data and add in signals, init_signal ', init_signal

  ; loop over data structure, find matching signals and add to the structure
  datastruct = walktreeforgeomsignals(datastruct, current_type=init_signal+'/', geomsignals=geom_signals_aliasmap, groups=command_group, $
                                      commands=signals_commands, signals=signals_aliasmap, sigdata=signal_data)

end

