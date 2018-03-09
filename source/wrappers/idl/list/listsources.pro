; ----------------------------
; IDL wrapper to mimic the old listSources function
; that were part of roIdam. Instead now uses the meta
; plugin.
;
; L. Kogan
;
; March 2018
; ----------------------------

function listSources, shot=shot, pulno=pulno, exp_number=exp_number, $
                      type=type, pass=pass

  ; shot number
  if exists(pulno) then shot = pulno
  if exists(exp_number) then shot = exp_number

  if undefined(shot) then begin
     print, 'listSources >> SHOT NUMBER IS REQUIRED: sourcelist = listSources(shot=<shotno>, (type=<type>), (pass=<pass>))'
     print, 'listSources >> to list all available sources use listAllSources()'
     return, -1
  endif

  ; Construct command
  meta_comm = 'meta::list(context=data,cast=column,shot='+strtrim(shot, 2)
  
  if exists(type) then meta_comm = meta_comm + ', type=' + strtrim(type, 2)
  if exists(pass) then meta_comm = meta_comm + ', pass=' + strtrim(pass, 2)

  meta_comm = meta_comm+', /listSources)'

  ; Call
  sources = getdata(meta_comm, '')

  if sources.erc ne 0 then begin
     return, -1
  endif

  sourcelist = sources.data
  n_sources = sourcelist.count

  ; Turn structure into listSources structure
  ; Not returned by meta plugin: SOURCE_ID, SERVER, PATH
  sourcestruct = replicate({ EXP_NUMBER: 0L,    $
                             PASS: 0L,          $
                             SOURCE_STATUS: 0L, $
                             TYPE: '',          $
                             SOURCE_ALIAS: '',  $
                             FORMAT: '',        $
                             FILENAME: '',      $
                             RUN_ID: 0L }, n_sources)

  sourcestruct.exp_number[*] = sourcelist.shot
  sourcestruct.pass = (*sourcelist.pass)
  sourcestruct.source_status = (*sourcelist.status)
  sourcestruct.type = sourcelist.type
  sourcestruct.source_alias = sourcelist.source_alias
  sourcestruct.format = sourcelist.format
  sourcestruct.filename = sourcelist.filename
  sourcestruct.run_id = (*sourcelist.run_id)

  return, sourcestruct  

end
