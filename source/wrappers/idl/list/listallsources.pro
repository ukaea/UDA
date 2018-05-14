; ----------------------------
; IDL wrapper to mimic the old listAllSources function
; that were part of roIdam. Instead now uses the meta
; plugin.
;
; L. Kogan
;
; March 2018
; ----------------------------

function listAllSources, type=type

  ; Construct command
  meta_comm = 'meta::list(context=data,cast=column'
  
  if exists(type) then meta_comm = meta_comm + ', type=' + strtrim(type, 2)

  meta_comm = meta_comm+', /listSources)'


  ; Call
  sources = getdata(meta_comm, '')

  if sources.erc ne 0 then begin
     return, -1
  endif

  sourcelist = sources.data
  n_sources = sourcelist.count

  ; Turn structure into listSources structure
  sourcestruct = replicate({ TYPE: '',          $
                             SOURCE_ALIAS: '' }, n_sources)

  sourcestruct.type = sourcelist.type
  sourcestruct.source_alias = sourcelist.source_alias

  return, sourcestruct


end
