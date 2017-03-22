;------------------------------------------------------
; FUNCTION: applygeommanipulation
; AUTHOR: L. Kogan
; DATE: March 2016
; 
; Function that takes geometry data structure (as read in by
; getgeomdata), and if there is manipulation needed for that data
; passes the structure to the correct manipulator.
;
;------------------------------------------------------

function applygeommanipulation, datastruct, signal, _extra=_extra

  newstruct = datastruct

  if strmatch(signal, '/magnetics/pickup*') or strmatch(signal, '/magnetics/mirnov*') $
     or strmatch(signal, 'b_*') or strmatch(signal, 'm_*') then begin
     newstruct = pickupmanipulation(newstruct, _extra=_extra)
  endif

  return, newstruct

end
