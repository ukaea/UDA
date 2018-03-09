; ----------------------------
; IDL wrapper to mimic the old listSignals functions
; that were part of roIdam. Instead now uses the meta
; plugin.
;
; L. Kogan
;
; September 2017
; ----------------------------

function listSignals, shot=shot, pulno=pulno, exp_number=exp_number, pass=pass, source=source, $
                      search=search, all=all, type=type, latest=latest



  if keyword_set(latest) then print, 'listSignals: WARNING: latest keyword no longer implemented. It will be ignored.'

 ; basic command (this is all you need for all keyword
  meta_comm = 'meta::listData(context=data,cast=column'

  ; shot number
  if exists(pulno) then shot = pulno
  if exists(exp_number) then shot = exp_number
  if exists(shot) then meta_comm = meta_comm+',shot='+strtrim(shot, 2)

  ; pass
  if exists(pass) then meta_comm = meta_comm+',pass='+strtrim(pass, 2)

  ; source
  if exists(source) then meta_comm = meta_comm+', source='+strtrim(source, 2)

  ; search
  if exists(search) then meta_comm = meta_comm+', signal_match='+strtrim(search, 2)

  ; type (A, R, M)
  if exists(type) then meta_comm = meta_comm+', type='+strtrim(type, 2)

  meta_comm = meta_comm + ')'

  signallist = getdata(meta_comm, '')

  if signallist.erc ne 0 then begin
     return, -1
  endif

  signallist = signallist.data
  n_signals = signallist.count

  ; Turn structure into listSignals structure
  if exists(shot) then begin
     signalstruct = replicate({ EXP_NUMBER: 0L,   $
                                PASS: 0L,         $
                                SIGNAL_ALIAS: '', $
                                GENERIC_NAME: '', $
                                TYPE: '',         $
                                DESCRIPTION: '',  $
                                SOURCE_ALIAS: '', $
                                SIGNAL_STATUS: 0L, $
                                MDS_NAME: '' }, n_signals)
     signalstruct.exp_number[*] = signallist.shot
     signalstruct.pass[*] = signallist.pass
     signalstruct.signal_status = (*signallist.signal_status)
  endif else begin
     signalstruct = replicate({ SIGNAL_ALIAS: '', $
                                GENERIC_NAME: '', $
                                TYPE: '',         $
                                DESCRIPTION: '',  $
                                SOURCE_ALIAS: '', $
                                MDS_NAME: '' }, n_signals)
  endelse

  help, signalstruct.signal_alias
  help, signallist.signal_name
  signalstruct.signal_alias = signallist.signal_name
  signalstruct.generic_name = signallist.generic_name
  signalstruct.type = signallist.type
  signalstruct.description = signallist.description
  signalstruct.source_alias = signallist.source_alias
  signalstruct.mds_name = signallist.mds_name

  return, signalstruct

end
