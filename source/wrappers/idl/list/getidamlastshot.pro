; ----------------------------
; IDL wrapper to mimic the old getidamlastshot function
; that was part of roIdam. Instead now uses the meta
; plugin.
;
; L. Kogan
;
; March 2018
; ----------------------------

function getidamlastshot

  comm = 'meta::get(context=data, /lastshot)'

  lastshot = getdata('meta::get(context=data, /lastshot)', '')

  if lastshot.erc ne 0 then begin
     return, -1
  endif

  return, lastshot.data
end
