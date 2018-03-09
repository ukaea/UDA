; ----------------------------
; IDL wrapper to mimic the old getidamlastestsourcepass function
; that was part of roIdam. Instead now uses the meta
; plugin.
;
; L. Kogan
;
; March 2018
; ----------------------------

function getidamlatestsourcepass, shot=shot, pulno=pulno, exp_number=exp_number, $
                                  source=source

  ; shot number
  if exists(pulno) then shot = pulno
  if exists(exp_number) then shot = exp_number

  ; Check for required arguments
  if undefined(shot) or undefined(source) then begin
     print, 'getidamlatestsourcepass >> SHOT NUMBER AND SOURCE ARE REQUIRED: pass = getidamlatestsourcepass(shot=<shotno>, source=<source>)'
     return, -1
  endif

  meta_comm = 'meta::get(context=data,shot='+strtrim(shot, 2)+',source='+strtrim(source,2)+',/lastpass)'

  ; Call
  pass = getdata(meta_comm, '')

  if pass.erc ne 0 then begin
     return, -1
  endif

  return, pass.data.lastpass

end
